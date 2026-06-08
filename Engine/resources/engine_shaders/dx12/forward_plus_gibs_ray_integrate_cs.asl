#include "forward_plus_common.asli"

wave_size(32)
[numthreads(32, 1, 1)] void
main_cs(uint32 thread_id sv_dispatch_thread_id)

{
	const gibs_data							 data			  = gibs_load_gibs_data();
	const rw_stack<uint32>					 alive_atack	  = gibs_load_alive_surfel_id_stack_curr(data);
	const rw_byte_array<uint32>				 ray_offset_arr	  = gibs_load_surfel_ray_count_prefix_rw_arr(data);
	const rw_byte_array<uint32>				 ray_count_arr	  = gibs_load_surfel_ray_count_ideal_rw_arr(data);
	const rw_byte_array<gibs_ray_result>	 ray_res_arr	  = gibs_load_ray_result_rw_arr(data);
	const rw_byte_array<surfel_recycle_data> recycle_data_arr = gibs_load_surfel_recycle_data_rw_arr(data);
	const rw_array<surfel>					 surfel_arr		  = gibs_load_surfel_rw_arr(data);
	const rw_byte_array<surfel_msme>		 msme_arr		  = gibs_load_surfel_msme_rw_arr(data);
	const uint32							 alive_count	  = alive_atack.size();

	attr_branch()

	if (alive_count == 0)
	{
		return;
	}

	if (thread_id >= alive_count)
	{
		return;
	}

	const uint32 alive_id  = thread_id;
	const uint32 surfel_id = alive_atack[thread_id];

	const uint32 ray_offset = ray_offset_arr[alive_id];
	const uint32 ray_count	= ray_count_arr[alive_id];

	if (ray_count == 0) { return; }

	const bool is_new_born = recycle_data_arr[surfel_id].is_new_born();

	const uint32_2		  atlas_offset	   = gibs_calc_atlas_offset(data, surfel_id);
	rw_texture_2d<float2> luminance_atlas  = global_resource_buffer[data.h_irradiance_atlas_uav_id];
	rw_texture_2d<float2> visibility_atlas = global_resource_buffer[data.h_visibility_atlas_uav_id];

	uint32		pack_counter = 0u;
	float3		radiance_sum = (float3)0;
	surfel		surfel		 = surfel_arr[surfel_id];
	surfel_msme msme		 = msme_arr[surfel_id];
	for (uint32 i = 0; i < ray_count; ++i)
	{
		const gibs_ray_result ray_res = ray_res_arr[ray_offset + i];

		// luminance
		const float lum_blend_factor = is_new_born ? 1.f : 0.01f;

		const float3  radiance	= decode_r11g11b10(ray_res.radiance_r11g11b10);
		const float3  dir_local = decode_oct_snorm16(ray_res.dir_oct_snorm8 & 0xffff);
		const int32_2 px		= gibs_calc_atlas_tile_px(atlas_offset, dir_local);

		const float2 original  = luminance_atlas[px];
		const float	 luminance = lerp(original.x, max(luminance_rec709(radiance), GIBS_MIN_LUMINANCE), lum_blend_factor);

		luminance_atlas[px] = float2(luminance, original.y);

		const float cos_theta = dir_local.z;

		// visibility
		const float	 vis_blend_factor = is_new_born ? 1.f : cos_theta * 0.1f;
		const float	 dist_norm		  = saturate(ray_res.distance / surfel.radius);
		const float2 chebyshev		  = float2(dist_norm, dist_norm * dist_norm);

		visibility_atlas[px] = lerp(visibility_atlas[px], chebyshev, vis_blend_factor);

		// radiance, cos weight
		radiance_sum += radiance * cos_theta
					  / max(epsilon_1e6, ray_res.pdf)
					  * min(1.f, GIBS_MAX_LUMINANCE_FOR_FIREFLY / luminance);

		++pack_counter;

		if (pack_counter == max(1u, ray_count / 4) or i == ray_count - 1)
		{
			gibs_update_msme(radiance_sum / pack_counter, msme);
			radiance_sum = (float3)0;
			pack_counter = 0u;
		}
	}

	surfel.radiance = msme.mean_long;

	// radiance sharing
	const gibs_lut_data lut_data = gibs_load_gibs_lut_data();

	const uint32 cell_id = gibs_flatten_cell_idx(data, gibs_calc_cell_idx(data, lut_data, surfel.position));

	const gibs_cell_entry	 cell_entry			   = gibs_load_cell_entry_arr(data)[cell_id];
	const byte_array<uint32> cell_to_surfel_id_arr = gibs_load_cell_to_surfel_id_arr(data);

	float3 normal = decode_oct_snorm16(surfel.normal_oct_snorm16);

	float4 radiance_shared = (float4)0.f;
	for (uint32 i = 0; i < cell_entry.count; ++i)
	{
		const uint32 surfel_id_nbr = cell_to_surfel_id_arr[cell_entry.offset + i];

		if (surfel_id == surfel_id_nbr) { continue; }

		const struct surfel surfel_nbr = surfel_arr[surfel_id_nbr];

		const float contribution = gibs_calc_surfel_contribution(data, surfel_nbr, surfel.position, normal);

		const float3  rel		= surfel.position - surfel_nbr.position;
		const int32_2 px		= gibs_calc_atlas_tile_px(gibs_calc_atlas_offset(data, surfel_id_nbr), normalize(rel));
		const float2  chebyshev = visibility_atlas[px];

		radiance_shared += float4(surfel_nbr.radiance, 1.f)
						 * contribution
						 * smoothstep(0.f, float(GIBS_RADIANCE_CACHE_DELAY), float(recycle_data_arr[surfel_id_nbr].frame_since_born))
						 * gibs_clac_visibility(chebyshev, saturate(length(rel) / surfel_nbr.radius));
	}

	if (radiance_shared.w > 0)
	{
		surfel.radiance = lerp(surfel.radiance, radiance_shared.xyz / radiance_shared.w, saturate(length(msme.variance)) * 0.5f);
	}

	surfel_arr.store(surfel_id, surfel);
	msme_arr.store(surfel_id, msme);
}
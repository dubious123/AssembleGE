#include "forward_plus_common.asli"

wave_size(32)
[numthreads(32, 1, 1)] void
main_cs(uint32 thread_id sv_dispatch_thread_id)

{
	const gibs_data							 data			  = gibs_load_gibs_data();
	const rw_stack<uint32>					 alive_stack	  = gibs_load_alive_surfel_id_stack_curr(data);
	const rw_byte_array<uint32>				 ray_offset_arr	  = gibs_load_surfel_ray_count_prefix_rw_arr(data);
	const rw_byte_array<uint32>				 ray_count_arr	  = gibs_load_surfel_ray_count_ideal_rw_arr(data);
	const rw_byte_array<gibs_ray_result>	 ray_res_arr	  = gibs_load_ray_result_rw_arr(data);
	const rw_byte_array<surfel_recycle_data> recycle_data_arr = gibs_load_surfel_recycle_data_rw_arr(data);
	const rw_array<surfel>					 surfel_arr		  = gibs_load_surfel_rw_arr(data);
	const rw_byte_array<surfel_msme>		 msme_arr		  = gibs_load_surfel_msme_rw_arr(data);
	const uint32							 alive_count	  = alive_stack.size();

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
	const uint32 surfel_id = alive_stack[thread_id];

	const uint32 ray_offset = ray_offset_arr[alive_id];
	const uint32 ray_count	= ray_count_arr[alive_id];


	const uint32_2 atlas_offset = gibs_calc_atlas_offset(data, surfel_id);

	rw_texture_2d<float2> luminance_atlas  = global_resource_buffer[data.h_irradiance_atlas_uav_id];
	rw_texture_2d<float2> visibility_atlas = global_resource_buffer[data.h_visibility_atlas_uav_id];

	float3				radiance_sum = (float3)0;
	surfel				surfel		 = surfel_arr[surfel_id];
	surfel_msme			msme		 = msme_arr[surfel_id];
	surfel_recycle_data recycle		 = recycle_data_arr[surfel_id];

	const bool is_new_born = recycle.is_new_born();


	if (ray_count == 0) { return; }
	if (surfel.radius == 0.f) { return; }

	for (uint32 i = 0; i < ray_count; ++i)
	{
		const gibs_ray_result ray_res = ray_res_arr[ray_offset + i];

		// luminance
		const float lum_blend_factor = is_new_born ? 1.f : 0.1f;

		const float3  radiance	= decode_r11g11b10(ray_res.radiance_r11g11b10);
		const float3  dir_local = decode_world_hemi_oct_snorm8(cast<uint16>(ray_res.dir_oct_snorm8 & 0xffffu));
		const int32_2 px		= gibs_calc_atlas_tile_px(atlas_offset, dir_local);
		assert(all(px - atlas_offset >= 0) and all(px - atlas_offset < 6), g::fmt_forward_plus_gibs_ray_integrate_cs);

		// assert(dir_local.y >= 0.f,g::fmt_forward_plus_gibs_ray_integrate_cs);
		const float cos_theta = dir_local.y;

		const float contribution = cos_theta / max(epsilon_1e6, ray_res.pdf);

		const float luminance = luminance_rec709(radiance);

		luminance_atlas[px] = float2(lerp(luminance_atlas[px].x, luminance * cos_theta, lum_blend_factor), luminance_atlas[px].y);

		// radiance, cos weight
		radiance_sum += radiance * contribution;
		//*min(1.f, GIBS_MAX_LUMINANCE_FOR_FIREFLY / max(epsilon_1e6, luminance));

		// visibility
		const float	 vis_blend_factor = is_new_born ? 1.f : cos_theta * 0.1f;
		const float	 dist_norm		  = saturate(ray_res.distance / (surfel.radius /** GIBS_SURFEL_OUTER_RADIUS_FACTOR*/));
		const float2 chebyshev		  = float2(dist_norm, dist_norm * dist_norm);

		visibility_atlas[px] = lerp(max(0.f, visibility_atlas[px]), chebyshev, vis_blend_factor);
	}

	radiance_sum /= ray_count;
	// for (uint32 i = 0; i < 6; ++i)
	//{
	//	for (uint32 j = 0; j < 6; ++j)
	//	{
	//		const uint32_2 px = atlas_offset + uint32_2(i, j);

	//		assert(all(luminance_atlas[px] >= 0.f), g::fmt_forward_plus_gibs_ray_integrate_cs, line);
	//		assert(all(visibility_atlas[px] >= 0.f), g::fmt_forward_plus_gibs_ray_integrate_cs, line);
	//	}
	//}

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

		const float			contribution = gibs_calc_surfel_contribution<true>(data, surfel_nbr, surfel.position, normal);
		surfel_recycle_data recycle_nbr	 = recycle_data_arr[surfel_id_nbr];
		if (recycle_nbr.is_new_born()) { continue; }

		radiance_shared += float4(surfel_nbr.radiance, 1.f)
						 * contribution
						 * smoothstep(0.f, float(GIBS_RADIANCE_CACHE_DELAY), float(recycle_nbr.frame_since_born))
						 * gibs_calc_visibility<false>(data, surfel_id_nbr, surfel_nbr, surfel.position);
	}


	if (radiance_shared.w > 0)
	{
		const float blend_factor = smoothstep(0.f, float(GIBS_RADIANCE_CACHE_DELAY), float(recycle.frame_since_born)) * 0.5f;
		gibs_update_msme(radiance_sum * blend_factor + radiance_shared.xyz / radiance_shared.w * (1.f - blend_factor), msme);
		// surfel.radiance = lerp(surfel.radiance, radiance_shared.xyz / radiance_shared.w, saturate(length(msme.variance)) * 0.5f);
	}
	else
	{
		gibs_update_msme(radiance_sum, msme);
	}


	surfel.radiance = msme.mean_long;

	surfel_arr.store(surfel_id, surfel);
	msme_arr.store(surfel_id, msme);

	// todo try separate radiance sharing pass
}
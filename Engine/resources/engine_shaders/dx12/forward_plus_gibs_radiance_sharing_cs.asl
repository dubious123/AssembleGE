#include "forward_plus_common.asli"

wave_size(32)
[numthreads(32, 1, 1)] void
main_cs(uint32 thread_id sv_dispatch_thread_id)

{
	const gibs_data							 data			  = gibs_load_gibs_data();
	const rw_stack<uint32>					 alive_stack	  = gibs_load_alive_surfel_id_stack_curr(data);
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

	const uint32_2 atlas_offset = gibs_calc_atlas_offset(data, surfel_id);

	surfel surfel		= surfel_arr[surfel_id];
	float3 radiance_sum = surfel.radiance;

	// const bool is_new_born = recycle.is_new_born();

	// radiance sharing

	if (surfel.radius < epsilon_1e4)
	{
		// debug_log(g::fmt_gibs_tile_coverage, surfel_id);
		return;
	}

	const gibs_lut_data		 lut_data			   = gibs_load_gibs_lut_data();
	const uint32			 cell_id			   = gibs_flatten_cell_idx(data, gibs_calc_cell_idx(data, lut_data, surfel.position));
	const gibs_cell_entry	 cell_entry			   = gibs_load_cell_entry_arr(data)[cell_id];
	const byte_array<uint32> cell_to_surfel_id_arr = gibs_load_cell_to_surfel_id_arr(data);

	const float3 normal = decode_oct_snorm16(surfel.normal_oct_snorm16);

	float4 radiance_shared = (float4)0.f;

	attr_loop()

	for (uint32 i = 0; i < min(0, cell_entry.count); ++i)
	{
		const uint32 surfel_id_nbr = cell_to_surfel_id_arr[cell_entry.offset + i];

		if (surfel_id == surfel_id_nbr) { continue; }

		const struct surfel surfel_nbr = surfel_arr[surfel_id_nbr];

		if (surfel_nbr.radius < epsilon_1e4) { continue; }

		const float contribution = gibs_calc_surfel_contribution<true>(data, surfel_nbr, surfel.position, normal);
		// surfel_recycle_data recycle_nbr	 = recycle_data_arr[surfel_id_nbr];
		// if (recycle_nbr.is_new_born()) { continue; }

		const float cos_theta = dot(normal, decode_oct_snorm16(surfel_nbr.normal_oct_snorm16));

		radiance_shared += float4(surfel_nbr.radiance, 1.f)
						 * max(0.f, cos_theta)
						 * contribution
						 //* smoothstep(0.f, float(GIBS_RADIANCE_CACHE_DELAY), float(recycle_nbr.frame_since_born))
						 * gibs_calc_visibility(data, surfel_id_nbr, surfel_nbr, surfel.position);
	}

	surfel_msme msme = gibs_load_surfel_msme_rw_arr(data)[surfel_id];
	if (radiance_shared.w > 0)
	{
		surfel_recycle_data recycle		 = recycle_data_arr[surfel_id];
		const float			blend_factor = smoothstep(0.f, float(GIBS_RADIANCE_CACHE_DELAY), float(recycle.frame_since_born)) * 0.5f;
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
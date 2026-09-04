#include "hrp_common.asli"

// wave_size(GIST_GENERAL_TPG)
//[numthreads(GIST_GENERAL_TPG, 1, 1)] void
// main_cs(uint32 thread_id sv_group_thread_id,
//		uint32 group_id	 sv_group_id)
//
//{
//	const uint32 offset_base = group_id * GIST_GENERAL_EPG;
//
//	const gist_data data = gist::load_data();
//
//	const uint32 ideal_ray_count_total = gist::cell::ideal_ray_count_total(data);
//	const float	 ray_count_factor	   = saturate(float(data.cell_surfel_ray_budget) / ideal_ray_count_total);
//
//	rw_byte_array<uint16> ray_count_arr		   = gist::cell::ray_count_rw_arr(data);
//	rw_byte_array<uint32> ray_count_prefix_arr = gist::cell::ray_count_prefix_rw_arr(data);
//
//	const uint32 alive_count_total = gist::cell::alive_count_curr(data);
//	// local sum of surfel_count_per_cell
//	uint32 local_sum = 0u;
//
//	uint32 local_prefix_arr[GIST_GENERAL_EPT];
//
//	expand_all()
//
//	for (uint32 i = 0; i < GIST_GENERAL_EPT; ++i)
//	{
//		const uint32 alive_id = offset_base
//							  + i * GIST_GENERAL_TPG
//							  + thread_id;
//
//		if (alive_id >= alive_count_total) { break; }
//
//		const uint16 ray_count_ideal = ray_count_arr[alive_id];
//		const uint16 ray_count		 = min(uint16(ray_count_ideal * ray_count_factor), uint16(data.cell_surfel_ray_count_max()));
//		// const uint32 ray_count = min(ray_count_ideal * float(GIST_RAY_BUDGET) / float(ideal_ray_count_total), GIST_MAX_RAY_PER_SURFEL);
//		ray_count_arr.store(alive_id, ray_count);
//
//		local_prefix_arr[i]	 = local_sum;
//		local_sum			+= ray_count;
//	}
//
//	const uint32 local_offset = wave_prefix_sum(local_sum);
//
//	uint32 group_offset_tmp = 0;
//	if (thread_id == GIST_GENERAL_TPG - 1)
//	{
//		group_offset_tmp = gist::cell::alloc_ray(data, local_offset + local_sum);
//	}
//	const uint32 group_offset = wave_read_lane_at(group_offset_tmp, GIST_GENERAL_TPG - 1);
//
//	expand_all()
//
//	for (uint32 i = 0; i < GIST_GENERAL_EPT; ++i)
//	{
//		const uint32 alive_idx = offset_base
//							   + i * GIST_GENERAL_TPG
//							   + thread_id;
//
//		if (alive_idx >= alive_count_total) { break; }
//
//		const uint32 ray_offset = group_offset
//								+ local_offset
//								+ local_prefix_arr[i];
//
//		ray_count_prefix_arr.store(alive_idx, ray_offset);
//	}
// }

wave_size(GIST_GENERAL_TPG)
[numthreads(GIST_GENERAL_TPG, 1, 1)] void
main_cs(uint32 thread_id sv_group_thread_id,
		uint32 group_id	 sv_group_id)

{
	const uint32 offset_base = group_id * GIST_GENERAL_EPG;

	const gist_data data = gist::load_data();

	const uint32 ideal_ray_count_total = gist::cell::ideal_ray_count_total(data);
	const float	 ray_count_factor	   = saturate(float(data.cell_surfel_ray_budget) / ideal_ray_count_total);

	rw_byte_array<uint16> ray_count_arr		   = gist::cell::ray_count_rw_arr(data);
	rw_byte_array<uint32> ray_count_prefix_arr = gist::cell::ray_count_prefix_rw_arr(data);

	const uint32 alive_count_total = gist::cell::alive_count_curr(data);
	// local sum of surfel_count_per_cell
	uint32 local_sum = 0u;

	for (uint32 i = 0; i < GIST_GENERAL_EPT; ++i)
	{
		const uint32 alive_id = offset_base
							  + i * GIST_GENERAL_TPG
							  + thread_id;

		if (alive_id >= alive_count_total) { break; }

		const uint16 ray_count_ideal = ray_count_arr[alive_id];
		const uint16 ray_count		 = min(uint16(ray_count_ideal * ray_count_factor), uint16(data.cell_surfel_ray_count_max()));
		// const uint32 ray_count = min(ray_count_ideal * float(GIST_RAY_BUDGET) / float(ideal_ray_count_total), GIST_MAX_RAY_PER_SURFEL);
		ray_count_arr.store(alive_id, ray_count);

		local_sum += ray_count;
	}

	const uint32 local_offset = wave_prefix_sum(local_sum);

	uint32 group_offset_tmp = 0;
	if (thread_id == GIST_GENERAL_TPG - 1)
	{
		group_offset_tmp = gist::cell::alloc_ray(data, local_offset + local_sum);
	}
	const uint32 group_offset = wave_read_lane_at(group_offset_tmp, GIST_GENERAL_TPG - 1);

	uint32 offset = group_offset + local_offset;
	for (uint32 i = 0; i < GIST_GENERAL_EPT; ++i)
	{
		const uint32 alive_idx = offset_base
							   + i * GIST_GENERAL_TPG
							   + thread_id;

		if (alive_idx >= alive_count_total) { break; }

		ray_count_prefix_arr.store(alive_idx, offset);
		offset += ray_count_arr[alive_idx];
	}
}
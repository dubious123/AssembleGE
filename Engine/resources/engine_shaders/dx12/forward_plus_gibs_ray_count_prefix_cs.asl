#include "forward_plus_common.asli"

wave_size(GIBS_RAY_COUNT_REDUCE_TPG)
[numthreads(GIBS_RAY_COUNT_REDUCE_TPG, 1, 1)] void
main_cs(uint32 thread_id sv_group_thread_id,
		uint32 group_id	 sv_group_id)

{
	const uint32 offset_base = group_id * GIBS_RAY_COUNT_REDUCE_EPG;

	const gibs_data data = gibs_load_gibs_data();

	const uint32 ideal_ray_count_total = gibs_get_ray_count_ideal_total(data);
	const float	 ray_count_factor	   = saturate(float(GIBS_RAY_BUDGET) / ideal_ray_count_total);

	rw_byte_array<uint32> ray_count_ideal_arr  = gibs_load_surfel_ray_count_ideal_rw_arr(data);
	rw_byte_array<uint32> ray_count_prefix_arr = gibs_load_surfel_ray_count_prefix_rw_arr(data);

	const rw_stack<uint32> alive_stack = gibs_load_alive_surfel_id_stack_curr(data);

	const uint32 alive_count_total = alive_stack.size();
	// local sum of surfel_count_per_cell
	uint32 local_sum = 0u;

	uint32 local_prefix_arr[GIBS_RAY_COUNT_REDUCE_EPT];

	expand_all()

	for (uint32 i = 0; i < GIBS_RAY_COUNT_REDUCE_EPT; ++i)
	{
		const uint32 alive_idx = offset_base
							   + i * GIBS_RAY_COUNT_REDUCE_TPG
							   + thread_id;

		if (alive_idx >= alive_count_total) { break; }

		const uint32 ray_count_ideal = ray_count_ideal_arr[alive_idx];
		const uint32 ray_count		 = min(uint32(ray_count_ideal * ray_count_factor), GIBS_MAX_RAY_PER_SURFEL);
		ray_count_ideal_arr.store(alive_idx, ray_count);

		local_prefix_arr[i]	 = local_sum;
		local_sum			+= ray_count;
	}

	const uint32 local_offset = wave_prefix_sum(local_sum);

	uint32 group_offset_tmp = 0;
	if (thread_id == GIBS_RAY_COUNT_REDUCE_TPG - 1)
	{
		group_offset_tmp = gibs_alloc_ray(data, local_offset + local_sum);
	}
	const uint32 group_offset = wave_read_lane_at(group_offset_tmp, GIBS_RAY_COUNT_REDUCE_TPG - 1);

	expand_all()

	for (uint32 i = 0; i < GIBS_RAY_COUNT_REDUCE_EPT; ++i)
	{
		const uint32 alive_idx = offset_base
							   + i * GIBS_RAY_COUNT_REDUCE_TPG
							   + thread_id;

		if (alive_idx >= alive_count_total) { break; }

		const uint32 ray_offset = group_offset
								+ local_offset
								+ local_prefix_arr[i];

		ray_count_prefix_arr.store(alive_idx, ray_offset);
	}
}
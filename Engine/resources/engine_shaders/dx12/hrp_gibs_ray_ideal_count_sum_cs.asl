#include "hrp_common.asli"

wave_size(GIBS_RAY_COUNT_REDUCE_TPG)
[numthreads(GIBS_RAY_COUNT_REDUCE_TPG, 1, 1)] void
main_cs(uint32 dispatch_thread_id sv_dispatch_thread_id,
		uint32 thread_id		  sv_group_thread_id,
		uint32 group_id			  sv_group_id)

{
	const uint32 base_offset = group_id * GIBS_RAY_COUNT_REDUCE_EPG;

	const gibs_data data = gibs_load_gibs_data();

	const uint32 group_sum_count	 = ceil(gibs_load_alive_count_prev(data), 32u);
	uint32		 local_ray_count_sum = 0u;

	const rw_byte_array<uint32> ray_count_ideal_wave_sum_arr = gibs_load_surfel_ray_count_ideal_wave_sum_rw_arr(data);

	expand_all()

	for (uint32 i = 0; i < GIBS_RAY_COUNT_REDUCE_EPT; ++i)
	{
		const uint32 idx = base_offset
						 + i * GIBS_RAY_COUNT_REDUCE_TPG
						 + thread_id;

		if (idx >= group_sum_count) { break; }

		local_ray_count_sum += ray_count_ideal_wave_sum_arr[idx];
	}

	const uint32 ray_count_wave_sum = wave_active_sum(local_ray_count_sum);

	if (wave_is_first_lane())
	{
		gibs_atomic_add_ray_count_ideal(data, ray_count_wave_sum);
	}

	if (dispatch_thread_id == 0)
	{
		rw_stack<uint32> alive_stack = gibs_load_alive_surfel_id_stack_curr(data);
		gibs_set_indirect_arg_ray_count_prefix(data, uint32_3(ceil(alive_stack.size(), GIBS_RAY_COUNT_REDUCE_EPG), 1, 1));
		gibs_set_indirect_arg_ray_entry(data, uint32_3(ceil(alive_stack.size(), 32), 1, 1));
		gibs_set_indirect_arg_surfel_scatter(data, uint32_3(ceil(alive_stack.size(), 32), 1, 1));


		gibs_set_indirect_arg_ray_integrate(data, uint32_3(ceil(alive_stack.size(), 32), 1, 1));
		gibs_set_indirect_arg_build_cdf(data, uint32_3(alive_stack.size(), 1, 1));

		gibs_set_indirect_arg_radiance_sharing(data, uint32_3(ceil(alive_stack.size(), 32), 1, 1));


		assert(alive_stack.size() + gibs_load_dead_surfel_id_stack(data).size() == data.max_surfel_count,
			   g::fmt_forward_plus_gibs_ray_ideal_count_sum_cs, line,
			   frame_index);
	}
}
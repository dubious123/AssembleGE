#include "forward_plus_common.asli"

wave_size(GIBS_RAY_COUNT_REDUCE_TPG)
[numthreads(GIBS_RAY_COUNT_REDUCE_TPG, 1, 1)] void
main_cs(uint32 thread_id sv_group_thread_id,
		uint32 group_id	 sv_group_id)

{
	const uint32 base_offset = group_id * GIBS_RAY_COUNT_REDUCE_EPG;

	const gibs_data data = gibs_load_gibs_data();

	const uint32 ray_sum_total = ceil(data.max_surfel_count, 32u);
	uint32		 local_sum	   = 0u;

	const rw_byte_array<uint32> ray_count_group_sum_arr = gibs_load_surfel_ray_count_prefix_rw_arr(data);

	expand_all()

	for (uint32 i = 0; i < GIBS_RAY_COUNT_REDUCE_EPT; ++i)
	{
		const uint32 ray_sum_id = base_offset
								+ i * GIBS_RAY_COUNT_REDUCE_TPG
								+ thread_id;

		if (ray_sum_id >= ray_sum_total) { break; }

		local_sum += ray_count_group_sum_arr[ray_sum_id];
	}

	const uint32 wave_sum = wave_active_sum(local_sum);

	if (wave_is_first_lane())
	{
		gibs_atomic_add_ray_count_ideal(data, wave_sum);
	}
}
#include "hrp_common.asli"

wave_size(GIST_GENERAL_TPG)
[numthreads(GIST_GENERAL_TPG, 1, 1)] void
main_cs(uint32 dispatch_thread_id sv_dispatch_thread_id,
		uint32 thread_id		  sv_group_thread_id,
		uint32 group_id			  sv_group_id)

{
	const uint32 base_offset = group_id * GIST_GENERAL_EPG;

	const gist_data data = gist::load_data();

	const uint32 group_sum_count	 = ceil(gist::cell::alive_count_curr(data), AGE_WAVE_SIZE);
	uint32		 local_ray_count_sum = 0u;

	const rw_byte_array<uint16> ideal_ray_count_wave_sum_arr = gist::cell::ideal_ray_count_wave_sum_rw_arr(data);

	// expand_all()
	for (uint32 i = 0; i < GIST_GENERAL_EPT; ++i)
	{
		const uint32 idx = base_offset
						 + i * GIST_GENERAL_TPG
						 + thread_id;

		if (idx >= group_sum_count) { break; }

		local_ray_count_sum += ideal_ray_count_wave_sum_arr[idx];
	}

	const uint32 ray_count_wave_sum = wave_active_sum(local_ray_count_sum);

	if (wave_is_first_lane())
	{
		gist::cell::atomic_add_ideal_ray_count(data, ray_count_wave_sum);
	}
}
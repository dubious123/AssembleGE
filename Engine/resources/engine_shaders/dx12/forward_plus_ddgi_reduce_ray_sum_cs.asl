#include "forward_plus_common.asli"

wave_size(32)
[numthreads(DDGI_UPDATE_PROBE_STATE_THREAD_PER_GROUP, 1, 1)] void
main_cs(uint32_3 group_id sv_group_id,
		uint32 thread_id  sv_group_thread_id)

{
	const uint32 sum_offset = group_id.x * DDGI_UPDATE_PROBE_STATE_THREAD_PER_GROUP * DDGI_UPDATE_PROBE_STATE_PROBE_PER_THREAD;

	// ith thread
	// i , i + 32, i + 64, ...

	uint32 ray_count_sum = 0u;

	for (uint32 i = 0; i < DDGI_UPDATE_PROBE_STATE_PROBE_PER_THREAD; ++i)
	{
		const uint32 sum_idx = sum_offset + thread_id + i * DDGI_UPDATE_PROBE_STATE_THREAD_PER_GROUP;

		ray_count_sum += load_ddgi_ray_sum(sum_idx);
	}

	const uint32 group_ray_sum = wave_active_sum(ray_count_sum);

	if (wave_is_first_lane())
	{
		store_ddgi_ray_sum_total(group_ray_sum);
	}
}
#include "forward_plus_common.asli"

wave_size(32)
[numthreads(32, 1, 1)] void
main_cs(uint32 thread_id sv_dispatch_thread_id)

{
	const ddgi_data data		= load_ddgi_data();
	const uint32	ppl			= load_ddgi_ppl(data);
	const uint32	level_count = load_ddgi_level_count(data);

	// update_probe_state_cs
	static const uint32 probe_per_group = DDGI_UPDATE_PROBE_STATE_THREAD_PER_GROUP * DDGI_UPDATE_PROBE_STATE_PROBE_PER_THREAD;
	const uint32		group_count_x	= ceil(ppl, probe_per_group);

	const uint32 ideal_sum_count = group_count_x * level_count;

	const uint32 ideal_ray_sum = thread_id < ideal_sum_count
								   ? ddgi_load_group_ray_sum_ideal(data, thread_id)
								   : 0u;

	const uint32 group_ray_sum = wave_active_sum(ideal_ray_sum);

	if (wave_is_first_lane())
	{
		ddgi_add_ray_sum_total_ideal(data, group_ray_sum);
	}
}
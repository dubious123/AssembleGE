#include "forward_plus_common.asli"

wave_size(32)
[numthreads(32, 1, 1)] void
main_cs(uint32 thread_id sv_dispatch_thread_id)

{
	const uint32 group_ray_sum = wave_active_sum(load_ddgi_ray_sum(thread_id));

	if (wave_is_first_lane())
	{
		add_ddgi_ray_sum_total(group_ray_sum);
	}
}
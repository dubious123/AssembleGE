#include "hrp_common.asli"

wave_size(AGE_WAVE_SIZE)
[numthreads(64, 1, 1)] void
main_cs(uint32 thread_id sv_dispatch_thread_id)

{
	const gist_data data = gist::load_data();

	if (thread_id >= gist::adaptive::specular_ray_count_total(data)) { return; }

	assert(false);
}
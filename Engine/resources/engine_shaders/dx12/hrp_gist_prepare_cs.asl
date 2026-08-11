#include "hrp_common.asli"

[numthreads(1, 1, 1)] void
main_cs(uint32 thread_id sv_dispatch_thread_id)

{
	const gist_data data = gist::load_data();

	gist::cell::alive_id_stack_curr(data).resize(0u);

	gist::ray::reset_alloc(data);

	gist::indirect_arg::set_update_cell_surfel_id_stack(data, uint32_3(ceil(gist::cell::alive_count_prev(data), AGE_WAVE_SIZE), 1, 1));
	// reset by clear uav
	// gist_reset_tile_surfel_alloc(data);
	// gist_reset_cell_surfel_alloc(data);
}
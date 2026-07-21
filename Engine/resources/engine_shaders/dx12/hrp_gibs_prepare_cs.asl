#include "hrp_common.asli"

[numthreads(1, 1, 1)] void
main_cs(uint32 thread_id sv_dispatch_thread_id)

{
	const gibs_data data = gibs::load_data();

	gibs::tile::alive_id_stack_curr(data).resize(0u);

	gibs::cell::alive_id_stack_curr(data).resize(0u);

	gibs::probe::alive_id_stack_curr(data).resize(0u);

	rw_stack<uint32> tile_surfel_alive_stack_prev = gibs::tile::alive_id_stack_prev(data);
	rw_stack<uint32> cell_surfel_alive_stack_prev = gibs::cell::alive_id_stack_prev(data);
	rw_stack<uint32> probe_alive_stack_prev		  = gibs::probe::alive_id_stack_prev(data);

	gibs::indirect_arg::set_update_tile_surfel_id_stack(data, uint32_3(ceil(tile_surfel_alive_stack_prev.size(), AGE_WAVE_SIZE), 1, 1));
	gibs::indirect_arg::set_update_cell_surfel_id_stack(data, uint32_3(ceil(cell_surfel_alive_stack_prev.size(), AGE_WAVE_SIZE), 1, 1));
	gibs::indirect_arg::set_update_surfel_probe_id_stack(data, uint32_3(ceil(probe_alive_stack_prev.size(), AGE_WAVE_SIZE), 1, 1));

	gibs::ray::reset_alloc(data);

	// reset by clear uav
	// gibs_reset_tile_surfel_alloc(data);
	// gibs_reset_cell_surfel_alloc(data);
}
#include "hrp_common.asli"

[numthreads(AGE_WAVE_SIZE, 1, 1)] void
main_cs(uint32 dispatch_thread_id sv_dispatch_thread_id)

{
	const gibs_data data = gibs::load_data();

	if (dispatch_thread_id < data.max_tile_surfel_count)
	{
		rw_stack<uint32> tile_surfel_dead_stack = gibs::tile::dead_id_stack(data);
		tile_surfel_dead_stack.resize(data.max_tile_surfel_count);
		tile_surfel_dead_stack.set(dispatch_thread_id, dispatch_thread_id);
	}

	if (dispatch_thread_id < data.max_cell_surfel_count)
	{
		rw_stack<uint32> cell_surfel_dead_stack = gibs::cell::dead_id_stack(data);
		cell_surfel_dead_stack.resize(data.max_cell_surfel_count);
		cell_surfel_dead_stack.set(dispatch_thread_id, dispatch_thread_id);
	}

	if (dispatch_thread_id < data.max_surfel_probe_count)
	{
		rw_stack<uint32> probe_dead_stack = gibs::probe::dead_id_stack(data);
		probe_dead_stack.resize(data.max_surfel_probe_count);
		probe_dead_stack.set(dispatch_thread_id, dispatch_thread_id);
	}

	if (dispatch_thread_id.x == 0)
	{
		gibs::tile::alive_id_stack_curr(data).resize(0u);
		gibs::tile::alive_id_stack_prev(data).resize(0u);

		gibs::cell::alive_id_stack_curr(data).resize(0u);
		gibs::cell::alive_id_stack_prev(data).resize(0u);

		gibs::probe::alive_id_stack_curr(data).resize(0u);
		gibs::probe::alive_id_stack_prev(data).resize(0u);
	}
}
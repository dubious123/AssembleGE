#include "hrp_common.asli"

[numthreads(1, 1, 1)] void
main_cs(uint32 thread_id sv_dispatch_thread_id)

{
	const gibs_data data = gibs_load_gibs_data();

	rw_stack<uint32> alive_stack_curr = gibs_load_alive_surfel_id_stack_curr(data);
	alive_stack_curr.resize(0u);

	rw_stack<uint32> alive_stack_prev = gibs_load_alive_surfel_id_stack_prev(data);

	gibs_set_indirect_arg_surfel_update(data, uint32_3(ceil(alive_stack_prev.size(), 32u), 1, 1));
	gibs_set_indirect_arg_ray_ideal_count_sum(data, uint32_3(ceil(ceil(alive_stack_prev.size(), 32u), GIBS_RAY_COUNT_REDUCE_EPG), 1, 1));

	gibs_reset_ray_alloc(data);

	// reset by clear uav
	// gibs_reset_tile_surfel_alloc(data);
	// gibs_reset_cell_surfel_alloc(data);
}
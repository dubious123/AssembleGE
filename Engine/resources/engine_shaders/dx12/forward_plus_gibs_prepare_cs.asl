#include "forward_plus_common.asli"

[numthreads(32, 1, 1)] void
main_cs(uint32 thread_id sv_dispatch_thread_id)

{
	const gibs_data data = gibs_load_gibs_data();

	rw_byte_array<gibs_cell_entry> cell_entry_arr = gibs_load_cell_entry_rw_arr(data);
	if (thread_id < data.cell_count_total)
	{
		cell_entry_arr.store(thread_id, gibs_cell_entry::init(0u, 0u));
	}

	const rw_byte_array<uint32> ray_count_arr			= gibs_load_surfel_ray_count_ideal_rw_arr(data);
	const rw_byte_array<uint32> ray_count_group_sum_arr = gibs_load_surfel_ray_count_prefix_rw_arr(data);
	if (thread_id < data.max_surfel_count)
	{
		ray_count_arr.store(thread_id, 0);
		ray_count_group_sum_arr.store(thread_id, 0);
	}

	if (thread_id == 0)
	{
		rw_stack<uint32> alive_stack_curr = gibs_load_alive_surfel_id_stack_curr(data);
		alive_stack_curr.resize(0u);

		gibs_reset_counter(data);
	}
}
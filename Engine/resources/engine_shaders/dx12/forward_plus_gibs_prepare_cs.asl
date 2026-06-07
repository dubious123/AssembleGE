#include "forward_plus_common.asli"

[numthreads(32, 1, 1)] void
main_cs(uint32 thread_id sv_dispatch_thread_id)

{
	const uint32 cell_id = thread_id;

	const gibs_data data = gibs_load_gibs_data();

	rw_byte_array<gibs_cell_entry> cell_entry_arr = gibs_load_cell_entry_rw_arr(data);

	cell_entry_arr.store(cell_id, gibs_cell_entry::init(0u, 0u));


	if (thread_id == 0)
	{
		rw_stack<uint32> alive_stack_curr = gibs_load_alive_surfel_id_stack_curr(data);
		alive_stack_curr.resize(0u);

		gibs_reset_counter(data);
	}
}
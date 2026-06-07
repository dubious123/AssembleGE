#include "forward_plus_common.asli"

[numthreads(32, 1, 1)] void
main_cs(uint32 surfel_id sv_dispatch_thread_id)

{
	const gibs_data data = gibs_load_gibs_data();

	if (surfel_id.x >= data.max_surfel_count) { return; }

	rw_stack<uint32> dead_stack = gibs_load_dead_surfel_id_stack(data);

	dead_stack.set(surfel_id, surfel_id);

	if (surfel_id.x == 0)
	{
		dead_stack.resize(data.max_surfel_count);
		rw_stack<uint32> alive_stack = gibs_load_alive_surfel_id_stack_curr(data);
		alive_stack.resize(0u);
	}
}
#include "hrp_common.asli"

wave_size(32)
[numthreads(32, 1, 1)] void
main_cs(uint32 group_id		   sv_group_id,
		uint32 group_thread_id sv_group_thread_id)

{
	aa::execute_ray_entry(false, group_id, group_thread_id);
}

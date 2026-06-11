#include "forward_plus_common.asli"

wave_size(32)
[numthreads(32, 1, 1)] void
main_cs(uint32 thread_id sv_dispatch_thread_id)

{
	const gibs_data		  data			 = gibs_load_gibs_data();
	rw_stack<uint32>	  alive_atack	 = gibs_load_alive_surfel_id_stack_curr(data);
	rw_byte_array<uint32> ray_offset_arr = gibs_load_surfel_ray_count_prefix_rw_arr(data);
	rw_byte_array<uint32> ray_count_arr	 = gibs_load_surfel_ray_count_ideal_rw_arr(data);
	const uint32		  alive_count	 = alive_atack.size();

	attr_branch()

	if (alive_count == 0)
	{
		return;
	}

	if (thread_id >= alive_count)
	{
		return;
	}

	const uint32 alive_id  = thread_id;
	const uint32 surfel_id = alive_atack[alive_id];

	const uint32 ray_offset = ray_offset_arr[alive_id];
	const uint32 ray_count	= ray_count_arr[alive_id];

	for (uint32 i = 0; i < ray_count; ++i)
	{
		gibs_ray_entry entry;
		entry.local_ray_id = i;
		entry.surfel_id	   = surfel_id;

		gibs_store_ray_entry(data, ray_offset + i, entry);
	}
}
#include "hrp_common.asli"

wave_size(AGE_WAVE_SIZE)
[numthreads(AGE_WAVE_SIZE, 1, 1)] void
main_cs(uint32 thread_id sv_dispatch_thread_id)

{
	const gist_data data = gist::load_data();

	byte_array<uint32> cell_alive_arr = gist::cell::alive_id_arr_curr(data);

	if (thread_id >= cell_alive_arr.size()) { return; }

	const uint32 alive_id	= thread_id;
	const uint32 ray_offset = gist::cell::ray_count_prefix_rw_arr(data)[alive_id];
	const uint32 ray_count	= gist::cell::ray_count_rw_arr(data)[alive_id];
	const uint32 surfel_id	= cell_alive_arr[alive_id];

	for (uint32 i = 0; i < ray_count; ++i)
	{
		gist_ray_entry entry;
		// entry.local_ray_id = i;
		entry.surfel_id = surfel_id;

		gist::ray::store_ray_entry(data, ray_offset + i, entry);
	}
}
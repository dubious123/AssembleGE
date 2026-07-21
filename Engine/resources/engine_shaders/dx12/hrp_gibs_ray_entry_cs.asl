#include "hrp_common.asli"

wave_size(AGE_WAVE_SIZE)
[numthreads(AGE_WAVE_SIZE, 1, 1)] void
main_cs(uint32 thread_id sv_dispatch_thread_id)

{
	const gibs_data data = gibs::load_data();

	byte_array<uint32> tile_alive_arr = gibs::tile::alive_id_arr_curr(data);
	byte_array<uint32> cell_alive_arr = gibs::cell::alive_id_arr_curr(data);

	const uint32 tile_surfel_count = tile_alive_arr.size();
	const uint32 cell_surfel_count = cell_alive_arr.size();

	const uint32 tile_surfel_ray_count_total = gibs::tile::ray_count_total_atomic_counter(data).value();

	uint32 ray_offset;
	uint32 ray_count;
	uint32 surfel_id;

	if (thread_id < tile_surfel_count)
	{
		uint32 alive_id = thread_id;
		ray_offset		= gibs::tile::ray_count_prefix_rw_arr(data)[alive_id];
		ray_count		= gibs::tile::ray_count_rw_arr(data)[alive_id];
		surfel_id		= tile_alive_arr[alive_id];
	}
	else if (thread_id - tile_surfel_count < cell_surfel_count)
	{
		uint32 alive_id = thread_id - tile_surfel_count;
		ray_offset		= gibs::cell::ray_count_prefix_rw_arr(data)[alive_id] + tile_surfel_ray_count_total;
		ray_count		= gibs::cell::ray_count_rw_arr(data)[alive_id];
		surfel_id		= cell_alive_arr[alive_id];
	}
	else
	{
		return;
	}

	for (uint32 i = 0; i < ray_count; ++i)
	{
		gibs_ray_entry entry;
		// entry.local_ray_id = i;
		entry.surfel_id = surfel_id;

		gibs::ray::store_ray_entry(data, ray_offset + i, entry);
	}
}
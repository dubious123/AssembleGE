#include "hrp_common.asli"

[numthreads(1, 1, 1)] void
main_cs()

{
	const gibs_data data = gibs::load_data();

	byte_array<uint32> tile_surfel_alive_id_arr_curr = gibs::tile::alive_id_arr_curr(data);
	byte_array<uint32> cell_surfel_alive_id_arr_curr = gibs::cell::alive_id_arr_curr(data);
	byte_array<uint32> probe_alive_id_arr_curr		 = gibs::probe::alive_id_arr_curr(data);

	gibs::indirect_arg::set_alloc_tile_surfel(data, uint32_3(ceil(tile_surfel_alive_id_arr_curr.size(), AGE_WAVE_SIZE), 1, 1));
	gibs::indirect_arg::set_alloc_cell_surfel(data, uint32_3(ceil(cell_surfel_alive_id_arr_curr.size(), AGE_WAVE_SIZE), 1, 1));
	gibs::indirect_arg::set_alloc_surfel_probe(data, uint32_3(ceil(probe_alive_id_arr_curr.size(), AGE_WAVE_SIZE), 1, 1));

	gibs::indirect_arg::set_update_cell_surfel(data, uint32_3(ceil(cell_surfel_alive_id_arr_curr.size(), AGE_WAVE_SIZE), 1, 1));
	gibs::indirect_arg::set_update_surfel_probe(data, uint32_3(ceil(probe_alive_id_arr_curr.size(), AGE_WAVE_SIZE), 1, 1));

	gibs::indirect_arg::set_tile_surfel_ideal_ray_count_reduce(data, uint32_3(ceil(ceil(tile_surfel_alive_id_arr_curr.size(), AGE_WAVE_SIZE), GIBS_RAY_COUNT_REDUCE_EPG), 1, 1));
	gibs::indirect_arg::set_cell_surfel_ideal_ray_count_reduce(data, uint32_3(ceil(ceil(cell_surfel_alive_id_arr_curr.size(), AGE_WAVE_SIZE), GIBS_RAY_COUNT_REDUCE_EPG), 1, 1));

	gibs::indirect_arg::set_tile_surfel_ray_count_prefix(data, uint32_3(ceil(tile_surfel_alive_id_arr_curr.size(), GIBS_RAY_COUNT_REDUCE_EPG), 1, 1));
	gibs::indirect_arg::set_cell_surfel_ray_count_prefix(data, uint32_3(ceil(cell_surfel_alive_id_arr_curr.size(), GIBS_RAY_COUNT_REDUCE_EPG), 1, 1));

	gibs::indirect_arg::set_ray_entry(data, uint32_3(ceil(tile_surfel_alive_id_arr_curr.size() + cell_surfel_alive_id_arr_curr.size(), AGE_WAVE_SIZE), 1, 1));

	gibs::indirect_arg::set_tile_surfel_scatter(data, uint32_3(ceil(tile_surfel_alive_id_arr_curr.size(), AGE_WAVE_SIZE), 1, 1));
	gibs::indirect_arg::set_cell_surfel_scatter(data, uint32_3(ceil(cell_surfel_alive_id_arr_curr.size(), AGE_WAVE_SIZE), 1, 1));
	gibs::indirect_arg::set_surfel_probe_scatter(data, uint32_3(ceil(probe_alive_id_arr_curr.size(), AGE_WAVE_SIZE), 1, 1));

	gibs::indirect_arg::set_surfel_probe_gather(data, uint32_3(ceil(probe_alive_id_arr_curr.size(), AGE_WAVE_SIZE), 1, 1));

	gibs::indirect_arg::set_ray_integrate(data, uint32_3(ceil(tile_surfel_alive_id_arr_curr.size() + cell_surfel_alive_id_arr_curr.size(), AGE_WAVE_SIZE), 1, 1));
	gibs::indirect_arg::set_build_cdf(data, uint32_3(ceil(tile_surfel_alive_id_arr_curr.size() + cell_surfel_alive_id_arr_curr.size(), AGE_WAVE_SIZE), 1, 1));
}
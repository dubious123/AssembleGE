#include "hrp_common.asli"

[numthreads(1, 1, 1)] void
main_cs()

{
	const gist_data data = gist::load_data();

	byte_array<uint32> cell_surfel_alive_id_arr_curr = gist::cell::alive_id_arr_curr(data);

	const uint32_2 adaptive_ray_entry_cap			= gist::adaptive::load_ray_entry_cap<false>(data);
	const uint32_2 adaptive_ray_entry_alloc_counter = gist::adaptive::load_ray_entry_alloc_counter<false>(data);
	const uint32   n_diffuse						= min(adaptive_ray_entry_alloc_counter.x, adaptive_ray_entry_cap.x);
	const uint32   n_specular						= min(adaptive_ray_entry_alloc_counter.y, adaptive_ray_entry_cap.y);

	const uint32 diffuse_ray_count	= gist::adaptive::diffuse_ray_count_total(data);
	const uint32 specular_ray_count = gist::adaptive::specular_ray_count_total(data);

	gist::indirect_arg::set_alloc_cell_surfel(data, uint32_3(ceil(cell_surfel_alive_id_arr_curr.size(), AGE_WAVE_SIZE), 1, 1));
	gist::indirect_arg::set_update_cell_surfel(data, uint32_3(ceil(cell_surfel_alive_id_arr_curr.size(), AGE_WAVE_SIZE), 1, 1));
	gist::indirect_arg::set_cell_surfel_ideal_ray_count_reduce(data, uint32_3(ceil(ceil(cell_surfel_alive_id_arr_curr.size(), AGE_WAVE_SIZE), GIST_GENERAL_EPG), 1, 1));
	gist::indirect_arg::set_cell_surfel_ray_count_prefix(data, uint32_3(ceil(cell_surfel_alive_id_arr_curr.size(), GIST_GENERAL_EPG), 1, 1));
	gist::indirect_arg::set_ray_entry(data, uint32_3(ceil(cell_surfel_alive_id_arr_curr.size(), AGE_WAVE_SIZE), 1, 1));
	gist::indirect_arg::set_cell_surfel_scatter(data, uint32_3(ceil(cell_surfel_alive_id_arr_curr.size(), AGE_WAVE_SIZE), 1, 1));
	gist::indirect_arg::set_ray_integrate(data, uint32_3(ceil(cell_surfel_alive_id_arr_curr.size(), AGE_WAVE_SIZE), 1, 1));
	gist::indirect_arg::set_build_cdf(data, uint32_3(cell_surfel_alive_id_arr_curr.size(), 1, 1));
	gist::indirect_arg::set_adaptive_ray_trace_diffuse(data, uint32_3(ceil(diffuse_ray_count, 64u), 1, 1));
	gist::indirect_arg::set_adaptive_ray_trace_specular(data, uint32_3(ceil(specular_ray_count, 64u), 1, 1));
	gist::indirect_arg::set_adaptive_gi_resolve_diffuse(data, uint32_3(ceil(diffuse_ray_count, 64u), 1, 1));
	gist::indirect_arg::set_adaptive_gi_resolve_specular(data, uint32_3(ceil(specular_ray_count, 64u), 1, 1));
}
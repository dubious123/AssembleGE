#include "hrp_common.asli"

wave_size(AGE_WAVE_SIZE)
[numthreads(AGE_WAVE_SIZE, 1, 1)] void
main_cs(uint32 alive_id sv_dispatch_thread_id)

{
	const gibs_data data = gibs::load_data();

	byte_array<uint32> alive_id_arr = gibs::cell::alive_id_arr_curr(data);

	if (alive_id >= alive_id_arr.size()) { return; }

	const uint32 surfel_id = alive_id_arr[alive_id];

	rw_structured_buffer<gibs_cell_surfel>		 surfel_buffer	   = global_resource_buffer[data.h_cell_surfel_buffer_uav_id];
	structured_buffer<gibs_cell_surfel_geometry> surfel_geo_buffer = global_resource_buffer[data.h_cell_surfel_geo_buffer_srv_id];

	gibs_cell_surfel				surfel	   = surfel_buffer[surfel_id];
	const gibs_cell_surfel_geometry surfel_geo = surfel_geo_buffer[surfel_id];

	gibs::update_world_space_surfel<gibs_cell_surfel_geometry, gibs_cell_surfel>(surfel_geo, surfel);

	surfel.recycle_data.next_frame();

	surfel.radius = gibs::calc_cell_surfel_radius(data, gibs::load_lut_data(), surfel.position);

	surfel_buffer[surfel_id] = surfel;
}
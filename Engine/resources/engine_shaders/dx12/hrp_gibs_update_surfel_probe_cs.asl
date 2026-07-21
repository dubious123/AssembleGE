#include "hrp_common.asli"

wave_size(AGE_WAVE_SIZE)
[numthreads(AGE_WAVE_SIZE, 1, 1)] void
main_cs(uint32 alive_id sv_dispatch_thread_id)

{
	const gibs_data data = gibs::load_data();

	byte_array<uint32> alive_id_arr = gibs::probe::alive_id_arr_curr(data);

	if (alive_id >= alive_id_arr.size()) { return; }

	const uint32 probe_id = alive_id_arr[alive_id];

	rw_structured_buffer<gibs_surfel_probe>		  probe_buffer		   = global_resource_buffer[data.h_surfel_probe_buffer_uav_id];
	rw_structured_buffer<gibs_recycle_data>		  probe_recycle_buffer = global_resource_buffer[data.h_surfel_probe_recycle_buffer_uav_id];
	structured_buffer<gibs_surfel_probe_geometry> probe_geo_buffer	   = global_resource_buffer[data.h_surfel_probe_geo_buffer_srv_id];

	gibs_surfel_probe				 probe		   = probe_buffer[probe_id];
	gibs_recycle_data				 probe_recycle = probe_recycle_buffer[probe_id];
	const gibs_surfel_probe_geometry probe_geo	   = probe_geo_buffer[probe_id];

	gibs::update_world_space_surfel<gibs_surfel_probe_geometry, gibs_surfel_probe>(probe_geo, probe);

	probe.surfel_radius = gibs::calc_cell_surfel_radius(data, gibs::load_lut_data(), probe.position);

	probe_recycle.next_frame();

	probe_buffer[probe_id]		   = probe;
	probe_recycle_buffer[probe_id] = probe_recycle;
}
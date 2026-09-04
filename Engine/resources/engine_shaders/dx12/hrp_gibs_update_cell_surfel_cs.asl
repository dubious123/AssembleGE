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

	{
		const mesh::surface_point_data surface_point = mesh::calc_surface_point(load_object_render_id(surfel_geo.object_id),
																				surfel_geo.primitive_id,
																				unorm16_2_to_float2(surfel_geo.barycentric_unorm16),
																				surfel.recycle_data.is_back_face());

		const pbr_surface_data pbr_surface = calc_pbr_surface(-surface_point.v.normal, surface_point.mat, surface_point.v);

		surfel.normal_oct_snorm16 = encode_oct_snorm16(surface_point.world_face_normal);
		surfel.position			  = surface_point.v.world_pos;
		surfel.radiance_r11g11b10 = encode_r11g11b10(calc_di<false>(pbr_surface, surface_point.world_face_normal) + calc_gi(pbr_surface, decode_r11g11b10(surfel.irradiance_r11g11b10)));
	}

	surfel.recycle_data.next_frame();

	surfel.radius = gibs::calc_cell_surfel_radius(data, gibs::load_lut_data(), surfel.position);

	surfel_buffer[surfel_id] = surfel;
}
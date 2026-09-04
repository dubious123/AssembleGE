#include "hrp_common.asli"

wave_size(AGE_WAVE_SIZE)
[numthreads(AGE_WAVE_SIZE, 1, 1)] void
main_cs(uint32 alive_id sv_dispatch_thread_id)

{
	const gist_data data = gist::load_data();

	byte_array<uint32> alive_id_arr = gist::cell::alive_id_arr_curr(data);

	if (alive_id >= alive_id_arr.size()) { return; }

	const uint32 surfel_id = alive_id_arr[alive_id];

	rw_structured_buffer<gist_cell_surfel>		 surfel_buffer	   = global_resource_buffer[data.h_cell_surfel_buffer_uav_id];
	structured_buffer<gist_cell_surfel_geometry> surfel_geo_buffer = global_resource_buffer[data.h_cell_surfel_geo_buffer_srv_id];

	gist_cell_surfel				surfel	   = surfel_buffer[surfel_id];
	const gist_cell_surfel_geometry surfel_geo = surfel_geo_buffer[surfel_id];

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

	attr_branch()

	if (gist::debug::freeze_cell_surfel_radius(data) is_false)
	{
		const half new_radius = gist::calc_cell_surfel_radius(data, gist::load_lut_data(), surfel.position);

		if (surfel.is_new_born() is_false and surfel.radius != new_radius)
		{
			const float ratio	 = float(surfel.radius) / max(float(new_radius), epsilon_1e6);
			const float ratio_sq = ratio * ratio;

			rw_byte_array<uint16> vis_arr = gist::cell::visibility_rw_arr(data, surfel_id);
			for (uint32 i = 0; i < data.atlas_texel_count(); ++i)
			{
				const uint16 chebyshev_packed = vis_arr[i];

				const float mean	= saturate(unorm8_to_float(uint32_x_to_uint8(chebyshev_packed)) * ratio);
				const float sq_mean = saturate(unorm8_to_float(uint32_y_to_uint8(chebyshev_packed)) * ratio_sq);

				vis_arr.store(i, uint16(float_to_unorm8(mean) | (float_to_unorm8(sq_mean) << 8u)));
			}
		}

		surfel.radius = new_radius;
	}

	surfel_buffer[surfel_id] = surfel;
}
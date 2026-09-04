#include "hrp_common.asli"

wave_size(AGE_WAVE_SIZE)
[numthreads(64, 1, 1)] void
main_cs(uint32 ray_id sv_dispatch_thread_id)

{
	const gist_data data = gist::load_data();

	const uint32 ray_count_total = data.tile_count_total()
								 + gist::cell::ray_count_total(data)
								 + gist::adaptive::ray_count_total(data);

	if (ray_id >= ray_count_total) { return; }

	structured_buffer<gist_ray_hit_result>		   ray_hit_buffer	   = global_resource_buffer[data.h_ray_hit_buffer_srv_id];
	rw_structured_buffer<gist_ray_lighting_result> ray_lighting_buffer = global_resource_buffer[data.h_ray_lighting_buffer_uav_id];

	const gist_ray_hit_result ray_hit = ray_hit_buffer[ray_id];

	float3 dir = decode_oct_snorm8(uint32_upper_to_uint16(ray_hit.dir_oct_snorm8));

	gist_ray_lighting_result res = zero<gist_ray_lighting_result>();

	if (ray_hit.distance == float_max)
	{
		res.radiance_r11g11b10		= encode_r11g11b10(calc_skybox_color(dir));
		ray_lighting_buffer[ray_id] = res;
		return;
	}

	if (ray_hit.object_render_id == invalid_id_uint32)
	{
		// opaque_ss or mask_ss back face
		res.radiance_r11g11b10		= encode_r11g11b10(zero<float3>());
		ray_lighting_buffer[ray_id] = res;
		return;
	}

	const object_render_data obj_render_data = load_object_render_data(ray_hit.object_render_id);
	const mesh_header		 msh_header		 = read_mesh_header(obj_render_data);
	const uint32			 submesh_id		 = mesh::calc_submesh_id_from_primitive(msh_header, ray_hit.primitive_id);
	const bool				 is_back_face	 = ray_hit.distance < 0;
	const bool				 is_blend		 = mesh::calc_rt_alpha_test_mode(msh_header, submesh_id, obj_render_data) == AGE_RT_ALPHA_TEST_MODE_BLEND;

	const mesh::surface_point_data surface_point = mesh::calc_surface_point(load_object_data(ray_hit.object_render_id),
																			obj_render_data,
																			msh_header,
																			submesh_id,
																			ray_hit.primitive_id,
																			unorm16_2_to_float2(ray_hit.barycentric_unorm16),
																			// ds opaque/mask back face
																			is_back_face and (is_blend is_false));
	if (is_back_face and is_blend)
	{
		// transparent back face
		dir = -dir;
	}

	const pbr_surface_data surface_data = calc_pbr_surface(dir, surface_point.mat, surface_point.v);

	const float3 di			= calc_di<false>(surface_data, surface_point.world_face_normal);
	const float3 irradiance = gist::sample_irradiance<true, true>(data, surface_data.world_pos, surface_point.world_face_normal, ray_id, ray_hit.object_render_id, ray_hit.primitive_id).xyz;
	const float3 gi			= calc_gi(surface_data, irradiance);

	res.radiance_r11g11b10		= encode_r11g11b10(di + gi);
	res.irradiance_r11g11b10	= encode_r11g11b10(irradiance);
	ray_lighting_buffer[ray_id] = res;
}

#include "hrp_common.asli"

wave_size(AGE_WAVE_SIZE)
[numthreads(64, 1, 1)] void
main_cs(uint32 ray_id sv_dispatch_thread_id)

{
	const gibs_data data = gibs::load_data();

	const uint32_2 ray_count_total = gibs::ray::count_total(data);

	if (ray_id >= ray_count_total.x + ray_count_total.y) { return; }

	structured_buffer<gibs_ray_hit_result>		   ray_hit_buffer	   = global_resource_buffer[data.h_ray_hit_buffer_srv_id];
	rw_structured_buffer<gibs_ray_lighting_result> ray_lighting_buffer = global_resource_buffer[data.h_ray_lighting_buffer_uav_id];

	const gibs_ray_hit_result ray_hit = ray_hit_buffer[ray_id];

	const bool is_back_face = ray_hit.distance < 0;

	float3 dir = decode_oct_snorm8(uint32_upper_to_uint16(ray_hit.dir_oct_snorm8));

	gibs_ray_lighting_result res;

	if (ray_hit.distance == float_max)
	{
		res.radiance_r11g11b10		= encode_r11g11b10(calc_skybox_color(dir));
		ray_lighting_buffer[ray_id] = res;
		return;
	}

	if (is_back_face and ray_hit.object_id == invalid_id_uint32)
	{
		// opaque back face

		res.radiance_r11g11b10		= encode_r11g11b10(zero<float3>());
		ray_lighting_buffer[ray_id] = res;
		return;
	}

	dir *= is_back_face ? -1.f : 1.f;

	const object_render_data render_data = load_object_render_data(ray_hit.object_id);
	const material			 mat		 = load_material(render_data.material_id);
	const object_data		 obj_data	 = load_object_data(ray_hit.object_id);
	const mesh_header		 msh_header	 = read_mesh_header<object_render_data>(render_data);
	const uint32_3			 prim_index	 = load_rt_triangle_index(render_data.rt_index_buffer_offset, ray_hit.primitive_id);

	const vertex_fat v0 = decode_vertex(msh_header, prim_index.x);
	const vertex_fat v1 = decode_vertex(msh_header, prim_index.y);
	const vertex_fat v2 = decode_vertex(msh_header, prim_index.z);

	const float2 barycentrics = float2(unorm16_to_float(ray_hit.barycentric_unorm16 & 0xffffu), unorm16_to_float(ray_hit.barycentric_unorm16 >> 16u));

	const float3 bary_weights = float3(1.f - barycentrics.x - barycentrics.y, barycentrics.x, barycentrics.y);

	const vertex_fat v = transform_vertex_to_world(interpolate_vertex_fat(v0, v1, v2, bary_weights), obj_data);

	const pbr_surface_data surface_data = calc_pbr_surface(dir, mat, v);

	const float3 local_face_normal = normalize(cross(v1.pos.xyz - v0.pos.xyz, v2.pos.xyz - v0.pos.xyz));

	const float3 world_face_normal = normalize(rotate(local_face_normal / cast<float3>(obj_data.scale), decode_quaternion(obj_data.quaternion)));

	const float3 di = calc_di<false>(surface_data, world_face_normal);

	const float3 irradiance = gibs::sample_irradiance(data, ray_id, surface_data.world_pos, world_face_normal, ray_hit.primitive_id);

	const float3 gi = calc_gi(surface_data, irradiance);

	res.radiance_r11g11b10		= encode_r11g11b10(di + gi);
	ray_lighting_buffer[ray_id] = res;
}
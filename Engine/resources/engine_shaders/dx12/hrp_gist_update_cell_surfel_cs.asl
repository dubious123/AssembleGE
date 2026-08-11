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
		const object_data		 obj		 = load_object_data(surfel_geo.object_id);
		const object_render_data render_data = load_object_render_data(surfel_geo.object_id);
		const material			 mat		 = load_material(render_data.material_id);
		const mesh_header		 msh_header	 = read_mesh_header<object_render_data>(render_data);

		const uint32_3 primitive_idx = load_rt_triangle_index(render_data.rt_index_buffer_offset, surfel_geo.primitive_id);
		const float2   barycentrics	 = float2(unorm16_to_float(surfel_geo.barycentric_unorm16 & 0xffff), unorm16_to_float((surfel_geo.barycentric_unorm16 >> 16u) & 0xffff));
		const float3   bary_weights	 = float3(1.f - barycentrics.x - barycentrics.y, barycentrics.x, barycentrics.y);

		const vertex_fat v0 = decode_vertex(msh_header, primitive_idx.x);
		const vertex_fat v1 = decode_vertex(msh_header, primitive_idx.y);
		const vertex_fat v2 = decode_vertex(msh_header, primitive_idx.z);

		const vertex_fat v = transform_vertex_to_world(interpolate_vertex_fat(v0, v1, v2, bary_weights), obj);

		const pbr_surface_data surface_data = calc_pbr_surface(-v.normal, mat, v);

		const float3 local_face_normal = normalize(cross(v1.pos.xyz - v0.pos.xyz, v2.pos.xyz - v0.pos.xyz));
		const float3 world_face_normal = normalize(rotate(local_face_normal / cast<float3>(obj.scale), decode_quaternion(obj.quaternion)));

		surfel.normal_oct_snorm16 = encode_oct_snorm16(world_face_normal);
		surfel.position			  = v.world_pos;
		surfel.radiance_r11g11b10 = encode_r11g11b10(calc_di<false>(surface_data, world_face_normal) + calc_gi(surface_data, decode_r11g11b10(surfel.irradiance_r11g11b10)));
	}

	surfel.recycle_data.next_frame();

	attr_branch()

	if (gist::debug::freeze_cell_surfel_radius(data) is_false)
	{
		surfel.radius = gist::calc_cell_surfel_radius(data, gist::load_lut_data(), surfel.position);
	}

	surfel_buffer[surfel_id] = surfel;
}
#include "hrp_common.asli"

wave_size(32)
[numthreads(16, 16, 1)] void
main_cs(uint32_3 thread_id sv_dispatch_thread_id)

{
	const int32_2 extent = cast<int32_2>(backbuffer_size);
	const int32_2 px	 = int32_2(thread_id.xy);

	if (any(thread_id.xy >= extent)) { return; }

	texture_2d<float>	 depth_buffer = global_resource_buffer[opaque_depth_buffer_srv_id];
	texture_2d<uint32_2> gbuffer	  = global_resource_buffer[opaque_gbuffer_srv_id];

	rw_texture_2d<float4> base_color_buffer		= global_resource_buffer[opaque_base_color_buffer_uav_id];
	rw_texture_2d<float2> mr_buffer				= global_resource_buffer[opaque_mr_buffer_uav_id];
	rw_texture_2d<float2> shading_normal_buffer = global_resource_buffer[opaque_shading_normal_buffer_uav_id];
	rw_texture_2d<float3> emissive_buffer		= global_resource_buffer[opaque_emissive_buffer_uav_id];

	const float z_depth = depth_buffer[px];

	if (z_depth == 0.f) { return; }

	const uint32 vis_packed = gbuffer[px].x;
	const uint32 render_id	= vis_packed & 0x01ffffff;
	const uint32 prim_id	= (vis_packed & 0xfe000000) >> (32u - 7u);

	const opaque_meshlet_render_data render_data = load_opaque_meshlet_render_data(render_id);
	const material					 mat		 = load_material(render_data.material_id);
	const object_data				 obj_data	 = load_object_data(render_data.object_id);
	const mesh_header				 mesh_header = read_mesh_header<opaque_meshlet_render_data>(render_data);
	const meshlet					 mshlt		 = read_meshlet(mesh_header, render_data.meshlet_id);

	uint32_3 meshlet_prim_idx = read_meshlet_primitive(mesh_header, mshlt, prim_id);

	const vertex_fat v0 = decode_vertex(mesh_header, read_global_vertex_index(mesh_header, mshlt, meshlet_prim_idx.x));
	const vertex_fat v1 = decode_vertex(mesh_header, read_global_vertex_index(mesh_header, mshlt, meshlet_prim_idx.y));
	const vertex_fat v2 = decode_vertex(mesh_header, read_global_vertex_index(mesh_header, mshlt, meshlet_prim_idx.z));

	const float3 world_pos	  = screen_px_to_world(px, z_depth, inv_backbuffer_size, view_proj_inv);
	const float3 local_pos	  = rotate_inv(obj_data.quaternion, world_pos - obj_data.pos) / obj_data.scale;
	const float3 barycentrics = calc_barycentric(local_pos, v0.pos.xyz, v1.pos.xyz, v2.pos.xyz);

	const vertex_fat v_local = interpolate_vertex_fat(v0, v1, v2, barycentrics);
	const vertex_fat v		 = transform_vertex_to_world(v_local, obj_data);

	const pbr_surface_data surface_data = calc_pbr_surface(normalize(v.world_pos - camera_pos), mat, v);

	base_color_buffer[px]	  = float4(linear_to_srgb(surface_data.base_color.rgb), surface_data.occlusion);
	mr_buffer[px]			  = float2(surface_data.metallic, surface_data.roughness);
	shading_normal_buffer[px] = encode_octahedral(surface_data.normal);
	emissive_buffer[px]		  = surface_data.emissive;
}

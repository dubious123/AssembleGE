#include "forward_plus_common.asli"

struct depth_ms_out
{
	float4 pos sys_val(SV_Position);
};

[numthreads(32, 1, 1)][output_topology("triangle")] void
main_ms(in payload opaque_as_to_ms ms_in,
		uint32_3				   group_id sv_group_id,
		uint32_3				   group_thread_id sv_group_thread_id,
		out vertices depth_ms_out  ms_out_vertex_arr[64],
		out indices uint32_3	   ms_out_triangle_arr[126]) {
	const uint32 meshlet_count_per_group			= 32;
	const uint32 not_culled_render_data_local_index = select32_nth_set_bit(ms_in.meshlet_alive_mask, group_id.x);
	const uint32 meshlet_render_data_id				= ms_in.meshlet_32_group_idx * meshlet_count_per_group + not_culled_render_data_local_index;

	const opaque_meshlet_render_data render_data = opaque_meshlet_render_data_buffer[meshlet_render_data_id];
	const mesh_header				 mesh_header = read_mesh_header(render_data.mesh_byte_offset);
	const meshlet					 mshlt		 = read_meshlet(mesh_header, render_data.meshlet_id);
	const object_data				 obj_data	 = object_data_buffer[render_data.object_id];

	const uint32 vertex_count	 = mshlt.vertex_count_prim_count_extra & 0xffu;
	const uint32 primitive_count = (mshlt.vertex_count_prim_count_extra >> 8u) & 0xffu;

	set_mesh_output_counts(vertex_count, primitive_count);

	const float4 quaternion = decode_quaternion(obj_data.quaternion);
	const float3 scale		= cast<float3>(obj_data.scale);
	const float3 pos		= obj_data.pos;

	expand(2)

	for (uint32 nth_vertex = group_thread_id.x; nth_vertex < vertex_count; nth_vertex += 32)
	{
		const uint32		 global_vertex_index = read_global_vertex_index(mesh_header, mshlt, nth_vertex);
		const vertex_encoded v_encoded			 = read_vertex_encoded(mesh_header, global_vertex_index);

		const float3 pos_local = (cast<float3>(v_encoded.pos)) / 65535.f
								   * mesh_header.aabb_size
							   + mesh_header.aabb_min;

		const float3 pos_world = rotate(pos_local * scale, quaternion) + pos;

		ms_out_vertex_arr[nth_vertex].pos = mul(view_proj, float4(pos_world, 1.0f));
	}

	expand(4)

	for (uint32 nth_primitive = group_thread_id.x; nth_primitive < primitive_count; nth_primitive += 32)
	{
		ms_out_triangle_arr[nth_primitive] = read_meshlet_primitive(mesh_header, mshlt, nth_primitive);
	}
}
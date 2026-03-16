#include "forward_plus_common.asli"

template <typename t>
t
decode_vertex(const vertex_encoded v_encoded, const float3 aabb_min, const float3 aabb_size)
{
	float3 pos_decoded = ((float3)v_encoded.pos) / 65535.f
						   * aabb_size
					   + aabb_min;

	float3 normal_decoded = decode_oct_snorm(v_encoded.normal_oct);

	float4 tangent_decoded = float4(
		decode_oct_snorm(v_encoded.tangent_oct),
		1.0f - 2.0f * float(v_encoded.extra & 1u));

	t res;

	res.pos		= float4(pos_decoded, 1.f);
	res.normal	= normal_decoded;
	res.tangent = tangent_decoded;
#if UV_COUNT >= 1
	res.uv0 = v_encoded.uv_set[0];
#endif
#if UV_COUNT >= 2
	res.uv1 = v_encoded.uv_set[1];
#endif
#if UV_COUNT >= 3
	res.uv2 = v_encoded.uv_set[2];
#endif
#if UV_COUNT >= 4
	res.uv3 = v_encoded.uv_set[3];
#endif

	return res;
}

[numthreads(32, 1, 1)][output_topology("triangle")] void
main_ms(
	in payload opaque_as_to_ms	 ms_in,
	uint32						 group_id sv_group_id,
	uint32						 group_thread_id sv_group_thread_id,
	out vertices opaque_ms_to_ps ms_out_vertex_arr[64],
	out indices uint32_3		 ms_out_triangle_arr[126])

{
	const uint32	  object_id		   = arg0;
	const uint32	  mesh_byte_offset = arg1;
	const mesh_header msh_header	   = read_mesh_header(mesh_byte_offset);
	const object_data obj_data		   = object_data_buffer[object_id];

	const uint32 meshlet_count_per_group			= 32;
	const uint32 not_culled_render_data_local_index = select32_nth_set_bit(ms_in.meshlet_alive_mask, group_id);
	const uint32 meshlet_idx						= ms_in.meshlet_32_group_idx * meshlet_count_per_group + not_culled_render_data_local_index;

	const meshlet mshlt = read_meshlet(msh_header, meshlet_idx);

	const uint32 vertex_count	 = mshlt.vertex_count_prim_count_extra & 0xffu;
	const uint32 primitive_count = (mshlt.vertex_count_prim_count_extra >> 8u) & 0xffu;

	set_mesh_output_counts(vertex_count, primitive_count);

	const float4 quaternion = decode_quaternion(obj_data.quaternion);
	const float3 scale		= cast<float3>(obj_data.scale);
	const float3 pos		= obj_data.pos;

	expand(2)

	for (uint32 nth_vertex = group_thread_id; nth_vertex < vertex_count; nth_vertex += 32)
	{
		const uint32		 global_vertex_index = read_global_vertex_index(msh_header, mshlt, nth_vertex);
		const vertex_encoded v_encoded			 = read_vertex_encoded(msh_header, global_vertex_index);

		const float3 aabb_min  = msh_header.aabb_min;
		const float3 aabb_size = msh_header.aabb_size;

		opaque_ms_to_ps v = decode_vertex<opaque_ms_to_ps>(v_encoded, aabb_min, aabb_size);

		v.pos.xyz = rotate(v.pos.xyz * scale, quaternion) + pos;

		v.world_pos = v.pos.xyz;

		v.pos = mul(view_proj, v.pos);

		v.normal = normalize(rotate(v.normal / scale, quaternion));

		const float3 t = normalize(rotate(v.tangent.xyz * scale, quaternion));

		v.tangent = float4(normalize(t - v.normal * dot(t, v.normal)), v.tangent.w * sign(scale.x * scale.y * scale.z));

		// v.meshlet_render_data_id = meshlet_idx;

		ms_out_vertex_arr[nth_vertex] = v;
	}

	expand(4)

	for (uint32 nth_primitive = group_thread_id; nth_primitive < primitive_count; nth_primitive += 32)
	{
		ms_out_triangle_arr[nth_primitive] = read_meshlet_primitive(msh_header, mshlt, nth_primitive);
	}
}
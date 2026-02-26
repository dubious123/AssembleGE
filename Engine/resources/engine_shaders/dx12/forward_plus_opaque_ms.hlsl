#define SHADER_STAGE_MS

#include "forward_plus_common.hlsli"

#undef SHADER_STAGE_MS

opaque_ms_to_ps
decode_vertex(const vertex_encoded v_encoded, const float3 aabb_min, const float3 aabb_size)
{
    float3 pos_decoded = ((float3)v_encoded.pos) / 65535.f
						  * aabb_size
						  + aabb_min;
    
    float3 normal_decoded = decode_oct_snorm(v_encoded.normal_oct);

    float4 tangent_decoded = float4(
		decode_oct_snorm(v_encoded.tangent_oct),
		1.0f - 2.0f * float(v_encoded.extra & 1u));
    
    opaque_ms_to_ps res;
    
    res.pos = float4(pos_decoded, 1.f);
    res.normal = normal_decoded;
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

[numthreads(32, 1, 1)]
[outputtopology("triangle")]
void
main_ms(
		in payload opaque_as_to_ms ms_in,
		uint32_3 group_id : SV_GroupID,
		uint32_3 group_thread_id : SV_GroupThreadID,
		out vertices opaque_ms_to_ps ms_out_vertex_arr[64],
		out indices uint3 ms_out_triangle_arr[126])
{
    const uint meshlet_count_per_group = 32;
    const uint not_culled_job_local_index = select32_nth_set_bit(ms_in.meshlet_alive_mask, group_id.x);
    const uint meshlet_render_job_id = ms_in.meshlet_32_group_idx * meshlet_count_per_group + not_culled_job_local_index;
    
    const job_data job = meshlet_render_job_buffer[meshlet_render_job_id];
    const mesh_header mesh_header = read_mesh_header(job.mesh_byte_offset);
    const meshlet mshlt = read_meshlet(mesh_header, job.meshlet_id);
    const object_data obj_data = object_data_buffer[job.object_id];
    
    const uint vertex_count = mshlt.vertex_count_prim_count_extra & 0xffu;
    const uint primitive_count = (mshlt.vertex_count_prim_count_extra >> 8u) & 0xffu;
    
    SetMeshOutputCounts(vertex_count, primitive_count);

    const float4 quaternion = decode_quaternion(obj_data.quaternion);
    const float3 scale = (float3)obj_data.scale;
    const float3 pos = obj_data.pos;
    
	[unroll(2)]
    for (uint nth_vertex = group_thread_id.x; nth_vertex < vertex_count; nth_vertex += 32)
    {
        const uint global_vertex_index = read_global_vertex_index(mesh_header, mshlt, nth_vertex);
        const vertex_encoded v_encoded = read_vertex_encoded(mesh_header, global_vertex_index);
        
        const float3 aabb_min = mesh_header.aabb_min;
        const float3 aabb_size = mesh_header.aabb_size;
        
        opaque_ms_to_ps v = decode_vertex(v_encoded, aabb_min, aabb_size);
        
        v.pos.xyz = rotate(v.pos.xyz * scale, quaternion) + pos;

        v.pos = mul(view_proj, v.pos);
        
        v.normal = normalize(rotate(v.normal / scale, quaternion));
        
        const float3 t = normalize(rotate(v.tangent.xyz * scale, quaternion));
        
        v.tangent = float4(normalize(t - v.normal * dot(t, v.normal)), v.tangent.w * sign(scale.x * scale.y * scale.z));

        v.meshlet_render_job_id = meshlet_render_job_id;
        
        ms_out_vertex_arr[nth_vertex] = v;
    }

	[unroll(4)]
    for (uint nth_primitive = group_thread_id.x; nth_primitive < primitive_count; nth_primitive += 32)
    {
        ms_out_triangle_arr[nth_primitive] = read_meshlet_primitive(mesh_header, mshlt, nth_primitive);
    }
}
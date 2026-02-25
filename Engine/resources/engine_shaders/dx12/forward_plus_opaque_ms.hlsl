#define SHADER_STAGE_MS

#include "forward_plus_common.hlsli"

#undef SHADER_STAGE_MS

// precondition:
//  - mask != 0
//  - n < countbits(mask)   (n은 0-based: 0번째 set bit, 1번째 set bit...)
// returns: 
// if mask = 1110, n = 2 => 1000 => 3
// if mask = 10100, n = 0 => 100 => 2
uint
select32_nth_set_bit(const uint mask, const uint n)
{
    uint b0 = (mask) & 0xFFu;
    uint b1 = (mask >> 8) & 0xFFu;
    uint b2 = (mask >> 16) & 0xFFu;
    uint b3 = (mask >> 24) & 0xFFu;

    uint c0 = countbits(b0);
    uint c1 = countbits(b1);
    uint c2 = countbits(b2);
    uint c3 = countbits(b3);

	// uint p0 = 0;
    uint p1 = c0;
    uint p2 = c0 + c1;
    uint p3 = c0 + c1 + c2;

    uint byte_idx = 0 + (n >= p1) + (n >= p2) + (n >= p3);
    
    uint p_arr[4] = {0u, p1, p2, p3};
    uint prefix = p_arr[byte_idx];

    uint local_n = n - prefix;

	// byte_mask cannot be 0 because n < countbits(mask)
    uint byte_mask = (mask >> (byte_idx * 8u)) & 0xFFu;

    uint pos_in_byte = firstbitlow(byte_mask);
	[unroll]
    for (uint i = 0; i < 8; ++i)
    {
        if (i < local_n)
        {
            byte_mask &= ~(1u << pos_in_byte);
            pos_in_byte = firstbitlow(byte_mask);
        }
    }

    return byte_idx * 8u + pos_in_byte;
}

t_opaque_ms_to_ps
decode_vertex(const t_vertex_encoded vertex_encoded, const float3 aabb_min, const float3 aabb_size)
{
    float3 pos_decoded = ((float3)vertex_encoded.pos) / 65535.f
						  * aabb_size
						  + aabb_min;
    
    float3 normal_decoded = decode_oct_snorm(vertex_encoded.normal_oct);

    float4 tangent_decoded = float4(
		decode_oct_snorm(vertex_encoded.tangent_oct),
		1.0f - 2.0f * float(vertex_encoded.extra & 1u));
    
    t_opaque_ms_to_ps res;
    
    res.pos = float4(pos_decoded, 1.f);
    res.normal = normal_decoded;
    res.tangent = tangent_decoded;
#if UV_COUNT >= 1
    res.uv0 = vertex_encoded.uv_set[0];
#endif
#if UV_COUNT >= 2
    res.uv1 = vertex_encoded.uv_set[1];
#endif
#if UV_COUNT >= 3
    res.uv2 = vertex_encoded.uv_set[2];
#endif
#if UV_COUNT >= 4
	res.uv3 = vertex_encoded.uv_set[3];
#endif
    
    return res;
}

[numthreads(32, 1, 1)]
[outputtopology("triangle")]
void
main_ms(
		in payload t_opaque_as_to_ms ms_in,
		uint3 group_id : SV_GroupID,
		uint3 group_thread_id : SV_GroupThreadID,
		out vertices t_opaque_ms_to_ps ms_out_vertex_arr[64],
		out indices uint3 ms_out_triangle_arr[126])
{
    const uint meshlet_count_per_group = 32;
    const uint not_culled_job_local_index = select32_nth_set_bit(ms_in.meshlet_alive_mask, group_id.x);
    const uint meshlet_render_job_id = ms_in.meshlet_32_group_idx * meshlet_count_per_group + not_culled_job_local_index;
    
    const t_meshlet_render_job meshlet_render_job = meshlet_render_job_buffer[meshlet_render_job_id];
    const t_mesh_header mesh_header = read_mesh_header(meshlet_render_job.mesh_byte_offset);
    const t_meshlet meshlet = read_meshlet(mesh_header, meshlet_render_job.meshlet_idx);
    const t_object_data object_data = object_data_buffer[meshlet_render_job.object_idx];
    
    const uint vertex_count = meshlet.vertex_count_prim_count_extra & 0xffu;
    const uint primitive_count = (meshlet.vertex_count_prim_count_extra >> 8u) & 0xffu;
    
    SetMeshOutputCounts(vertex_count, primitive_count);

    const float4 quaternion = decode_quaternion(object_data.quaternion);
    const float3 scale = (float3)object_data.scale;
    const float3 pos = object_data.pos;
    
	[unroll(2)]
    for (uint nth_vertex = group_thread_id.x; nth_vertex < vertex_count; nth_vertex += 32)
    {
        const uint global_vertex_index = read_global_vertex_index(mesh_header, meshlet, nth_vertex);
        const t_vertex_encoded vertex_encoded = read_vertex_encoded(mesh_header, global_vertex_index);
        
        const float3 aabb_min = mesh_header.aabb_min;
        const float3 aabb_size = mesh_header.aabb_size;
        
        t_opaque_ms_to_ps v = decode_vertex(vertex_encoded, aabb_min, aabb_size);
        
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
        ms_out_triangle_arr[nth_primitive] = read_meshlet_primitive(mesh_header, meshlet, nth_primitive);
    }
}

[numthreads(32, 1, 1)]
[outputtopology("triangle")]
void main_ms2(in payload t_opaque_as_to_ms ms_in,
              uint3 group_thread_id : SV_GroupThreadID,
              out vertices t_opaque_ms_to_ps vertices[3],
              out indices uint3 ms_out_triangle_arr[1])
{
    SetMeshOutputCounts(3, 1);
    ms_out_triangle_arr[0] = uint3(0, 1, 2);
    
    vertices[0].pos = float4(-0.5, -0.5, 0, 1);
    vertices[1].pos = float4(0.0, 0.5, 0, 1);
    vertices[2].pos = float4(0.5, -0.5, 0, 1);
        
        
    vertices[0].normal = float3(0, 0, 1);
    vertices[0].tangent = float4(0, 0, 0, 1);
    vertices[0].meshlet_render_job_id = 0;
        
    vertices[1].normal = float3(0, 0, 1);
    vertices[1].tangent = float4(0, 0, 0, 1);
    vertices[1].meshlet_render_job_id = 0;
        
    vertices[2].normal = float3(0, 0, 1);
    vertices[2].tangent = float4(0, 0, 0, 1);
    vertices[2].meshlet_render_job_id = 0;
}
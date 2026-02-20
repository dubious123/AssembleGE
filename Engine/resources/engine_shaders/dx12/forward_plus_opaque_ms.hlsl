#define SHADER_STAGE_MS

#include "forward_plus_common.hlsli"

#undef SHADER_STAGE_MS

// precondition:
//  - mask != 0
//  - n < countbits(mask)   (n은 0-based: 0번째 set bit, 1번째 set bit...)
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

    uint byte = 0 + (n >= p1) + (n >= p2) + (n >= p3);

    uint prefix =
		(byte == 0) ? 0u : (byte == 1) ? p1
					   : (byte == 2) ? p2
									   : p3;

    uint local_n = n - prefix;

	// byte_mask cannot be 0 because n < countbits(mask)
    uint byte_mask = (mask >> (byte * 8u)) & 0xFFu;

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

    return byte * 8u + pos_in_byte;
}

uint3
decode_primitive(const uint primitive_offset, const uint nth_primitive)
{
	// if offset = 4n + i (i = 0,1,2,3)
	// load [4n, 4n+1, 4n+2, 4n+3], [4(n+1), 4(n+1)+1, 4(n+1)+2, 4(n+1)+ 3]

    const uint raw_offset = primitive_offset + nth_primitive * 3;
    const uint load_offset = raw_offset & ~0x3u;
    const uint bit_shift = (raw_offset & 0x3u) * 8;

    const uint2 raw = meshlet_primitive_index_buffer.Load2(load_offset);
    const uint64_t raw64 = (uint64_t(raw.y) << 32) | uint64_t(raw.x);

    return uint3(
		(raw64 >> (bit_shift + 0)) & 0xff,
		(raw64 >> (bit_shift + 8)) & 0xff,
		(raw64 >> (bit_shift + 16)) & 0xff);
}

void
decode_vertex(out t_ms_to_ps res, const uint vertex_index, const vector<int16_t, 3> aabb_min, const vector<uint16_t, 3> aabb_size)
{
    const t_vertex_encoded vertex_encoded = vertex_buffer[vertex_index];

    float3 pos_decoded = ((float3)vertex_encoded.pos) / 65535.f
							  * (float3)aabb_size
						  + (float3)aabb_min;
    float3 normal_decoded = decode_oct_snorm(vertex_encoded.normal_oct);

    float4 tangent_decoded = float4(
		decode_oct_snorm(vertex_encoded.tangent_oct),
		1.0f - 2.0f * float(vertex_encoded.extra & 1u));

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
}

[numthreads(32, 1, 1)]
[outputtopology("triangle")]
void
main_ms(
		in payload t_as_to_ms ms_in,
		uint3 group_id : SV_GroupID,
		uint3 group_thread_id : SV_GroupThreadID,
		out vertices t_ms_to_ps vertex_arr[64],
		out indices uint3 triangle_arr[126])
{
    const uint meshlet_count_per_group = 32;
    const uint meshlet_index = ms_in.meshlet_32_group_idx * meshlet_count_per_group + select32_nth_set_bit(ms_in.meshlet_alive_mask, group_id.x);

    const t_meshlet meshlet = meshlet_buffer[meshlet_index];
    const uint vertex_count = meshlet.vertex_count_prim_count_extra & 0xffu;
    const uint primitive_count = (meshlet.vertex_count_prim_count_extra >> 8) & 0xffu;

    SetMeshOutputCounts(vertex_count, primitive_count);

    const t_object_data object_data = object_data_buffer[job_data_buffer[meshlet_index].object_idx];

    t_transform transform_encoded = object_data.transform;
    const float4 quaternion = decode_quaternion(transform_encoded.quaternion);
    const float3 scale = (float3)transform_encoded.scale;
    const float3 pos = transform_encoded.pos;

	[unroll(2)]
    for (uint nth_vertex = group_thread_id.x; nth_vertex < vertex_count; nth_vertex += 32)
    {
        const uint global_index = meshlet_global_index_buffer[meshlet.global_index_offset + nth_vertex];
        
        t_ms_to_ps v;
        decode_vertex(v, global_index, meshlet.aabb_min, meshlet.aabb_size);

        v.pos.xyz = rotate(v.pos.xyz * scale, quaternion) + pos;
        v.pos = mul(view_proj, v.pos);
        
        v.normal = normalize(rotate(v.normal / scale, quaternion));
        
        const float3 t = normalize(rotate(v.tangent.xyz * scale, quaternion));
        
        v.tangent = float4(normalize(t - v.normal * dot(t, v.normal)), v.tangent.w * sign(scale.x * scale.y * scale.z));

        vertex_arr[nth_vertex] = v;
    }

	[unroll(4)]
    for (uint nth_primitive = group_thread_id.x; nth_primitive < primitive_count; nth_primitive += 32)
    {
        triangle_arr[nth_primitive] = decode_primitive(meshlet.primitive_offset, nth_primitive);
    }
}
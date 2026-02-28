#ifndef AGE_FORWARD_PLUS_COMMON_HLSLI
#define AGE_FORWARD_PLUS_COMMON_HLSLI

#include "age_hlsl.h"
#include "age_graphics_backend_dx12_render_pipeline_forward_plus_shared_types.h"

StructuredBuffer<job_data> meshlet_render_job_buffer : register(t0);
StructuredBuffer<object_data> object_data_buffer : register(t1);
ByteAddressBuffer mesh_data_buffer : register(t2);
SamplerState linear_clamp_sampler : register(s0);

mesh_header
read_mesh_header(uint32 mesh_byte_offset)
{
    mesh_header res;
    
    res.vertex_buffer_offset = 44 + mesh_byte_offset;
    {
        uint32_4 raw4 = mesh_data_buffer.Load4(mesh_byte_offset);
        res.global_vertex_index_buffer_offset = mesh_byte_offset + raw4.x;
        res.local_vertex_index_buffer_offset = mesh_byte_offset + raw4.y;
        res.meshlet_header_buffer_offset = mesh_byte_offset + raw4.z;
        res.meshlet_buffer_offset = mesh_byte_offset + raw4.w;
    }
    
    {
        uint32_4 raw4 = mesh_data_buffer.Load4(mesh_byte_offset + sizeof(uint32_4));
        res.meshlet_count = raw4.x;
        res.aabb_min.x = asfloat(raw4.y);
        res.aabb_min.y = asfloat(raw4.z);
        res.aabb_min.z = asfloat(raw4.w);
    }
    
    {
        uint32_4 raw4 = mesh_data_buffer.Load4(mesh_byte_offset + sizeof(uint32_4) * 2);
        res.aabb_size.x = asfloat(raw4.x);
        res.aabb_size.y = asfloat(raw4.y);
        res.aabb_size.z = asfloat(raw4.z);
    }
    
    return res;
}


meshlet_header
read_meshlet_header(mesh_header header, uint32 meshlet_idx)
{
    meshlet_header res;
    
    //    uint32 raw = mesh_data_buffer.Load(header.meshlet_header_buffer_offset + meshlet_idx * sizeof(meshlet_header));
    
    //res.cone_axis_oct = uint32_lower_to_uint16(raw);
    //res.cone_cull_cutoff_and_offset = uint32_upper_to_uint16(raw);

    uint32_4 raw4 = mesh_data_buffer.Load4(header.meshlet_header_buffer_offset + meshlet_idx * sizeof(meshlet_header));

    
    res.cone_axis_oct = uint32_lower_to_uint16(raw4.x);
    res.cone_cull_cutoff_and_extra = uint32_upper_to_uint16(raw4.x);
    
    res.aabb_min = int16_3(
        uint32_lower_to_int16(raw4.y),
        uint32_upper_to_int16(raw4.y),
        uint32_lower_to_int16(raw4.z));
    
    res.aabb_size = uint16_3(
        uint32_upper_to_uint16(raw4.z),
        uint32_lower_to_uint16(raw4.w),
        uint32_upper_to_uint16(raw4.w));
    
    return res;
}

meshlet
read_meshlet(mesh_header header, uint32 meshlet_idx)
{
    meshlet res;
    
    //uint32_3 raw1 = mesh_data_buffer.Load3(header.meshlet_buffer_offset + meshlet_idx * sizeof(meshlet));
    uint32_3 raw1 = mesh_data_buffer.Load3(header.meshlet_buffer_offset + meshlet_idx * sizeof(meshlet));
    // uint32_4 raw1 = mesh_data_buffer.Load4(header.meshlet_buffer_offset + meshlet_idx * 16);
    //uint32_4 raw1 = mesh_data_buffer.Load4(header.meshlet_buffer_offset + meshlet_idx * sizeof(meshlet));
    //uint32_2 raw2 = mesh_data_buffer.Load2(header.meshlet_buffer_offset + meshlet_idx * sizeof(meshlet) + sizeof(raw1));
    
    res.global_index_offset = raw1.x;
    res.primitive_offset = raw1.y;
    res.vertex_count_prim_count_extra = raw1.z;
    //res.aabb_min = int16_3(
    //    uint32_lower_to_int16(raw1.w),
    //    uint32_upper_to_int16(raw1.w),
    //    uint32_lower_to_int16(raw2.x));
    
    //res.aabb_size = uint16_3(
    //    uint32_upper_to_uint16(raw2.x),
    //    uint32_lower_to_uint16(raw2.y),
    //    uint32_upper_to_uint16(raw2.y));
    
    return res;
}

// meshlet local vertex_idx : 0 ~ 64
uint32
read_global_vertex_index(mesh_header header, meshlet mshlt, uint32 vertex_idx)
{
    return mesh_data_buffer.Load(header.global_vertex_index_buffer_offset + (mshlt.global_index_offset + vertex_idx) * sizeof(uint32));
}

vertex_encoded
read_vertex_encoded(mesh_header header, uint32 global_vertex_idx)
{
    vertex_encoded res;
    uint32 offset = header.vertex_buffer_offset + global_vertex_idx * sizeof(vertex_encoded);
    
    uint32_3 raw3 = mesh_data_buffer.Load3(offset);
    
    res.pos = uint16_t3(
        uint32_lower_to_uint16(raw3.x),
        uint32_upper_to_uint16(raw3.x),
        uint32_lower_to_uint16(raw3.y));
    
    res.normal_oct = uint32_upper_to_uint16(raw3.y);
    res.tangent_oct = uint32_lower_to_uint16(raw3.z);
    res.extra = uint32_upper_to_uint16(raw3.z);
    
    offset += sizeof(raw3);
#if UV_COUNT >= 1
    {
        uint32 raw = mesh_data_buffer.Load(offset);
        res.uv_set[0] = uint32_to_half2(raw);
        offset += sizeof(half2);
    }
#endif
#if UV_COUNT >= 2
    {
        uint32 raw = mesh_data_buffer.Load(offset);
        res.uv_set[1] = uint32_to_half2(raw);
        offset += sizeof(half2);
    }
#endif
#if UV_COUNT >= 3
    {
        uint32 raw = mesh_data_buffer.Load(offset);
        res.uv_set[2] = uint32_to_half2(raw);
        offset += sizeof(half2);
    }
#endif
#if UV_COUNT >= 4
    {
        uint32 raw = mesh_data_buffer.Load(offset);
        res.uv_set[3] = uint32_to_half2(raw);
        offset += sizeof(half2);
    }
#endif
    return res;
}

// meshlet local primitive_idx : 0 ~ 126
uint32_3
read_meshlet_primitive(mesh_header header, meshlet mshlt, uint32 primitive_idx)
{
    const uint32 load_pos_1byte_aligned = header.local_vertex_index_buffer_offset + mshlt.primitive_offset + primitive_idx * 3;
    const uint32 load_pos_4bytes_aligned = load_pos_1byte_aligned & ~0x3u;
    const uint32 bit_shift_required = (load_pos_1byte_aligned & 0x3u) * 8;

    const uint32_2 raw = mesh_data_buffer.Load2(load_pos_4bytes_aligned);
    const uint64 raw64 = (uint64_t(raw.y) << 32) | uint64_t(raw.x);

    return uint32_3(
		(raw64 >> (bit_shift_required + 0)) & 0xff,
		(raw64 >> (bit_shift_required + 8)) & 0xff,
		(raw64 >> (bit_shift_required + 16)) & 0xff);
}


// opaque
struct opaque_as_to_ms
{
    uint32 meshlet_32_group_idx;
    uint32 meshlet_alive_mask;
};

struct opaque_ms_to_ps
{
    float4 pos	        : SV_Position;
    float3 normal       : NORMAL;
    nointerpolation uint meshlet_render_job_id  : MESHLET_INDEX;
    float4 tangent      : TANGENT;
 
#if UV_COUNT >= 1
    half2 uv0 SYS_VAL(TEXCOORD0);
#endif
#if UV_COUNT >= 2
    half2 uv1 SYS_VAL(TEXCOORD1);
#endif
#if UV_COUNT >= 3
		half2 uv2 SYS_VAL(TEXCOORD2);
#endif
#if UV_COUNT >= 4
		half2 uv3 SYS_VAL(TEXCOORD3);
#endif
};

// presentation

struct presentation_ps_out
{
    float4 color : SV_Target0;
};

#endif
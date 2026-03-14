#ifndef AGE_FORWARD_PLUS_COMMON_HLSLI
#define AGE_FORWARD_PLUS_COMMON_HLSLI

#include "age_hlsl.h"
#include "age_graphics_backend_dx12_render_pipeline_forward_plus_shared_types.h"

// global
StructuredBuffer<opaque_meshlet_render_data> opaque_meshlet_render_data_buffer : register(t0, space0);
StructuredBuffer<object_data> object_data_buffer : register(t1, space0);
ByteAddressBuffer mesh_data_buffer : register(t2, space0);

StructuredBuffer<directional_light> directional_light_buffer : register(t3, space0);
StructuredBuffer<unified_light> unified_light_buffer : register(t4, space0);

StructuredBuffer<frame_data_rw> frame_data_rw_buffer_srv : register(t5, space0);
RWStructuredBuffer<frame_data_rw> frame_data_rw_buffer_uav : register(u5, space0);


// shadow
StructuredBuffer<shadow_light_header> shadow_light_header_buffer : register(t0, space1);

StructuredBuffer<shadow_light> shadow_light_buffer_srv : register(t1, space1);
RWStructuredBuffer<shadow_light> shadow_light_buffer_uav : register(u1, space1);



// light culling
StructuredBuffer<uint32> sort_buffer_srv : register(t0, space2);
RWStructuredBuffer<uint32> sort_buffer : register(u0, space2);

StructuredBuffer<zbin_entry> zbin_buffer_srv : register(t1, space2);
RWStructuredBuffer<zbin_entry> zbin_buffer_uav : register(u1, space2);

StructuredBuffer<uint32> tile_mask_buffer_srv : register(t2, space2);
RWStructuredBuffer<uint32> tile_mask_buffer_uav : register(u2, space2);

StructuredBuffer<unified_light> unified_sorted_light_buffer_srv : register(t3, space2);
RWStructuredBuffer<unified_light> unified_sorted_light_buffer_uav : register(u3, space2);

StructuredBuffer<transparent_object_render_data> transparent_object_render_data_buffer : register(t0, space3);

// debug
RWStructuredBuffer<debug_77> debug_buffer : register(u7, space7);


// sampler
SamplerState linear_clamp_sampler : register(s0);
SamplerComparisonState shadow_sampler : register(s1);

uint32 depth_to_bin(float linear_depth)
{
    float t = (linear_depth - cam_near_z) / (cam_far_z - cam_near_z);
    return (uint32)(t * (Z_SLICE_COUNT - 1));
}

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
    
    uint32_3 raw1 = mesh_data_buffer.Load3(header.meshlet_buffer_offset + meshlet_idx * sizeof(meshlet));

    res.global_index_offset = raw1.x;
    res.primitive_offset = raw1.y;
    res.vertex_count_prim_count_extra = raw1.z;
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

float safe_dot_camera_forward(float3 dir)
{
    return max(dot(dir, camera_forward), epsilon_1e4);
}

// opaque
struct opaque_as_to_ms
{
    uint32 meshlet_32_group_idx;
    uint32 meshlet_alive_mask;
};

struct opaque_ms_to_ps
{
    float4 pos	                                   : SV_Position;
    float3 world_pos                               : WORLD_POS;
    float3 normal                                  : NORMAL;
    // nointerpolation uint32 meshlet_render_data_id  : MESHLET_INDEX;
    float4 tangent                                 : TANGENT;
 
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




// light culling tile mask
struct tile_mask_ms_to_ps
{
    float4 pos : SV_Position;
    nointerpolation uint32 sorted_id : SORTED_ID;
};

// presentation

struct presentation_ps_out
{
    float4 color : SV_Target0;
};

#endif
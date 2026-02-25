#ifndef AGE_FORWARD_PLUS_COMMON_HLSLI
#define AGE_FORWARD_PLUS_COMMON_HLSLI

#define UV_COUNT 2

#if UV_COUNT == 1
#define UV_FIELDS half2 uv0 : TEXCOORD0;
#elif UV_COUNT == 2
#define UV_FIELDS          \
		half2 uv0 : TEXCOORD0; \
		half2 uv1 : TEXCOORD1;
#elif UV_COUNT == 3
#define UV_FIELDS          \
		half2 uv0 : TEXCOORD0; \
		half2 uv1 : TEXCOORD1; \
		half2 uv2 : TEXCOORD2;
#elif UV_COUNT == 4
#define UV_FIELDS          \
		half2 uv0 : TEXCOORD0; \
		half2 uv1 : TEXCOORD1; \
		half2 uv2 : TEXCOORD2; \
		half2 uv3 : TEXCOORD3;
#endif

static const float sqrt_2 = 1.41421356f;
static const float sqrt_2_inv = 0.70710678f;

float int8_to_float(uint raw)
{
    return float(int((raw & 0xffu) << 24) >> 24);
}

float
snorm8_to_float(int raw)
{
    return ((raw << 24) >> 24) / 127.f;
}

float
unorm8_to_float(uint raw)
{
    return (raw & 0xffu) / 255.f;
}

int16_t
uint32_upper_to_int16(uint u)
{
    return int16_t(u >> 16);
}

int16_t
uint32_lower_to_int16(uint u)
{
    return int16_t(u & 0xffff);
}

uint16_t
uint32_upper_to_uint16(uint u)
{
    return uint16_t(u >> 16);
}

uint16_t
uint32_lower_to_uint16(uint u)
{
    return uint16_t(u & 0xffff);
}

half
uint32_lower_to_half(uint u)
{
    return asfloat16(uint32_lower_to_uint16(u));
}

half
uint32_upper_to_half(uint u)
{
    return asfloat16(uint32_upper_to_uint16(u));
}

half2
uint32_to_half2(uint u)
{
    return half2(uint32_lower_to_half(u), uint32_upper_to_half(u));
}

// 10-10-10-2 Smallest Three Quaternion decoding
float4
decode_quaternion(uint encoded_q)
{
    uint index = (encoded_q >> 30) & 0x3;
    float3 c = float3(
		(float)((encoded_q >> 0) & 0x3ff), // x
		(float)((encoded_q >> 10) & 0x3ff), // y
		(float)((encoded_q >> 20) & 0x3ff)); // z

    c = c / 1023.f * sqrt_2 - sqrt_2_inv;

    float missing = sqrt(saturate(1.f - dot(c, c)));

    switch (index)
    {
        case 0:
            return float4(missing, c.x, c.y, c.z);
        case 1:
            return float4(c.x, missing, c.y, c.z);
        case 2:
            return float4(c.x, c.y, missing, c.z);
        default: // case 3
            return float4(c.x, c.y, c.z, missing);
    }
}

float3
rotate(float3 v, float4 q)
{
	// v' = v + 2*cross(q.xyz, cross(q.xyz, v) + q.w*v)
    float3 t = 2.0f * cross(q.xyz, v);
    return v + q.w * t + cross(q.xyz, t);
}

float3
decode_oct_snorm(const uint16_t oct)
{
    const float x = snorm8_to_float(oct & 0xffu);
    const float y = snorm8_to_float((oct >> 8) & 0xffu);

    float3 res;
    res.xy = float2(x, y);
    res.z = 1.f - abs(res.x) - abs(res.y);

    if (res.z < 0.f)
    {
        const float old_x = res.x;
        const float old_y = res.y;

        res.x = (1.f - abs(old_y)) * (old_x >= 0.f ? 1.f : -1.f);
        res.y = (1.f - abs(old_x)) * (old_y >= 0.f ? 1.f : -1.f);
    }

    return res;
}

float3
decode_oct_unorm(const uint16_t oct)
{
    const float x = unorm8_to_float(oct & 0xffu);
    const float y = unorm8_to_float((oct >> 8) & 0xffu);

    float3 res;
    res.xy = float2(x, y) * 2.f - 1.f;
    res.z = 1.f - abs(res.x) - abs(res.y);

    if (res.z < 0.f)
    {
        const float old_x = res.x;
        const float old_y = res.y;

        res.x = (1.f - abs(old_y)) * (old_x >= 0.f ? 1.f : -1.f);
        res.y = (1.f - abs(old_x)) * (old_y >= 0.f ? 1.f : -1.f);
    }

    return res;
}

// data only used by amplification shader
struct t_meshlet_header
{
    uint16_t cone_axis_oct;
    uint16_t cone_cull_cutoff_and_offset;
};

struct t_meshlet
{
    uint global_index_offset; // 4 bytes
    uint primitive_offset; // 4 bytes

    uint vertex_count_prim_count_extra; // [vertex_count(8bit)][primitive_count(8bit)][extra(16bit)]

    int16_t3 aabb_min; // 6byte
    uint16_t3 aabb_size; // 6byte 
};

struct t_vertex_encoded
{
    uint16_t3 pos;
    uint16_t normal_oct;
    uint16_t tangent_oct;
    uint16_t extra;
#if UV_COUNT > 0
    half2 uv_set[UV_COUNT];
#endif
};

struct t_vertex_decoded
{
    float4 pos	   : SV_Position;
    float3 normal  : NORMAL;
    float4 tangent : TANGENT;
	UV_FIELDS
};

struct t_transform
{
    float3 pos;
    uint quaternion; // 10 10 10 2
    half3 scale;
};

struct t_object_data
{
    float3 pos;
    uint quaternion; // 10 10 10 2
    half3 scale;
    //uint16_t instance_idx; // 2
    //uint16_t asset_idx; // 2
    uint16_t extra; // 2
};

struct t_meshlet_render_job
{
    uint object_idx;
    uint mesh_byte_offset;
    uint meshlet_idx;
};

cbuffer frame_data : register(b0)
{
    row_major float4x4 view_proj; // 64 bytes
    row_major float4x4 view_proj_inv; // 64 bytes
    float3 camera_pos; // 12 
    float time; // 4 
    float4 frustum_planes[6]; // 96
    uint frame_index; // 4
    float2 inv_backbuffer_size; // 8
    uint main_buffer_texture_id; // 4
                                    // total: 256 bytes 
};

StructuredBuffer<t_meshlet_render_job> meshlet_render_job_buffer : register(t0);
StructuredBuffer<t_object_data> object_data_buffer : register(t1);
ByteAddressBuffer mesh_data_buffer : register(t2);

// mesh header
struct t_mesh_header
{
	// uint32 vertex_offset = sizeof(mesh_baked_header), sizeof(mesh_baked_header) == 20
    uint vertex_buffer_offset; // not from gpu, calculated from read_mesh_header function
    
    uint global_vertex_index_buffer_offset;
    uint local_vertex_index_buffer_offset;
    uint meshlet_header_buffer_offset;
    uint meshlet_buffer_offset;
    uint meshlet_count;
    float3 aabb_min;
    float3 aabb_size;
};

t_mesh_header
read_mesh_header(uint mesh_byte_offset)
{
    t_mesh_header res;
    
    res.vertex_buffer_offset = 44 + mesh_byte_offset;
    {
        uint4 raw4 = mesh_data_buffer.Load4(mesh_byte_offset);
        res.global_vertex_index_buffer_offset = mesh_byte_offset + raw4.x;
        res.local_vertex_index_buffer_offset = mesh_byte_offset + raw4.y;
        res.meshlet_header_buffer_offset = mesh_byte_offset + raw4.z;
        res.meshlet_buffer_offset = mesh_byte_offset + raw4.w;
    }
    
    {
        uint4 raw4 = mesh_data_buffer.Load4(mesh_byte_offset + sizeof(uint4));
        res.meshlet_count = raw4.x;
        res.aabb_min.x = asfloat(raw4.y);
        res.aabb_min.y = asfloat(raw4.z);
        res.aabb_min.z = asfloat(raw4.w);
    }
    
    {
        uint4 raw4 = mesh_data_buffer.Load4(mesh_byte_offset + sizeof(uint4) * 2);
        res.aabb_size.x = asfloat(raw4.x);
        res.aabb_size.y = asfloat(raw4.y);
        res.aabb_size.z = asfloat(raw4.z);
    }
    
    return res;
}


t_meshlet_header
read_meshlet_header(t_mesh_header header, uint meshlet_idx)
{
    t_meshlet_header res;

    uint raw = mesh_data_buffer.Load(header.meshlet_header_buffer_offset + meshlet_idx * sizeof(t_meshlet_header));
    
    res.cone_axis_oct = uint32_lower_to_uint16(raw);
    res.cone_cull_cutoff_and_offset = uint32_upper_to_uint16(raw);
    
    return res;
}

t_meshlet
read_meshlet(t_mesh_header header, uint meshlet_idx)
{
    t_meshlet res;
    
    uint4 raw1 = mesh_data_buffer.Load4(header.meshlet_buffer_offset + meshlet_idx * sizeof(t_meshlet));
    uint2 raw2 = mesh_data_buffer.Load2(header.meshlet_buffer_offset + meshlet_idx * sizeof(t_meshlet) + sizeof(uint4));
    
    res.global_index_offset = raw1.x;
    res.primitive_offset = raw1.y;
    res.vertex_count_prim_count_extra = raw1.z;
    res.aabb_min = int16_t3(
        uint32_lower_to_int16(raw1.w),
        uint32_upper_to_int16(raw1.w),
        uint32_lower_to_int16(raw2.x));
    
    res.aabb_size = uint16_t3(
        uint32_upper_to_uint16(raw2.x),
        uint32_lower_to_uint16(raw2.y),
        uint32_upper_to_uint16(raw2.y));
    
    return res;
}

// meshlet local vertex_idx : 0 ~ 64
uint
read_global_vertex_index(t_mesh_header header, t_meshlet meshlet, uint vertex_idx)
{
    return mesh_data_buffer.Load(header.global_vertex_index_buffer_offset + (meshlet.global_index_offset + vertex_idx) * sizeof(uint));
}

t_vertex_encoded
read_vertex_encoded(t_mesh_header header, uint global_vertex_idx)
{
    t_vertex_encoded res;
    uint offset = header.vertex_buffer_offset + global_vertex_idx * sizeof(t_vertex_encoded);
    
    uint3 raw3 = mesh_data_buffer.Load3(offset);
    
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
        uint raw = mesh_data_buffer.Load(offset);
        res.uv_set[0] = uint32_to_half2(raw);
        offset += sizeof(half2);
    }
#endif
#if UV_COUNT >= 2
    {
        uint raw = mesh_data_buffer.Load(offset);
        res.uv_set[1] = uint32_to_half2(raw);
        offset += sizeof(half2);
    }
#endif
#if UV_COUNT >= 3
    {
        uint raw = mesh_data_buffer.Load(offset);
        res.uv_set[2] = uint32_to_half2(raw);
        offset += sizeof(half2);
    }
#endif
#if UV_COUNT >= 4
    {
        uint raw = mesh_data_buffer.Load(offset);
        res.uv_set[3] = uint32_to_half2(raw);
        offset += sizeof(half2);
    }
#endif
    return res;
}

// meshlet local primitive_idx : 0 ~ 126
uint3
read_meshlet_primitive(t_mesh_header header, t_meshlet meshlet, uint primitive_idx)
{
    const uint load_pos_1byte_aligned = header.local_vertex_index_buffer_offset + meshlet.primitive_offset + primitive_idx * 3;
    const uint load_pos_4bytes_aligned = load_pos_1byte_aligned & ~0x3u;
    const uint bit_shift_required = (load_pos_1byte_aligned & 0x3u) * 8;

    const uint2 raw = mesh_data_buffer.Load2(load_pos_4bytes_aligned);
    const uint64_t raw64 = (uint64_t(raw.y) << 32) | uint64_t(raw.x);

    return uint3(
		(raw64 >> (bit_shift_required + 0)) & 0xff,
		(raw64 >> (bit_shift_required + 8)) & 0xff,
		(raw64 >> (bit_shift_required + 16)) & 0xff);
}

SamplerState linear_clamp_sampler : register(s0);

// opaque
struct t_opaque_as_to_ms
{
    uint meshlet_32_group_idx;
    uint meshlet_alive_mask;
};

struct t_opaque_ms_to_ps
{
    float4 pos	        : SV_Position;
    float3 normal       : NORMAL;
    nointerpolation uint meshlet_render_job_id  : MESHLET_INDEX;
    float4 tangent      : TANGENT;
	UV_FIELDS
};

// presentation

struct t_presentation_ps_out
{
    float4 color : SV_Target0;
};

#endif
#ifndef AGE_FORWARD_PLUS_COMMON_HLSLI
#define AGE_FORWARD_PLUS_COMMON_HLSLI

#define UV_COUNT 3

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

    vector<int16_t, 3> aabb_min; // 6byte
    vector<uint16_t, 3> aabb_size; // 6byte 
};

struct t_vertex_encoded
{
    vector<uint16_t, 3> pos;
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
    t_transform transform; // 22
    uint16_t instance_idx; // 2
    uint16_t asset_idx; // 2
    uint16_t extra; // 2
};

struct t_asset_data
{
    uint meshlet_vertex_buffer_offset;
    uint meshlet_global_index_buffer_offset;
    uint meshlet_primitive_index_buffer_offset;
};

struct t_job_data
{
    uint object_idx;
};

cbuffer frame_data : register(b0)
{
    float4x4 view_proj; // 64 bytes
    float4x4 view_proj_inv; // 64 bytes
    float3 camera_pos; // 12 
    float time; // 4 
    float4 frustum_planes[6]; // 96
    uint frame_index; // 4
    float2 inv_backbuffer_size; // 8
    uint main_buffer_texture_id; // 4
                                    // total: 256 bytes 
};

StructuredBuffer<t_asset_data> asset_data_buffer : register(t0, space1);

#if defined(PRESENTATION_PS)
SamplerState linear_clamp_sampler : register(s0);
#endif

#if defined(SHADER_STAGE_AS) || defined(SHADER_STAGE_MS)
StructuredBuffer<t_job_data> job_data_buffer :register(t0);
#endif

#if defined(SHADER_STAGE_AS) || defined(SHADER_STAGE_MS)
StructuredBuffer<t_object_data> object_data_buffer :register(t1);
#endif

#if defined(SHADER_STAGE_AS) || defined(SHADER_STAGE_MS)
StructuredBuffer<t_meshlet_header> meshlet_header_buffer :register(t2);
#endif

#if defined(SHADER_STAGE_AS) || defined(SHADER_STAGE_MS)
StructuredBuffer<t_meshlet> meshlet_buffer :register(t3);
#endif

#if defined(SHADER_STAGE_MS)
StructuredBuffer<t_vertex_encoded> vertex_buffer :register(t4);
StructuredBuffer<uint> meshlet_global_index_buffer :register(t5);
ByteAddressBuffer meshlet_primitive_index_buffer :register(t6);
#endif

struct t_as_to_ms
{
    uint meshlet_32_group_idx;
    uint meshlet_alive_mask;
};

struct t_ms_to_ps
{
    float4 pos	        : SV_Position;
    float3 normal       : NORMAL;
    nointerpolation
    uint meshlet_index  : MESHLET_INDEX;
    float4 tangent      : TANGENT;
	UV_FIELDS
};

struct t_ps_out
{
    float4 color : SV_Target0;
};

#endif
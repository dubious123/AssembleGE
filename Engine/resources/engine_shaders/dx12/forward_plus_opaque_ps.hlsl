#define SHADER_STAGE_PS
#include "forward_plus_common.hlsli"
#undef SHADER_STAGE_PS

float3 get_random_color(uint index)
{
    index = (index ^ 61) ^ (index >> 16);
    index *= 9;
    index = index ^ (index >> 4);
    index *= 0x27d4eb2d;
    index = index ^ (index >> 15);

    float3 raw_color = float3(
        (float)((index >> 0) & 0xFF) / 255.0f,
        (float)((index >> 8) & 0xFF) / 255.0f,
        (float)((index >> 16) & 0xFF) / 255.0f
    );
    
    return raw_color * 0.8f + 0.2f;
}

float4
main_ps(t_opaque_ms_to_ps fragment): SV_Target0
{
    const float3 meshlet_color = get_random_color(fragment.meshlet_render_job_id);
    
    const float3 light_dir = normalize(float3(0.5, 1.0, -0.5)); // 대각선 위에서 오는 빛
    const float3 normal = normalize(fragment.normal);
    const float diff = saturate(dot(normal, light_dir)) * 0.6 + 0.4; // Ambient 0.4 추가

    const float3 final_color = meshlet_color * diff;
    
    //if (select32_nth_set_bit(0x5, 1) == 2)
    //{
    //    return float4(0.0f, 0.0f, 1.0f, 1.0f); // 파란색: Identity 맞음
    //}
    //else
    //{
    //    return float4(1.0f, 0.0f, 0.0f, 1.0f); // 빨간색: 회전 적용됨
    //}
    
    return float4(final_color, 1.0f);
}
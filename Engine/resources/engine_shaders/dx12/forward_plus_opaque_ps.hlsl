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

    return float3(
        (float)((index >> 0) & 0xFF) / 255.0f,
        (float)((index >> 8) & 0xFF) / 255.0f,
        (float)((index >> 16) & 0xFF) / 255.0f
    );
}

t_ps_out
main_ps(t_ms_to_ps fragment)
{
    const float3 meshlet_color = get_random_color(fragment.meshlet_index);
    
    const float3 light_dir = normalize(float3(0.5, 1.0, -0.5)); // 대각선 위에서 오는 빛
    const float3 normal = normalize(fragment.normal);
    const float diff = saturate(dot(normal, light_dir)) * 0.6 + 0.4; // Ambient 0.4 추가

    // 3. 최종 색상 조합
    const float3 final_color = meshlet_color * diff;
    
    t_ps_out ps_out;
    ps_out.color = float4(final_color, 1.0f);

    return ps_out;
}
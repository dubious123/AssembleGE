#define PRESENTATION_PS
#include "forward_plus_common.hlsli"
#undef PRESENTATION_PS

float4
main_ps(float4 pos : SV_Position) : SV_Target0
{
    float2 uv = (pos.xy + 0.5) * inv_backbuffer_size;
    
    Texture2D main_buffer_tex = ResourceDescriptorHeap[main_buffer_texture_id];
    
    float3 color = main_buffer_tex.Sample(linear_clamp_sampler, uv).rgb;

    color = max(color, 0.0);
    const float a = 2.51f;
    const float b = 0.03f;
    const float c = 2.43f;
    const float d = 0.59f;
    const float e = 0.14f;
    color = saturate((color * (a * color + b)) / (color * (c * color + d) + e));

    return float4(color, 1.f);
}
#define SHADER_STAGE_CS
#include "forward_plus_common.hlsli"
#undef SHADER_STAGE_CS

bool sphere_frustum_test(float3 center, float radius)
{
    [unroll]
    for (uint32 i = 0; i < 6; i++)
    {
        if (dot(frustum_planes[i], float4(center, 1.f)) < -radius)
        {
            return false;
        }
    }
    return true;
}

[numthreads(LIGHT_CULL_CS_THREAD_COUNT, 1, 1)]
void main_cs(uint32 light_id : SV_DispatchThreadID)      // 0..255 (16x16)
{
    const uint32 light_count = point_light_count + spot_light_count;
    float3 pos;
    float range;
    uint32 light_type;
    
    if (light_id < point_light_count)
    {
        const point_light light = point_light_buffer[light_id];
        pos = light.position;
        range = light.range;
        light_type = LIGHT_TYPE_POINT;
    }
    else
    {
        const spot_light light = spot_light_buffer[light_id];
        pos = light.position;
        range = light.range;
        light_type = LIGHT_TYPE_SPOT;
    }
    
    if (sphere_frustum_test(pos, range))
    {
        uint32 idx;
        InterlockedAdd(global_counter[0], 1, idx);
        
        if (idx < LIGHT_CULL_CS_MAX_CULL_LIGHT_COUNT)
        {
            //visible_light_list[idx] = pack_light_index(light_type, light_id);
        }
    }
}

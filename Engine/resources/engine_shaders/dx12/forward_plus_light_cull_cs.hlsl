#define SHADER_STAGE_CS
#include "forward_plus_common.hlsli"
#undef SHADER_STAGE_CS

[numthreads(LIGHT_CULL_THREAD_COUNT, 1, 1)]
void main_cs(uint32 light_id : SV_DispatchThreadID)
{
    if (light_id < unified_light_count)
    {
        const unified_light light = unified_light_buffer[light_id];
        
        if (sphere_frustum_test(light.position, light.range, frustum_planes))
        {
            uint32 idx;
            InterlockedAdd(frame_data_rw_buffer[0].not_culled_light_count, 1, idx);
        
            if (idx < MAX_VISIBLE_LIGHT_COUNT)
            {
                culled_light_buffer[idx] = light_id;
            }
        }
    }
}
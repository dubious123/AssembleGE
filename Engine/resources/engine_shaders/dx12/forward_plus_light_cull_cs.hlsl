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
            const float view_z = dot(light.position - camera_pos, camera_forward);
            const float min_z = view_z - light.range;
            
            sort_buffer[LIGHT_SORT_SORT_KEYS_OFFSET + light_id] = float_to_sortable(min_z);
            sort_buffer[LIGHT_SORT_SORT_VALUES_OFFSET + light_id] = light_id;
            
            return;
        }
    }
    
    sort_buffer[LIGHT_SORT_SORT_KEYS_OFFSET + light_id] = invalid_id_uint32;
    sort_buffer[LIGHT_SORT_SORT_VALUES_OFFSET + light_id] = invalid_id_uint32;
}
#define SHADER_STAGE_CS
#include "forward_plus_common.hlsli"
#undef SHADER_STAGE_CS

//[numthreads(LIGHT_CULL_CS_THREAD_COUNT, 1, 1)]
//void main_cs(uint32 light_id : SV_DispatchThreadID)
//{
//    if (light_id < point_light_count)
//    {
//        const uint32 point_light_id = light_id;
//        const point_light light = point_light_buffer[point_light_id - 0];
        
//        if (sphere_frustum_test(light.position, light.range, frustum_planes))
//        {
//            uint32 idx;
//            InterlockedAdd(frame_data_rw_buffer[0].not_culled_light_count, 1, idx);
        
//            if (idx < MAX_VISIBLE_LIGHT_COUNT)
//            {
//                culled_light_buffer[idx] = pack_light_index(LIGHT_TYPE_POINT, point_light_id);
//            }
//        }
//    }
//    else if (light_id < point_light_count + spot_light_count)
//    {
//        const uint32 spot_light_id = light_id - point_light_count;
//        const spot_light light = spot_light_buffer[spot_light_id];
        
//        const float3 end_center = light.position + light.direction * light.range;
//        const float3 sphere_center = (light.position + end_center) * 0.5;
//        const float sphere_radius = light.range * 0.5;
    
//        if (sphere_frustum_test(sphere_center, sphere_radius, frustum_planes))
//        {
//            uint32 idx;
//            InterlockedAdd(frame_data_rw_buffer[0].not_culled_light_count, 1, idx);
        
//            if (idx < MAX_VISIBLE_LIGHT_COUNT)
//            {
//                culled_light_buffer[idx] = pack_light_index(LIGHT_TYPE_SPOT, spot_light_id);
//            }
//        }
//    }
//}

[numthreads(LIGHT_CULL_CS_THREAD_COUNT, 1, 1)]
void main_cs(uint32 light_id : SV_DispatchThreadID)
{
    if (light_id < unified_light_count)
    {
        const unified_light light = unified_light_buffer[light_id - 0];
        
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
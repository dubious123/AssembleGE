#define SHADER_STAGE_CS
#include "forward_plus_common.hlsli"
#undef SHADER_STAGE_CS

//[numthreads(256, 1, 1)]
//void main_cs(uint32 sorted_id : SV_DispatchThreadID,
//             uint32 group_id : SV_GroupID)
//{
//    const t_unified_light_id light_id = sort_buffer[LIGHT_SORT_SORT_VALUES_OFFSET + sorted_id];
    
//    //todo
//    if (light_id == invalid_id_uint32)
//    {
//        return;
//    }
    
//    const unified_light light = unified_light_buffer[light_id];
        
//    unified_sorted_light_buffer_uav[sorted_id] = light;
    
//    const float3 pos = light.position;
//    const float range = light.range;

//    const float view_z = dot(pos - camera_pos, camera_forward);
//    const float min_z = view_z - range;
//    const float max_z = view_z + range;
    
//    const uint32 bin_begin = clamp(depth_to_bin(min_z), 0, Z_SLICE_COUNT - 1);
//    const uint32 bin_end = clamp(depth_to_bin(max_z), 0, Z_SLICE_COUNT - 1);
    
//    for (uint32 i = bin_begin; i <= bin_end; ++i)
//    {
//        InterlockedMin(zbin_buffer_uav[i].min_idx, sorted_id);
//        InterlockedMax(zbin_buffer_uav[i].max_idx, sorted_id);
//    }
//}









groupshared uint gs_min_idx[Z_SLICE_COUNT];
groupshared uint gs_max_idx[Z_SLICE_COUNT];

[numthreads(LIGHT_ZBIN_THREAD_COUNT, 1, 1)]
void main_cs(uint32 sorted_id   : SV_DispatchThreadID,
             uint32 group_index : SV_GroupIndex)
{
    for (uint32 i = group_index; i < Z_SLICE_COUNT; i += LIGHT_ZBIN_THREAD_COUNT)
    {
        gs_min_idx[i] = 0xffffffff;
        gs_max_idx[i] = 0;
    }
    GroupMemoryBarrierWithGroupSync();

    const t_unified_light_id light_id = sort_buffer[LIGHT_SORT_SORT_VALUES_OFFSET + sorted_id];
    
    if (light_id < MAX_LIGHT_COUNT && light_id != invalid_id_uint32)
    {
        const unified_light light = unified_light_buffer[light_id];
        unified_sorted_light_buffer_uav[sorted_id] = light;
        
        const float view_z = dot(light.position - camera_pos, camera_forward);
        const float min_z = view_z - light.range;
        const float max_z = view_z + light.range;
        
        const uint32 bin_begin = clamp(depth_to_bin(min_z), 0, Z_SLICE_COUNT - 1);
        const uint32 bin_end = clamp(depth_to_bin(max_z), 0, Z_SLICE_COUNT - 1);
        
        for (uint32 j = bin_begin; j <= bin_end; ++j)
        {
            InterlockedMin(gs_min_idx[j], sorted_id);
            InterlockedMax(gs_max_idx[j], sorted_id);
        }
    }
    GroupMemoryBarrierWithGroupSync();

    for (uint32 j = group_index; j < Z_SLICE_COUNT; j += LIGHT_ZBIN_THREAD_COUNT)
    {
        if (gs_min_idx[j] != 0xffffffff)
        {
            InterlockedMin(zbin_buffer_uav[j].min_idx, gs_min_idx[j]);
            InterlockedMax(zbin_buffer_uav[j].max_idx, gs_max_idx[j]);
        }
    }
}
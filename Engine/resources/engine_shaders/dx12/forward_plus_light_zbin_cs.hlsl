#define SHADER_STAGE_CS
#include "forward_plus_common.hlsli"
#undef SHADER_STAGE_CS

[numthreads(LIGHT_SORT_CS_THREAD_COUNT, 1, 1)]
void main_cs(uint32 sorted_id : SV_DispatchThreadID)
{
    const uint32 visible_count = min(frame_data_rw_buffer[0].not_culled_light_count, MAX_VISIBLE_LIGHT_COUNT);
    if (sorted_id >= visible_count)
    {
        return;
    }
    
    const t_unified_light_id light_id = sort_buffer[LIGHT_SORT_CS_SORT_VALUES_OFFSET + sorted_id];
    
    const unified_light light = unified_light_buffer[light_id];
        
    unified_sorted_light_buffer_uav[sorted_id] = light;
    
    const float3 pos = light.position;
    const float range = light.range;

    const float view_z = dot(pos - camera_pos, camera_forward);
    const float min_z = view_z - range;
    const float max_z = view_z + range;
    
    const uint32 bin_begin = clamp(depth_to_bin(min_z), 0, Z_SLICE_COUNT - 1);
    const uint32 bin_end = clamp(depth_to_bin(max_z), 0, Z_SLICE_COUNT - 1);
    
    for (uint32 i = bin_begin; i <= bin_end; ++i)
    {
        InterlockedMin(zbin_buffer_uav[i].min_idx, sorted_id);
        InterlockedMax(zbin_buffer_uav[i].max_idx, sorted_id);
    }
}
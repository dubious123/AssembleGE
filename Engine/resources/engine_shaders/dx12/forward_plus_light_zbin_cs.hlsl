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









groupshared uint local_min_idx[Z_SLICE_COUNT];
groupshared uint local_max_idx[Z_SLICE_COUNT];

[numthreads(LIGHT_ZBIN_THREAD_COUNT, 1, 1)]
void main_cs(uint32 sorted_id   : SV_DispatchThreadID,
             uint32 group_thread_id  : SV_GroupThreadID)
{
    for (uint32 i = group_thread_id; i < Z_SLICE_COUNT; i += LIGHT_ZBIN_THREAD_COUNT)
    {
        local_min_idx[i] = 0xffffffff;
        local_max_idx[i] = 0;
    }
    GroupMemoryBarrierWithGroupSync();

    const t_unified_light_id light_id = sort_buffer[LIGHT_SORT_SORT_VALUES_OFFSET + sorted_id];
    
    if ( /*light_id < MAX_VISIBLE_LIGHT_COUNT &&*/light_id != invalid_id_uint32)
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
            InterlockedMin(local_min_idx[j], sorted_id);
            InterlockedMax(local_max_idx[j], sorted_id);
        }
        
        
        const float3 pos = light.position;
        const float range = light.range;

        const float3 cam_to_light = pos - camera_pos;
        const float dist = length(cam_to_light);

        const uint32 word_index = sorted_id / 32;
        const uint32 bit = 1u << (sorted_id % 32);
        
        uint32_4 tile_aabb = uint32_4(0, cluster_tile_count_x - 1, 0, cluster_tile_count_y - 1);
        
        const float4 light_pos_clip = mul(view_proj, float4(pos, 1));
        
        if (dist > range && light_pos_clip.w > 0.1f)
        {
            const float2 light_pos_ndc = light_pos_clip.xy / light_pos_clip.w;
        
            const float projected_radius = range * dist / sqrt(dist * dist - range * range);
        
            const float4 light_right_clip = mul(view_proj, float4(pos + camera_right * projected_radius, 1));
            const float2 light_right_ndc = light_right_clip.xy / light_right_clip.w;
        
            const float light_radius_ndc = length(light_right_ndc - light_pos_ndc);

            const float2 ndc_min = light_pos_ndc - float2(light_radius_ndc, light_radius_ndc);
            const float2 ndc_max = light_pos_ndc + float2(light_radius_ndc, light_radius_ndc);
        
            const float2 screen_a = ndc_xy_to_screen(ndc_min, backbuffer_size);
            const float2 screen_b = ndc_xy_to_screen(ndc_max, backbuffer_size);
        
            const float2 screen_min = float2(screen_a.x, screen_b.y);
            const float2 screen_max = float2(screen_b.x, screen_a.y);
            
            tile_aabb.x = clamp(int(screen_min.x) / LIGHT_TILE_SIZE, 0, int(cluster_tile_count_x - 1));
            tile_aabb.y = clamp(int(screen_max.x + (LIGHT_TILE_SIZE - 1)) / LIGHT_TILE_SIZE, 0, int(cluster_tile_count_x - 1));
            tile_aabb.z = clamp(int(screen_min.y) / LIGHT_TILE_SIZE, 0, int(cluster_tile_count_y - 1));
            tile_aabb.w = clamp(int(screen_max.y + (LIGHT_TILE_SIZE - 1)) / LIGHT_TILE_SIZE, 0, int(cluster_tile_count_y - 1));
        }
        
        uint32 packed_aabb = (tile_aabb.x << 24) | (tile_aabb.y << 16) | (tile_aabb.z << 8) | tile_aabb.w;
        sort_buffer[LIGHT_TILE_AABB_OFFSET + sorted_id] = packed_aabb;
        
        InterlockedAdd(frame_data_rw_buffer[0].not_culled_light_count, 1);
    }
    GroupMemoryBarrierWithGroupSync();

    for (uint32 i = group_thread_id; i < Z_SLICE_COUNT; i += LIGHT_ZBIN_THREAD_COUNT)
    {
        if (local_min_idx[i] != 0xffffffff)
        {
            InterlockedMin(zbin_buffer_uav[i].min_idx, local_min_idx[i]);
            InterlockedMax(zbin_buffer_uav[i].max_idx, local_max_idx[i]);
        }
    }
}
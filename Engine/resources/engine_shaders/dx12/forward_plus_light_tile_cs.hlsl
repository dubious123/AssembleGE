#define SHADER_STAGE_CS
#include "forward_plus_common.hlsli"
#undef SHADER_STAGE_CS

groupshared uint32 shared_bitmask[LIGHT_BITMASK_UINT32_COUNT];

[numthreads(LIGHT_SORT_CS_THREAD_COUNT, 1, 1)]
void main_cs(uint32_3 group_id : SV_GroupID,
             uint32 group_thread_id : SV_GroupThreadID)
{
    const uint32 tile_x = group_id.x;
    const uint32 tile_y = group_id.y;
    const uint32 tile_id = tile_y * cluster_tile_count_x + tile_x;
    
    [unroll(LIGHT_BITMASK_UINT32_COUNT / LIGHT_SORT_CS_THREAD_COUNT)]
    for (uint32 i = group_thread_id; i < LIGHT_BITMASK_UINT32_COUNT; i += LIGHT_SORT_CS_THREAD_COUNT)
    {
        shared_bitmask[i] = 0;
    }
    GroupMemoryBarrierWithGroupSync();
    
    const uint32 visible_count = min(frame_data_rw_buffer[0].not_culled_light_count, MAX_VISIBLE_LIGHT_COUNT);
    
    for (uint32 sorted_id = group_thread_id; sorted_id < visible_count; sorted_id += LIGHT_SORT_CS_THREAD_COUNT)
    {
        const unified_light light = unified_sorted_light_buffer_uav[sorted_id];
        
        const float3 pos = light.position;
        const float range = light.range;

        const float3 cam_to_light = pos - camera_pos;
        const float dist = length(cam_to_light);

        const uint32 word_index = sorted_id / 32;
        const uint32 bit = 1u << (sorted_id % 32);
        
        if (dist < range)
        {
            InterlockedOr(shared_bitmask[word_index], bit);
        }
        else
        {
            const float4 light_pos_clip = mul(view_proj, float4(pos, 1));
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

            const uint32 tile_min_x = clamp(int32(screen_min.x) / LIGHT_TILE_SIZE, 0, int32(cluster_tile_count_x - 1));
            const uint32 tile_max_x = clamp(int32(screen_max.x) / LIGHT_TILE_SIZE, 0, int32(cluster_tile_count_x - 1));
            const uint32 tile_min_y = clamp(int32(screen_min.y) / LIGHT_TILE_SIZE, 0, int32(cluster_tile_count_y - 1));
            const uint32 tile_max_y = clamp(int32(screen_max.y) / LIGHT_TILE_SIZE, 0, int32(cluster_tile_count_y - 1));
            
            if (tile_x >= tile_min_x
                && tile_x <= tile_max_x
                && tile_y >= tile_min_y
                && tile_y <= tile_max_y)
            {
                InterlockedOr(shared_bitmask[word_index], bit);
            }
        }
    }
    
    GroupMemoryBarrierWithGroupSync();
    const uint32 offset = tile_id * LIGHT_BITMASK_UINT32_COUNT;
   
    [unroll(LIGHT_BITMASK_UINT32_COUNT / LIGHT_SORT_CS_THREAD_COUNT)]
    for (uint32 j = group_thread_id; j < LIGHT_BITMASK_UINT32_COUNT; j += LIGHT_SORT_CS_THREAD_COUNT)
    {
        tile_mask_buffer_uav[offset + j] = shared_bitmask[j];
    }
    
    //if (tile_id < 100 && group_thread_id == 0)
    //{
    //    uint32 total = 0;
    //    for (uint32 k = 0; k < LIGHT_BITMASK_UINT32_COUNT; ++k)
    //        total += countbits(shared_bitmask[k]);
    //    debug_buffer[0].tile_bit_mask_arr[tile_id] = total;
    //}
}
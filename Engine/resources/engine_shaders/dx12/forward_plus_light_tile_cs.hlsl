#define SHADER_STAGE_CS
#include "forward_plus_common.hlsli"
#undef SHADER_STAGE_CS

groupshared uint32 shared_bitmask[LIGHT_BITMASK_UINT32_COUNT];

[numthreads(LIGHT_SORT_THREAD_COUNT, 1, 1)]
void main_cs(uint32_3 group_id : SV_GroupID,
             uint32 group_thread_id : SV_GroupThreadID)
{
    const uint32 tile_x = group_id.x;
    const uint32 tile_y = group_id.y;
    const uint32 tile_id = tile_y * light_tile_count_x + tile_x;
    
    [unroll(LIGHT_BITMASK_UINT32_COUNT / LIGHT_SORT_THREAD_COUNT)]
    for (uint32 i = group_thread_id; i < LIGHT_BITMASK_UINT32_COUNT; i += LIGHT_SORT_THREAD_COUNT)
    {
        shared_bitmask[i] = 0;
    }
    GroupMemoryBarrierWithGroupSync();
    
    //const uint32 visible_count = min(frame_data_rw_buffer[0].not_culled_light_count, MAX_VISIBLE_LIGHT_COUNT);
    const uint32 visible_count = frame_data_rw_buffer_uav[0].not_culled_light_count;
    for (uint32 sorted_id = group_thread_id; sorted_id < visible_count; sorted_id += LIGHT_SORT_THREAD_COUNT)
    {
        uint32 packed_aabb = sort_buffer_srv[LIGHT_TILE_AABB_OFFSET + sorted_id];

        uint32 tile_min_x = (packed_aabb >> 24) & 0xff;
        uint32 tile_max_x = (packed_aabb >> 16) & 0xff;
        uint32 tile_min_y = (packed_aabb >> 8) & 0xff;
        uint32 tile_max_y = (packed_aabb) & 0xff;
        
        const uint32 word_index = sorted_id / 32;
        const uint32 bit = 1u << (sorted_id % 32);
        
        if (tile_x >= tile_min_x
                && tile_x <= tile_max_x
                && tile_y >= tile_min_y
                && tile_y <= tile_max_y)
        {
            InterlockedOr(shared_bitmask[word_index], bit);
        }
    }
    
    GroupMemoryBarrierWithGroupSync();
    const uint32 offset = tile_id * LIGHT_BITMASK_UINT32_COUNT;
   
    [unroll(LIGHT_BITMASK_UINT32_COUNT / LIGHT_SORT_THREAD_COUNT)]
    for (uint32 j = group_thread_id; j < LIGHT_BITMASK_UINT32_COUNT; j += LIGHT_SORT_THREAD_COUNT)
    {
        tile_mask_buffer_uav[offset + j] = shared_bitmask[j];
    }
}
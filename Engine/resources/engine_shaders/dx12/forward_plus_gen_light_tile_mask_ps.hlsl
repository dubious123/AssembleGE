#define SHADER_STAGE_PS

#include "forward_plus_common.hlsli"

#undef SHADER_STAGE_PS

void main_ps(tile_mask_ms_to_ps fragment)
{
    //const uint32 tile_x = uint32(fragment.pos.x) / CLUSTER_TILE_SIZE;
    //const uint32 tile_y = uint32(fragment.pos.y) / CLUSTER_TILE_SIZE;
    
    const uint32 tile_x = min(uint32(fragment.pos.x) / CLUSTER_TILE_SIZE, cluster_tile_count_x - 1);
    const uint32 tile_y = min(uint32(fragment.pos.y) / CLUSTER_TILE_SIZE, cluster_tile_count_y - 1);
    
    const uint32 tile_index = tile_x + tile_y * cluster_tile_count_x;
    
    const uint32 visible_count = min(frame_data_rw_buffer[0].not_culled_light_count, LIGHT_SORT_CS_MAX_VISIBLE_LIGHT_COUNT);
    if (fragment.sorted_id >= visible_count)
    {
        return;
    }
    
    //const uint32 packed = sort_buffer_srv[LIGHT_SORT_CS_SORT_VALUES_OFFSET + fragment.sorted_id];
    
    //if (packed == invalid_id_uint32)
    //{
    //    return;
    //}

    const uint32 word_index = fragment.sorted_id / 32;
    const uint32 bit_index = fragment.sorted_id % 32;

    InterlockedOr(tile_mask_buffer_uav[tile_index * LIGHT_BITMASK_UINT32_COUNT + word_index], 1u << bit_index);
}
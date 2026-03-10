#define SHADER_STAGE_CS
#include "forward_plus_common.hlsli"
#undef SHADER_STAGE_CS

[numthreads(256, 1, 1)]
void main_cs(uint32 thread_id : SV_DispatchThreadID)
{
    if (thread_id < Z_SLICE_COUNT)
    {
        zbin_buffer_uav[thread_id].min_idx = 0xFFFFFFFF;
        zbin_buffer_uav[thread_id].max_idx = 0;
    }
    
    if (thread_id < cluster_tile_count_x * cluster_tile_count_y * LIGHT_BITMASK_UINT32_COUNT)
    {
        tile_mask_buffer_uav[thread_id] = 0;
    }
    
    if (thread_id == 0)
    {
        frame_data_rw_buffer[0].not_culled_light_count = 0;
        frame_data_rw_buffer[0].generic_counter = 0;
        
        debug_buffer[0].invalid_count = 0;
        debug_buffer[0].visible_count = 0;
    }
    
    // culled_light_buffer[thread_id] = invalid_id_uint32;
}
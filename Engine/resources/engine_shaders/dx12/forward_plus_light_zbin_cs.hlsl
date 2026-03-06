#define SHADER_STAGE_CS
#include "forward_plus_common.hlsli"
#undef SHADER_STAGE_CS

[numthreads(LIGHT_SORT_CS_THREAD_COUNT, 1, 1)]
void main_cs(uint32 sorted_id : SV_DispatchThreadID)
{
    const uint32 visible_count = min(frame_data_rw_buffer[0].not_culled_light_count, LIGHT_SORT_CS_MAX_VISIBLE_LIGHT_COUNT);
    if (sorted_id >= visible_count)
    {
        return;
    }
    
    const uint32 packed = sort_buffer[LIGHT_SORT_CS_SORT_VALUES_OFFSET + sorted_id];
    
    //if (packed == invalid_id_uint32)
    //{
    //    return;
    //}
    
    const uint32 light_type = unpack_light_type(packed);
    const uint32 light_id = unpack_light_index(packed);
    
    float3 pos = float3(0, 0, 0);
    float range = 0;
    if (light_type == LIGHT_TYPE_POINT)
    {
        pos = point_light_buffer[light_id].position;
        range = point_light_buffer[light_id].range;
    }
    else
    {
        pos = spot_light_buffer[light_id].position;
        range = spot_light_buffer[light_id].range;
    }
    
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

    const float3 cam_to_light = pos - camera_pos;
    const float dist = length(cam_to_light);

    const uint32 word_index = sorted_id / 32;
    const uint32 bit = 1u << (sorted_id % 32);

    if (dist < range)
    {
        for (uint32 y = 0; y < cluster_tile_count_y; ++y)
        {
            for (uint32 x = 0; x < cluster_tile_count_x; ++x)
            {
                const uint32 tile_id = x + y * cluster_tile_count_x;
                InterlockedOr(tile_mask_buffer_uav[tile_id * LIGHT_BITMASK_UINT32_COUNT + word_index], bit);
            }
        }
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

        const uint32 tile_min_x = clamp(int32(screen_min.x) / CLUSTER_TILE_SIZE, 0, int32(cluster_tile_count_x - 1));
        const uint32 tile_max_x = clamp(int32(screen_max.x) / CLUSTER_TILE_SIZE, 0, int32(cluster_tile_count_x - 1));
        const uint32 tile_min_y = clamp(int32(screen_min.y) / CLUSTER_TILE_SIZE, 0, int32(cluster_tile_count_y - 1));
        const uint32 tile_max_y = clamp(int32(screen_max.y) / CLUSTER_TILE_SIZE, 0, int32(cluster_tile_count_y - 1));
       

        for (uint32 y = tile_min_y; y <= tile_max_y; ++y)
        {
            for (uint32 x = tile_min_x; x <= tile_max_x; ++x)
            {
                const uint32 tile_id = x + y * cluster_tile_count_x;
                InterlockedOr(tile_mask_buffer_uav[tile_id * LIGHT_BITMASK_UINT32_COUNT + word_index], bit);
            }
        }
    }
}
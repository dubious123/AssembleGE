#define SHADER_STAGE_CS
#include "forward_plus_common.hlsli"
#undef SHADER_STAGE_CS

// per-cluster shared memory — shared among 256 threads (16x16) working on the same cluster frustum

// number of lights intersecting this cluster
groupshared uint32 shared_light_count;
// packed light indices (type + index) that passed intersection test
groupshared uint32 shared_light_indices[CLUSTER_MAX_LIGHT_COUNT_PER_CLUSTER];
 // allocated start position in global_light_index_list
groupshared uint32 shared_global_offset;

groupshared float3 shared_aabb_min;
groupshared float3 shared_aabb_max;

//void append_to_shared_light_list(bool hit, uint packed_index)
//{
//    const uint32 wave_hit_count = WaveActiveCountBits(hit);
//    const uint32 thread_offset = WavePrefixCountBits(hit);

//    uint32 wave_base = 0;
//    if (wave_hit_count > 0 && WaveIsFirstLane())
//    {
//        InterlockedAdd(shared_light_count, wave_hit_count, wave_base);
//    }
    
//    wave_base = WaveReadLaneFirst(wave_base);

//    if (hit)
//    {
//        const uint32 idx = wave_base + thread_offset;
//        if (idx < CLUSTER_MAX_LIGHT_COUNT_PER_CLUSTER)
//        {
//            shared_light_indices[idx] = packed_index;
//        }
//    }
//}


void append_to_shared_light_list(bool hit, uint packed_index)
{
    if (hit)
    {
        uint32 idx;
        InterlockedAdd(shared_light_count, 1, idx);
        if (idx < CLUSTER_MAX_LIGHT_COUNT_PER_CLUSTER)
        {
            shared_light_indices[idx] = packed_index;
        }
    }
}

[numthreads(CLUSTER_TILE_SIZE, CLUSTER_TILE_SIZE, 1)]
void main_cs(
    uint32_3 group_id : SV_GroupID, // (tile_x, tile_y, depth_slice)
    uint32_3 group_thread_id : SV_GroupThreadID,
    uint32 group_index : SV_GroupIndex)      // 0..255 (16x16)
{
    uint32 cluster_id = group_id.x
                      + group_id.y * cluster_tile_count_x
                      + group_id.z * cluster_tile_count_x * cluster_tile_count_y;
    
    // init shared memory
    if (group_index == 0)
    {
        shared_light_count = 0;
        shared_global_offset = 0;
        
        const float2 tile_min_screen = float2(group_id.x, group_id.y) * CLUSTER_TILE_SIZE;
        const float2 tile_max_screen = float2(group_id.x + 1, group_id.y + 1) * CLUSTER_TILE_SIZE;
    
        const float2 tile_min_ndc = screen_to_ndc(tile_min_screen, inv_backbuffer_size);
        const float2 tile_max_ndc = screen_to_ndc(tile_max_screen, inv_backbuffer_size);
    
    // slice_depth(i) = near * (far/near) ^ (i/slices)
        static const float inv_depth_slices = 1.0f / float(CLUSTER_DEPTH_SLICE_COUNT);
        const float slice_near = cluster_near_z * exp2(float(group_id.z) * cluster_log_far_near_ratio * inv_depth_slices);
        const float slice_far = cluster_near_z * exp2(float(group_id.z + 1) * cluster_log_far_near_ratio * inv_depth_slices);
        
        float4 c0 = mul(view_proj_inv, float4(tile_min_ndc.x, tile_min_ndc.y, 1, 1));
        float4 c1 = mul(view_proj_inv, float4(tile_max_ndc.x, tile_min_ndc.y, 1, 1));
        float4 c2 = mul(view_proj_inv, float4(tile_min_ndc.x, tile_max_ndc.y, 1, 1));
        float4 c3 = mul(view_proj_inv, float4(tile_max_ndc.x, tile_max_ndc.y, 1, 1));
        c0 /= c0.w;
        c1 /= c1.w;
        c2 /= c2.w;
        c3 /= c3.w;
    
        const float3 cam_to_aabb_corner_ld_dir = normalize(c0.xyz - camera_pos);
        const float3 cam_to_aabb_corner_rd_dir = normalize(c1.xyz - camera_pos);
        const float3 cam_to_aabb_corner_lu_dir = normalize(c2.xyz - camera_pos);
        const float3 cam_to_aabb_corner_ru_dir = normalize(c3.xyz - camera_pos);
    
        const float3 p0 = camera_pos + cam_to_aabb_corner_ld_dir * (slice_near / safe_dot_camera_forward(cam_to_aabb_corner_ld_dir));
        const float3 p1 = camera_pos + cam_to_aabb_corner_rd_dir * (slice_near / safe_dot_camera_forward(cam_to_aabb_corner_rd_dir));
        const float3 p2 = camera_pos + cam_to_aabb_corner_lu_dir * (slice_near / safe_dot_camera_forward(cam_to_aabb_corner_lu_dir));
        const float3 p3 = camera_pos + cam_to_aabb_corner_ru_dir * (slice_near / safe_dot_camera_forward(cam_to_aabb_corner_ru_dir));
        const float3 p4 = camera_pos + cam_to_aabb_corner_ld_dir * (slice_far / safe_dot_camera_forward(cam_to_aabb_corner_ld_dir));
        const float3 p5 = camera_pos + cam_to_aabb_corner_rd_dir * (slice_far / safe_dot_camera_forward(cam_to_aabb_corner_rd_dir));
        const float3 p6 = camera_pos + cam_to_aabb_corner_lu_dir * (slice_far / safe_dot_camera_forward(cam_to_aabb_corner_lu_dir));
        const float3 p7 = camera_pos + cam_to_aabb_corner_ru_dir * (slice_far / safe_dot_camera_forward(cam_to_aabb_corner_ru_dir));
    
        shared_aabb_min = min(min(min(p0, p1), min(p2, p3)), min(min(p4, p5), min(p6, p7)));
        shared_aabb_max = max(max(max(p0, p1), max(p2, p3)), max(max(p4, p5), max(p6, p7)));
    }
    
    GroupMemoryBarrierWithGroupSync();

    // --- iterate lights, intersection test ---
    static const uint32 thread_count = CLUSTER_TILE_SIZE * CLUSTER_TILE_SIZE; // 256
    
    // point lights — 256 threads divide the work
    for (uint32 point_light_idx = group_index; point_light_idx < point_light_count; point_light_idx += thread_count)
    {
        const point_light light = point_light_buffer[point_light_idx];
        const bool hit = sphere_aabb_intersect(light.position, light.range, shared_aabb_min, shared_aabb_max);
        
        append_to_shared_light_list(hit, pack_light_index(0, point_light_idx));
    }
    
    // spot lights
    for (uint32 spot_light_idx = group_index; spot_light_idx < spot_light_count; spot_light_idx += thread_count)
    {
        const spot_light light = spot_light_buffer[spot_light_idx];
        const float3 end_center = light.position + light.direction * light.range;
        const float3 sphere_center = (light.position + end_center) * 0.5;
        const float sphere_radius = light.range * 0.5;
        const bool hit = sphere_aabb_intersect(sphere_center, sphere_radius, shared_aabb_min, shared_aabb_max);
        
        append_to_shared_light_list(hit, pack_light_index(1, spot_light_idx));
    }
    
    GroupMemoryBarrierWithGroupSync();

    // allocate global space and write cluster info
    uint32 count = min(shared_light_count, CLUSTER_MAX_LIGHT_COUNT_PER_CLUSTER);

    if (group_index == 0)
    {
        if (count > 0)
        {
            InterlockedAdd(frame_data_rw_buffer[0].generic_counter, count, shared_global_offset);
        }
        
        cluster_light_info_buffer_uav[cluster_id].offset = shared_global_offset;
        cluster_light_info_buffer_uav[cluster_id].count = count;
    }
    
    GroupMemoryBarrierWithGroupSync();

    // copy shared list to global list
    for (uint32 i = group_index; i < count; i += thread_count)
    {
        uint32 idx = shared_global_offset + i;
        if (idx < MAX_GLOBAL_LIGHT_INDEX_COUNT)
        {
            global_light_index_buffer_uav[idx] = shared_light_indices[i];
        }
    }
}

#define SHADER_STAGE_CS
#include "forward_plus_common.hlsli"
#undef SHADER_STAGE_CS

groupshared uint shared_light_count;
groupshared uint shared_light_indices[CLUSTER_MAX_LIGHT_COUNT_PER_CLUSTER];
groupshared uint shared_global_offset;

[numthreads(CLUSTER_TILE_SIZE, CLUSTER_TILE_SIZE, 1)]
void main_cs(
    uint32_3 group_id : SV_GroupID, // (tile_x, tile_y, depth_slice)
    uint32_3 group_thread_id : SV_GroupThreadID,
    uint32 group_index : SV_GroupIndex)      // 0..255 (16x16)
{
    uint cluster_id = group_id.x
                    + group_id.y * cluster_tile_count_x
                    + group_id.z * cluster_tile_count_x * cluster_tile_count_y;

    // init shared memory
    if (group_index == 0)
    {
        shared_light_count = 0;
    }
    GroupMemoryBarrierWithGroupSync();

    // TODO: construct cluster frustum AABB
    // TODO: iterate lights, intersection test
    // TODO: InterlockedAdd to shared_light_count, store index

    GroupMemoryBarrierWithGroupSync();

    // first thread writes to global buffer
    if (group_index == 0)
    {
        uint count = shared_light_count;

        if (count > 0)
        {
            // atomic allocate space in global list
            InterlockedAdd(global_counter[0], count, shared_global_offset);
        }
        
        cluster_light_info info;
        
        info.count = count;
        info.offset = shared_global_offset;

        cluster_light_info_buffer_uav[cluster_id] = info;
    }
    GroupMemoryBarrierWithGroupSync();

    // copy shared list to global list
    for (uint i = group_index; i < shared_light_count; i += CLUSTER_TILE_SIZE * CLUSTER_TILE_SIZE)
    {
        global_light_index_buffer_uav[shared_global_offset + i] = shared_light_indices[i];
    }
}
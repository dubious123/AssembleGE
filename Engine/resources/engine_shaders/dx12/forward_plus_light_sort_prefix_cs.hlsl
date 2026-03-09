#define SHADER_STAGE_CS
#include "forward_plus_common.hlsli"
#undef SHADER_STAGE_CS

DECLARE_CALC_THREAD_GROUP_PREFIX_SUM(LIGHT_SORT_THREAD_COUNT)

groupshared uint32 bin_prefix_table[LIGHT_SORT_ELEMENT_COUNT_PER_THREAD][LIGHT_SORT_THREAD_COUNT];

[numthreads(LIGHT_SORT_THREAD_COUNT, 1, 1)]
void main_cs(uint32 bin_entry : SV_GroupID,
             uint32 thread_id : SV_GroupThreadID)
{
    [unroll(LIGHT_SORT_ELEMENT_COUNT_PER_THREAD)]
    for (uint32 i = 0; i < LIGHT_SORT_ELEMENT_COUNT_PER_THREAD; ++i)
    {
        // block_size == thread_count * element_count_per_thread
        //  group_count <= block_size   
        // histogram_uav : uint32[bin_count][group_count]
        const uint32 index = i * LIGHT_SORT_THREAD_COUNT + thread_id;
        const uint32 col = index / LIGHT_SORT_ELEMENT_COUNT_PER_THREAD;
        const uint32 row = index % LIGHT_SORT_ELEMENT_COUNT_PER_THREAD;
        
        // transpose data
        bin_prefix_table[row][col] = index < LIGHT_SORT_GROUP_COUNT
                                   ? sort_buffer[LIGHT_SORT_HISTOGRAM_OFFSET + bin_entry * LIGHT_SORT_GROUP_COUNT + index]
                                   : 0;
    }
    GroupMemoryBarrierWithGroupSync();

    uint32 thread_local_sum = 0;
    for (uint32 i = 0; i < LIGHT_SORT_ELEMENT_COUNT_PER_THREAD; ++i)
    {
        uint32 temp = bin_prefix_table[i][thread_id];
        bin_prefix_table[i][thread_id] = thread_local_sum;
        thread_local_sum += temp;
    }
    
    const uint32 prefix = calc_thread_group_prefix_sum(thread_local_sum, thread_id);
    
    for (uint32 i = 0; i < LIGHT_SORT_ELEMENT_COUNT_PER_THREAD; ++i)
    {
        bin_prefix_table[i][thread_id] += prefix;
    }
    GroupMemoryBarrierWithGroupSync();
    
    [unroll(LIGHT_SORT_ELEMENT_COUNT_PER_THREAD)]
    for (uint32 i = 0; i < LIGHT_SORT_ELEMENT_COUNT_PER_THREAD; ++i)
    {
        // block_size == thread_count * element_count_per_thread
        // group_count <= block_size
        // histogram_uav : uint32[bin_count][group_count]
        const uint32 index = i * LIGHT_SORT_THREAD_COUNT + thread_id;
        const uint32 col = index / LIGHT_SORT_ELEMENT_COUNT_PER_THREAD;
        const uint32 row = index % LIGHT_SORT_ELEMENT_COUNT_PER_THREAD;
        
        if (index < LIGHT_SORT_GROUP_COUNT)
        {
            sort_buffer[LIGHT_SORT_HISTOGRAM_OFFSET + bin_entry * LIGHT_SORT_GROUP_COUNT + index] = bin_prefix_table[row][col];
        }
    }
    
    if (thread_id == LIGHT_SORT_THREAD_COUNT - 1)
    {
        sort_buffer[LIGHT_SORT_BIN_COUNT_OFFSET + bin_entry] = prefix + thread_local_sum;
    }
}


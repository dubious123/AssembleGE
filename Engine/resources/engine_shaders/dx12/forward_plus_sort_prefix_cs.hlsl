#define SHADER_STAGE_CS
#include "forward_plus_common.hlsli"
#undef SHADER_STAGE_CS

DECLARE_CALC_THREAD_GROUP_PREFIX_SUM(SORT_THREAD_COUNT)

groupshared uint32 bin_prefix_table[SORT_ELEMENT_COUNT_PER_THREAD][SORT_THREAD_COUNT];

[numthreads(SORT_THREAD_COUNT, 1, 1)]
void main_cs(uint32 bin_entry : SV_GroupID,
             uint32 thread_id : SV_GroupThreadID)
{
    [unroll(SORT_ELEMENT_COUNT_PER_THREAD)]
    for (uint32 i = 0; i < SORT_ELEMENT_COUNT_PER_THREAD; ++i)
    {
        // block_size == thread_count * element_count_per_thread
        //  group_count <= block_size   
        // histogram_uav : uint32[bin_count][group_count]
        const uint32 index = i * SORT_THREAD_COUNT + thread_id;
        const uint32 col = index / SORT_ELEMENT_COUNT_PER_THREAD;
        const uint32 row = index % SORT_ELEMENT_COUNT_PER_THREAD; 
        
        // transpose data
        bin_prefix_table[row][col] = index < SORT_GROUP_COUNT
                                   ? sort_buffer[SORT_HISTOGRAM_OFFSET + bin_entry * SORT_GROUP_COUNT + index]
                                   : 0;
    }
    GroupMemoryBarrierWithGroupSync();

    uint32 thread_local_sum = 0;
    for (uint32 i = 0; i < SORT_ELEMENT_COUNT_PER_THREAD; ++i)
    {
        uint32 temp = bin_prefix_table[i][thread_id];
        bin_prefix_table[i][thread_id] = thread_local_sum;
        thread_local_sum += temp;
    }
    
    const uint32 prefix = calc_thread_group_prefix_sum(thread_local_sum, thread_id);
    
    for (uint32 i = 0; i < SORT_ELEMENT_COUNT_PER_THREAD; ++i)
    {
        bin_prefix_table[i][thread_id] += prefix;
    }
    GroupMemoryBarrierWithGroupSync();
    
    [unroll(SORT_ELEMENT_COUNT_PER_THREAD)]
    for (uint32 i = 0; i < SORT_ELEMENT_COUNT_PER_THREAD; ++i)
    {
        // block_size == thread_count * element_count_per_thread
        // group_count <= block_size
        // histogram_uav : uint32[bin_count][group_count]
        const uint32 index = i * SORT_THREAD_COUNT + thread_id;
        const uint32 col = index / SORT_ELEMENT_COUNT_PER_THREAD;
        const uint32 row = index % SORT_ELEMENT_COUNT_PER_THREAD;
        
        if (index < SORT_GROUP_COUNT)
        {
            sort_buffer[SORT_HISTOGRAM_OFFSET + bin_entry * SORT_GROUP_COUNT + index] = bin_prefix_table[row][col];
        }
    }
    
    if (thread_id == SORT_THREAD_COUNT - 1)
    {
        sort_buffer[SORT_BIN_COUNT_OFFSET + bin_entry] = prefix + thread_local_sum;
    }
}


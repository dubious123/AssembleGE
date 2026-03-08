#define SHADER_STAGE_CS
#include "forward_plus_common.hlsli"
#undef SHADER_STAGE_CS

groupshared uint32 bin_offset[LIGHT_SORT_CS_THREAD_COUNT];

[numthreads(LIGHT_SORT_CS_THREAD_COUNT, 1, 1)]
void main_cs(uint32 group_thread_id : SV_GroupThreadID,
             uint32 visible_light_id : SV_DispatchThreadID)
{
    const uint32 group_count = MAX_VISIBLE_LIGHT_COUNT / LIGHT_SORT_CS_THREAD_COUNT;
    
    // for each bin 0xXX
    uint32 bin_total = 0;
    for (uint32 group_id = 0; group_id < group_count; ++group_id)
    {
        bin_total += sort_buffer[LIGHT_SORT_CS_HISTOGRAM_OFFSET + group_id * LIGHT_SORT_CS_THREAD_COUNT + group_thread_id];
    }
    
    bin_offset[group_thread_id] = bin_total;
    GroupMemoryBarrierWithGroupSync();
    
    // Inclusive prefix sum (Hillis-Steele algorithm)
    // Result: bin_offset[n] = bin_offset[0] + bin_offset[1] + ... + bin_offset[n]
    //
    // stride 1:   each element adds its left neighbor   -> sum of 2 consecutive elements
    // stride 2:   each element adds the value 2 left    -> sum of 4 consecutive elements
    // stride 4:   each element adds the value 4 left    -> sum of 8 consecutive elements
    // ...
    // stride 128: done. log2(256) = 8 iterations
    for (uint32 stride = 1; stride < 256; stride <<= 1)
    {
        // Read the value 'stride' positions to the left.
        // Threads with index < stride have nothing to add, so val stays 0.
        uint32 val = 0;
        if (group_thread_id >= stride)
        {
            val = bin_offset[group_thread_id - stride];
        }
        
        // Wait for all threads to finish reading.
        // Without this: thread A could update bin_offset[i], then thread B
        // reads the same bin_offset[i] and gets a corrupted value.
        GroupMemoryBarrierWithGroupSync();
        
        // Accumulate the previously read value into the current position.
        if (group_thread_id >= stride)
        {
            bin_offset[group_thread_id] += val;
        }
        
        // Wait for all threads to finish writing.
        // Without this: next stride iteration could read values
        // that have not been updated yet.
        GroupMemoryBarrierWithGroupSync();
    }
    
    uint32 bin_offset_curr = (group_thread_id == 0) ? 0 : bin_offset[group_thread_id - 1];
    for (uint32 group_id = 0; group_id < group_count; ++group_id)
    {
        uint32 nth_bin_addr = LIGHT_SORT_CS_HISTOGRAM_OFFSET + group_id * LIGHT_SORT_CS_THREAD_COUNT + group_thread_id;
        uint32 bin_count = sort_buffer[nth_bin_addr];
        sort_buffer[nth_bin_addr] = bin_offset_curr;
        bin_offset_curr += bin_count;
    }
}


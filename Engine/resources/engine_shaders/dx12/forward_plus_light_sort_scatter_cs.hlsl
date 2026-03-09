#define SHADER_STAGE_CS
#include "forward_plus_common.hlsli"
#undef SHADER_STAGE_CS

DECLARE_CALC_THREAD_GROUP_PREFIX_SUM(LIGHT_SORT_THREAD_COUNT)


groupshared uint32 bin_offset_arr[LIGHT_SORT_BIN_COUNT];
groupshared uint32 gs_generic[LIGHT_SORT_THREAD_COUNT];

groupshared uint32 local_histogram[LIGHT_SORT_BIN_COUNT];

[numthreads(LIGHT_SORT_THREAD_COUNT, 1, 1)]
void main_cs(uint32 group_id : SV_GroupID,
             uint32 thread_id : SV_GroupThreadID)
{
    if (thread_id < LIGHT_SORT_BIN_COUNT)
    {
        const uint32 group_local_bin_offset = sort_buffer[LIGHT_SORT_HISTOGRAM_OFFSET + thread_id * LIGHT_SORT_GROUP_COUNT + group_id];
        const uint32 offset = sort_buffer[LIGHT_SORT_BIN_COUNT_OFFSET + thread_id];
        bin_offset_arr[thread_id] = WavePrefixSum(offset) + group_local_bin_offset;
    }
    
    GroupMemoryBarrierWithGroupSync();
    
    
    uint32 src_keys_offset, dst_keys_offset;
    uint32 src_values_offset, dst_values_offset;
    if (light_radix_sort_pass % 2 == 0)
    {
        src_keys_offset = LIGHT_SORT_SORT_KEYS_OFFSET;
        dst_keys_offset = LIGHT_SORT_SORT_KEYS_ALT_OFFSET;
        src_values_offset = LIGHT_SORT_SORT_VALUES_OFFSET;
        dst_values_offset = LIGHT_SORT_SORT_VALUES_ALT_OFFSET;
    }
    else
    {
        src_keys_offset = LIGHT_SORT_SORT_KEYS_ALT_OFFSET;
        dst_keys_offset = LIGHT_SORT_SORT_KEYS_OFFSET;
        src_values_offset = LIGHT_SORT_SORT_VALUES_ALT_OFFSET;
        dst_values_offset = LIGHT_SORT_SORT_VALUES_OFFSET;
    }
    
    for (uint32 block_index = 0; block_index < LIGHT_SORT_BLOCK_COUNT_PER_GROUP; ++block_index)
    {
        // expect compiler to prefetch all elements, hiding memory latency
        [unroll(LIGHT_SORT_ELEMENT_COUNT_PER_THREAD)]
        for (uint32 i = 0; i < LIGHT_SORT_ELEMENT_COUNT_PER_THREAD; ++i)
        {
            static const uint32 bin_bit_mask = LIGHT_SORT_BIN_COUNT - 1;
            
            const uint32 index = group_id * LIGHT_SORT_BLOCK_COUNT_PER_GROUP * LIGHT_SORT_BLOCK_SIZE
                               + block_index * LIGHT_SORT_BLOCK_SIZE
                               + i * LIGHT_SORT_THREAD_COUNT
                               + thread_id;
            
            //uint32 key = index < MAX_VISIBLE_LIGHT_COUNT ? sort_buffer[src_keys_offset + index] : invalid_id_uint32;
            //uint32 value = index < MAX_VISIBLE_LIGHT_COUNT ? sort_buffer[src_values_offset + index] : invalid_id_uint32;
            
            uint32 key = sort_buffer[src_keys_offset + index];
            uint32 value = sort_buffer[src_values_offset + index];
            
            
            // sort thread_count(s) keys 
            for (uint32 bit_shift = 0; bit_shift < LIGHT_SORT_BIN_BIT_WIDTH; bit_shift += 2)
            {
                const uint32 bin = (key >> (light_radix_sort_pass * LIGHT_SORT_BIN_BIT_WIDTH)) & bin_bit_mask;
                const uint32 bit_bin = (bin >> bit_shift) & ((1 << 2) - 1);
                
                const uint32 bit_bin_shifted = (1u << (bit_bin * 8));
                
                const uint32 histogram_prefix_sum = calc_thread_group_prefix_sum(bit_bin_shifted, thread_id);
                
                if (thread_id == LIGHT_SORT_THREAD_COUNT - 1)
                {
                    gs_generic[0] = histogram_prefix_sum + bit_bin_shifted;
                }
                
                GroupMemoryBarrierWithGroupSync();
                
                // histogram : uint8[4]
                // histogram[2] == count of bit_bin #2
                const uint32 histogram = gs_generic[0];
                
                // base_offset : uint8[4]
                // base_offset[2] == base_offset of bit_bin #2
                // [ #2 + #1 + #0 | #1 + #0 | #0 | 0 ]
                const uint32 packed_prefix_offset = (histogram << 8) + (histogram << 16) + (histogram << 24);
                
                // packed_bit_bin_offset : uint8[4]
                // packed_bit_bin_offset[2] == offset of bit_bin #2
                // [ #2 + #1 + #0 | #1 + #0 + local_offset_of_bin_2 | #0 | 0 ]
                const uint32 packed_bit_bin_offset = packed_prefix_offset + histogram_prefix_sum;
                
                const uint32 bit_bin_offset = (packed_bit_bin_offset >> (bit_bin * 8)) & (LIGHT_SORT_THREAD_COUNT - 1);
            
                // swap key and value
                gs_generic[bit_bin_offset] = key;
                
                GroupMemoryBarrierWithGroupSync();
                
                key = gs_generic[thread_id];
                
                GroupMemoryBarrierWithGroupSync();
                
                gs_generic[bit_bin_offset] = value;
                
                GroupMemoryBarrierWithGroupSync();
                
                value = gs_generic[thread_id];
                
                GroupMemoryBarrierWithGroupSync();
            }
            
            const uint32 bin = (key >> (light_radix_sort_pass * LIGHT_SORT_BIN_BIT_WIDTH)) & bin_bit_mask;
            
            if (thread_id < LIGHT_SORT_BIN_COUNT)
            {
                local_histogram[thread_id] = 0;
            }
            
            GroupMemoryBarrierWithGroupSync();
            
            InterlockedAdd(local_histogram[bin], 1);
            
            GroupMemoryBarrierWithGroupSync();
            
            if (thread_id < LIGHT_SORT_BIN_COUNT)
            {
                const uint32 histogram_prefix = WavePrefixSum(local_histogram[thread_id]);
                gs_generic[thread_id] = histogram_prefix;
            }
            
            GroupMemoryBarrierWithGroupSync();
            
            // gs_generic[bin] : prefix of bin offset
            // thread_id : prefix of bin offset + local index of bin 
            // thread_id - gs_generic[bin] : local_index of bin
            const uint32 bin_offset = bin_offset_arr[bin] + (thread_id - gs_generic[bin]);
            
            //if (bin_offset < MAX_VISIBLE_LIGHT_COUNT)
            //{
            //    sort_buffer[dst_keys_offset + bin_offset] = key;
            //    sort_buffer[dst_values_offset + bin_offset] = value;
            //}
            sort_buffer[dst_keys_offset + bin_offset] = key;
            sort_buffer[dst_values_offset + bin_offset] = value;
            
            if (thread_id < LIGHT_SORT_BIN_COUNT)
            {
                bin_offset_arr[thread_id] += local_histogram[thread_id];
            }
            
            //GroupMemoryBarrierWithGroupSync();
        }
    }
}


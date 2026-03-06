#define SHADER_STAGE_CS
#include "forward_plus_common.hlsli"
#undef SHADER_STAGE_CS

groupshared uint32 local_bin_offset[256];

[numthreads(LIGHT_SORT_CS_THREAD_COUNT, 1, 1)]
void main_cs(uint32 visible_light_id : SV_DispatchThreadID,
             uint32 thread_id : SV_GroupThreadID)
{
    local_bin_offset[thread_id] = sort_buffer[LIGHT_SORT_CS_HISTOGRAM_OFFSET + visible_light_id];
    GroupMemoryBarrierWithGroupSync();
    
    uint32 src_keys_offset, dst_keys_offset;
    uint32 src_values_offset, dst_values_offset;

    if (light_radix_sort_pass % 2 == 0)
    {
        src_keys_offset = LIGHT_SORT_CS_SORT_KEYS_OFFSET;
        dst_keys_offset = LIGHT_SORT_CS_SORT_KEYS_ALT_OFFSET;
        src_values_offset = LIGHT_SORT_CS_SORT_VALUES_OFFSET;
        dst_values_offset = LIGHT_SORT_CS_SORT_VALUES_ALT_OFFSET;
    }
    else
    {
        src_keys_offset = LIGHT_SORT_CS_SORT_KEYS_ALT_OFFSET;
        dst_keys_offset = LIGHT_SORT_CS_SORT_KEYS_OFFSET;
        src_values_offset = LIGHT_SORT_CS_SORT_VALUES_ALT_OFFSET;
        dst_values_offset = LIGHT_SORT_CS_SORT_VALUES_OFFSET;
    }
    
    const uint32 key = sort_buffer[src_keys_offset + visible_light_id];
    const uint32 value = sort_buffer[src_values_offset + visible_light_id];
    const uint32 radix = (key >> (light_radix_sort_pass * 8)) & 0xFF;
    
    uint32 dest;
    InterlockedAdd(local_bin_offset[radix], 1, dest);
    
    sort_buffer[dst_keys_offset + dest] = key;
    sort_buffer[dst_values_offset + dest] = value;
}


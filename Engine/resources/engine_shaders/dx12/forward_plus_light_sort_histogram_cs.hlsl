#define SHADER_STAGE_CS
#include "forward_plus_common.hlsli"
#undef SHADER_STAGE_CS

groupshared uint32 local_histogram[LIGHT_SORT_CS_THREAD_COUNT];

[numthreads(LIGHT_SORT_CS_THREAD_COUNT, 1, 1)]
void main_cs(uint32 visible_light_id : SV_DispatchThreadID,
             uint32 thread_id : SV_GroupThreadID)
{
    local_histogram[thread_id] = 0;
    GroupMemoryBarrierWithGroupSync();
    
    const uint32 src_keys_offset = (light_radix_sort_pass % 2 == 0)
        ? LIGHT_SORT_CS_SORT_KEYS_OFFSET
        : LIGHT_SORT_CS_SORT_KEYS_ALT_OFFSET;
    
    const uint32 key = sort_buffer[src_keys_offset + visible_light_id];
    const uint32 radix = (key >> (8 * light_radix_sort_pass)) & 0xff;
    InterlockedAdd(local_histogram[radix], 1);
    GroupMemoryBarrierWithGroupSync();
    
    // histogram[256 * group_id + nth_bin] = local_histogram[nth_bin], nth_bin = 0x00 ~ 0xff
    sort_buffer[LIGHT_SORT_CS_HISTOGRAM_OFFSET + visible_light_id] = local_histogram[thread_id];
}


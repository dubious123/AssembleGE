#define SHADER_STAGE_CS
#include "forward_plus_common.hlsli"
#undef SHADER_STAGE_CS

// gen_keys
// group0                   group1
// [0x11, 0x42, 0x11, 0x42] [0x11, 0x33, 0x33, 0x42] : values 

// histogram
// group0               group1
// [0x11 : 2, 0x42 : 2] [0x11 : 1, 0x33 : 2, 0x42 : 1] 

// prefix 
// bin_offset [ 0x11 : 2 + 1 + ... , 0x33 : 2, 0x42 : 2 + 1 + ... ]
// bin_offset [ 0x11 : 3, 0x33 : 2 + 3, 0x42 : 3 + 2 + 3, ... ]
// bin_offset [ 0x11 : 0, 0x33 : 3, 0x42 : 5, ... ]
// 
// group0[0x11] = bin_offset[0x11]
// group1[0x11] = group0[0x11] + countof 0x11 in group 0 = group0[0x11] + histogram[0 * 256 + 0x11]
// group2[0x11] = group1[0x11] + countof 0x11 in group 1 = group1[0x11] + histogram[1 * 256 + 0x11]

// scatter
// local_bin_offset = bin_offset[group_n]
// offset = interlock_add(local_bin_offset[ radix = value & 0xff ], 1)  
// dst_value[offset] = src_value
// dst_key[offset] = src_key

[numthreads(LIGHT_SORT_THREAD_COUNT, 1, 1)]
void main_cs(uint32 light_culled_id : SV_DispatchThreadID)
{
    //const uint32 visible_count = min(frame_data_rw_buffer[0].not_culled_light_count, MAX_VISIBLE_LIGHT_COUNT);
    //if (light_culled_id >= visible_count)
    //{
    //    sort_buffer[LIGHT_SORT_SORT_KEYS_OFFSET + light_culled_id] = invalid_id_uint32;
    //    sort_buffer[LIGHT_SORT_SORT_VALUES_OFFSET + light_culled_id] = invalid_id_uint32;
    //    return;
    //}
    
    const t_unified_light_id light_id = culled_light_buffer[light_culled_id];
    

    
    if (light_id == invalid_id_uint32)
    {
        sort_buffer[LIGHT_SORT_SORT_KEYS_OFFSET + light_culled_id] = invalid_id_uint32;
        sort_buffer[LIGHT_SORT_SORT_VALUES_OFFSET + light_culled_id] = invalid_id_uint32;
        return;
    }
    
    const unified_light light = unified_light_buffer[light_id];
    
    float3 pos = light.position;
    float range = light.range;
    
    const float view_z = dot(pos - camera_pos, camera_forward);
    const float min_z = view_z - range;
    
    sort_buffer[LIGHT_SORT_SORT_KEYS_OFFSET + light_culled_id] = float_to_sortable(min_z);
    sort_buffer[LIGHT_SORT_SORT_VALUES_OFFSET + light_culled_id] = light_id;
}


#define SHADER_STAGE_CS
#include "forward_plus_common.hlsli"
#undef SHADER_STAGE_CS

DECLARE_CALC_THREAD_GROUP_MIN_MAX((16 * 16))

[numthreads(16, 16, 1)]
void main_cs(
uint32_2 local_thread_id : SV_GroupThreadID,
uint32_2 thread_id : SV_DispatchThreadID)
{
    uint32_2 z_min_max = uint32_2(0xffffffff, 0);
    
    if (all(thread_id.xy < backbuffer_size))
    {
        Texture2D<float> depth_buffer_tex = ResourceDescriptorHeap[depth_buffer_texture_id];
    
        const float z = depth_buffer_tex[thread_id];
        
        if (z > epsilon_1e4) 
        {
            uint32 linear_z = asuint(linearize_reverse_z(z, cam_near_z, cam_far_z));
        
            z_min_max = uint32_2(linear_z, linear_z);
        }
    }
    
    uint32_2 group_min_max = calc_thread_group_min_max(z_min_max.x, z_min_max.y, local_thread_id.y * 16 + local_thread_id.x);
    
    if (all(local_thread_id == uint32_2(0, 0)))
    {
        InterlockedMin(frame_data_rw_buffer_uav[0].z_min, group_min_max.x);
        InterlockedMax(frame_data_rw_buffer_uav[0].z_max, group_min_max.y);
    }
}
#define SHADER_STAGE_AS
#include "forward_plus_common.hlsli"
#undef SHADER_STAGE_AS

bool
is_visible(const t_object_data object_data, const t_meshlet_header meshlet_header, const t_meshlet meshlet)
{
    t_transform transform_encoded = object_data.transform;
    const float4 quaternion = decode_quaternion(transform_encoded.quaternion);
    const float3 scale = (float3)transform_encoded.scale;
    const float3 pos = transform_encoded.pos;
    //1. frustum culling using meshlet shpere   
    
    const float3 sphere_center =
              rotate(((float3)meshlet.aabb_min + (float3)meshlet.aabb_size * 0.5f) * scale, quaternion)
            + pos;
        
    const float sphere_radius = length((float3)meshlet.aabb_size * 0.5f) * max(max(scale.x, scale.y), scale.z);
        
    [unroll]
    for (uint i = 0; i < 6; ++i)
    {
        float4 p = frustum_planes[i];
        float d = dot(p.xyz, sphere_center) + p.w;
        if (d < -sphere_radius)
        {
            return false;
        }
    }
    
    //2. back face culling using normal cone
    
    const float3 cone_axis = rotate(decode_oct_snorm(meshlet_header.cone_axis_oct), quaternion);
    const float cone_cull_cutoff = snorm8_to_float(meshlet_header.cone_cull_cutoff_and_offset & 0xffu);
    const float cone_cull_offset = snorm8_to_float((meshlet_header.cone_cull_cutoff_and_offset >> 8) & 0xffu);
    const float3 view_dir = normalize(sphere_center - cone_cull_offset * length(cone_axis * scale) * cone_axis - camera_pos);
    
    if (dot(cone_axis, view_dir) > cone_cull_cutoff)
    {
        return false;
    }
    
    return true;
}

groupshared
t_as_to_ms as_out;

[numthreads(32, 1, 1)]
void main_as(
    uint3 dispatch_thread_id : SV_DispatchThreadID,
    uint3 group_id : SV_GroupID,
    uint3 group_thread_id : SV_GroupThreadID)
{
    uint visible_mask;
    
    {
        const uint job_idx = dispatch_thread_id.x;
        const uint meshlet_idx = job_idx;
        const t_job_data job_data = job_data_buffer[job_idx];
        const t_object_data object_data = object_data_buffer[job_data.object_idx];
        const t_meshlet_header meshlet_header = meshlet_header_buffer[meshlet_idx];
    
        const bool visible = is_visible(object_data, meshlet_header, meshlet_buffer[meshlet_idx]);
    
        const uint4 ballot = WaveActiveBallot(visible);
        visible_mask = ballot.x;
    }

    
    if (WaveIsFirstLane())
    {
        as_out.meshlet_32_group_idx = group_id.x;
        as_out.meshlet_alive_mask = visible_mask;
    }

    DispatchMesh(countbits(visible_mask), 1, 1, as_out);
}
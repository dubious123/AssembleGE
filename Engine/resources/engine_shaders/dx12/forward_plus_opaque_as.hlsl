#define SHADER_STAGE_AS
#include "forward_plus_common.hlsli"
#undef SHADER_STAGE_AS

bool
is_visible(const object_data obj_data, const meshlet_header m_header)
{
    const float4 quaternion = decode_quaternion(obj_data.quaternion);
    const float3 scale = (float3)obj_data.scale;
    const float3 pos = obj_data.pos;
    
    //1. frustum culling using meshlet shpere   
    
    const float3 sphere_center =
              rotate(((float3)m_header.aabb_min + (float3)m_header.aabb_size * 0.5f) * scale, quaternion)
            + pos;
        
    const float sphere_radius = length((float3)m_header.aabb_size * 0.5f) * max(max(scale.x, scale.y), scale.z);
    
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
    
    const float3 cone_axis_local = decode_oct_snorm(m_header.cone_axis_oct);
    const float3 cone_axis_world = rotate(cone_axis_local, quaternion);
    const float cone_cull_cutoff = snorm8_to_float(m_header.cone_cull_cutoff_and_offset & 0xffu);
    
    const float cone_cull_offset = int8_to_float(m_header.cone_cull_cutoff_and_offset >> 8);
    
    const float3 view_dir = normalize(sphere_center - cone_cull_offset * length(cone_axis_local * scale) * cone_axis_world - camera_pos);
    
    //if (dot(cone_axis_world, view_dir) >= cone_cull_cutoff)
    //{
    //    return false;
    //}
    
    return true;
}

groupshared
opaque_as_to_ms as_out;

[numthreads(32, 1, 1)]
void main_as(
    uint3 dispatch_thread_id : SV_DispatchThreadID,
    uint3 group_id : SV_GroupID,
    uint3 group_thread_id : SV_GroupThreadID)
{
    uint visible_mask;
    
    {
        const uint job_id = dispatch_thread_id.x;
        const job_data job = meshlet_render_job_buffer[job_id];
        const object_data obj_data = object_data_buffer[job.object_id];
        
        const uint meshlet_idx = job.meshlet_id;
        
        const mesh_header msh_header = read_mesh_header(job.mesh_byte_offset);
        const meshlet_header mshlt_header = read_meshlet_header(msh_header, meshlet_idx);
    
        const bool visible = is_visible(obj_data, mshlt_header);
    
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
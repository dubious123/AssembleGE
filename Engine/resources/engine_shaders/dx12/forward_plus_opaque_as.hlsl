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
    
    // 2. cone culling
    const float3 cone_axis_world = rotate(decode_oct_snorm(m_header.cone_axis_oct), quaternion);
    const float cone_cutoff = snorm8_to_float(m_header.cone_cull_cutoff_and_extra & 0xffu);
    const float3 center_to_cam = sphere_center - camera_pos;
    const float dist = length(center_to_cam);

    if (dot(center_to_cam, cone_axis_world) >= cone_cutoff * dist + sphere_radius)
    {
        return false;
    }
    
    return true;
}

groupshared
opaque_as_to_ms as_out;

[numthreads(32, 1, 1)]
void main_as(
    uint32_3 dispatch_thread_id : SV_DispatchThreadID,
    uint32_3 group_id : SV_GroupID,
    uint32_3 group_thread_id : SV_GroupThreadID)
{
    const uint32 job_id = dispatch_thread_id.x;
    
    bool visible = false;
    
    if (job_id < job_count)
    {
        const job_data job = meshlet_render_job_buffer[job_id];
        const object_data obj_data = object_data_buffer[job.object_id];

        const uint meshlet_idx = job.meshlet_id;

        const mesh_header msh_header = read_mesh_header(job.mesh_byte_offset);
        const meshlet_header mshlt_header = read_meshlet_header(msh_header, meshlet_idx);

        visible = is_visible(obj_data, mshlt_header);
    }
       
    const uint32_4 ballot = WaveActiveBallot(visible);
    uint32 visible_mask = ballot.x;

    
    if (WaveIsFirstLane())
    {
        as_out.meshlet_32_group_idx = group_id.x;
        as_out.meshlet_alive_mask = visible_mask;
    }
    
    DispatchMesh(countbits(visible_mask), 1, 1, as_out);
}
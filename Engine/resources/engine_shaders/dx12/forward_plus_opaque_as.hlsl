#define SHADER_STAGE_AS
#include "forward_plus_common.hlsli"
#undef SHADER_STAGE_AS

bool
is_visible(const t_object_data object_data, const t_meshlet_header meshlet_header, const t_meshlet meshlet)
{
    const float4 quaternion = decode_quaternion(object_data.quaternion);
    const float3 scale = (float3)object_data.scale;
    const float3 pos = object_data.pos;
    
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
    
    const float3 cone_axis_local = decode_oct_snorm(meshlet_header.cone_axis_oct);
    const float3 cone_axis_world = rotate(cone_axis_local, quaternion);
    const float cone_cull_cutoff = snorm8_to_float(meshlet_header.cone_cull_cutoff_and_offset & 0xffu);
    
    const float cone_cull_offset = int8_to_float(meshlet_header.cone_cull_cutoff_and_offset >> 8);
    
    const float3 view_dir = normalize(sphere_center - cone_cull_offset * length(cone_axis_local * scale) * cone_axis_world - camera_pos);
    
    //if (dot(cone_axis_world, view_dir) >= cone_cull_cutoff)
    //{
    //    return false;
    //}
    
    return true;
}

groupshared
t_opaque_as_to_ms as_out;

[numthreads(32, 1, 1)]
void main_as(
    uint3 dispatch_thread_id : SV_DispatchThreadID,
    uint3 group_id : SV_GroupID,
    uint3 group_thread_id : SV_GroupThreadID)
{
    uint visible_mask;
    
    {
        const uint job_id = dispatch_thread_id.x;
        const t_meshlet_render_job meshlet_render_job = meshlet_render_job_buffer[job_id];
        const t_object_data object_data = object_data_buffer[meshlet_render_job.object_idx];
        
        const uint meshlet_idx = meshlet_render_job.meshlet_idx;
        
        const t_mesh_header mesh_header = read_mesh_header(meshlet_render_job.mesh_byte_offset);
        const t_meshlet_header meshlet_header = read_meshlet_header(mesh_header, meshlet_idx);
        const t_meshlet meshlet = read_meshlet(mesh_header, meshlet_idx);
    
        const bool visible = is_visible(object_data, meshlet_header, meshlet);
    
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
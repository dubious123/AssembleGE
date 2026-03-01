#define SHADER_STAGE_MS

#include "forward_plus_common.hlsli"

#undef SHADER_STAGE_MS

struct depth_ms_out
{
    float4 pos : SV_Position;
};

[numthreads(32, 1, 1)]
[outputtopology("triangle")]
void
main_ms(
		in payload opaque_as_to_ms ms_in,
		uint32_3 group_id : SV_GroupID,
		uint32_3 group_thread_id : SV_GroupThreadID,
		out vertices depth_ms_out ms_out_vertex_arr[64],
		out indices uint32_3 ms_out_triangle_arr[126])
{
    const uint meshlet_count_per_group = 32;
    const uint not_culled_job_local_index = select32_nth_set_bit(ms_in.meshlet_alive_mask, group_id.x);
    const uint meshlet_render_job_id = ms_in.meshlet_32_group_idx * meshlet_count_per_group + not_culled_job_local_index;
    
    const job_data job = meshlet_render_job_buffer[meshlet_render_job_id];
    const mesh_header mesh_header = read_mesh_header(job.mesh_byte_offset);
    const meshlet mshlt = read_meshlet(mesh_header, job.meshlet_id);
    const object_data obj_data = object_data_buffer[job.object_id];
    
    const uint vertex_count = mshlt.vertex_count_prim_count_extra & 0xffu;
    const uint primitive_count = (mshlt.vertex_count_prim_count_extra >> 8u) & 0xffu;
    
    SetMeshOutputCounts(vertex_count, primitive_count);

    const float4 quaternion = decode_quaternion(obj_data.quaternion);
    const float3 scale = (float3)obj_data.scale;
    const float3 pos = obj_data.pos;
    
    [unroll(2)]
    for (uint32 nth_vertex = group_thread_id.x; nth_vertex < vertex_count; nth_vertex += 32)
    {
        const uint32 global_vertex_index = read_global_vertex_index(mesh_header, mshlt, nth_vertex);
        const vertex_encoded v_encoded = read_vertex_encoded(mesh_header, global_vertex_index);
        
        const float3 pos_local = ((float3)v_encoded.pos) / 65535.f
						  * mesh_header.aabb_size
						  + mesh_header.aabb_min;

        const float3 pos_world = rotate(pos_local * scale, quaternion) + pos;

        ms_out_vertex_arr[nth_vertex].pos = mul(view_proj, float4(pos_world, 1.0));
    }

	[unroll(4)]
    for (uint nth_primitive = group_thread_id.x; nth_primitive < primitive_count; nth_primitive += 32)
    {
        ms_out_triangle_arr[nth_primitive] = read_meshlet_primitive(mesh_header, mshlt, nth_primitive);
    }
}
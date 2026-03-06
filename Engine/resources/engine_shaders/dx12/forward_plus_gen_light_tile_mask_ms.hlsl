#define SHADER_STAGE_MS

#include "forward_plus_common.hlsli"

#undef SHADER_STAGE_MS

#define LIGHTS_PER_GROUP 4
#define THREADS_PER_LIGHT 8
#define VERTEX_PER_LIGHT 16
#define PRIM_PER_LIGHT 14

static const float2 unit_circle[VERTEX_PER_LIGHT] =
{
    float2(1.0, 0.0),
    float2(0.9239, 0.3827),
    float2(0.7071, 0.7071),
    float2(0.3827, 0.9239),
    float2(0.0, 1.0),
    float2(-0.3827, 0.9239),
    float2(-0.7071, 0.7071),
    float2(-0.9239, 0.3827),
    float2(-1.0, 0.0),
    float2(-0.9239, -0.3827),
    float2(-0.7071, -0.7071),
    float2(-0.3827, -0.9239),
    float2(0.0, -1.0),
    float2(0.3827, -0.9239),
    float2(0.7071, -0.7071),
    float2(0.9239, -0.3827),
};

[numthreads(32, 1, 1)]
[outputtopology("triangle")]
void
main_ms2(
		uint32_3 group_id : SV_GroupID,
		uint32_3 group_thread_id : SV_GroupThreadID,
		out vertices tile_mask_ms_to_ps ms_out_vertex_arr[VERTEX_PER_LIGHT * LIGHTS_PER_GROUP],
		out indices uint32_3 ms_out_triangle_arr[PRIM_PER_LIGHT * LIGHTS_PER_GROUP])
{
    SetMeshOutputCounts(1, 1);
    ms_out_vertex_arr[0].pos = float4(0, 0, 0, 1);
    ms_out_vertex_arr[0].sorted_id = 0;
    return;
}

[numthreads(32, 1, 1)]
[outputtopology("triangle")]
void
main_ms(
		uint32_3 group_id : SV_GroupID,
		uint32_3 group_thread_id : SV_GroupThreadID,
		out vertices tile_mask_ms_to_ps ms_out_vertex_arr[VERTEX_PER_LIGHT * LIGHTS_PER_GROUP],
		out indices uint32_3 ms_out_triangle_arr[PRIM_PER_LIGHT * LIGHTS_PER_GROUP])
{
    const uint32 local_light_id = group_thread_id.x / THREADS_PER_LIGHT; // 0, 1, 2, 3
    const uint32 local_thread = group_thread_id.x % THREADS_PER_LIGHT; // 0, 1, ..., 7
    const uint32 sorted_id = group_id.x * LIGHTS_PER_GROUP + local_light_id;
    
    const uint32 packed = sort_buffer_srv[LIGHT_SORT_CS_SORT_VALUES_OFFSET + sorted_id];
    
    const uint32 light_type = unpack_light_type(packed);
    const uint32 light_id = unpack_light_index(packed);

    const uint32 visible_count = min(frame_data_rw_buffer[0].not_culled_light_count, LIGHT_SORT_CS_MAX_VISIBLE_LIGHT_COUNT);
    
    const uint32 light_offset = group_id.x * LIGHTS_PER_GROUP;
    uint32 light_count = min(LIGHTS_PER_GROUP, visible_count > light_offset ? visible_count - light_offset : 0);

    SetMeshOutputCounts(light_count * VERTEX_PER_LIGHT, light_count * PRIM_PER_LIGHT);

    //if (local_light_id >= light_count || packed == invalid_id_uint32 || sorted_id == invalid_id_uint32)
    //{
    //    return;
    //}
    
    if (local_light_id >= light_count)
    {
        return;
    }


    float3 light_pos;
    float range;
    if (light_type == LIGHT_TYPE_POINT)
    {
        light_pos = point_light_buffer[light_id].position;
        range = point_light_buffer[light_id].range;
    }
    else
    {
        light_pos = spot_light_buffer[light_id].position;
        range = spot_light_buffer[light_id].range;
    }

    const float3 cam_to_light = light_pos - camera_pos;
    const float dist = length(cam_to_light);

    const uint32 vertex_offset = local_light_id * VERTEX_PER_LIGHT;
    const uint32 prim_offset = local_light_id * PRIM_PER_LIGHT;

    if (dist < range)
    {
        ms_out_vertex_arr[vertex_offset + local_thread].pos = float4(0, 0, 0, 1);
        ms_out_vertex_arr[vertex_offset + local_thread].sorted_id = sorted_id;
        ms_out_vertex_arr[vertex_offset + local_thread + THREADS_PER_LIGHT].pos = float4(0, 0, 0, 1);
        ms_out_vertex_arr[vertex_offset + local_thread + THREADS_PER_LIGHT].sorted_id = sorted_id;

        if (local_thread < 3)
        {
            const float2 corners[3] = {float2(-1, -1), float2(3, -1), float2(-1, 3)};
            ms_out_vertex_arr[vertex_offset + local_thread].pos = float4(corners[local_thread], 0, 1);
        }

        if (local_thread == 0)
        {
            ms_out_triangle_arr[prim_offset] = uint32_3(vertex_offset, vertex_offset + 1, vertex_offset + 2);
            for (uint32 t = 1; t < PRIM_PER_LIGHT; ++t)
            {
                ms_out_triangle_arr[prim_offset + t] = uint32_3(vertex_offset, vertex_offset, vertex_offset);
            }
        }
    }
    else
    {
        // projected_radius = dist * tan
        const float projected_radius = range * dist / sqrt(dist * dist - range * range);

        const float3 forward = cam_to_light / dist;
        const float3 right = normalize(cross(float3(0, 1, 0), forward));
        const float3 up = cross(forward, right);
        
        for (uint32 v = 0; v < 2; ++v)
        {
            const uint32 vert_id = local_thread * 2 + v;
            if (vert_id < VERTEX_PER_LIGHT)
            {
                const float3 world_pos = light_pos
                    + (right * unit_circle[vert_id].x + up * unit_circle[vert_id].y) * projected_radius;

                ms_out_vertex_arr[vertex_offset + vert_id].pos = mul(view_proj, float4(world_pos, 1));
                ms_out_vertex_arr[vertex_offset + vert_id].sorted_id = sorted_id;
            }
        }

        for (uint32 t = 0; t < 2; ++t)
        {
            const uint32 tri_id = local_thread * 2 + t;
            if (tri_id < PRIM_PER_LIGHT)
            {
                ms_out_triangle_arr[prim_offset + tri_id] = uint32_3(vertex_offset, vertex_offset + tri_id + 1, vertex_offset + tri_id + 2);
            }
        }
    }
}
#define SHADER_STAGE_CS
#include "forward_plus_common.hlsli"
#undef SHADER_STAGE_CS

void handle_directional_light_shadow(uint32 directional_light_id, uint32 cascade_idx, uint32 shadow_id)
{
    const directional_light light = directional_light_buffer[directional_light_id];
    float depth_min = asfloat(frame_data_rw_buffer_uav[0].z_min);
    float depth_max = asfloat(frame_data_rw_buffer_uav[0].z_max);

    // fallback
    if (depth_min >= depth_max)
    {
        depth_min = cam_near_z; 
        depth_max = cam_far_z;
    }

    const float t_near = (float)cascade_idx / (float)DIRECTIONAL_SHADOW_CASCADE_COUNT;
    const float t_far = (float)(cascade_idx + 1) / (float)DIRECTIONAL_SHADOW_CASCADE_COUNT;

    const float log_near = depth_min * pow(depth_max / depth_min, t_near);
    const float uni_near = depth_min + (depth_max - depth_min) * t_near;
    const float log_far = depth_min * pow(depth_max / depth_min, t_far);
    const float uni_far = depth_min + (depth_max - depth_min) * t_far;

    const float split_near = lerp(uni_near, log_near, SHADOW_CASCADE_SPLIT_FACTOR);
    const float split_far = lerp(uni_far, log_far, SHADOW_CASCADE_SPLIT_FACTOR);

    static const float4 ndc_corners[8] =
    {
        {-1, -1, 1, 1}, // near bottom-left   (reverse-z: near = z=1)
        {+1, -1, 1, 1}, // near bottom-right
        {-1, +1, 1, 1}, // near top-left
        {+1, +1, 1, 1}, // near top-right
        {-1, -1, 0, 1}, // far bottom-left    (reverse-z: far = z=0)
        {+1, -1, 0, 1}, // far bottom-right
        {-1, +1, 0, 1}, // far top-left
        {+1, +1, 0, 1}, // far top-right
    };

    float3 world_corners[8];
    {
        [unroll]
        for (uint32 i = 0; i < 8; ++i)
        {
            const float4 c = mul(view_proj_inv, ndc_corners[i]);
            world_corners[i] = c.xyz / c.w;
        }
    }

    float3 cascade_corners[8];
    {   
        const float range = cam_far_z - cam_near_z;

        [unroll]
        for (uint32 i = 0; i < 4; ++i)
        {
            const float3 near_corner = world_corners[i];
            const float3 far_corner = world_corners[i + 4];
            const float3 dir = far_corner - near_corner;

            cascade_corners[i] = near_corner + dir * ((split_near - cam_near_z) / range);
            cascade_corners[i + 4] = near_corner + dir * ((split_far - cam_near_z) / range);
        }
    }


    // bounding sphere
    float3 frustum_center = (float3)0;
    {
        [unroll]
        for (uint32 i = 0; i < 8; ++i)
        {
            frustum_center += cascade_corners[i];
        }
        frustum_center /= 8.0f;
    }


    float radius = 0;
    {
        [unroll]
        for (uint32 m = 0; m < 8; ++m)
        {
            radius = max(radius, length(cascade_corners[m] - frustum_center));
        
        }
    }

    static const float back_offset = DIRECTIONAL_SHADOW_BACKOFF;

    const float4x4 light_view_mat = view_look_to(frustum_center - light.direction * (radius + back_offset), light.direction);

    // texel snapping
    const float texels_per_unit = (float)SHADOW_MAP_WIDTH / (radius * 2.0f);
    const float3 center_light_space = mul(light_view_mat, float4(frustum_center, 1)).xyz;
    const float snap_x = floor(center_light_space.x * texels_per_unit) / texels_per_unit;
    const float snap_y = floor(center_light_space.y * texels_per_unit) / texels_per_unit;
    const float offset_x = snap_x - center_light_space.x;
    const float offset_y = snap_y - center_light_space.y;
    
    // ortho projection (reverse-z)
    const float diameter = radius * 2.0;
    const float rng = diameter;
    
    const float4x4 light_proj = proj_orthographic_reversed(
        radius * 2.0f,
        radius * 2.0f,
        epsilon_1e4,
        radius * 2.0f + back_offset);

    const float4x4 light_view_proj = mul(mul(light_proj, translation(offset_x, offset_y, 0.f)), light_view_mat);
    
    shadow_light_buffer_uav[shadow_id + cascade_idx].view_proj = light_view_proj;
    gen_frustum_planes(light_view_proj, shadow_light_buffer_uav[shadow_id + cascade_idx].frustum_planes);
    
    frame_data_rw_buffer_uav[0].cascade_splits[cascade_idx / 4][cascade_idx % 4] = split_far;
    
    // frame_data_rw_buffer[0].radius[cascade_idx] = radius;
}

//[noinline]
void handle_point_light_shadow(uint32 id, uint32 face_idx, uint32 shadow_id)
{
    // DXC SM 6.9 suspected inlining bug (DXC 1.9.2602.16)
    // When this function is inlined, certain thread_id values (e.g., 3 and 4 out of 0-5)
    // appear to be skipped when using static const array indexing.
    // Replacing array indexing with a switch statement resolves the issue.
    // Marking the function as [noinline] also resolves the issue.
    // SM 6.8 is not affected. Possibly related to optimizer constant propagation
    // misidentifying some array indices as unreachable in the caller's branch context.
    
    //static const float3 face_dirs[6] =
    //{
    //    float3(1, 0, 0), // +X
    //    float3(-1, 0, 0), // -X
    //    float3(0, 1, 0), // +Y
    //    float3(0, -1, 0), // -Y
    //    float3(0, 0, 1), // +Z
    //    float3(0, 0, -1), // -Z
    //};
    
    const unified_light light = unified_light_buffer[id];
    
    float4x4 light_view;
    switch(face_idx)
    {
        case 0:
            light_view = view_look_to(light.position, float3(1, 0, 0));
            break;
        case 1:
            light_view = view_look_to(light.position, float3(-1, 0, 0));
            break;
        case 2:
            light_view = view_look_to(light.position, float3(0, 1, 0));
            break;
        case 3:
            light_view = view_look_to(light.position, float3(0, -1, 0));
            break;
        case 4:
            light_view = view_look_to(light.position, float3(0, 0, 1));
            break;
        case 5:
            light_view = view_look_to(light.position, float3(1, 0, -1));
            break;
        
    }

    //const float4x4 light_view = view_look_to(light.position, float3(0, -1, 0));
    //const float4x4 light_view = view_look_to(light.position, face_dirs[face_idx]);

    const float4x4 light_proj = proj_perspective_reversed(
        pi_half,
        1.0f, 
        0.1f,
        light.range);

    const float4x4 light_view_proj = mul(light_proj, light_view);

    shadow_light_buffer_uav[shadow_id + face_idx].view_proj = light_view_proj;
    gen_frustum_planes(light_view_proj, shadow_light_buffer_uav[shadow_id + face_idx].frustum_planes);
}

void handle_spot_light_shadow(uint32 id, uint32 shadow_id)
{
    const unified_light light = unified_light_buffer[id];
    
    const float4x4 light_view = view_look_to(light.position, (float3)light.direction);
      
    const float4x4 light_proj = proj_perspective_reversed(
        2.0f * acos(light.cos_outer),
        1,
        epsilon_1e4,
        light.range); 
    
    const float4x4 light_view_proj = mul(light_proj, light_view);
    
    shadow_light_buffer_uav[shadow_id].view_proj = light_view_proj;
    gen_frustum_planes(light_view_proj, shadow_light_buffer_uav[shadow_id].frustum_planes);
}

[numthreads(6, 1, 1)]
void main_cs(uint32 shadow_light_header_id : SV_GroupID,
             uint32 thread_id : SV_GroupThreadID)
{
    const shadow_light_header header = shadow_light_header_buffer[shadow_light_header_id];
    
    if (header.light_kind == LIGHT_KIND_DIRECTIONAL)
    {
        if (thread_id < DIRECTIONAL_SHADOW_CASCADE_COUNT)  
        {
            handle_directional_light_shadow(header.light_id, thread_id, header.shadow_id);
        }
    }
    else if (header.light_kind == LIGHT_KIND_POINT)
    {
        if (thread_id < 6)
        {
            handle_point_light_shadow(header.light_id, thread_id, header.shadow_id);
        }
    }
    else if (header.light_kind == LIGHT_KIND_SPOT)
    {
        if (thread_id < 1)
        {
            handle_spot_light_shadow(header.light_id, header.shadow_id);
        }
    }
}
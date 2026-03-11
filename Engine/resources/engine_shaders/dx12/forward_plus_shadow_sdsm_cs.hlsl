#define SHADER_STAGE_CS
#include "forward_plus_common.hlsli"
#undef SHADER_STAGE_CS

[numthreads(DIRECTIONAL_SHADOW_CASCADE_COUNT, 1, 1)]
void main_cs(uint32 directional_light_id : SV_GroupID,
             uint32 cascade_idx : SV_GroupThreadID)
{
    const directional_light light = directional_light_buffer[directional_light_id];
    float depth_min = asfloat(frame_data_rw_buffer[0].z_min);
    float depth_max = asfloat(frame_data_rw_buffer[0].z_max);

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

    // frustum planes (Gribb-Hartmann)
    const float4 r0 = light_view_proj[0];
    const float4 r1 = light_view_proj[1];
    const float4 r2 = light_view_proj[2];
    const float4 r3 = light_view_proj[3];

    const uint32 shadow_id = directional_light_id * DIRECTIONAL_SHADOW_CASCADE_COUNT + cascade_idx;

    shadow_light_buffer_uav[shadow_id].view_proj = light_view_proj;
    shadow_light_buffer_uav[shadow_id].frustum_planes[0] = normalize_plane(r3 + r0); // left
    shadow_light_buffer_uav[shadow_id].frustum_planes[1] = normalize_plane(r3 - r0); // right
    shadow_light_buffer_uav[shadow_id].frustum_planes[2] = normalize_plane(r3 - r1); // top
    shadow_light_buffer_uav[shadow_id].frustum_planes[3] = normalize_plane(r3 + r1); // bottom
    shadow_light_buffer_uav[shadow_id].frustum_planes[4] = normalize_plane(r2); // near
    shadow_light_buffer_uav[shadow_id].frustum_planes[5] = normalize_plane(r3 - r2); // far
    
    frame_data_rw_buffer[0].cascade_splits[cascade_idx / 4][cascade_idx % 4] = split_far;
    // frame_data_rw_buffer[0].radius[cascade_idx] = radius;
}
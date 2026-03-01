#define SHADER_STAGE_PS
#include "forward_plus_common.hlsli"
#undef SHADER_STAGE_PS

float3 get_random_color(uint index)
{
    index = (index ^ 61) ^ (index >> 16);
    index *= 9;
    index = index ^ (index >> 4);
    index *= 0x27d4eb2d;
    index = index ^ (index >> 15);

    float3 raw_color = float3(
        (float)((index >> 0) & 0xFF) / 255.0f,
        (float)((index >> 8) & 0xFF) / 255.0f,
        (float)((index >> 16) & 0xFF) / 255.0f
    );
    
    return raw_color * 0.8f + 0.2f;
}

float3 calc_blinn_phong_directional_light_color(directional_light light, float3 surface_normal, float3 view_dir)
{
    const float shininess = 64.0f;

    const float3 surface_to_light = -light.direction;
    const float3 half_dir = normalize(surface_to_light + view_dir);

    const float diffuse = saturate(dot(surface_normal, surface_to_light));
    const float specular = pow(saturate(dot(surface_normal, half_dir)), shininess);

    return light.color * light.intensity * (diffuse + specular);
}

float3 calc_blinn_phong_point_light_color(point_light light, float3 surface_pos, float3 surface_normal, float3 view_dir)
{
    const float shininess = 64.0f;

    const float3 surface_to_light = light.position - surface_pos;
    const float distance = length(surface_to_light);

    const float3 surface_to_light_dir = surface_to_light / distance;
    const float3 half_dir = normalize(surface_to_light_dir + view_dir);

    const float diffuse = saturate(dot(surface_normal, surface_to_light_dir));
    const float specular = pow(saturate(dot(surface_normal, half_dir)), shininess);
    
    const float distance_ratio = distance / light.range;
    const float window = saturate(1.0 - distance_ratio * distance_ratio);
    const float attenuation = 1.0 / max(distance * distance, epsilon_1e4) * window * window;

    return light.color * light.intensity * (diffuse + specular) * attenuation;
}

float3 calc_blinn_phong_spot_light_color(spot_light light, float3 surface_pos, float3 surface_normal, float3 view_dir)
{
    const float shininess = 64.0f;

    const float3 surface_to_light = light.position - surface_pos;
    const float distance = length(surface_to_light);

    const float3 surface_to_light_dir = surface_to_light / distance;
    const float cos_angle = dot(-surface_to_light_dir, light.direction);

    const float3 half_dir = normalize(surface_to_light_dir + view_dir);

    const float diffuse = saturate(dot(surface_normal, surface_to_light_dir));
    const float specular = pow(saturate(dot(surface_normal, half_dir)), shininess);
    
    const float distance_ratio = distance / light.range;
    const float window = saturate(1.0 - distance_ratio * distance_ratio);
    const float attenuation = 1.0 / max(distance * distance, epsilon_1e4) * window * window;

    const float cone_falloff = saturate((cos_angle - light.cos_outer) / (light.cos_inner - light.cos_outer));

    return light.color * light.intensity * (diffuse + specular) * attenuation * cone_falloff * cone_falloff;
}

float4
main_ps2(opaque_ms_to_ps fragment): SV_Target0
{
    const float3 meshlet_color = get_random_color(fragment.meshlet_render_job_id);
    
    const float3 light_dir = normalize(float3(0.5, 1.0, -0.5)); // 대각선 위에서 오는 빛
    const float3 normal = normalize(fragment.normal);
    const float diff = saturate(dot(normal, light_dir)) * 0.6 + 0.4; // Ambient 0.4 추가

    const float3 final_color = meshlet_color * diff;
    
    //if (sizeof(meshlet_header) == 16)
    //    return float4(0, 0, 1, 1); // 파랑 = 정상
    //else
    //    return float4(1, 0, 0, 1); // 빨강 = 문제
    
    return float4(final_color, 1.0f);
}

float4
main_ps(opaque_ms_to_ps fragment): SV_Target0
{
    const float3 ambient_light = float3(0.03, 0.03, 0.03);
    
    //const float3 albedo = float3(0.8, 0.8, 0.8);
    
    const float3 meshlet_color = get_random_color(fragment.meshlet_render_job_id);
    
    const float3 surface_normal = normalize(fragment.normal);
    const float3 view_dir = normalize(camera_pos - fragment.world_pos);
    const uint32 directional_light_count = directional_light_count_and_extra & 0xff;
    
    float3 lighting = ambient_light;
    
    for (uint d = 0; d < directional_light_count; ++d)
    {
        lighting += calc_blinn_phong_directional_light_color(directional_light_buffer[d], surface_normal, view_dir);
        
    }

    for (uint p = 0; p < point_light_count; ++p)
    {
        lighting += calc_blinn_phong_point_light_color(point_light_buffer[p], fragment.world_pos, surface_normal, view_dir);
        
    }

    for (uint s = 0; s < spot_light_count; ++s)
    {
        lighting += calc_blinn_phong_spot_light_color(spot_light_buffer[s], fragment.world_pos, surface_normal, view_dir);
    }

    
    //return float4(lighting * albedo, 1.0f);
    return float4(lighting * meshlet_color, 1.0f);
}
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
    const float3 surface_to_light = -light.direction;
    const float3 half_dir = normalize(surface_to_light + view_dir);

    const float diffuse = saturate(dot(surface_normal, surface_to_light));
    float specular = saturate(dot(surface_normal, half_dir));
    specular *= specular; // 2
    specular *= specular; // 4
    specular *= specular; // 8
    specular *= specular; // 16
    specular *= specular; // 32
    specular *= specular; // 64

    return light.color * light.intensity * (diffuse + specular);
}

float3 calc_blinn_phong_point_light_color(point_light light, float3 surface_pos, float3 surface_normal, float3 view_dir)
{
    const float3 surface_to_light = light.position - surface_pos;
    const float distance = length(surface_to_light);
    
    if (light.range < distance)
    {
        return float3(0, 0, 0);
    }

    const float3 surface_to_light_dir = surface_to_light / distance;
    const float3 half_dir = normalize(surface_to_light_dir + view_dir);

    const float diffuse = saturate(dot(surface_normal, surface_to_light_dir));
    float specular = saturate(dot(surface_normal, half_dir));
    specular *= specular; // 2
    specular *= specular; // 4
    specular *= specular; // 8
    specular *= specular; // 16
    specular *= specular; // 32
    specular *= specular; // 64
    
    const float distance_ratio = distance / light.range;
    const float window = saturate(1.0 - distance_ratio * distance_ratio);
    const float attenuation = 1.0 / max(distance * distance, epsilon_1e4) * window * window;

    return light.color * light.intensity * (diffuse + specular) * attenuation;
}

float3 calc_blinn_phong_spot_light_color(spot_light light, float3 surface_pos, float3 surface_normal, float3 view_dir)
{
    const float3 surface_to_light = light.position - surface_pos;
    const float distance = length(surface_to_light);
    
    if (light.range < distance)
    {
        return float3(0, 0, 0);
    }

    const float3 surface_to_light_dir = surface_to_light / distance;
    const float cos_angle = dot(-surface_to_light_dir, light.direction);

    const float3 half_dir = normalize(surface_to_light_dir + view_dir);

    const float diffuse = saturate(dot(surface_normal, surface_to_light_dir));
    float specular = saturate(dot(surface_normal, half_dir));
    specular *= specular; // 2
    specular *= specular; // 4
    specular *= specular; // 8
    specular *= specular; // 16
    specular *= specular; // 32
    specular *= specular; // 64
    
    const float distance_ratio = distance / light.range;
    const float window = saturate(1.0 - distance_ratio * distance_ratio);
    const float attenuation = 1.0 / max(distance * distance, epsilon_1e4) * window * window;

    const float cone_falloff = saturate((cos_angle - light.cos_outer) / (light.cos_inner - light.cos_outer));

    return light.color * light.intensity * (diffuse + specular) * attenuation * cone_falloff * cone_falloff;
}

float4
main_ps_(opaque_ms_to_ps fragment): SV_Target0
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

float4
main_ps(opaque_ms_to_ps fragment): SV_Target0
{
    const float3 ambient_light = float3(0.03, 0.03, 0.03);
    const float3 albedo = float3(0.8, 0.8, 0.8);
    //const float3 albedo = get_random_color(fragment.meshlet_render_job_id);

    const float3 surface_normal = normalize(fragment.normal);
    const float3 view_dir = normalize(camera_pos - fragment.world_pos);
    
    const uint32 directional_light_count = directional_light_count_and_extra & 0xff;

    const uint32 tile_x = uint32(fragment.pos.x) / LIGHT_TILE_SIZE;
    const uint32 tile_y = uint32(fragment.pos.y) / LIGHT_TILE_SIZE;
    const uint32 tile_id = tile_x + tile_y * cluster_tile_count_x;
    
    float3 lighting = ambient_light;
    
    for (uint d = 0; d < directional_light_count; ++d)
    {
        lighting += calc_blinn_phong_directional_light_color(directional_light_buffer[d], surface_normal, view_dir);
    }
    
    const float linear_depth = dot(fragment.world_pos - camera_pos, camera_forward);
    
    const uint32 bin = clamp(depth_to_bin(linear_depth), 0, Z_SLICE_COUNT - 1);
    
    const uint32 z_min = zbin_buffer_srv[bin].min_idx;
    const uint32 z_max = zbin_buffer_srv[bin].max_idx;

    const uint32 wave_z_min = WaveActiveMin(z_min);
    const uint32 wave_z_max = WaveActiveMax(z_max);
    
    const uint32 word_begin = wave_z_min / 32;
    const uint32 word_end = wave_z_max / 32;

    for (uint32 w = word_begin; w <= word_end; ++w)
    {
        uint32 bit_mask = tile_mask_buffer_srv[tile_id * LIGHT_BITMASK_UINT32_COUNT + w];
        bit_mask = WaveActiveBitOr(bit_mask);

        while (bit_mask != 0)
        {
            const uint32 bit = firstbitlow(bit_mask);
            const uint32 sorted_id = w * 32 + bit;
            bit_mask &= ~(1u << bit);

            const uint32 packed = sort_buffer_srv[LIGHT_SORT_CS_SORT_VALUES_OFFSET + sorted_id];
            
            const uint32 light_type = unpack_light_type(packed);
            const uint32 light_index = unpack_light_index(packed);
           
            //lighting += calc_blinn_phong_point_light_color(point_light_buffer[light_index], fragment.world_pos, surface_normal, view_dir);

            if (light_type == LIGHT_TYPE_POINT)
            {
                lighting += calc_blinn_phong_point_light_color(point_light_buffer[light_index], fragment.world_pos, surface_normal, view_dir);
            }
            else if (light_type == LIGHT_TYPE_SPOT)
            {
                lighting += calc_blinn_phong_spot_light_color(spot_light_buffer[light_index], fragment.world_pos, surface_normal, view_dir);
            }
        }
    }

    return float4(lighting * albedo, 1.0f);
}
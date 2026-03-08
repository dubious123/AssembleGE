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

float3 calc_blinn_phong_light_color(unified_light light, float3 surface_pos, float3 surface_normal, float3 view_dir)
{
    const float3 surface_to_light = light.position - surface_pos;
    const float distance = length(surface_to_light);
    
    if (light.range < distance)
    {
        return float3(0, 0, 0);
    }
    
    const float3 color = (float3)light.color;
    const float intensity = (float)light.intensity;
    const float3 dir = (float3)light.direction;
    const float cos_inner = (float)light.cos_inner;
    const float cos_outer = (float)light.cos_outer;

    const float3 surface_to_light_dir = surface_to_light / distance;
    const float cos_angle = dot(-surface_to_light_dir, dir);

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

    const float cone_falloff = saturate((cos_angle - cos_outer) / (cos_inner - cos_outer));

    return color * intensity * (diffuse + specular) * attenuation * cone_falloff * cone_falloff;
}

float sample_shadow_pcf(Texture2D<float> atlas, SamplerComparisonState samp, float2 atlas_uv, float depth, float texel_size)
{
    float shadow = 0;
    for (int y = -1; y <= 1; ++y)
    {
        for (int x = -1; x <= 1; ++x)
        {
            shadow += atlas.SampleCmp(samp, atlas_uv + float2(x, y) * texel_size, depth);
        }
    }
    return shadow / 9.0;
}

float sample_directional_shadow(float3 world_pos, float linear_depth, uint32 shadow_offset)
{
    uint32 cascade_index = DIRECTIONAL_SHADOW_CASCADE_COUNT - 1;
    for (uint32 c = 0; c < DIRECTIONAL_SHADOW_CASCADE_COUNT; ++c)
    {
        const uint32 arr_idx = c / 4;
        const uint32 comp = c % 4;
        const float split = cascade_splits[arr_idx][comp];
    
        if (linear_depth < split)
        {
            cascade_index = c;
            break;
        }
    }
   
    
    cascade_index = 1;
    
    const shadow_light light = shadow_light_buffer[shadow_offset + cascade_index];
    
    float4 light_clip = mul(light.view_proj, float4(world_pos, 1.0));
    float3 light_ndc = light_clip.xyz / light_clip.w;
    
    float2 shadow_uv = float2(light_ndc.x * 0.5 + 0.5, -light_ndc.y * 0.5 + 0.5);
    
    uint32 col = (shadow_offset + cascade_index) % SHADOW_ATLAS_SEG_U;
    uint32 row = (shadow_offset + cascade_index) / SHADOW_ATLAS_SEG_U;
    
    float2 atlas_uv;
    atlas_uv.x = col * (1.f / SHADOW_ATLAS_SEG_U) + shadow_uv.x / SHADOW_ATLAS_SEG_U;
    atlas_uv.y = row * (1.f / SHADOW_ATLAS_SEG_V) + shadow_uv.y / SHADOW_ATLAS_SEG_V;
    
    Texture2D<float> shadow_atlas = ResourceDescriptorHeap[shadow_atlas_id];
    
    float texel_size = 1.0 / (float)(SHADOW_MAP_WIDTH * SHADOW_ATLAS_SEG_U);
    
    float shadow = 0;
    for (int32 y = -1; y <= 1; ++y)
    {
        for (int32 x = -1; x <= 1; ++x)
        {
            shadow += shadow_atlas.SampleCmp(shadow_sampler, atlas_uv + float2(x, y) * texel_size, light_ndc.z);
        }
    }
    return shadow / 9.0;
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
    
    const float linear_depth = dot(fragment.world_pos - camera_pos, camera_forward);
    
    const uint32 bin = clamp(depth_to_bin(linear_depth), 0, Z_SLICE_COUNT - 1);
    
    const uint32 z_min = zbin_buffer_srv[bin].min_idx;
    const uint32 z_max = zbin_buffer_srv[bin].max_idx;

    const uint32 wave_z_min = WaveActiveMin(z_min);
    const uint32 wave_z_max = WaveActiveMax(z_max);
    
    const uint32 word_begin = wave_z_min / 32;
    const uint32 word_end = wave_z_max / 32;
    
    for (uint d = 0; d < directional_light_count; ++d)
    {
        float3 ddx_pos = ddx(fragment.world_pos);
        float3 ddy_pos = ddy(fragment.world_pos);
        float3 normal = normalize(cross(ddx_pos, ddy_pos));
        float n_dot_l = saturate(dot(normal, -directional_light_buffer[d].direction));
        float shadow = sample_directional_shadow(fragment.world_pos, linear_depth, directional_light_buffer[d].shadow_id);
        shadow = min(shadow, smoothstep(0.0, 0.05, n_dot_l));
        lighting += shadow * calc_blinn_phong_directional_light_color(directional_light_buffer[d], surface_normal, view_dir);
    }
    
    for (uint32 w = word_begin; w <= word_end; ++w)
    {
        uint32 bit_mask = tile_mask_buffer_srv[tile_id * LIGHT_BITMASK_UINT32_COUNT + w];
        bit_mask = WaveActiveBitOr(bit_mask);

        while (bit_mask != 0)
        {
            const uint32 bit = firstbitlow(bit_mask);
            const uint32 sorted_id = w * 32 + bit;
            bit_mask &= ~(1u << bit);
            
            const unified_light light = unified_sorted_light_buffer_srv[sorted_id];
            
            lighting += calc_blinn_phong_light_color(unified_sorted_light_buffer_srv[sorted_id], fragment.world_pos, surface_normal, view_dir);
        }
    }
    
    return float4(lighting * albedo, 1.0f);
}
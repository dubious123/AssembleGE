#include "forward_plus_common.asli"

float3
get_random_color(uint32 index)
{
	index  = (index ^ 61) ^ (index >> 16);
	index *= 9;
	index  = index ^ (index >> 4);
	index *= 0x27d4eb2d;
	index  = index ^ (index >> 15);

	float3 raw_color = float3(
		(float)((index >> 0) & 0xFF) / 255.0f,
		(float)((index >> 8) & 0xFF) / 255.0f,
		(float)((index >> 16) & 0xFF) / 255.0f);

	return raw_color * 0.8f + 0.2f;
}

float3
calc_blinn_phong_directional_light_color(directional_light light, float3 surface_normal, float3 view_dir)
{
	const float3 surface_to_light = -light.direction;
	const float3 half_dir		  = normalize(surface_to_light + view_dir);

	const float diffuse	  = saturate(dot(surface_normal, surface_to_light));
	float		specular  = saturate(dot(surface_normal, half_dir));
	specular			 *= specular;	 // 2
	specular			 *= specular;	 // 4
	specular			 *= specular;	 // 8
	specular			 *= specular;	 // 16
	specular			 *= specular;	 // 32
	specular			 *= specular;	 // 64

	return light.color * light.intensity * (diffuse + specular);
}

float3
calc_blinn_phong_light_color(unified_light light, float3 surface_pos, float3 surface_normal, float3 view_dir)
{
	const float3 surface_to_light = light.position - surface_pos;
	const float	 distance		  = length(surface_to_light);

	// if (light.range < distance)
	//{
	//     return float3(0, 0, 0);
	// }

	const float3 color	   = cast<float3>(light.color);
	const float	 intensity = light.intensity;
	const float3 dir	   = cast<float3>(light.direction);
	const float	 cos_inner = light.cos_inner;
	const float	 cos_outer = light.cos_outer;

	const float3 surface_to_light_dir = surface_to_light / distance;
	const float	 cos_angle			  = dot(-surface_to_light_dir, dir);

	const float3 half_dir = normalize(surface_to_light_dir + view_dir);

	const float diffuse	  = saturate(dot(surface_normal, surface_to_light_dir));
	float		specular  = saturate(dot(surface_normal, half_dir));
	specular			 *= specular;	 // 2
	specular			 *= specular;	 // 4
	specular			 *= specular;	 // 8
	specular			 *= specular;	 // 16
	specular			 *= specular;	 // 32
	specular			 *= specular;	 // 64

	const float distance_ratio = distance / light.range;
	const float window		   = saturate(1.0 - distance_ratio * distance_ratio);
	const float attenuation	   = 1.0 / max(distance * distance, epsilon_1e4) * window * window;

	const float cone_falloff = saturate((cos_angle - cos_outer) / (cos_inner - cos_outer));

	return color * intensity * (diffuse + specular) * attenuation * cone_falloff * cone_falloff;
}

float
sample_shadow_pcf(texture_2d<float> atlas, sampler_cmp_state samp, float2 atlas_uv, float depth, float texel_size)
{
	float shadow = 0;
	for (int y = -1; y <= 1; ++y)
	{
		for (int x = -1; x <= 1; ++x)
		{
			shadow += sample_cmp(atlas, samp, atlas_uv + float2(x, y) * texel_size, depth);
		}
	}
	return shadow / 9.0;
}

float
sample_contact_shadow(float3 world_pos, float3 light_dir_ws, float3 surface_normal)
{
	const uint32 step_count	  = 16;
	const float	 max_distance = 0.3;
	const float	 thickness	  = 0.05;

	float3 ray_step = light_dir_ws * (max_distance / step_count);
	float3 ray_pos	= world_pos + light_dir_ws * 0.01 + surface_normal * 0.02;

	texture_2d<float> depth_tex = global_resource_buffer[depth_buffer_texture_id];

	for (uint32 i = 0; i < step_count; ++i)
	{
		ray_pos += ray_step;

		float4 clip = mul(view_proj, float4(ray_pos, 1));
		float2 uv	= float2(clip.x / clip.w * 0.5 + 0.5, -clip.y / clip.w * 0.5 + 0.5);

		if (any(uv < 0) || any(uv > 1))
		{
			return 1.0;
		}

		float scene_z = linearize_reverse_z(depth_tex[cast<int32_2>(uv * backbuffer_size)], cam_near_z, cam_far_z);
		float ray_z	  = clip.w;	   // view-space depth = clip.w (perspective projection)

		float depth_diff = ray_z - scene_z;

		if (depth_diff > 0.005 && depth_diff < thickness)
		{
			return 0.0;
		}
	}
	return 1.0;
}

float
sample_directional_shadow(float3 world_pos, float linear_depth, uint32 shadow_id)
{
	uint32 cascade_index = SHADOW_CASCADE_COUNT - 1;
	for (uint32 c = 0; c < SHADOW_CASCADE_COUNT; ++c)
	{
		const float split = load_cascade_split(c);

		if (linear_depth < split)
		{
			cascade_index = c;
			break;
		}
	}

	const shadow_light light = load_shadow_light(shadow_id + cascade_index);

	const float4 light_clip = mul(light.view_proj, float4(world_pos, 1.0));
	const float3 light_ndc	= light_clip.xyz / light_clip.w;

	const float2 shadow_uv = float2(light_ndc.x * 0.5 + 0.5, -light_ndc.y * 0.5 + 0.5);

	const uint32 col = (shadow_id + cascade_index) % SHADOW_ATLAS_SEG_U;
	const uint32 row = (shadow_id + cascade_index) / SHADOW_ATLAS_SEG_U;

	float2 atlas_uv;
	atlas_uv.x = col * (1.f / SHADOW_ATLAS_SEG_U) + shadow_uv.x / SHADOW_ATLAS_SEG_U;
	atlas_uv.y = row * (1.f / SHADOW_ATLAS_SEG_V) + shadow_uv.y / SHADOW_ATLAS_SEG_V;

	texture_2d<float> shadow_atlas = global_resource_buffer[shadow_atlas_id];

	static const float texel_size = 1.f / (float)(SHADOW_MAP_WIDTH * SHADOW_ATLAS_SEG_U);

	float shadow = 0;
	for (int32 y = -1; y <= 1; ++y)
	{
		for (int32 x = -1; x <= 1; ++x)
		{
			shadow += sample_cmp_level_zero(shadow_atlas, shadow_sampler, atlas_uv + float2(x, y) * texel_size, light_ndc.z);
		}
	}
	return shadow / 9.f;

	// return shadow_atlas.SampleCmp(shadow_sampler, atlas_uv, light_ndc.z);
}

float
sample_unified_shadow(float3 world_pos, uint32 shadow_id)
{
	const shadow_light light = load_shadow_light(shadow_id);

	const float4 light_clip = mul(light.view_proj, float4(world_pos, 1.0));
	const float3 light_ndc	= light_clip.xyz / light_clip.w;

	const float2 shadow_uv = float2(light_ndc.x * 0.5 + 0.5, -light_ndc.y * 0.5 + 0.5);

	const uint32 col = (shadow_id) % SHADOW_ATLAS_SEG_U;
	const uint32 row = (shadow_id) / SHADOW_ATLAS_SEG_U;

	float2 atlas_uv;
	atlas_uv.x = col * (1.f / SHADOW_ATLAS_SEG_U) + shadow_uv.x / SHADOW_ATLAS_SEG_U;
	atlas_uv.y = row * (1.f / SHADOW_ATLAS_SEG_V) + shadow_uv.y / SHADOW_ATLAS_SEG_V;

	texture_2d<float> shadow_atlas = global_resource_buffer[shadow_atlas_id];

	static const float texel_size = 1.f / (float)(SHADOW_MAP_WIDTH * SHADOW_ATLAS_SEG_U);

	float shadow = 0;
	for (int32 y = -1; y <= 1; ++y)
	{
		for (int32 x = -1; x <= 1; ++x)
		{
			shadow += sample_cmp_level_zero(shadow_atlas, shadow_sampler, atlas_uv + float2(x, y) * texel_size, light_ndc.z);
		}
	}
	return shadow / 9.f;

	// return shadow_atlas.SampleCmpLevelZero(shadow_sampler, atlas_uv, light_ndc.z);
}

uint32
get_unified_shadow_offset(const unified_light light, float3 world_pos)
{
	if (light.cos_outer == -2.h)
	{
		const float3 dir	 = world_pos - light.position;
		const float3 dir_abs = abs(dir);
		if (dir_abs.x >= dir_abs.y && dir_abs.x >= dir_abs.z)
		{
			return dir.x > 0 ? 0 : 1;	 // +X, -X
		}
		if (dir_abs.y >= dir_abs.x && dir_abs.y >= dir_abs.z)
		{
			return dir.y > 0 ? 2 : 3;	 // +Y, -Y
		}
		return dir.z > 0 ? 4 : 5;		 // +Z, -Z
	}
	else
	{
		return 0;
	}
}

float4
main_ps(opaque_ms_to_ps fragment) sv_target_0
{
	const float3 ambient_light = float3(0.03, 0.03, 0.03);
	const float3 albedo		   = float3(0.8, 0.8, 0.8);
	// const float3 albedo = get_random_color(fragment.meshlet_render_data_id);

	const float3 surface_normal = normalize(fragment.normal);

	const float3 view_dir = normalize(camera_pos - fragment.world_pos);

	const uint32 directional_light_count = directional_light_count_and_extra & 0xff;

	const uint32 tile_x	 = uint32(fragment.pos.x) / LIGHT_TILE_SIZE;
	const uint32 tile_y	 = uint32(fragment.pos.y) / LIGHT_TILE_SIZE;
	const uint32 tile_id = tile_x + tile_y * light_tile_count_x;

	float3 lighting = ambient_light;

	const float linear_depth = dot(fragment.world_pos - camera_pos, camera_forward);

	const uint32 bin = clamp(depth_to_bin(linear_depth), 0, Z_SLICE_COUNT - 1);

	const uint32 z_min = load_zbin_entry(bin).min_idx;
	const uint32 z_max = load_zbin_entry(bin).max_idx;

	const uint32 wave_z_min = wave_active_min(z_min);
	const uint32 wave_z_max = wave_active_max(z_max);

	const uint32 word_begin = wave_z_min / 32;
	const uint32 word_end	= wave_z_max / 32;

	const float3 ddx_pos = ddx(fragment.world_pos);
	const float3 ddy_pos = ddy(fragment.world_pos);
	const float3 normal	 = normalize(cross(ddx_pos, ddy_pos));

	// const float3 normal = fragment.normal;

	for (uint32 d = 0; d < directional_light_count; ++d)
	{
		const directional_light light = load_directional_light(d);

		float shadow  = 1.f;
		float contact = 1.f;

		uint32 shadow_id = light.shadow_id_and_extra & 0xf;

		if (shadow_id != 0xf)
		{
			float n_dot_l = saturate(dot(normal, -light.direction));
			shadow		  = sample_directional_shadow(fragment.world_pos, linear_depth, shadow_id);
			shadow		  = min(shadow, smoothstep(0.0, 0.05, n_dot_l));

			// contact = sample_contact_shadow(fragment.world_pos, -light.direction, surface_normal);
			// contact = lerp(1.0, contact, saturate(n_dot_l * 4.0));
		}
		lighting += shadow * contact * calc_blinn_phong_directional_light_color(light, surface_normal, view_dir);
	}

	for (uint32 w = word_begin; w <= word_end; ++w)
	{
		uint32 bit_mask = load_tile_mask(tile_id, w);
		bit_mask		= wave_active_bit_or(bit_mask);

		while (bit_mask != 0)
		{
			const uint32 bit		= first_bit_low(bit_mask);
			const uint32 sorted_id	= w * 32 + bit;
			bit_mask			   &= ~(1u << bit);

			const unified_light light = load_sorted_light(sorted_id);

			const uint32 shadow_id = light.shadow_id_and_extra & 0xf;

			float shadow  = 1.f;
			float contact = 1.f;

			if (shadow_id != 0xf)
			{
				const uint32 offset = get_unified_shadow_offset(light, fragment.world_pos);

				const float3 to_light = normalize(light.position - fragment.world_pos);
				const float	 n_dot_l  = saturate(dot(normal, to_light));
				shadow				  = sample_unified_shadow(fragment.world_pos, shadow_id + offset);

				shadow = min(shadow, smoothstep(0.0, 0.05, n_dot_l));

				// contact = sample_contact_shadow(fragment.world_pos, -light.direction, surface_normal);
				// contact = lerp(1.0, contact, saturate(n_dot_l * 4.0));
			}

			lighting += shadow * contact * calc_blinn_phong_light_color(light, fragment.world_pos, surface_normal, view_dir);
		}
	}

	// uint32 loop_count = 0;
	// for (uint32 w = word_begin; w <= word_end; ++w)
	//{
	//     const uint32 bit_mask = tile_mask_buffer_srv[tile_id * LIGHT_BITMASK_UINT32_COUNT + w];

	//    uint32 wave_mask = WaveActiveBitOr(bit_mask);

	//    while (wave_mask != 0)
	//    {
	//        const uint32 bit = firstbitlow(wave_mask);
	//        wave_mask &= ~(1u << bit);

	//        if (bit_mask & (1u << bit))
	//        {
	//            const uint32 sorted_id = w * 32 + bit;

	//            const unified_light light = unified_sorted_light_buffer_srv[sorted_id];
	//            lighting += calc_blinn_phong_light_color(light, fragment.world_pos, surface_normal, view_dir);
	//        }
	//    }
	//}
	// return float4(color, 1);
	// return float4(light_count / 4.f, 0, 0, 1);
	// return float4(0, 0, shadow_count / 8.f, 1);

	// return float4(normal, 1.f);
	return float4(lighting * albedo, 1.0f);
}
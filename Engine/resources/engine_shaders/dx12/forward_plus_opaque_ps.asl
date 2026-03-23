#include "forward_plus_common.asli"

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

float4
main_ps(opaque_ms_to_ps fragment) sv_target_0
{
	const float3 ambient_light = float3(0.03, 0.03, 0.03);
	const float3 albedo		   = float3(0.8, 0.8, 0.8);
	// const float3 albedo = get_random_color(fragment.meshlet_render_data_id);

	const float3 vertex_normal = normalize(fragment.normal);

	const float3 world_to_cam_dir = normalize(camera_pos - fragment.world_pos);

	float3 lighting = ambient_light;

	const float linear_depth = dot(fragment.world_pos - camera_pos, camera_forward);

	const float3 ddx_pos	 = ddx(fragment.world_pos);
	const float3 ddy_pos	 = ddy(fragment.world_pos);
	const float3 face_normal = normalize(cross(ddx_pos, ddy_pos));

	const uint32 directional_light_count = directional_light_count_and_extra & 0xff;

	for (uint32 d = 0; d < directional_light_count; ++d)
	{
		const directional_light light = load_directional_light(d);

		lighting += calc_directional_light(light, vertex_normal, world_to_cam_dir)
				  * calc_directional_shadow(light, fragment.world_pos, face_normal, linear_depth);
	}


	const uint32 tile_x	 = uint32(fragment.pos.x) / LIGHT_TILE_SIZE;
	const uint32 tile_y	 = uint32(fragment.pos.y) / LIGHT_TILE_SIZE;
	const uint32 tile_id = tile_x + tile_y * light_tile_count_x;

	const uint32 bin = clamp(depth_to_bin(linear_depth), 0, Z_SLICE_COUNT - 1);

	const uint32 z_min = load_zbin_entry(bin).min_idx;
	const uint32 z_max = load_zbin_entry(bin).max_idx;

	const uint32 wave_z_min = wave_active_min(z_min);
	const uint32 wave_z_max = wave_active_max(z_max);

	const uint32 word_begin = wave_z_min / 32;
	const uint32 word_end	= wave_z_max / 32;


	for (uint32 w = word_begin; w <= word_end; ++w)
	{
		uint32 bit_mask		 = load_tile_mask(tile_id, w);
		uint32 wave_bit_mask = wave_active_bit_or(bit_mask);

		while (wave_bit_mask != 0)
		{
			const uint32 bit		= first_bit_low(wave_bit_mask);
			const uint32 sorted_id	= w * 32 + bit;
			wave_bit_mask		   &= ~(1u << bit);

			// if (bit_mask & (1u << bit))
			{
				const unified_light light = load_sorted_light(sorted_id);

				lighting += calc_unified_light(light, fragment.world_pos, vertex_normal, world_to_cam_dir)
						  * calc_unified_shadow(light, fragment.world_pos, face_normal);
			}
		}
	}

	// return float4(color, 1);
	// return float4(light_count / 4.f, 0, 0, 1);
	// return float4(0, 0, shadow_count / 8.f, 1);

	// return float4(face_normal, 1.f);
	// return float4(vertex_normal, 1.f);

	// return float4(fragment.uv0, 0, 1.f);
	return float4(lighting * albedo, 1.0f);
}
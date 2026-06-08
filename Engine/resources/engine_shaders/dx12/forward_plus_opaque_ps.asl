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
[earlydepthstencil] float4
main_ps(opaque_ms_to_ps fragment) sv_target_0 {
	const uint32	 mat_id = fragment.mat_id;
	const vertex_fat v		= fragment.v;

	// v.world_pos = ndc_to_world(view_proj_inv, float3(screen_to_ndc(v.pos.xy, inv_backbuffer_size), v.pos.z));

	const material mat = load_material(mat_id);

	const pbr_surface_data surface_data = calc_pbr_surface(camera_pos, mat, v);

	const float3 ddx_pos		   = ddx(v.world_pos);
	const float3 ddy_pos		   = ddy(v.world_pos);
	const float3 world_face_normal = normalize(cross(ddx_pos, ddy_pos));

	float3 ambient_light = float3(0, 0, 0);

	attr_branch()

	if (ddgi_enabled())
	{
		// todo need fresnel?
		// from https://google.github.io/filament/Filament.md.html
		const float3 f_avg = surface_data.f0 + (float3(1.f, 1.f, 1.f) - surface_data.f0) / 21;

		const float3 gi_diffuse	 = calc_pbr_ddgi(surface_data, world_face_normal);
		ambient_light			+= (1.f - f_avg) * gi_diffuse * surface_data.occlusion;

		expand(MAX_ENV_LIGHT)

		for (uint32 i = 0; i < env_light_count; ++i)
		{
			ambient_light += calc_pbr_ibl_specular(surface_data, load_env_light(i)) * surface_data.occlusion;
		}
	}
	else if (gibs_enabled())
	{
		const float3 f_avg = surface_data.f0 + (float3(1.f, 1.f, 1.f) - surface_data.f0) / 21;

		const float3 gi_diffuse	 = calc_pbr_gibs(surface_data, world_face_normal);
		ambient_light			+= (1.f - f_avg) * gi_diffuse * surface_data.occlusion;

		expand(MAX_ENV_LIGHT)

		for (uint32 i = 0; i < env_light_count; ++i)
		{
			ambient_light += calc_pbr_ibl_specular(surface_data, load_env_light(i)) * surface_data.occlusion;
		}
	}
	else
	{
		expand(MAX_ENV_LIGHT)

		for (uint32 i = 0; i < env_light_count; ++i)
		{
			ambient_light += calc_pbr_ibl(surface_data, load_env_light(i));
		}
	}

	float3 lighting	 = ambient_light;
	lighting		+= surface_data.emissive;
	// const float3 albedo		   = srgb_to_linear(float3(0.8, 0.8, 0.8));

	const float3 albedo = mat.base_color_factor.rgb;

	const float3 vertex_normal = normalize(v.normal);

	const float3 world_to_cam_dir = normalize(camera_pos - v.world_pos);


	const uint32 directional_light_count = directional_light_count_and_extra & 0xff;

	for (uint32 d = 0; d < directional_light_count; ++d)
	{
		const directional_light light = load_directional_light(d);

		lighting += calc_pbr_light(surface_data, light)
				  * calc_directional_shadow_rt(light, v, world_face_normal);
	}

	const uint32_3 light_bin_axis = world_to_light_bin_axis(v.world_pos);

	const zbin_entry x_entry = load_bin_entry_x(light_bin_axis.x);
	const zbin_entry y_entry = load_bin_entry_y(light_bin_axis.y);
	const zbin_entry z_entry = load_bin_entry_z(light_bin_axis.z);

	const uint32 min_id = max(x_entry.min_idx, max(y_entry.min_idx, z_entry.min_idx));
	const uint32 max_id = min(x_entry.max_idx, min(y_entry.max_idx, z_entry.max_idx));

	const uint32 wave_min = wave_active_min(min_id);
	const uint32 wave_max = wave_active_max(max_id);

	const uint32 word_begin = wave_min / 32;
	const uint32 word_end	= wave_max / 32;

	uint32 c = 0u;
	for (uint32 w = word_begin; w <= word_end; ++w)
	{
		uint32 x_mask	= load_bin_mask_x(light_bin_axis.x, w);
		uint32 y_mask	= load_bin_mask_y(light_bin_axis.y, w);
		uint32 z_mask	= load_bin_mask_z(light_bin_axis.z, w);
		uint32 bit_mask = x_mask & y_mask & z_mask;

		uint32 wave_bit_mask = wave_active_bit_or(bit_mask);

		while (wave_bit_mask != 0)
		{
			const uint32 bit		= first_bit_low(wave_bit_mask);
			const uint32 sorted_id	= w * 32 + bit;
			wave_bit_mask		   &= ~(1u << bit);

			if (bit_mask & (1u << bit))
			{
				const unified_light light = load_sorted_light(sorted_id);

				lighting += calc_pbr_light(surface_data, light)
						  * calc_unified_shadow_rt(light, v, world_face_normal);

				++c;
			}
		}
	}

	// return float4(c / 10.f, c / 100.f, c, 1.f);

	return float4(lighting, 1.0f);
}
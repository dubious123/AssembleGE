#include "forward_plus_common.asli"

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

		const float3 gi_diffuse	 = calc_pbr_gibs(rt_arg::init_gibs(false), invalid_id_uint32, surface_data, world_face_normal);
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

	// return float4(vertex_normal, 1.f);

	// return float4(c / 10.f, c / 100.f, c, 1.f);

	return float4(lighting, 1.0f);
}
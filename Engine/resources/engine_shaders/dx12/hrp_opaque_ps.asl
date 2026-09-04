#include "hrp_common.asli"

float4
main_ps(float4 pos sv_position) sv_target_0
{
	const gibs_lut_data		lut_data	 = gibs::load_lut_data();
	const texture_2d<float> depth_buffer = global_resource_buffer[opaque_depth_buffer_srv_id];

	uint32_2	px		= uint32_2(pos.xy);
	const float z_depth = depth_buffer[px];

	if (z_depth == 0.f)
	{
		discard;
		return zero<float4>();
	}

	c_auto surface_point  = mesh::calc_surface_point(global_resource_buffer[opaque_gbuffer_srv_id], px, z_depth);
	auto   pbr_surface	  = calc_pbr_surface_screen(px, z_depth);
	pbr_surface.world_pos = surface_point.v.world_pos;

	float3 ambient_light = float3(0, 0, 0);

	// const float3 local_face_normal = normalize(cross(v1.pos.xyz - v0.pos.xyz, v2.pos.xyz - v0.pos.xyz));
	// const float3 world_face_normal = normalize(rotate(local_face_normal / cast<float3>(obj_data.scale), decode_quaternion(obj_data.quaternion)));

	attr_branch()

	if (ddgi_enabled())
	{
		// todo need fresnel?
		// from https://google.github.io/filament/Filament.md.html
		const float3 f_avg = pbr_surface.f0 + (float3(1.f, 1.f, 1.f) - pbr_surface.f0) / 21;

		const float3 gi_diffuse	 = calc_pbr_ddgi(pbr_surface, pbr_surface.vertex_normal);
		ambient_light			+= (1.f - f_avg) * gi_diffuse * pbr_surface.occlusion;

		expand(MAX_ENV_LIGHT)

		for (uint32 i = 0; i < env_light_count; ++i)
		{
			ambient_light += calc_pbr_ibl_specular(pbr_surface, load_env_light(i)) * pbr_surface.occlusion;
		}
	}
	else if (gibs::enabled())
	{
		texture_2d<float3> gi_resolve_buffer = global_resource_buffer[gibs::load_data().h_gi_resolve_curr_buffer_srv_id];

		ambient_light += calc_gi(pbr_surface, gi_resolve_buffer[px]);
	}
	else if (gist::enabled())
	{
		ambient_light += gist::calc_opaque_gi(gist::load_data(), pbr_surface, px);
	}
	else
	{
		expand(MAX_ENV_LIGHT)

		for (uint32 i = 0; i < env_light_count; ++i)
		{
			ambient_light += calc_pbr_ibl(pbr_surface, load_env_light(i));
		}
	}

	float3 lighting = ambient_light;

	lighting += calc_di<true>(pbr_surface, surface_point.world_face_normal);

	//{
	//	float3 res = zero<float3>();

	//	res += surface_data.emissive;

	//	const uint32 directional_light_count = directional_light_count_and_extra & 0xff;

	//	for (uint32 d = 0; d < directional_light_count; ++d)
	//	{
	//		const directional_light light = load_directional_light(d);

	//		res += calc_pbr_light(surface_data, light)
	//			 * calc_directional_shadow_rt(light, surface_data.world_pos, world_face_normal);
	//	}


	//	const uint32_3 light_bin_axis = world_to_light_bin_axis(surface_data.world_pos);

	//	const zbin_entry x_entry = load_bin_entry_x(light_bin_axis.x);
	//	const zbin_entry y_entry = load_bin_entry_y(light_bin_axis.y);
	//	const zbin_entry z_entry = load_bin_entry_z(light_bin_axis.z);

	//	const uint32 min_id = max(x_entry.min_idx, max(y_entry.min_idx, z_entry.min_idx));
	//	const uint32 max_id = min(x_entry.max_idx, min(y_entry.max_idx, z_entry.max_idx));

	//	const uint32 word_begin = wave_active_min(min_id) / 32;
	//	const uint32 word_end	= wave_active_max(max_id) / 32;

	//	for (uint32 w = word_begin; w <= word_end; ++w)
	//	{
	//		const uint32 x_mask	  = load_bin_mask_x(light_bin_axis.x, w);
	//		const uint32 y_mask	  = load_bin_mask_y(light_bin_axis.y, w);
	//		const uint32 z_mask	  = load_bin_mask_z(light_bin_axis.z, w);
	//		const uint32 bit_mask = x_mask & y_mask & z_mask;

	//		uint32 wave_bit_mask = wave_active_bit_or(bit_mask);

	//		while (wave_bit_mask != 0)
	//		{
	//			const uint32 bit	   = first_bit_low(wave_bit_mask);
	//			const uint32 sorted_id = w * 32 + bit;
	//			// wave_bit_mask		   &= ~(1u << bit);
	//			wave_bit_mask &= (wave_bit_mask - 1u);

	//			if ((bit_mask & (1u << bit)))
	//			{
	//				const unified_light light = load_sorted_light(sorted_id);

	//				res += calc_pbr_light(surface_data, light)
	//					 * calc_unified_shadow_rt(light, surface_data.world_pos, world_face_normal);
	//			}
	//		}
	//	}

	//	lighting += res;
	//}

	// return float4(world_face_normal, 1.f);
	// return float4(vertex_normal, 1.f);

	// return float4(c / 10.f, c / 100.f, c, 1.f);

	//// motion
	// const object_data obj_data_prev = load_object_prev_data(render_data.object_id);

	//// todo, maybe reconstruct prev_local_pos for skinning or animated object?
	// const float3 world_pos_prev = rotate(obj_data_prev.quaternion, local_pos * obj_data_prev.scale) + obj_data_prev.pos;

	// const float3 ndc_prev		= world_to_ndc(view_proj_prev, world_pos_prev);
	// const float2 screen_uv_prev = ndc_xy_to_screen_uv(ndc_prev.xy);

	// const float2 motion = pos.xy * inv_backbuffer_size - screen_uv_prev;

	return float4(lighting, 1.f);
}
#include "hrp_common.asli"

float4
main_ps(float4 pos sv_position) sv_target_0
{
	const gibs_lut_data		   lut_data	 = gibs::load_lut_data();
	const texture_2d<float>	   depth_tex = global_resource_buffer[opaque_depth_buffer_srv_id];
	const texture_2d<uint32_2> gbuffer	 = global_resource_buffer[opaque_gbuffer_srv_id];

	uint32_2	px		= uint32_2(pos.xy);
	const float z_depth = depth_tex[px];

	if (z_depth == 0.f)
	{
		discard;
		return zero<float4>();
	}

	const pbr_surface_data surface_data = calc_pbr_surface_screen(px, z_depth);

	float3 ambient_light = float3(0, 0, 0);

	// const float3 local_face_normal = normalize(cross(v1.pos.xyz - v0.pos.xyz, v2.pos.xyz - v0.pos.xyz));
	// const float3 world_face_normal = normalize(rotate(local_face_normal / cast<float3>(obj_data.scale), decode_quaternion(obj_data.quaternion)));

	attr_branch()

	if (ddgi_enabled())
	{
		// todo need fresnel?
		// from https://google.github.io/filament/Filament.md.html
		const float3 f_avg = surface_data.f0 + (float3(1.f, 1.f, 1.f) - surface_data.f0) / 21;

		const float3 gi_diffuse	 = calc_pbr_ddgi(surface_data, surface_data.vertex_normal);
		ambient_light			+= (1.f - f_avg) * gi_diffuse * surface_data.occlusion;

		expand(MAX_ENV_LIGHT)

		for (uint32 i = 0; i < env_light_count; ++i)
		{
			ambient_light += calc_pbr_ibl_specular(surface_data, load_env_light(i)) * surface_data.occlusion;
		}
	}
	else if (gibs::enabled())
	{
		texture_2d<float3> gi_resolve_buffer = global_resource_buffer[gibs::load_data().h_gi_resolve_curr_buffer_srv_id];

		ambient_light += calc_gi(surface_data, gi_resolve_buffer[px]);
	}
	else if (gist::enabled())
	{
		texture_2d<float3> gi_resolve_buffer = global_resource_buffer[gist::load_data().h_gi_resolve_curr_buffer_srv_id];

		ambient_light += calc_gi(surface_data, gi_resolve_buffer[px]);
	}
	else
	{
		expand(MAX_ENV_LIGHT)

		for (uint32 i = 0; i < env_light_count; ++i)
		{
			ambient_light += calc_pbr_ibl(surface_data, load_env_light(i));
		}
	}

	float3 lighting = ambient_light;

	lighting += calc_di<true>(surface_data, surface_data.vertex_normal);

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
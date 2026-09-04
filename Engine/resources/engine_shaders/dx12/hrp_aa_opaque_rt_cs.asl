#include "hrp_common.asli"

float
sample_ao(float2 screen_pos, float3 world_pos, float3 normal)
{
	attr_branch()

	if (ao::enabled() is_false)
	{
		return 1.f;
	}

	texture_2d<float4> ao_buffer = global_resource_buffer[ao::load_data().h_ao_buffer_srv_id];
	return denoise::sample_bilateral(ao_buffer, screen_pos, world_pos, normal).x;
}

struct opaque_aa_gi_func
{
	float2 screen_pos;

	float3
	operator()(const in pbr_surface_data surface_data, const in float3 face_normal)
	{
		float3 ambient_light = zero<float3>();

		attr_branch()

		if (ddgi_enabled())
		{
			// todo need fresnel?
			// from https://google.github.io/filament/Filament.md.html
			const float3 f_avg = surface_data.f0 + (float3(1.f, 1.f, 1.f) - surface_data.f0) / 21;

			const float3 gi_diffuse	 = calc_pbr_ddgi(surface_data, face_normal);
			ambient_light			+= (1.f - f_avg) * gi_diffuse * surface_data.occlusion;

			expand(MAX_ENV_LIGHT)

			for (uint32 i = 0; i < env_light_count; ++i)
			{
				ambient_light += calc_pbr_ibl_specular(surface_data, load_env_light(i)) * surface_data.occlusion;
			}
		}
		else if (gibs::enabled())
		{
			// todo, shading normal??
			const float	 ao			= sample_ao(screen_pos, surface_data.world_pos, face_normal);
			const float3 irradiance = ao * gibs::sample_screen_irradiance(screen_pos, invalid_id_uint32, surface_data.world_pos, face_normal);

			ambient_light += calc_gi(surface_data, irradiance);
		}
		else if (gist::enabled())
		{
			const float4 irradiance = gist::sample_irradiance<true, false>(gist::load_data(), surface_data.world_pos, surface_data.vertex_normal);

			ambient_light += calc_gi(surface_data, irradiance.xyz);
		}
		else
		{
			expand(MAX_ENV_LIGHT)

			for (uint32 i = 0; i < env_light_count; ++i)
			{
				ambient_light += calc_pbr_ibl(surface_data, load_env_light(i));
			}
		}

		return ambient_light;
	}
};

[numthreads(AGE_WAVE_SIZE, 1, 1)] void
main_cs(uint32 dispatch_thread_id sv_dispatch_thread_id,
		uint32 group_thread_id	  sv_group_thread_id)

{
	const aa_data data = load_aa_data();

	byte_address_buffer ray_buffer = global_resource_buffer[data.h_ray_buffer_srv_id];

	const uint32_2 scratch			= load<uint32_2>(ray_buffer, sizeof(uint16_2) * data.px_headroom);
	const uint32   opaque_rpp		= scratch.x;
	const uint32   opaque_ray_count = scratch.y;

	if (dispatch_thread_id >= opaque_ray_count)
	{
		return;
	}


	// rpp is pow of 2
	// rpp < 32

	// entry_id = tid / rpp
	const uint32 entry_id = dispatch_thread_id >> first_bit_high(opaque_rpp);

	// local_id = tid % rpp
	const uint16 local_id = cast<uint16>(dispatch_thread_id & (opaque_rpp - 1u));


	const uint16_2 px = load<uint16_2>(ray_buffer, sizeof(uint16_2) * entry_id);

	// px + 0.5 + rng( [0, 1] ) - 0.5
	const float2 screen_pos = px + random_r2_sequence(1 + local_id);

	const float3 world_far = ndc_to_world(view_proj_inv, screen_to_ndc(screen_pos, 0.f, inv_backbuffer_size));

	const float3 ray_dir = normalize(world_far - camera_pos);

	opaque_aa_gi_func func;
	func.screen_pos = screen_pos;

	const float3 col = lighting::calc_ray_color<true, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, false, RAY_FLAG_NONE, true, opaque_aa_gi_func>(func, camera_pos, ray_dir).rgb;

	const float3 col_prefix = wave_prefix_sum(col);

	// if (local_id == opaque_rpp - 1)
	//{
	const float3 col_prefix_prev = group_thread_id >= opaque_rpp ? wave_read_lane_at(col_prefix, group_thread_id - opaque_rpp + 1) : zero<float3>();
	const float3 col_sum		 = col_prefix - col_prefix_prev + col;

	rw_texture_2d<float4> res_tex = global_resource_buffer[blend_buffer_uav_id];

	if (local_id == opaque_rpp - 1)
	{
		res_tex[px] = float4(max(zero<float3>(), col_sum) / opaque_rpp, 1.f - 1.f / (opaque_rpp + 1));
	}
	//}
}
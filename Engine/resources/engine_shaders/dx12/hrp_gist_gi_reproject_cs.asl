#include "hrp_common.asli"

[numthreads(16, 16, 1)] void
main_cs(uint32_3 dispatch_thread_id sv_dispatch_thread_id)

{
	const gist_data data = gist::load_data();

	const int32_2 extent = int32_2(backbuffer_size);
	const int32_2 px	 = int32_2(dispatch_thread_id.xy);

	if (any(px >= extent)) { return; }

	rw_texture_2d<uint32> gi_resolve_age_curr_buffer = global_resource_buffer[data.h_gi_resolve_age_curr_buffer_uav_id];
	texture_2d<uint32>	  gi_resolve_age_prev_buffer = global_resource_buffer[data.h_gi_resolve_age_prev_buffer_srv_id];

	rw_texture_2d<float2> gi_resolve_moments_curr_buffer = global_resource_buffer[data.h_gi_resolve_moments_curr_buffer_uav_id];
	texture_2d<float2>	  gi_resolve_moments_prev_buffer = global_resource_buffer[data.h_gi_resolve_moments_prev_buffer_srv_id];

	texture_2d<uint32_2> opaque_geo_prev_buffer = global_resource_buffer[opaque_geo_prev_buffer_srv_id];

	rw_texture_2d<float3> gi_resolve_curr_buffer = global_resource_buffer[data.h_gi_resolve_curr_buffer_uav_id];
	texture_2d<float3>	  gi_resolve_prev_buffer = global_resource_buffer[data.h_gi_resolve_prev_buffer_srv_id];

	texture_2d<float>	 depth_buffer  = global_resource_buffer[opaque_depth_buffer_srv_id];
	texture_2d<uint32_2> gbuffer	   = global_resource_buffer[opaque_gbuffer_srv_id];
	texture_2d<float2>	 motion_buffer = global_resource_buffer[motion_buffer_srv_id];

	const float px_depth = depth_buffer[px];

	if (px_depth == 0.f)
	{
		gi_resolve_curr_buffer[px]		   = zero<float3>();
		gi_resolve_moments_curr_buffer[px] = zero<float2>();
		gi_resolve_age_curr_buffer[px]	   = GIST_GI_RESOLVE_MAX_AGE;
		return;
	}

	const float2 motion		  = denoise::sample_motion(motion_buffer, depth_buffer, px, extent);
	const float3 px_normal	  = decode_oct_snorm16(gbuffer[px].y);
	const float3 px_world_pos = screen_px_to_world(px, px_depth, inv_backbuffer_size, view_proj_inv);
	const float	 px_z_lin	  = calc_linear_z_reversed(cam_near_z, cam_far_z, px_depth);

	const denoise::reproject_taps rp_taps = denoise::reproject_taps::init(opaque_geo_prev_buffer, px, extent, motion, px_world_pos, px_normal, px_z_lin);

	if (rp_taps.is_prev_valid and rp_taps.w_sum > 0.f)
	{
		float3 gi_sum	   = zero<float3>();
		float2 moments_sum = zero<float2>();
		float  gi_age_sum  = 0.f;

		expand_all()

		for (uint32 i = 0; i < 4; ++i)
		{
			if (rp_taps.w[i] <= 0.f) { continue; }

			gi_sum		+= gi_resolve_prev_buffer[rp_taps.px[i]] * rp_taps.w[i];
			moments_sum += gi_resolve_moments_prev_buffer[rp_taps.px[i]] * rp_taps.w[i];
			gi_age_sum	+= float(gi_resolve_age_prev_buffer[rp_taps.px[i]]) * rp_taps.w[i];
		}

		const float3 rng_gi = random_pcg3d(uint32_3(uint32(px.x), uint32(px.y), frame_index + g::shader_hash));
		const float3 rng_m	= random_pcg3d(uint32_3(uint32(px.y), uint32(px.x), frame_index + g::shader_hash + 77777u));

		gi_resolve_curr_buffer[px]		   = round_fp16_stochastic(gi_sum / rp_taps.w_sum, rng_gi);
		gi_resolve_moments_curr_buffer[px] = round_fp16_stochastic(moments_sum / rp_taps.w_sum, rng_m.xy);

		const float gi_age_avg = gi_age_sum / rp_taps.w_sum;
		const float gi_age_res = rp_taps.quality < 1.f
								   ? gi_age_avg * sqrt(rp_taps.quality)
								   : gi_age_avg;

		gi_resolve_age_curr_buffer[px] = min(uint32(round(gi_age_res)), uint32(GIST_GI_RESOLVE_MAX_AGE));
	}
	else
	{
		gi_resolve_age_curr_buffer[px]	   = 0u;
		gi_resolve_moments_curr_buffer[px] = zero<float2>();

		const float4 gi_irradiance_sum = gist::gather_neighbor_gi(gi_resolve_prev_buffer, opaque_geo_prev_buffer, px, px_world_pos, px_normal, px_z_lin, extent);

		gi_resolve_curr_buffer[px] = gi_irradiance_sum.w > 0.f
									   ? gi_irradiance_sum.xyz / gi_irradiance_sum.w
									   : zero<float3>();
	}
}
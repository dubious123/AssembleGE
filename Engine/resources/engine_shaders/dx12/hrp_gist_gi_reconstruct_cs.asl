#include "hrp_common.asli"

[numthreads(16, 16, 1)] void
main_cs(uint32_3 thread_id sv_dispatch_thread_id)

{
	const gist_data data = gist::load_data();

	const int32_2 extent = int32_2(backbuffer_size);

	if (any(thread_id.xy >= uint32_2(extent))) { return; }

	const int32_2 px = int32_2(thread_id.xy);

	const uint16 step		= uint32_lower_to_uint16(rc_scratch_0);
	const bool	 is_first	= uint32_upper_to_uint16(rc_scratch_0) == 1;
	const bool	 is_last	= uint32_upper_to_uint16(rc_scratch_0) == 2;
	const uint32 src_srv_id = rc_scratch_1;
	const uint32 dst_uav_id = rc_scratch_2;

	// step : power of two
	const uint32 pass_idx	   = firstbithigh(uint32(step));
	const uint32 var_write_idx = pass_idx & 1u;
	const uint32 var_read_idx  = var_write_idx ^ 1u;

	texture_2d<float4>	  src_buffer = global_resource_buffer[src_srv_id];
	rw_texture_2d<float4> dst_buffer = global_resource_buffer[dst_uav_id];
	rw_texture_2d<float2> var_buffer = global_resource_buffer[data.h_gi_resolve_moments_prev_buffer_uav_id];

	texture_2d<float2> moments_buffer = global_resource_buffer[data.h_gi_resolve_moments_curr_buffer_srv_id];
	texture_2d<uint32> age_buffer	  = global_resource_buffer[data.h_gi_resolve_age_curr_buffer_srv_id];

	texture_2d<float>	 depth_buffer	= global_resource_buffer[opaque_depth_buffer_srv_id];
	texture_2d<uint32_2> opaque_gbuffer = global_resource_buffer[opaque_gbuffer_srv_id];
	const float3		 px_normal		= decode_oct_snorm16(opaque_gbuffer[px].y);
	const float			 px_depth		= depth_buffer[px];

	// sky
	if (px_depth == 0.f)
	{
		dst_buffer[px] = zero<float4>();
		var_buffer[px] = zero<float2>();
		return;
	}

	const uint32 age	  = age_buffer[px];
	const bool	 is_young = age < 4u;

	const int32 stride = is_young ? int32(step) * 2 : int32(step);

	const float px_z_lin = calc_linear_z_reversed(cam_near_z, cam_far_z, px_depth);

	const float3 px_world_pos = screen_px_to_world(px, px_depth, inv_backbuffer_size, view_proj_inv);

	const float3 view_dir = normalize(camera_pos - px_world_pos);
	const float	 n_dot_v  = max(dot(px_normal, view_dir), 0.1f);

	const float4 src_center = src_buffer[px];
	const float	 lum_center = min(luminance_rec709(src_center.xyz), 256.f);

	float var_center;
	bool  lum_gate;

	if (is_first)
	{
		const float2 m = moments_buffer[px];

		var_center = max(epsilon_1e6, m.y - m.x * m.x);
		lum_gate   = is_young is_false;
	}
	else
	{
		var_center = var_buffer[px][var_read_idx];
		lum_gate   = true;
	}

	const float inv_lum_sigma = 1.f / (8.f * sqrt(var_center) + epsilon_1e6);

	float4 gi_res	  = zero<float4>();
	float  var_sum	  = 0.f;
	float  lum_m1_sum = 0.f;
	float  lum_m2_sum = 0.f;
	float  kernel_sum = 0.f;

	static const float k_atrous[5] = { 0.0625f, 0.25f, 0.375f, 0.25f, 0.0625f };

	for (int32 dy = -2; dy <= 2; ++dy)
	{
		for (int32 dx = -2; dx <= 2; ++dx)
		{
			const int32_2 px_tap	   = clamp(px + int32_2(dx, dy) * stride, zero<int32_2>(), extent - 1);
			const float	  px_depth_tap = depth_buffer[px_tap];

			const float kernel_w = k_atrous[dx + 2] * k_atrous[dy + 2];

			kernel_sum += kernel_w;

			if (px_depth_tap == 0.f) { continue; }

			const float3 px_normal_tap	  = decode_oct_snorm16(opaque_gbuffer[px_tap].y);
			const float3 px_world_pos_tap = ndc_to_world(view_proj_inv, screen_to_ndc(float2(px_tap) + 0.5f, px_depth_tap, inv_backbuffer_size));

			const float4 src	 = src_buffer[px_tap];
			const float	 lum_tap = min(luminance_rec709(src.xyz), 256.f);

			const int32_2 px_mid   = (px + px_tap) / 2;
			const float	  conn_mid = src_buffer[px_mid].w;

			const float connect_w = is_first ? 1.f : src.w * conn_mid;

			const float cos_theta = dot(px_normal, px_normal_tap);

			const float3 rel  = px_world_pos_tap - px_world_pos;
			const float	 asym = abs(dot(rel, px_normal + px_normal_tap)) * 0.5f;

			const float px_size_y_per_z = 2 * tan_fov_y_half * inv_backbuffer_size.y;
			const float tolerance		= px_z_lin * px_size_y_per_z * 2.f / n_dot_v;

			const float lum_w = lum_gate ? exp(-abs(lum_center - lum_tap) * inv_lum_sigma) : 1.f;

			const float w = kernel_w
						  * connect_w
						  * pow(max(cos_theta, 0.f), 32.f)
						  * exp(-asym / tolerance)
						  * lum_w;

			const float var_tap = is_first
									? max(0.f, moments_buffer[px_tap].y - moments_buffer[px_tap].x * moments_buffer[px_tap].x)
									: var_buffer[px_tap][var_read_idx];

			var_sum	   += var_tap * w * w;
			lum_m1_sum += lum_tap * w;
			lum_m2_sum += lum_tap * lum_tap * w;

			gi_res += float4(src.xyz, 1.f) * w;
		}
	}

	const float connect_res = clamp(gi_res.w / kernel_sum, 0.05f, 1.f);

	float var_res;

	if (gi_res.w > 0.f)
	{
		gi_res.xyz = gi_res.xyz / gi_res.w;
		var_res	   = var_sum / (gi_res.w * gi_res.w);

		if (is_first and is_young)
		{
			const float m1 = lum_m1_sum / gi_res.w;
			const float m2 = lum_m2_sum / gi_res.w;

			var_res = max(0.f, m2 - m1 * m1);
		}
	}
	else
	{
		gi_res.xyz = src_center.xyz;
		var_res	   = var_center;
	}

	if (is_last and ao::enabled())
	{
		texture_2d<float3> ao_buffer  = global_resource_buffer[ao::load_data().h_ao_buffer_srv_id];
		gi_res.xyz					 *= ao_buffer[px].x;
	}

	float2 var		   = var_buffer[px];
	var[var_write_idx] = var_res;
	var_buffer[px]	   = var;

	float3 gi_out = gi_res.xyz;
	if (is_first)
	{
		gi_out = round_fp16_stochastic(gi_res.xyz, random_pcg3d(uint32_3(uint32(px.x), uint32(px.y), g::shader_hash + frame_index)));
	}

	dst_buffer[px] = float4(gi_out, connect_res);
}
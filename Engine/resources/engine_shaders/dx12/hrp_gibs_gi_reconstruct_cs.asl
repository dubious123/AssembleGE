#include "hrp_common.asli"

wave_size(AGE_WAVE_SIZE)
[numthreads(16, 16, 1)] void
main_cs(uint32_3 thread_id sv_dispatch_thread_id)

{
	const gibs_data data = gibs::load_data();

	const int32_2 extent = int32_2(backbuffer_size);

	if (any(thread_id.xy >= uint32_2(extent))) { return; }

	const int32_2 px = int32_2(thread_id.xy);

	const uint16 step		= uint32_lower_to_uint16(rc_scratch_0);
	const bool	 is_first	= uint32_upper_to_uint16(rc_scratch_0) == 1;
	const bool	 is_last	= uint32_upper_to_uint16(rc_scratch_0) == 2;
	const uint32 src_srv_id = rc_scratch_1;
	const uint32 dst_uav_id = rc_scratch_2;

	texture_2d<float4>	  src_buffer = global_resource_buffer[src_srv_id];
	rw_texture_2d<float4> dst_buffer = global_resource_buffer[dst_uav_id];

	texture_2d<float>	 depth_buffer	= global_resource_buffer[opaque_depth_buffer_srv_id];
	texture_2d<uint32_2> opaque_gbuffer = global_resource_buffer[opaque_gbuffer_srv_id];
	const float3		 px_normal		= decode_oct_snorm16(opaque_gbuffer[px].y);
	const float			 px_depth		= depth_buffer[px];

	// sky
	if (px_depth == 0.f)
	{
		dst_buffer[px] = zero<float4>();
		return;
	}

	const float px_z_lin = calc_linear_z_reversed(cam_near_z, cam_far_z, px_depth);

	const float3 px_world_pos = ndc_to_world(view_proj_inv, screen_to_ndc(float2(px) + 0.5f, px_depth, inv_backbuffer_size));

	const float3 view_dir = normalize(camera_pos - px_world_pos);
	const float	 n_dot_v  = max(dot(px_normal, view_dir), 0.1f);

	const float plane_threshold = 0.001f * px_z_lin / n_dot_v * float(step);

	float4 res		  = zero<float4>();
	float  kernel_sum = 0.f;

	static const float k_atrous[3] = { 0.25f, 0.5f, 0.25f };

	for (int32 dy = -1; dy <= 1; ++dy)
	{
		for (int32 dx = -1; dx <= 1; ++dx)
		{
			const int32_2 px_tap	   = clamp(px + int32_2(dx, dy) * int32(step), zero<int32_2>(), extent - 1);
			const float	  px_depth_tap = depth_buffer[px_tap];

			const float kernel_w = k_atrous[dx + 1] * k_atrous[dy + 1];

			kernel_sum += kernel_w;

			if (px_depth_tap == 0.f) { continue; }

			const float3 px_normal_tap	  = decode_oct_snorm16(opaque_gbuffer[px_tap].y);
			const float3 px_world_pos_tap = ndc_to_world(view_proj_inv, screen_to_ndc(float2(px_tap) + 0.5f, px_depth_tap, inv_backbuffer_size));

			const float4 src = src_buffer[px_tap];

			const int32_2 px_mid   = (px + px_tap) / 2;
			const float	  conn_mid = src_buffer[px_mid].w;

			const float connect_w = is_first ? 1.f : src.w * conn_mid;

			const float dist_to_plane = abs(dot(px_world_pos_tap - px_world_pos, px_normal));
			const float cos_theta	  = dot(px_normal, px_normal_tap);

			const float w = kernel_w
						  * connect_w
						  * pow(max(cos_theta, 0.f), 32.f)
						  * exp(-dist_to_plane / plane_threshold);

			res += float4(src.xyz, 1.f) * w;
		}
	}

	const float connect_res = clamp(res.w / kernel_sum, 0.05f, 1.f);

	res.xyz = res.w > 0.f ? res.xyz / res.w : src_buffer[px].xyz;

	if (is_last and ao::enabled())
	{
		texture_2d<float3> ao_buffer  = global_resource_buffer[ao::load_data().h_ao_buffer_srv_id];
		res.xyz						 *= ao_buffer[px].x;
	}

	dst_buffer[px] = float4(res.xyz, connect_res);
}
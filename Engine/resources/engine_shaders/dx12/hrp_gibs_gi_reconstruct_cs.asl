#include "hrp_common.asli"

// float4
// bilateral_upsample(int32_2 center_px, float center_depth, float3 center_normal)
//{
//	const gibs_data data = gibs_load_gibs_data();
//
//	uint32_2 extent_dst = uint32_2(backbuffer_size.x, backbuffer_size.y);
//	uint32_2 extent_src = uint32_2(ceil(extent_dst.x, GIBS_GI_RESOLVE_SCALE), ceil(extent_dst.y, GIBS_GI_RESOLVE_SCALE));
//
//	texture_2d<float3> src_buffer = global_resource_buffer[data.h_gi_resolve_low_res_buffer_srv_id];
//
//	texture_2d<float>	 depth_buffer = global_resource_buffer[opaque_depth_buffer_srv_id];
//	texture_2d<uint32_2> gbuffer	  = global_resource_buffer[opaque_gbuffer_srv_id];
//
//	float4 res = zero<float4>();
//
//	const float center_z_lin = calc_linear_z_reversed(cam_near_z, cam_far_z, center_depth);
//
//	const int32_2 center_offset = int32_2(GIBS_GI_RESOLVE_SCALE / 2, GIBS_GI_RESOLVE_SCALE / 2);
//	const float2  base			= (float2(center_px) - float2(center_offset)) / GIBS_GI_RESOLVE_SCALE;
//	const int32_2 base_i		= int32_2(floor(base));
//
//	const int32_2 src_px_00 = min(base_i + int32_2(0, 0), extent_src - 1);
//	const int32_2 src_px_01 = min(base_i + int32_2(1, 0), extent_src - 1);
//	const int32_2 src_px_10 = min(base_i + int32_2(0, 1), extent_src - 1);
//	const int32_2 src_px_11 = min(base_i + int32_2(1, 1), extent_src - 1);
//
//	const int32_2 dst_px_00 = clamp(src_px_00 * GIBS_GI_RESOLVE_SCALE + center_offset, zero<int32_2>(), extent_dst - 1);
//	const int32_2 dst_px_01 = clamp(src_px_01 * GIBS_GI_RESOLVE_SCALE + center_offset, zero<int32_2>(), extent_dst - 1);
//	const int32_2 dst_px_10 = clamp(src_px_10 * GIBS_GI_RESOLVE_SCALE + center_offset, zero<int32_2>(), extent_dst - 1);
//	const int32_2 dst_px_11 = clamp(src_px_11 * GIBS_GI_RESOLVE_SCALE + center_offset, zero<int32_2>(), extent_dst - 1);
//
//	const float2 f = frac(base);
//	// const float2 f		  = frac(float2(center_px) / GIBS_GI_RESOLVE_SCALE);
//	const float2 ratio_00 = float2(1.f - f.x, 1.f - f.y);
//	const float2 ratio_01 = float2(f.x, 1.f - f.y);
//	const float2 ratio_10 = float2(1.f - f.x, f.y);
//	const float2 ratio_11 = float2(f.x, f.y);
//
//	const float src_z_lin_00 = calc_linear_z_reversed(cam_near_z, cam_far_z, depth_buffer[dst_px_00]);
//	const float src_z_lin_01 = calc_linear_z_reversed(cam_near_z, cam_far_z, depth_buffer[dst_px_01]);
//	const float src_z_lin_10 = calc_linear_z_reversed(cam_near_z, cam_far_z, depth_buffer[dst_px_10]);
//	const float src_z_lin_11 = calc_linear_z_reversed(cam_near_z, cam_far_z, depth_buffer[dst_px_11]);
//
//	const float3 src_normal_00 = decode_oct_snorm16(gbuffer[dst_px_00].y);
//	const float3 src_normal_01 = decode_oct_snorm16(gbuffer[dst_px_01].y);
//	const float3 src_normal_10 = decode_oct_snorm16(gbuffer[dst_px_10].y);
//	const float3 src_normal_11 = decode_oct_snorm16(gbuffer[dst_px_11].y);
//
//	res += float4(src_buffer[src_px_00], 1.f) * calc_bilateral_weight(center_z_lin, center_normal, src_z_lin_00, src_normal_00, ratio_00);
//	res += float4(src_buffer[src_px_01], 1.f) * calc_bilateral_weight(center_z_lin, center_normal, src_z_lin_01, src_normal_01, ratio_01);
//	res += float4(src_buffer[src_px_10], 1.f) * calc_bilateral_weight(center_z_lin, center_normal, src_z_lin_10, src_normal_10, ratio_10);
//	res += float4(src_buffer[src_px_11], 1.f) * calc_bilateral_weight(center_z_lin, center_normal, src_z_lin_11, src_normal_11, ratio_11);
//
//	return res;
// }
//
// float4
// bilateral_upsample(int32_2 center_px, int32_2 target_px)
//{
//	const gibs_data		 data		  = gibs_load_gibs_data();
//	texture_2d<float>	 depth_buffer = global_resource_buffer[opaque_depth_buffer_srv_id];
//	texture_2d<uint32_2> gbuffer	  = global_resource_buffer[opaque_gbuffer_srv_id];
//
//	const int32_2 extent_dst = int32_2(backbuffer_size.x, backbuffer_size.y);
//	target_px				 = clamp(target_px, zero<int32_2>(), extent_dst - 1);
//
//	const float	 center_depth  = depth_buffer[center_px];
//	const float	 center_z_lin  = calc_linear_z_reversed(cam_near_z, cam_far_z, center_depth);
//	const float3 center_normal = decode_oct_snorm16(gbuffer[center_px].y);
//
//	const float	 target_depth  = depth_buffer[target_px];
//	const float	 target_z_lin  = calc_linear_z_reversed(cam_near_z, cam_far_z, target_depth);
//	const float3 target_normal = decode_oct_snorm16(gbuffer[target_px].y);
//
//	return bilateral_upsample(target_px, target_depth, target_normal) * calc_bilateral_weight(center_z_lin, center_normal, target_z_lin, target_normal);
//	// return bilateral_upsample(target_px, center_depth, center_normal);
// }
//
//// based on Jorge Jimenez SIGGRAPH 2014
//// "Next Generation Post Processing in Call of Duty: Advanced Warfare"
//// https://advances.realtimerendering.com/s2014/
//[numthreads(16, 16, 1)] void
// main_cs(uint32_3 thread_id sv_dispatch_thread_id)
//
//{
//	const gibs_data data = gibs_load_gibs_data();
//
//	uint32_2 extent_dst = uint32_2(backbuffer_size.x, backbuffer_size.y);
//
//	if (thread_id.x >= extent_dst.x or thread_id.y >= extent_dst.y) { return; }
//
//	rw_texture_2d<float3> dst_buffer = global_resource_buffer[data.h_gi_resolve_buffer_uav_id];
//
//	texture_2d<float2> motion_buffer = global_resource_buffer[motion_buffer_srv_id];
//
//	// a - b - c
//	// d - e - f
//	// g - h - i
//
//	const int32_2 px_e = thread_id.xy;
//	const int32_2 px_a = px_e + int32_2(-GIBS_GI_RESOLVE_SCALE * 1, -GIBS_GI_RESOLVE_SCALE * 1);
//	const int32_2 px_b = px_e + int32_2(-GIBS_GI_RESOLVE_SCALE * 0, -GIBS_GI_RESOLVE_SCALE * 1);
//	const int32_2 px_c = px_e + int32_2(+GIBS_GI_RESOLVE_SCALE * 1, -GIBS_GI_RESOLVE_SCALE * 1);
//	const int32_2 px_f = px_e + int32_2(+GIBS_GI_RESOLVE_SCALE * 1, -GIBS_GI_RESOLVE_SCALE * 0);
//	const int32_2 px_i = px_e + int32_2(+GIBS_GI_RESOLVE_SCALE * 1, +GIBS_GI_RESOLVE_SCALE * 1);
//	const int32_2 px_h = px_e + int32_2(+GIBS_GI_RESOLVE_SCALE * 0, +GIBS_GI_RESOLVE_SCALE * 1);
//	const int32_2 px_g = px_e + int32_2(-GIBS_GI_RESOLVE_SCALE * 1, +GIBS_GI_RESOLVE_SCALE * 1);
//	const int32_2 px_d = px_e + int32_2(-GIBS_GI_RESOLVE_SCALE * 1, +GIBS_GI_RESOLVE_SCALE * 0);
//
//	const float4 col_e = bilateral_upsample(px_e, px_e);
//	const float4 col_a = bilateral_upsample(px_e, px_a);
//	const float4 col_b = bilateral_upsample(px_e, px_b);
//	const float4 col_c = bilateral_upsample(px_e, px_c);
//	const float4 col_f = bilateral_upsample(px_e, px_f);
//	const float4 col_i = bilateral_upsample(px_e, px_i);
//	const float4 col_h = bilateral_upsample(px_e, px_h);
//	const float4 col_g = bilateral_upsample(px_e, px_g);
//	const float4 col_d = bilateral_upsample(px_e, px_d);
//
//	const float4 res = col_e * 0.25 + (col_b + col_f + col_h + col_d) * 0.125 + (col_a + col_c + col_i + col_g) * 0.0625f;
//
//	// [ 1 - 2 - 1 ]
//	// [ 2 - 4 - 2 ] / 16
//	// [ 1 - 2 - 1 ]
//
//	float ao_res = 1.f;
//	if (ao::enabled())
//	{
//		texture_2d<float4> ao_buffer = global_resource_buffer[ao::load_data().h_ao_buffer_srv_id];
//		ao_res						 = ao_buffer[thread_id.xy].x;
//	}
//
//	if (res.w > 0)
//	{
//		dst_buffer[thread_id.xy] = res.xyz / res.w * ao_res;
//	}
//	// todo, add config
//	else
//	{
//		texture_2d<float>	 depth_buffer = global_resource_buffer[opaque_depth_buffer_srv_id];
//		texture_2d<uint32_2> gbuffer	  = global_resource_buffer[opaque_gbuffer_srv_id];
//
//		const float	 px_depth  = depth_buffer[thread_id.xy];
//		const float3 px_normal = decode_oct_snorm16(gbuffer[thread_id.xy].y);
//
//		const float3 ndc	   = screen_px_to_ndc(thread_id.xy, px_depth, inv_backbuffer_size);
//		const float3 world_pos = ndc_to_world(view_proj_inv, ndc);
//
//		const uint32 vis_packed = gbuffer[thread_id.xy].x;
//		const uint32 render_id	= vis_packed & 0x01ffffff;
//		const uint32 prim_id	= (vis_packed & 0xfe000000) >> (32u - 7u);
//
//		const opaque_meshlet_render_data render_data = load_opaque_meshlet_render_data(render_id);
//
//		const float3 fallback_res = gibs_sample_screen_irradiance(thread_id.xy, render_data.object_id, world_pos, px_normal);
//		dst_buffer[thread_id.xy]  = fallback_res * ao_res;
//	}
//	// else
//	//{
//	//	texture_2d<float3> src_buffer = global_resource_buffer[data.h_gi_resolve_low_res_buffer_srv_id];
//
//	//	const int32_2 extent_src = ceil(extent_dst.x, GIBS_GI_RESOLVE_SCALE);
//	//	const int32_2 src_px	 = min(px_e / GIBS_GI_RESOLVE_SCALE, int32_2(extent_src) - 1);
//
//	//	dst_buffer[thread_id.xy] = src_buffer[src_px] * ao_res;
//	//}
//};

// wave_size(32)
//[numthreads(16, 16, 1)] void
// main_cs(uint32_3 thread_id sv_dispatch_thread_id)
//
//{
//	const gibs_data data = gibs_load_gibs_data();
//
//	const int32_2 extent = int32_2(backbuffer_size);
//
//	if (any(thread_id.xy >= extent)) { return; }
//
//	const int32_2 px = int32_2(thread_id.xy);
//
//	const uint16 step		= uint32_lower_to_uint16(rc_scratch_0);
//	const bool	 is_first	= uint32_upper_to_uint16(rc_scratch_0) == 1;
//	const bool	 is_last	= uint32_upper_to_uint16(rc_scratch_0) == 2;
//	const uint32 src_srv_id = rc_scratch_1;
//	const uint32 dst_uav_id = rc_scratch_2;
//
//	texture_2d<float4>	  src_buffer = global_resource_buffer[src_srv_id];
//	texture_2d<uint32_2>  geo_buffer = global_resource_buffer[data.h_gi_resolve_geo_curr_buffer_srv_id];
//	rw_texture_2d<float4> dst_buffer = global_resource_buffer[dst_uav_id];
//
//	const uint32_2 px_geo = geo_buffer[px];
//
//	// sky
//	if (px_geo.y == 0u)
//	{
//		dst_buffer[px] = zero<float4>();
//		return;
//	}
//
//	const float3 px_normal = decode_oct_snorm16(px_geo.x);
//	const float	 px_depth  = as_float(px_geo.y);
//	const float	 px_z_lin  = calc_linear_z_reversed(cam_near_z, cam_far_z, px_depth);
//
//	const float3 px_world_pos = ndc_to_world(view_proj_inv, screen_to_ndc(float2(px) + 0.5f, px_depth, inv_backbuffer_size));
//
//	const float3 view_dir = normalize(camera_pos - px_world_pos);
//	const float	 n_dot_v  = max(dot(px_normal, view_dir), 0.1f);
//
//	const float plane_threshold = 0.001f * px_z_lin / n_dot_v * float(step);
//
//	float4 res = zero<float4>();
//
//	float kernel_sum = 0.f;
//
//	static const float k_atrous[3] = { 0.25f, 0.5f, 0.25f };
//
//	for (int32 dy = -1; dy <= 1; ++dy)
//	{
//		for (int32 dx = -1; dx <= 1; ++dx)
//		{
//			const int32_2  px_tap		 = clamp(px + int32_2(dx, dy) * int32(step), zero<int32_2>(), extent - 1);
//			const uint32_2 px_geo_tap	 = geo_buffer[px_tap];
//			const float3   px_normal_tap = decode_oct_snorm16(px_geo_tap.x);
//			const float	   px_depth_tap	 = as_float(px_geo_tap.y);
//
//			const float4 src	 = src_buffer[px_tap];
//			const float3 irr_src = src.xyz;
//
//			const float connect_w = is_first ? 1.f : src.w;
//			const float kernel_w  = k_atrous[dx + 1] * k_atrous[dy + 1];
//
//			kernel_sum += kernel_w;
//
//			if (px_geo_tap.y == 0) { continue; }
//
//			const float3 px_world_pos_tap = ndc_to_world(view_proj_inv, screen_to_ndc(float2(px_tap) + 0.5f, px_depth_tap, inv_backbuffer_size));
//
//			const float dist_to_plane = abs(dot(px_world_pos_tap - px_world_pos, px_normal));
//			const float cos_theta	  = dot(px_normal, px_normal_tap);
//
//			const float w = kernel_w
//						  * connect_w
//						  * pow(max(cos_theta, 0.f), 32.f)
//						  * exp(-dist_to_plane / plane_threshold);
//
//			res += float4(irr_src, 1.f) * w;
//		}
//	}
//
//	res.xyz = res.w > 0.f ? res.xyz / res.w : src_buffer[px].xyz;
//
//	const float connect_res = res.w / kernel_sum;
//
//	attr_branch()
//
//	if (is_last and ao::enabled())
//	{
//		texture_2d<float3> ao_buffer  = global_resource_buffer[ao::load_data().h_ao_buffer_srv_id];
//		res.xyz						 *= ao_buffer[px].x;
//	}
//
//	dst_buffer[px] = float4(res.xyz, connect_res);
// }

wave_size(32)
[numthreads(16, 16, 1)] void
main_cs(uint32_3 thread_id sv_dispatch_thread_id)

{
	const gibs_data data = gibs_load_gibs_data();

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
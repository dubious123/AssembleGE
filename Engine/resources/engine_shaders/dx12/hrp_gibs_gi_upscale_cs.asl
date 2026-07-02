#include "hrp_common.asli"

float4
bilateral_upsample(int32_2 center_px, float center_depth, float3 center_normal)
{
	const gibs_data data = gibs_load_gibs_data();

	uint32_2 extent_dst = uint32_2(backbuffer_size.x, backbuffer_size.y);
	uint32_2 extent_src = uint32_2(ceil(extent_dst.x, GIBS_GI_RESOLVE_SCALE), ceil(extent_dst.y, GIBS_GI_RESOLVE_SCALE));

	texture_2d<float3> src_buffer = global_resource_buffer[data.h_gi_resolve_low_res_buffer_srv_id];

	texture_2d<float>	 depth_buffer = global_resource_buffer[opaque_depth_buffer_srv_id];
	texture_2d<uint32_2> gbuffer	  = global_resource_buffer[opaque_gbuffer_srv_id];

	float4 res = zero<float4>();

	const float center_z_lin = calc_linear_z_reversed(cam_near_z, cam_far_z, center_depth);

	const int32_2 center_offset = int32_2(GIBS_GI_RESOLVE_SCALE / 2, GIBS_GI_RESOLVE_SCALE / 2);
	const float2  base			= (float2(center_px) - float2(center_offset)) / GIBS_GI_RESOLVE_SCALE;
	const int32_2 base_i		= int32_2(floor(base));

	const int32_2 src_px_00 = min(base_i + int32_2(0, 0), extent_src - 1);
	const int32_2 src_px_01 = min(base_i + int32_2(1, 0), extent_src - 1);
	const int32_2 src_px_10 = min(base_i + int32_2(0, 1), extent_src - 1);
	const int32_2 src_px_11 = min(base_i + int32_2(1, 1), extent_src - 1);

	// const int32_2 src_px_00 = min(center_px / GIBS_GI_RESOLVE_SCALE, extent_src - 1);
	// const int32_2 src_px_01 = min(src_px_00 + int32_2(1, 0), extent_src - 1);
	// const int32_2 src_px_10 = min(src_px_00 + int32_2(0, 1), extent_src - 1);
	// const int32_2 src_px_11 = min(src_px_00 + int32_2(1, 1), extent_src - 1);

	const int32_2 dst_px_00 = clamp(src_px_00 * GIBS_GI_RESOLVE_SCALE + center_offset, zero<int32_2>(), extent_dst - 1);
	const int32_2 dst_px_01 = clamp(src_px_01 * GIBS_GI_RESOLVE_SCALE + center_offset, zero<int32_2>(), extent_dst - 1);
	const int32_2 dst_px_10 = clamp(src_px_10 * GIBS_GI_RESOLVE_SCALE + center_offset, zero<int32_2>(), extent_dst - 1);
	const int32_2 dst_px_11 = clamp(src_px_11 * GIBS_GI_RESOLVE_SCALE + center_offset, zero<int32_2>(), extent_dst - 1);

	const float2 f = frac(base);
	// const float2 f		  = frac(float2(center_px) / GIBS_GI_RESOLVE_SCALE);
	const float2 ratio_00 = float2(1.f - f.x, 1.f - f.y);
	const float2 ratio_01 = float2(f.x, 1.f - f.y);
	const float2 ratio_10 = float2(1.f - f.x, f.y);
	const float2 ratio_11 = float2(f.x, f.y);

	const float src_z_lin_00 = calc_linear_z_reversed(cam_near_z, cam_far_z, depth_buffer[dst_px_00]);
	const float src_z_lin_01 = calc_linear_z_reversed(cam_near_z, cam_far_z, depth_buffer[dst_px_01]);
	const float src_z_lin_10 = calc_linear_z_reversed(cam_near_z, cam_far_z, depth_buffer[dst_px_10]);
	const float src_z_lin_11 = calc_linear_z_reversed(cam_near_z, cam_far_z, depth_buffer[dst_px_11]);

	const float3 src_normal_00 = decode_oct_snorm16(gbuffer[dst_px_00].y);
	const float3 src_normal_01 = decode_oct_snorm16(gbuffer[dst_px_01].y);
	const float3 src_normal_10 = decode_oct_snorm16(gbuffer[dst_px_10].y);
	const float3 src_normal_11 = decode_oct_snorm16(gbuffer[dst_px_11].y);

	res += float4(src_buffer[src_px_00], 1.f) * calc_bilateral_weight(center_z_lin, center_normal, src_z_lin_00, src_normal_00, ratio_00);
	res += float4(src_buffer[src_px_01], 1.f) * calc_bilateral_weight(center_z_lin, center_normal, src_z_lin_01, src_normal_01, ratio_01);
	res += float4(src_buffer[src_px_10], 1.f) * calc_bilateral_weight(center_z_lin, center_normal, src_z_lin_10, src_normal_10, ratio_10);
	res += float4(src_buffer[src_px_11], 1.f) * calc_bilateral_weight(center_z_lin, center_normal, src_z_lin_11, src_normal_11, ratio_11);

	return res;
}

float4
bilateral_upsample(int32_2 center_px, int32_2 target_px)
{
	const gibs_data		 data		  = gibs_load_gibs_data();
	texture_2d<float>	 depth_buffer = global_resource_buffer[opaque_depth_buffer_srv_id];
	texture_2d<uint32_2> gbuffer	  = global_resource_buffer[opaque_gbuffer_srv_id];

	const int32_2 extent_dst = int32_2(backbuffer_size.x, backbuffer_size.y);
	target_px				 = clamp(target_px, zero<int32_2>(), extent_dst - 1);

	const float	 center_depth  = depth_buffer[center_px];
	const float	 center_z_lin  = calc_linear_z_reversed(cam_near_z, cam_far_z, center_depth);
	const float3 center_normal = decode_oct_snorm16(gbuffer[center_px].y);

	const float	 target_depth  = depth_buffer[target_px];
	const float	 target_z_lin  = calc_linear_z_reversed(cam_near_z, cam_far_z, target_depth);
	const float3 target_normal = decode_oct_snorm16(gbuffer[target_px].y);

	return bilateral_upsample(target_px, target_depth, target_normal) * calc_bilateral_weight(center_z_lin, center_normal, target_z_lin, target_normal);
	// return bilateral_upsample(target_px, center_depth, center_normal);
}


// based on Jorge Jimenez SIGGRAPH 2014
// "Next Generation Post Processing in Call of Duty: Advanced Warfare"
// https://advances.realtimerendering.com/s2014/
[numthreads(16, 16, 1)] void
main_cs(uint32_3 thread_id sv_dispatch_thread_id)

{
	const gibs_data data = gibs_load_gibs_data();

	uint32_2 extent_dst = uint32_2(backbuffer_size.x, backbuffer_size.y);

	if (thread_id.x >= extent_dst.x or thread_id.y >= extent_dst.y) { return; }

	rw_texture_2d<float3> dst_buffer = global_resource_buffer[data.h_gi_resolve_full_res_buffer_uav_id];

	// a - b - c
	// d - e - f
	// g - h - i

	const int32_2 px_e = thread_id.xy;
	const int32_2 px_a = px_e + int32_2(-GIBS_GI_RESOLVE_SCALE * 1, -GIBS_GI_RESOLVE_SCALE * 1);
	const int32_2 px_b = px_e + int32_2(-GIBS_GI_RESOLVE_SCALE * 0, -GIBS_GI_RESOLVE_SCALE * 1);
	const int32_2 px_c = px_e + int32_2(+GIBS_GI_RESOLVE_SCALE * 1, -GIBS_GI_RESOLVE_SCALE * 1);
	const int32_2 px_f = px_e + int32_2(+GIBS_GI_RESOLVE_SCALE * 1, -GIBS_GI_RESOLVE_SCALE * 0);
	const int32_2 px_i = px_e + int32_2(+GIBS_GI_RESOLVE_SCALE * 1, +GIBS_GI_RESOLVE_SCALE * 1);
	const int32_2 px_h = px_e + int32_2(+GIBS_GI_RESOLVE_SCALE * 0, +GIBS_GI_RESOLVE_SCALE * 1);
	const int32_2 px_g = px_e + int32_2(-GIBS_GI_RESOLVE_SCALE * 1, +GIBS_GI_RESOLVE_SCALE * 1);
	const int32_2 px_d = px_e + int32_2(-GIBS_GI_RESOLVE_SCALE * 1, +GIBS_GI_RESOLVE_SCALE * 0);

	const float4 col_e = bilateral_upsample(px_e, px_e);
	const float4 col_a = bilateral_upsample(px_e, px_a);
	const float4 col_b = bilateral_upsample(px_e, px_b);
	const float4 col_c = bilateral_upsample(px_e, px_c);
	const float4 col_f = bilateral_upsample(px_e, px_f);
	const float4 col_i = bilateral_upsample(px_e, px_i);
	const float4 col_h = bilateral_upsample(px_e, px_h);
	const float4 col_g = bilateral_upsample(px_e, px_g);
	const float4 col_d = bilateral_upsample(px_e, px_d);

	const float4 res = col_e * 0.25 + (col_b + col_f + col_h + col_d) * 0.125 + (col_a + col_c + col_i + col_g) * 0.0625f;

	// [ 1 - 2 - 1 ]
	// [ 2 - 4 - 2 ] / 16
	// [ 1 - 2 - 1 ]

	float ao_res = 1.f;
	if (ao::enabled())
	{
		texture_2d<float4> ao_buffer = global_resource_buffer[ao::load_data().h_ao_buffer_srv_id];
		ao_res						 = ao_buffer[thread_id.xy].x;
	}

	if (res.w > 0)
	{
		dst_buffer[thread_id.xy] = res.xyz / res.w * ao_res;
	}
	// todo, add config
	else
	{
		texture_2d<float>	 depth_buffer = global_resource_buffer[opaque_depth_buffer_srv_id];
		texture_2d<uint32_2> gbuffer	  = global_resource_buffer[opaque_gbuffer_srv_id];

		const float	 px_depth  = depth_buffer[thread_id.xy];
		const float3 px_normal = decode_oct_snorm16(gbuffer[thread_id.xy].y);

		const float3 ndc	   = screen_px_to_ndc(thread_id.xy, px_depth, inv_backbuffer_size);
		const float3 world_pos = ndc_to_world(view_proj_inv, ndc);

		const uint32 vis_packed = gbuffer[thread_id.xy].x;
		const uint32 render_id	= vis_packed & 0x01ffffff;
		const uint32 prim_id	= (vis_packed & 0xfe000000) >> (32u - 7u);

		const opaque_meshlet_render_data render_data = load_opaque_meshlet_render_data(render_id);

		// todo, fallback fail => ub (srv read from uav)
		const float3 fallback_res = gibs_sample_screen_irradiance(thread_id.xy, render_data.object_id, world_pos, px_normal);
		dst_buffer[thread_id.xy]  = fallback_res * ao_res;
	}
	// else
	//{
	//	texture_2d<float3> src_buffer = global_resource_buffer[data.h_gi_resolve_low_res_buffer_srv_id];

	//	const int32_2 extent_src = ceil(extent_dst.x, GIBS_GI_RESOLVE_SCALE);
	//	const int32_2 src_px	 = min(px_e / GIBS_GI_RESOLVE_SCALE, int32_2(extent_src) - 1);

	//	dst_buffer[thread_id.xy] = src_buffer[src_px] * ao_res;
	//}
}

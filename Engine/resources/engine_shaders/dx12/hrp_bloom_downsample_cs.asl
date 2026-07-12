#include "hrp_common.asli"
// based on Jorge Jimenez SIGGRAPH 2014
// "Next Generation Post Processing in Call of Duty: Advanced Warfare"
// https://advances.realtimerendering.com/s2014/
[numthreads(8, 8, 1)] void
main_cs(uint32_3 thread_id sv_dispatch_thread_id)

{
	const uint32 bloom_mip_level_and_extra = rc_scratch_0;

	const uint16 mip_src = uint32_lower_to_uint16(bloom_mip_level_and_extra);
	const uint16 mip_dst = mip_src + 1;

	const bloom bloom = load_bloom();

	const uint16 width_src	= bloom.width >> mip_src;
	const uint16 height_src = bloom.height >> mip_src;
	const uint16 width_dst	= bloom.width >> mip_dst;
	const uint16 height_dst = bloom.height >> mip_dst;

	if (thread_id.x >= width_dst or thread_id.y >= height_dst) { return; }

	texture_2d<float3>	  src_buffer = global_resource_buffer[bloom.srv_texture_id];
	rw_texture_2d<float3> dst_buffer = global_resource_buffer[bloom.uav_texture_id_arr[mip_dst]];

	// a - b - c
	// - j - k -
	// d - e - f
	// - l - m -
	// g - h - i

	const float2 t = 1.f / float2(width_src, height_src);

	const float2 uv_e = float2((thread_id.x + 0.5f) / float(width_dst), (thread_id.y + 0.5f) / float(height_dst));

	const float2 uv_a = uv_e + float2(-t.x * 2, -t.y * 2);
	const float2 uv_b = uv_e + float2(-t.x * 0, -t.y * 2);
	const float2 uv_c = uv_e + float2(+t.x * 2, -t.y * 2);
	const float2 uv_f = uv_e + float2(+t.x * 2, -t.y * 0);
	const float2 uv_i = uv_e + float2(+t.x * 2, +t.y * 2);
	const float2 uv_h = uv_e + float2(+t.x * 0, +t.y * 2);
	const float2 uv_g = uv_e + float2(-t.x * 2, +t.y * 2);
	const float2 uv_d = uv_e + float2(-t.x * 2, +t.y * 0);

	const float2 uv_j = uv_e + float2(-t.x * 1, -t.y * 1);
	const float2 uv_k = uv_e + float2(+t.x * 1, -t.y * 1);
	const float2 uv_m = uv_e + float2(+t.x * 1, +t.y * 1);
	const float2 uv_l = uv_e + float2(-t.x * 1, +t.y * 1);

	const float3 col_e = sample_level(src_buffer, get_linear_clamp_sampler(), uv_e, mip_src).rgb;

	const float3 col_a = sample_level(src_buffer, get_linear_clamp_sampler(), uv_a, mip_src).rgb;
	const float3 col_b = sample_level(src_buffer, get_linear_clamp_sampler(), uv_b, mip_src).rgb;
	const float3 col_c = sample_level(src_buffer, get_linear_clamp_sampler(), uv_c, mip_src).rgb;
	const float3 col_f = sample_level(src_buffer, get_linear_clamp_sampler(), uv_f, mip_src).rgb;
	const float3 col_i = sample_level(src_buffer, get_linear_clamp_sampler(), uv_i, mip_src).rgb;
	const float3 col_h = sample_level(src_buffer, get_linear_clamp_sampler(), uv_h, mip_src).rgb;
	const float3 col_g = sample_level(src_buffer, get_linear_clamp_sampler(), uv_g, mip_src).rgb;
	const float3 col_d = sample_level(src_buffer, get_linear_clamp_sampler(), uv_d, mip_src).rgb;

	const float3 col_j = sample_level(src_buffer, get_linear_clamp_sampler(), uv_j, mip_src).rgb;
	const float3 col_k = sample_level(src_buffer, get_linear_clamp_sampler(), uv_k, mip_src).rgb;
	const float3 col_m = sample_level(src_buffer, get_linear_clamp_sampler(), uv_m, mip_src).rgb;
	const float3 col_l = sample_level(src_buffer, get_linear_clamp_sampler(), uv_l, mip_src).rgb;

	const float3 b_center = (col_j + col_k + col_l + col_m) * 0.25f;
	const float3 b_tl	  = (col_a + col_b + col_d + col_e) * 0.25f;
	const float3 b_tr	  = (col_b + col_c + col_e + col_f) * 0.25f;
	const float3 b_br	  = (col_e + col_f + col_h + col_i) * 0.25f;
	const float3 b_bl	  = (col_d + col_e + col_g + col_h) * 0.25f;

	const float3 res = b_center * 0.5f
					 + b_tl * 0.125f
					 + b_tr * 0.125f
					 + b_br * 0.125f
					 + b_bl * 0.125f;

	dst_buffer[thread_id.xy] = res;
}

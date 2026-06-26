#include "hrp_common.asli"
// based on Jorge Jimenez SIGGRAPH 2014
// "Next Generation Post Processing in Call of Duty: Advanced Warfare"
// https://advances.realtimerendering.com/s2014/
[numthreads(8, 8, 1)] void
main_cs(uint32_3 thread_id sv_dispatch_thread_id)

{
	const bloom bloom = load_bloom();

	if (thread_id.x >= bloom.width or thread_id.y >= bloom.height) { return; }

	texture_2d<float4>	  main_buffer  = global_resource_buffer[main_buffer_texture_id];
	rw_texture_2d<float3> bloom_buffer = global_resource_buffer[bloom.uav_texture_id_arr[0]];

	// a - b - c
	// - j - k -
	// d - e - f
	// - l - m -
	// g - h - i

	const float2 t = inv_backbuffer_size;

	const float2 uv_e = float2((thread_id.x + 0.5f) / float(bloom.width), (thread_id.y + 0.5f) / float(bloom.height));

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

	const float3 col_e = sample_level(main_buffer, get_linear_clamp_sampler(), uv_e, 0).rgb;
	const float3 col_a = sample_level(main_buffer, get_linear_clamp_sampler(), uv_a, 0).rgb;
	const float3 col_b = sample_level(main_buffer, get_linear_clamp_sampler(), uv_b, 0).rgb;
	const float3 col_c = sample_level(main_buffer, get_linear_clamp_sampler(), uv_c, 0).rgb;
	const float3 col_f = sample_level(main_buffer, get_linear_clamp_sampler(), uv_f, 0).rgb;
	const float3 col_i = sample_level(main_buffer, get_linear_clamp_sampler(), uv_i, 0).rgb;
	const float3 col_h = sample_level(main_buffer, get_linear_clamp_sampler(), uv_h, 0).rgb;
	const float3 col_g = sample_level(main_buffer, get_linear_clamp_sampler(), uv_g, 0).rgb;
	const float3 col_d = sample_level(main_buffer, get_linear_clamp_sampler(), uv_d, 0).rgb;
	const float3 col_j = sample_level(main_buffer, get_linear_clamp_sampler(), uv_j, 0).rgb;
	const float3 col_k = sample_level(main_buffer, get_linear_clamp_sampler(), uv_k, 0).rgb;
	const float3 col_m = sample_level(main_buffer, get_linear_clamp_sampler(), uv_m, 0).rgb;
	const float3 col_l = sample_level(main_buffer, get_linear_clamp_sampler(), uv_l, 0).rgb;

	const float3 b_center = karis_average(col_j, col_k, col_l, col_m);
	const float3 b_tl	  = karis_average(col_a, col_b, col_d, col_e);
	const float3 b_tr	  = karis_average(col_b, col_c, col_e, col_f);
	const float3 b_br	  = karis_average(col_e, col_f, col_h, col_i);
	const float3 b_bl	  = karis_average(col_d, col_e, col_g, col_h);

	const float3 res = b_center * 0.5f
					 + b_tl * 0.125f
					 + b_tr * 0.125f
					 + b_br * 0.125f
					 + b_bl * 0.125f;

	bloom_buffer[thread_id.xy] = res * soft_knee_threshold(luminance_rec709(res), bloom.threshold, bloom.knee);
}

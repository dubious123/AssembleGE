#include "bake_common.asli"

[numthreads(8, 8, 1)] void
main_cs(uint32_3 thread_id sv_dispatch_thread_id)

{
	rw_texture_2d<float2> brdf_lut = global_resource_buffer[brdf_lut_uav_id];

	const float n_dot_v	  = (float(thread_id.x) + 0.5) / float(brdf_lut_width);
	const float roughness = (float(thread_id.y) + 0.5) / float(brdf_lut_height);
	const float alpha	  = roughness * roughness;
	const float alpha2	  = alpha * alpha;

	const float3 v = float3(sqrt(1.f - n_dot_v * n_dot_v), 0, n_dot_v);	   // (sin, 0, cos)

	float scale = 0.f;
	float bias	= 0.f;

	for (uint32 n = 0; n < ENV_LIGHT_BRDF_LUT_SAMPLE_COUNT; ++n)
	{
		const float2 xi		   = hammersley(n, ENV_LIGHT_BRDF_LUT_SAMPLE_COUNT);
		const float	 phi	   = 2.f * pi * xi.y;
		const float	 cos_theta = sqrt((1.f - xi.x) / (1.f + (alpha2 - 1.f) * xi.x));
		const float	 sin_theta = sqrt(1.f - cos_theta * cos_theta);
		const float3 h		   = float3(sin_theta * cos(phi), sin_theta * sin(phi), cos_theta);
		const float3 l		   = reflect(-v, h);

		const float n_dot_l = saturate(l.z);
		const float n_dot_h = saturate(h.z);
		const float v_dot_h = saturate(dot(v, h));

		if (n_dot_l <= 0.f) { continue; }

		const float ggx_v = n_dot_l * sqrt(n_dot_v * n_dot_v * (1.0 - alpha2) + alpha2);
		const float ggx_l = n_dot_v * sqrt(n_dot_l * n_dot_l * (1.0 - alpha2) + alpha2);
		const float vis	  = 0.5 / (ggx_v + ggx_l + epsilon_1e6);
		const float g_vis = vis * v_dot_h / n_dot_h * 4.0 * n_dot_l;

		const float fc = pow_5(1.0 - v_dot_h);

		scale += (1.f - fc) * g_vis;
		bias  += fc * g_vis;
	}

	brdf_lut[thread_id.xy] = float2(scale, bias) / float(ENV_LIGHT_BRDF_LUT_SAMPLE_COUNT);
}

#include "bake_common.asli"

float
ggx_d(float n_dot_h, float alpha)
{
	float d = (n_dot_h * alpha * alpha - n_dot_h) * n_dot_h + 1.0;
	return alpha * alpha / (pi * d * d);
};

float
calc_ggx_pdf(float n_dot_h, float v_dot_h, float alpha)
{
	return ggx_d(n_dot_h, alpha) * n_dot_h / (4.0 * v_dot_h + epsilon_1e6);
};

float
calc_eis_pdf(float2 eq_uv, float pdf_uv)
{
	return pdf_uv * float(env_light_input_texture_width * env_light_input_texture_height) / (2.0 * pi * pi * sin(pi * eq_uv.y) + epsilon_1e6);
};

[numthreads(8, 8, 1)] void
main_cs(uint32_3 thread_id sv_dispatch_thread_id)

{
	const uint32 prefilter_mip_size = env_light_prefilter_size >> env_light_prefilter_mip_count;

	if (thread_id.x >= prefilter_mip_size || thread_id.y >= prefilter_mip_size) { return; }

	texture_cube<float4>		radiance  = global_resource_buffer[env_light_radiance_texture_srv_id];
	rw_texture_2d_array<float4> prefilter = global_resource_buffer[env_light_prefilter_texture_uav_id];


	const float2   prefilter_face_uv = (float2(thread_id.xy) + 0.5) / float(prefilter_mip_size);
	const float3   normal			 = cube_face_uv_to_world_dir(prefilter_face_uv, thread_id.z);
	const float3x3 local_to_world	 = gen_normal_transform(normal);

	if (env_light_prefilter_mip_count == 0)
	{
		prefilter[thread_id] = sample_level(radiance, get_linear_clamp_sampler(), normal, 0);
		return;
	}

	const float	 roughness = float(env_light_prefilter_mip_count)
						   / float(env_light_prefilter_mip_max_count - 1);
	const float	 alpha	   = roughness * roughness;
	const float3 v		   = normal;

	float3 res			= float3(0, 0, 0);
	float  total_weight = 0.f;
	for (uint32 n = 0; n < ENV_LIGHT_GGX_SAMPLE_COUNT; ++n)
	{
		const float2 xi = hammersley(n, ENV_LIGHT_SAMPLE_COUNT);

		const float3 h = sample_hemisphere_cosine(xi, local_to_world);

		const float3 l = reflect(-v, h);

		const float n_dot_l = dot(normal, l);
		const float n_dot_h = max(dot(normal, h), 0);
		const float v_dot_h = max(dot(v, h), 0);

		if (n_dot_l <= 0.f) { continue; }

		const float pdf_ggx = calc_ggx_pdf(n_dot_h, v_dot_h, alpha);

		const float2 eq_uv = world_dir_to_equirect_uv(l);

		const uint32 x		= min(uint32(eq_uv.x * float(env_light_input_texture_width)), env_light_input_texture_width - 1u);
		const uint32 y		= min(uint32(eq_uv.y * float(env_light_input_texture_height)), env_light_input_texture_height - 1u);
		const float	 pdf_uv = load_conditional_pdf(y, x) * load_marginal_pdf(y);	// p(x|y) * p(y)

		const float pdf_eis = calc_eis_pdf(eq_uv, pdf_uv);

		const float w_mis = pdf_ggx / (pdf_ggx + pdf_eis + epsilon_1e6);

		const float omega_s = 1.f / (float(ENV_LIGHT_GGX_SAMPLE_COUNT) * pdf_ggx);
		const float omega_p = 4.f * pi / (6.f * env_light_radiance_size * env_light_radiance_size);
		const float mip_lod = max(0.5 * log2(omega_s / omega_p), 0);


		const float weight = w_mis * n_dot_l * 4;	 // w_mis * ggx_d * n_dot_l / (ggx_d * n_dot_h / (4 * v_dot_h)) , n_dot_h == v_dot_h,

		res			 += sample_level(radiance, get_linear_clamp_sampler(), l, mip_lod).rgb * weight;
		total_weight += weight;
	}

	for (uint32 n = 0; n < ENV_LIGHT_EIS_SAMPLE_COUNT; ++n)
	{
		const float2 xi	   = hammersley(n + ENV_LIGHT_GGX_SAMPLE_COUNT, ENV_LIGHT_SAMPLE_COUNT);
		const uint32 y	   = inverse_cdf_marginal(xi.x);
		const uint32 x	   = inverse_cdf_conditional(y, xi.y);
		const float2 eq_uv = (float2(x, y) + 0.5) / float2(env_light_input_texture_width, env_light_input_texture_height);
		const float3 l	   = equirect_uv_to_world_dir(eq_uv);

		const float n_dot_l = dot(normal, l);
		if (n_dot_l <= 0 || y >= env_light_input_texture_height) continue;

		const float pdf_uv = load_conditional_pdf(y, x) * load_marginal_pdf(y);	   // p(x|y) * p(y)

		const float pdf_eis = calc_eis_pdf(eq_uv, pdf_uv);

		const float3 h		 = normalize(v + l);
		const float	 n_dot_h = max(dot(normal, h), 0);
		const float	 v_dot_h = max(dot(v, h), 0);
		const float	 d		 = ggx_d(n_dot_h, alpha);
		const float	 pdf_ggx = d * n_dot_h / (4.0 * v_dot_h + epsilon_1e6);

		const float w_mis = pdf_eis / (pdf_ggx + pdf_eis + epsilon_1e6);

		const float weight = w_mis * d * n_dot_l / (pdf_eis + epsilon_1e6);

		res			 += sample_level(radiance, get_linear_clamp_sampler(), l, 0).rgb * weight;
		total_weight += weight;
	}

	prefilter[thread_id] = float4(res / max(total_weight, epsilon_1e6), 1.0);
}

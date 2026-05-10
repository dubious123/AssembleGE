#include "bake_common.asli"

[numthreads(8, 8, 1)] void
main_cs(uint32_3 thread_id sv_dispatch_thread_id) {
	if (thread_id.x >= env_light_irradiance_size || thread_id.y >= env_light_irradiance_size) return;

	texture_cube<float4>		radiance   = global_resource_buffer[env_light_radiance_texture_srv_id];
	rw_texture_2d_array<float4> irradiance = global_resource_buffer[env_light_irradiance_texture_uav_id];

	const float2   irradiance_face_uv = (float2(thread_id.xy) + 0.5) / float(env_light_irradiance_size);
	const float3   normal			  = cube_face_uv_to_world_dir(irradiance_face_uv, thread_id.z);
	const float3x3 local_to_world	  = gen_normal_transform(normal);

	float3 res = float3(0, 0, 0);

	for (uint32 n = 0; n < ENV_LIGHT_IRRADIANCE_SAMPLE_COUNT; ++n)
	{
		const float2 xi = hammersley(n, ENV_LIGHT_IRRADIANCE_SAMPLE_COUNT);

		const float phi		  = 2.0 * pi * xi.x;
		const float cos_theta = sqrt(xi.y);
		const float sin_theta = sqrt(1.0 - xi.y);

		const float3 l_local = float3(sin_theta * cos(phi), sin_theta * sin(phi), cos_theta);
		const float3 l		 = mul(local_to_world, l_local);

		const float pdf		= max(cos_theta / pi, epsilon_1e6);
		const float omega_s = 1.f / (float(ENV_LIGHT_IRRADIANCE_SAMPLE_COUNT) * pdf);
		const float omega_p = 4.f * pi / (6.f * env_light_radiance_size * env_light_radiance_size);
		const float mip_lod = max(0.5 * log2(omega_s / omega_p), 0.0);

		res += sample_level(radiance, get_linear_clamp_sampler(), l, mip_lod).rgb;
	}

	irradiance[thread_id] = float4(res / float(ENV_LIGHT_IRRADIANCE_SAMPLE_COUNT) * pi, 1.0);
}
#include "hrp_common.asli"

struct ddgi_probe_trace_gi_func
{
	float3
	operator()(const in pbr_surface_data surface_data, float3 world_face_normal)
	{
		// todo need fresnel?
		// from https://google.github.io/filament/Filament.md.html
		const float3 f_avg = surface_data.f0 + (float3(1.f, 1.f, 1.f) - surface_data.f0) / 21;

		float3 ambient_light = (float3(1.f, 1.f, 1.f) - f_avg) * calc_pbr_ddgi(surface_data, world_face_normal) * surface_data.occlusion;

		expand(MAX_ENV_LIGHT)
		for (uint32 i = 0; i < env_light_count; ++i)
		{
			ambient_light += calc_pbr_ibl_specular(surface_data, load_env_light(i)) * surface_data.occlusion;
		}

		return ambient_light;
	}
};

ddgi_ray_result
ddgi_trace_ray(float3 pos, float3 dir /*normalized*/)
{
	ddgi_probe_trace_gi_func func;
	float					 opaque_distance;
	const float4			 color = lighting::calc_ray_color<true, RAY_FLAG_NONE, true, RAY_FLAG_NONE, false, ddgi_probe_trace_gi_func>(func, opaque_distance, pos, dir);

	const float3 radiance = color.rgb + calc_skybox_color(dir) * (1.f - color.a);

	ddgi_ray_result res;

	res.distance				= opaque_distance;
	res.dir_oct_snorm_and_extra = encode_oct_snorm8(dir);
	res.radiance_r11g11b10		= encode_r11g11b10(radiance);

	return res;
}

[numthreads(DDGI_TRACE_THREAD_PER_GROUP, 1, 1)] void
main_cs(uint32 ray_id sv_dispatch_thread_id)

{
	const ddgi_data ddgi_data = load_ddgi_data();
	if (ray_id >= ddgi_load_ray_count_total(ddgi_data) or ray_id >= DDGI_RAY_BUDGET) { return; }

	const uint32	 probe_id	  = ddgi_binary_search_probe(ddgi_data, ray_id);
	const uint32	 level		  = probe_id >> load_ddgi_ppl_log2(ddgi_data);
	const uint32	 ray_local_id = ray_id - ddgi_load_ray_offset(ddgi_data, probe_id);
	const ddgi_probe probe		  = load_ddgi_probe_srv(probe_id);

	const float3 probe_pos	  = ddgi_calc_probe_pos(ddgi_data, probe_id, level) + probe.offset;
	const float	 probe_radius = max(ddgi_data.base_probe_spacing) * (1u << level);

	// const uint32 ray_count = min(ddgi_load_ray_count(ddgi_data, probe_id), DDGI_PROBE_RAY_COUNT_NEW_BORN);
	const uint32 ray_count = ddgi_load_ray_count(ddgi_data, probe_id);

	// const float3 probe_normal = decode_oct_snorm(probe.normal_oct_snorm8);

	const uint32 idx = ray_local_id;
	const float2 xi	 = frac(random_spherical_fibonacci(idx, ray_count) + ddgi_cranley_patterson_rotation);
	// const float3 dir = normalize(probe_normal + sample_sphere_uniform(xi));

	// const float3 dir = xi.x > 0.5f
	//					 ? sample_hemisphere_cosine(xi, probe_normal)
	//					 : -sample_hemisphere_cosine(xi, probe_normal);

	const float3 dir = sample_sphere_uniform(xi);

	const ddgi_ray_result res = ddgi_trace_ray(probe_pos, dir);

	ddgi_store_ray_result(ddgi_data, ray_id, res);
}
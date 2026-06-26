#include "hrp_common.asli"

ddgi_ray_result
ddgi_trace_ray(float3 pos, float3 dir /*normalized*/, float radius)
{
	float  distance;
	float3 color;

	ray_desc desc;
	desc.Origin	   = pos;
	desc.Direction = dir;
	desc.TMin	   = 0.f;
	desc.TMax	   = 3000.f;

	rt_acceleration_structure tlas = global_resource_buffer[rt_tlas_buffer_id];

	// check opaque
	{
		ray_query<RAY_FLAG_NONE> query;

		color = rt_calc_opaque_color(rt_arg::init_empty(), desc, query);

		const uint32 status = rt_committed_status(query);

		if (status == COMMITTED_NOTHING)
		{
			distance = 3000.f;
		}
		else if (rt_committed_triangle_front_face(query) is_false)
		{
			distance = -rt_committed_ray_t(query);
		}
		else
		{
			distance = rt_committed_ray_t(query);
		}
	}

	if (distance > 0.f)	   // not inside wall
	{
		desc.TMax = distance;

		ray_query<RAY_FLAG_CULL_BACK_FACING_TRIANGLES> query;

		const float4 transparent_color = rt_calc_transparent_color(rt_arg::init_empty(), desc, query);

		color = color * (1.f - transparent_color.a) + transparent_color.rgb;
	}

	ddgi_ray_result res;

	res.distance				= distance;
	res.dir_oct_snorm_and_extra = encode_oct_snorm8(dir);
	res.radiance_r11g11b10		= encode_r11g11b10(color);


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


	const ddgi_ray_result res = ddgi_trace_ray(probe_pos, dir, probe_radius);

	ddgi_store_ray_result(ddgi_data, ray_id, res);
}
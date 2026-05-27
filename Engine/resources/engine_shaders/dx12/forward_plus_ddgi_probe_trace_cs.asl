#include "forward_plus_common.asli"

struct ddgi_trace_info
{
	uint32 radiance;
	float  depth;
	uint32 dir_packed;
};

struct ddgi_trace_res
{
	float3 color;
	float  distance;
	bool   inside_wall;
};

ddgi_trace_res
ddgi_trace_ray(float3 pos, float3 dir)
{
	ddgi_trace_res res;

	ray_desc desc;
	desc.Origin	   = pos;
	desc.Direction = dir;
	desc.TMin	   = 0.001;
	desc.TMax	   = float_max;

	rt_acceleration_structure tlas = global_resource_buffer[rt_tlas_buffer_id];

	// check opaque
	{
		ray_query<RAY_FLAG_NONE> query;

		rt_trace_ray_inline(query, tlas, RAY_FLAG_NONE, RT_MASK_OPAQUE, desc);
		rt_proceed(query);

		res.distance = rt_committed_ray_t(query);

		if (rt_committed_triangle_front_face(query) is_false)
		{
			res.inside_wall = true;
			return res;
		}
	}

	// hit_data hit_data_arr[MAX_RAY_HIT];
	uint32 hit_count = 0;

	return res;
}

groupshared ddgi_trace_info trace_info_arr[DDGI_PROBE_RAY_COUNT_NEW_BORN];

[numthreads(DDGI_VISIBILITY_RESOLUTION, DDGI_VISIBILITY_RESOLUTION, 1)] void
main_cs(uint32_3 group_id  sv_group_id,
		uint32_3 thread_id sv_group_thread_id)

{
	const ddgi_data	 ddgi_data	   = load_ddgi_data();
	const uint16_3	 ppl_log2_axis = load_ddgi_ppl_log2_axis(ddgi_data);
	const uint32	 probe_id	   = group_id.x
								   | (group_id.y << ppl_log2_axis.x)
								   | (group_id.z << (ppl_log2_axis.x + ppl_log2_axis.y));
	const ddgi_probe probe		   = load_ddgi_probe_uav(probe_id);

	const uint32 level = probe_id >> uint32_w_to_uint8(ddgi_data.ppl_log_2_and_ppl_bitwidth);

	const float3 probe_pos = ddgi_calc_probe_pos(ddgi_data, probe_id, level);

	const uint16 probe_state		   = probe.state_and_ray_count_ideal & 0xff;
	const uint16 probe_ideal_ray_count = (probe.state_and_ray_count_ideal >> 8) & 0xff;

	attr_branch()

	if (probe.state_and_ray_count_ideal == DDGI_PROBE_STATE_OFF
		or probe_ideal_ray_count == 0
		or ddgi_is_probe_in_hole(ddgi_data, probe_pos, level))
	{
		return;
	}

	const uint32 ray_sum_total = load_ddgi_ray_sum_total();
	const float	 ray_ratio	   = probe_ideal_ray_count / float(ray_sum_total);

	const uint32 ray_count = min(uint32(DDGI_RAY_BUDGET * ray_ratio), DDGI_PROBE_RAY_COUNT_NEW_BORN);

	const float front_weight	= probe.msme_front.inconsistency + probe.msme_front.relative_variance;
	const float back_weight		= probe.msme_back.inconsistency + probe.msme_back.relative_variance;
	const float total_weight	= front_weight + back_weight;
	const float ray_ratio_front = total_weight > 1e-6 ? front_weight / total_weight : 0.5f;

	const uint32 ray_count_front = cast<uint32>(ray_count * ray_ratio_front);
	const uint32 ray_count_back	 = ray_count - ray_count_front;

	const uint32 thread_id_flat = thread_id.x + thread_id.y * DDGI_VISIBILITY_RESOLUTION;

	const float3 normal = decode_oct_snorm(probe.normal_oct_snorm8);

	if (thread_id_flat < ray_count_front)
	{
		const float2 xi	 = hammersley(thread_id_flat, ray_count_front);
		const float3 dir = sample_hemisphere_cosine(xi, normal);

		// const ddgi_trace_res res = ddgi_trace_ray(probe_pos, dir);

		// distance, color
	}
	else if (thread_id_flat < ray_count_front + ray_count_back)
	{
		const float2 xi	 = hammersley(thread_id_flat - ray_count_front, ray_count_back);
		const float3 dir = sample_hemisphere_cosine(xi, -normal);

		// const ddgi_trace_info res = ddgi_trace_ray(probe_pos, dir);
	}
}
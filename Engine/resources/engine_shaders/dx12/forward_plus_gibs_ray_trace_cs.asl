#include "forward_plus_common.asli"

gibs_ray_result
gibs_trace_ray(float3 pos, float3 dir /*normalized*/)
{
	float  distance;
	float3 color;

	ray_desc desc;
	desc.Origin	   = pos;
	desc.Direction = dir;
	desc.TMin	   = 0.f;
	desc.TMax	   = float_max;

	rt_acceleration_structure tlas = global_resource_buffer[rt_tlas_buffer_id];

	// check opaque
	{
		ray_query<RAY_FLAG_NONE> query;

		color = rt_calc_opaque_color(desc, query);

		const uint32 status = rt_committed_status(query);

		if (status == COMMITTED_NOTHING)
		{
			distance = float_max;
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

		const float4 transparent_color = rt_calc_transparent_color(desc, query);

		color = color * (1.f - transparent_color.a) + transparent_color.rgb;
	}

	gibs_ray_result res;

	res.dir_oct_snorm_and_extra = encode_oct_snorm8(dir);
	res.distance				= distance;
	res.radiance_r11g11b10		= encode_r11g11b10(color);

	return res;
}

wave_size(32)
[numthreads(32, 1, 1)] void
main_cs(uint32 ray_id sv_dispatch_thread_id)

{
	const gibs_data data = gibs_load_gibs_data();

	if (ray_id >= gibs_get_ray_count_total(data)) { return; }

	const gibs_ray_entry ray_entry = gibs_load_ray_entry_rw_arr(data)[ray_id];

	const surfel surfel = gibs_load_surfel_rw_arr(data)[ray_entry.surfel_id];

	const float2 xi = random_pcg2d(uint32_2(ray_id, frame_index));

	// todo important sample
	const float3 dir = sample_hemisphere_cosine(xi, decode_oct_snorm16(surfel.normal_oct_snorm16));

	const gibs_ray_result res = gibs_trace_ray(surfel.position, dir);

	gibs_load_ray_result_rw_arr(data).store(ray_id, res);
}
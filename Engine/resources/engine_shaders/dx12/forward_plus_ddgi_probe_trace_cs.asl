#include "forward_plus_common.asli"

struct ddgi_trace_res_packed
{
	uint32 radiance;
	float  distance;	// f_max => no hit, <0 => inside_wall
	uint16 dir;			// oct_snorm
	uint16 _;
};

struct ddgi_trace_res
{
	float3 color;
	float  distance;
	float3 dir;
};

ddgi_trace_res_packed
pack_trace_res(ddgi_trace_res res)
{
	ddgi_trace_res_packed packed;

	packed.distance = res.distance;
	packed.dir		= encode_oct_snorm8(res.dir);
	packed.radiance = encode_r11g11b10(res.color);

	return packed;
}

// ddgi_trace_res
// pack_trace_res(ddgi_trace_res_packed packed)
//{
//	ddgi_trace_res res;
//
//	res.distance = packed.distance;
//	res.color	 = decode_r11g11b10(packed.radiance);
//	res.dir		 = decode_oct_snorm(packed.dir);
//
//	return res;
// }

ddgi_trace_res
ddgi_trace_ray(float3 pos, float3 dir /*normalized*/, float radius)
{
	ddgi_trace_res res = (ddgi_trace_res)0;
	res.dir			   = dir;

	ray_desc desc;
	desc.Origin	   = pos;
	desc.Direction = dir;
	desc.TMin	   = 0.001;
	desc.TMax	   = radius;

	rt_acceleration_structure tlas = global_resource_buffer[rt_tlas_buffer_id];

	// check opaque
	{
		ray_query<RAY_FLAG_NONE> query;

		res.color = rt_calc_opaque_color(desc, query);

		const uint32 status = rt_committed_status(query);

		if (status == COMMITTED_NOTHING)
		{
			res.distance = radius;
		}
		else if (rt_committed_triangle_front_face(query) is_false)
		{
			res.distance = -rt_committed_ray_t(query);
		}
		else
		{
			res.distance = rt_committed_ray_t(query);
		}
	}

	if (res.distance > 0.f)	   // not inside wall
	{
		desc.TMax = res.distance;

		ray_query<RAY_FLAG_CULL_BACK_FACING_TRIANGLES> query;

		const float4 transparent_color = rt_calc_transparent_color(desc, query);

		res.color = res.color * (1.f - transparent_color.a) + transparent_color.rgb;
	}

	return res;
}

// based on ray-tracing-gems ch25
// https://github.com/Apress/ray-tracing-gems/blob/master/Ch_25_Hybrid_Rendering_for_Real-Time_Ray_Tracing/MultiscaleMeanEstimator.hlsl
// returns blend factor (catch_up_blend)
float
ddgi_update_msme(inout ddgi_msme data, float y)
{
	float mean_long			= data.mean_long;
	float mean_short		= data.mean_short;
	float relative_variance = data.relative_variance;
	float inconsistency		= data.inconsistency;
	float vbbr				= data.vbbr;

	float variance = relative_variance * mean_long * mean_long;

	// Suppress fireflies.
	{
		const float dev				= sqrt(max(1e-5f, variance));
		const float high_threshold	= 0.1f + mean_short + dev * 8.f;
		const float overflow		= max(0.f, y - high_threshold);
		y						   -= overflow;
	}

	const float delta  = y - mean_short;
	mean_short		   = lerp(mean_short, y, DDGI_MSME_SHORT_WINDOW_BLEND);
	const float delta2 = y - mean_short;

	// This should be a longer window than shortWindowBlend to avoid bias
	// from the variance getting smaller when the short-term mean does.
	const float variance_blend = DDGI_MSME_SHORT_WINDOW_BLEND * 0.5f;
	variance				   = lerp(variance, delta * delta2, variance_blend);
	const float dev			   = sqrt(max(1e-5f, variance));

	const float short_diff	  = mean_long - mean_short;
	const float relative_diff = abs(short_diff) / max(1e-5f, dev);
	inconsistency			  = lerp(inconsistency, relative_diff, 0.08f);

	const float variance_based_blend_reduction = clamp(0.5f * mean_short / max(1e-5f, dev), 1.f / 32.f, 1.f);

	float catch_up_blend  = clamp(smoothstep(0.f, 1.f, relative_diff * max(0.02f, inconsistency - 0.2f)), 1.f / 256.f, 1.f);
	catch_up_blend		 *= vbbr;

	vbbr	  = lerp(vbbr, variance_based_blend_reduction, 0.1f);
	mean_long = lerp(mean_long, y, saturate(catch_up_blend));

	// Output
	data.mean_long		   = mean_long;
	data.mean_short		   = mean_short;
	data.relative_variance = variance / max(1e-5f, mean_long * mean_long);
	data.inconsistency	   = inconsistency;
	data.vbbr			   = vbbr;

	return catch_up_blend;
}

DECLARE_CALC_THREAD_GROUP_SUM_FLOAT(DDGI_PROBE_RAY_COUNT_NEW_BORN)

groupshared uint32 gs_radiance_packed[DDGI_PROBE_RAY_COUNT_NEW_BORN];
groupshared float  gs_distance[DDGI_PROBE_RAY_COUNT_NEW_BORN];
groupshared uint16 gs_dir_packed[DDGI_PROBE_RAY_COUNT_NEW_BORN];

groupshared float	  gs_blend_factor_front;
groupshared float	  gs_blend_factor_back;
groupshared ddgi_msme gs_probe_msme_back;

[numthreads(DDGI_VISIBILITY_RESOLUTION, DDGI_VISIBILITY_RESOLUTION, 1)] void
main_cs(uint32_3 group_id  sv_group_id,
		uint32_3 thread_id sv_group_thread_id)

{
	const ddgi_data ddgi_data	  = load_ddgi_data();
	const uint16_3	ppl_log2_axis = load_ddgi_ppl_log2_axis(ddgi_data);
	const uint32	probe_id	  = group_id.x
								  | (group_id.y << ppl_log2_axis.x)
								  | (group_id.z << (ppl_log2_axis.x + ppl_log2_axis.y));
	ddgi_probe		probe		  = load_ddgi_probe_uav(probe_id);

	const uint32 level		  = probe_id >> uint32_w_to_uint8(ddgi_data.ppl_log_2_and_ppl_bitwidth);
	const float	 probe_radius = max(ddgi_data.base_probe_spacing) * (1u << level);

	const float3 probe_pos = ddgi_calc_probe_pos(ddgi_data, probe_id, level) + probe.offset;

	const uint16 probe_state		   = probe.state_and_ray_count_ideal & 0xff;
	const uint32 probe_ideal_ray_count = (probe.state_and_ray_count_ideal >> 8) & 0xff;

	attr_branch()

	if (probe_state == DDGI_PROBE_STATE_OFF
		// or probe_ideal_ray_count == 0
		or ddgi_is_probe_in_hole(ddgi_data, probe_pos, level))
	{
		// return;
	}

	const uint32 ray_sum_total = max(load_ddgi_ray_sum_total(), DDGI_RAY_BUDGET);
	const float	 ray_ratio	   = float(probe_ideal_ray_count) / float(ray_sum_total);
	const uint32 ray_count	   = min(DDGI_RAY_BUDGET * ray_ratio, DDGI_PROBE_RAY_COUNT_NEW_BORN);

	// if (ray_count == 0) { return; }

	const float front_weight	= probe.msme_front.inconsistency + probe.msme_front.relative_variance;
	const float back_weight		= probe.msme_back.inconsistency + probe.msme_back.relative_variance;
	const float total_weight	= front_weight + back_weight;
	const float ray_ratio_front = total_weight > 1e-6 ? front_weight / total_weight : 0.5f;

	const uint32 ray_count_front = cast<uint32>(ray_count * ray_ratio_front);
	const uint32 ray_count_back	 = ray_count - ray_count_front;

	const uint32 thread_id_flat = thread_id.x + thread_id.y * DDGI_VISIBILITY_RESOLUTION;

	const float3 probe_normal = decode_oct_snorm(probe.normal_oct_snorm8);

	ddgi_trace_res res = (ddgi_trace_res)0;
	float3		   dir = float3(0, 0, 0);

	float luminance_front = 0.f;
	float luminance_back  = 0.f;

	if (thread_id_flat < ray_count_front)
	{
		float2 xi = random_spherical_fibonacci(thread_id_flat, ray_count_front);
		xi		  = frac(xi + ddgi_cranley_patterson_rotation);
		dir		  = sample_hemisphere_cosine(xi, probe_normal);

		res = ddgi_trace_ray(probe_pos, dir, probe_radius);

		ddgi_trace_res_packed packed = pack_trace_res(res);

		gs_radiance_packed[thread_id_flat] = packed.radiance;
		gs_distance[thread_id_flat]		   = packed.distance;
		gs_dir_packed[thread_id_flat]	   = packed.dir;

		luminance_front = luminance_rec709(res.color);
	}
	else if (thread_id_flat < ray_count_front + ray_count_back)
	{
		float2 xi = random_spherical_fibonacci(thread_id_flat - ray_count_front, ray_count_back);
		xi		  = frac(xi + ddgi_cranley_patterson_rotation);
		dir		  = sample_hemisphere_cosine(xi, -probe_normal);

		res = ddgi_trace_ray(probe_pos, dir, probe_radius);

		ddgi_trace_res_packed packed = pack_trace_res(res);

		gs_radiance_packed[thread_id_flat] = packed.radiance;
		gs_distance[thread_id_flat]		   = packed.distance;
		gs_dir_packed[thread_id_flat]	   = packed.dir;

		luminance_back = luminance_rec709(res.color);
	}

	luminance_front = calc_thread_group_sum_float(luminance_front, thread_id_flat) / max(ray_count_front, 1);
	luminance_back	= calc_thread_group_sum_float(luminance_back, thread_id_flat) / max(ray_count_back, 1);

	if (thread_id_flat == 0)
	{
		gs_blend_factor_front = ddgi_update_msme(probe.msme_front, luminance_front);
	}
	else if (thread_id_flat == 1)
	{
		gs_blend_factor_back = ddgi_update_msme(probe.msme_back, luminance_back);
		gs_probe_msme_back	 = probe.msme_back;
	}

	group_memory_barrier_with_sync();

	if (thread_id_flat < DDGI_IRRADIANCE_RESOLUTION * DDGI_IRRADIANCE_RESOLUTION)
	{
		const uint32 tx		   = thread_id_flat % DDGI_IRRADIANCE_RESOLUTION;
		const uint32 ty		   = thread_id_flat / DDGI_IRRADIANCE_RESOLUTION;
		const float2 uv		   = (float2(tx, ty) + 0.5f) / float(DDGI_IRRADIANCE_RESOLUTION) * 2.0f - 1.0f;
		const float3 texel_dir = decode_octahedral(uv);


		float3 irradiance_sum = float3(0, 0, 0);
		float  weight_sum	  = 0.f;

		for (uint32 i = 0; i < ray_count; ++i)
		{
			const float3 ray_dir  = decode_oct_snorm(gs_dir_packed[i]);
			const float	 distance = gs_distance[i];

			// distance < 0.f == inside wall
			const float density_ratio = (i < ray_count_front)
										  ? float(ray_count) / float(max(ray_count_front, 1u))
										  : float(ray_count) / float(max(ray_count_back, 1u));
			const float weight		  = (distance > 0.f)
										  ? max(0.f, dot(texel_dir, ray_dir)) * density_ratio
										  : 0.f;

			const float3 radiance = decode_r11g11b10(gs_radiance_packed[i]) * DDGI_IRRADIANCE_ENERGY_CONSERVATION;

			irradiance_sum += radiance * weight;
			weight_sum	   += weight;
		}

		// irradiance_sum / weight_sum : estimation of E[irradiance] / pi
		irradiance_sum = irradiance_sum / weight_sum;

		const bool	is_front	 = dot(texel_dir, probe_normal) > 0.f;
		const float blend_factor = is_front ? gs_blend_factor_front : gs_blend_factor_back;

		texture_2d<float3>	  irradiance_src = global_resource_buffer[ddgi_data.irradiance_atlas_prev_srv_id];
		rw_texture_2d<float3> irradiance_dst = global_resource_buffer[ddgi_data.irradiance_atlas_uav_id];

		const uint32_2 atlas_coord = calc_probe_atlas_offset(probe_id, load_ddgi_tile_count_w_log2(ddgi_data), DDGI_IRRADIANCE_TILE_SIZE)
								   + uint32_2(tx + DDGI_BORDER, ty + DDGI_BORDER);

		const float3 src_irradiance = load(irradiance_src, atlas_coord, 0);

		if (weight_sum == 0.f or probe_state == DDGI_PROBE_STATE_NEW_BORN)
		{
			irradiance_dst[atlas_coord] = src_irradiance;
			// irradiance_dst[atlas_coord] = float3(0, 0, 1);
		}
		else
		{
			irradiance_dst[atlas_coord] = lerp(src_irradiance, irradiance_sum, blend_factor);
			// irradiance_dst[atlas_coord] = float3(1, 0, 0);
		}
	}

	group_memory_barrier_with_sync();

	if (thread_id_flat < DDGI_VISIBILITY_RESOLUTION * DDGI_VISIBILITY_RESOLUTION)
	{
		const uint32 tx		   = thread_id_flat % DDGI_VISIBILITY_RESOLUTION;
		const uint32 ty		   = thread_id_flat / DDGI_VISIBILITY_RESOLUTION;
		const float2 uv		   = (float2(tx, ty) + 0.5f) / float(DDGI_VISIBILITY_RESOLUTION) * 2.0f - 1.0f;
		const float3 texel_dir = decode_octahedral(uv);

		float2 visibility_sum = float2(0, 0);	 // (dist, dist_sq)
		float  weight_sum	  = 0.f;

		for (uint32 i = 0; i < ray_count; ++i)
		{
			const float3 ray_dir  = decode_oct_snorm(gs_dir_packed[i]);
			const float	 distance = gs_distance[i];

			const float density_ratio = (i < ray_count_front)
										  ? float(ray_count) / float(max(ray_count_front, 1u))
										  : float(ray_count) / float(max(ray_count_back, 1u));

			const float weight = distance > 0.f
								   ? pow(max(0.f, dot(texel_dir, ray_dir)), DDGI_VISIBILITY_SHARPNESS) * density_ratio
								   : 0.f;

			const float dist = clamp(distance, 0.f, max(max(ddgi_data.base_probe_spacing.x, ddgi_data.base_probe_spacing.y), ddgi_data.base_probe_spacing.z) * (1u << level));

			visibility_sum += float2(dist, dist * dist) * weight;
			weight_sum	   += weight;
		}

		visibility_sum = visibility_sum / weight_sum;

		texture_2d<float2>	  visibility_src = global_resource_buffer[ddgi_data.visibility_atlas_prev_srv_id];
		rw_texture_2d<float2> visibility_dst = global_resource_buffer[ddgi_data.visibility_atlas_uav_id];

		const uint32_2 atlas_coord = calc_probe_atlas_offset(probe_id, load_ddgi_tile_count_w_log2(ddgi_data), DDGI_VISIBILITY_TILE_SIZE)
								   + uint32_2(tx + DDGI_BORDER, ty + DDGI_BORDER);

		const float2 src_visibility = load(visibility_src, atlas_coord, 0);


		if (weight_sum == 0.f or probe_state == DDGI_PROBE_STATE_NEW_BORN)
		{
			visibility_dst[atlas_coord] = src_visibility;
		}
		else
		{
			visibility_dst[atlas_coord] = lerp(src_visibility, visibility_sum, DDGI_VISIBILITY_BLEND_FACTOR);
		}
	}


	// if (res.distance < 0)
	//{
	//	// inside wall
	//	probe.offset -= cast<half3>(dir * 0.1f);
	// }

	if (thread_id_flat == 0)
	{
		probe.msme_back = gs_probe_msme_back;
		store_ddgi_probe_uav(probe_id, probe);
	}
}
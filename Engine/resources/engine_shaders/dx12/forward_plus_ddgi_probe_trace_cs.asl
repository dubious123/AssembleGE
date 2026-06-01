#include "forward_plus_common.asli"

ddgi_ray_result
ddgi_trace_ray(float3 pos, float3 dir /*normalized*/, float radius)
{
	float  distance;
	float3 color;

	ray_desc desc;
	desc.Origin	   = pos;
	desc.Direction = dir;
	desc.TMin	   = 0.001;
	desc.TMax	   = radius;

	rt_acceleration_structure tlas = global_resource_buffer[rt_tlas_buffer_id];

	// check opaque
	{
		ray_query<RAY_FLAG_NONE> query;

		color = rt_calc_opaque_color(desc, query);

		const uint32 status = rt_committed_status(query);

		if (status == COMMITTED_NOTHING)
		{
			distance = radius;
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
	if (ray_id >= ddgi_load_ray_count_total(ddgi_data)) { return; }

	const uint32	 probe_id	  = ddgi_binary_search_probe(ddgi_data, ray_id);
	const uint32	 level		  = probe_id >> load_ddgi_ppl_log2(ddgi_data);
	const uint32	 ray_local_id = ray_id - ddgi_load_ray_offset(ddgi_data, probe_id);
	const ddgi_probe probe		  = load_ddgi_probe_srv(probe_id);

	const float3 probe_pos	  = ddgi_calc_probe_pos(ddgi_data, probe_id, level) + probe.offset;
	const float	 probe_radius = max(ddgi_data.base_probe_spacing) * (1u << level);

	// const uint32 ray_count		 = min(ddgi_load_ray_count(ddgi_data, probe_id), DDGI_PROBE_RAY_COUNT_NEW_BORN);
	const uint32 ray_count		 = ddgi_load_ray_count(ddgi_data, probe_id);
	const float	 front_weight	 = probe.msme_front.inconsistency + probe.msme_front.relative_variance;
	const float	 back_weight	 = probe.msme_back.inconsistency + probe.msme_back.relative_variance;
	const float	 total_weight	 = front_weight + back_weight;
	const float	 ray_ratio_front = total_weight > epsilon_1e6 ? clamp(front_weight / total_weight, 0.2f, 0.8f) : 0.5f;
	const uint32 ray_count_front = cast<uint32>(ray_count * ray_ratio_front);

	const float3 probe_normal = decode_oct_snorm(probe.normal_oct_snorm8);

	const bool is_front = ray_local_id < ray_count_front;

	const uint32 idx = is_front ? ray_local_id : ray_local_id - ray_count_front;
	const uint32 n	 = max(is_front ? ray_count_front : ray_count - ray_count_front, 1u);
	const float2 xi	 = frac(random_spherical_fibonacci(idx, n) + ddgi_cranley_patterson_rotation);
	const float3 dir = sample_hemisphere_uniform(xi, is_front ? probe_normal : -probe_normal);

	const ddgi_ray_result res = ddgi_trace_ray(probe_pos, dir, probe_radius * 2.f);

	ddgi_store_ray_result(ddgi_data, ray_id, res);


	// ddgi_trace_res res = (ddgi_trace_res)0;
	// float3		   dir = float3(0, 0, 0);

	// float luminance_front = 0.f;
	// float luminance_back  = 0.f;

	// if (thread_id_flat < ray_count_front)
	//{
	//	float2 xi = random_spherical_fibonacci(thread_id_flat, ray_count_front);
	//	xi		  = frac(xi + ddgi_cranley_patterson_rotation);
	//	dir		  = sample_hemisphere_cosine(xi, probe_normal);

	//	res = ddgi_trace_ray(probe_pos, dir, probe_radius);

	//	ddgi_trace_res_packed packed = pack_trace_res(res);

	//	gs_radiance_packed[thread_id_flat] = packed.radiance;
	//	gs_distance[thread_id_flat]		   = packed.distance;
	//	gs_dir_packed[thread_id_flat]	   = packed.dir;

	//	luminance_front = luminance_rec709(res.color);
	//}
	// else if (thread_id_flat < ray_count_front + ray_count_back)
	//{
	//	float2 xi = random_spherical_fibonacci(thread_id_flat - ray_count_front, ray_count_back);
	//	xi		  = frac(xi + ddgi_cranley_patterson_rotation);
	//	dir		  = sample_hemisphere_cosine(xi, -probe_normal);

	//	res = ddgi_trace_ray(probe_pos, dir, probe_radius);

	//	ddgi_trace_res_packed packed = pack_trace_res(res);

	//	gs_radiance_packed[thread_id_flat] = packed.radiance;
	//	gs_distance[thread_id_flat]		   = packed.distance;
	//	gs_dir_packed[thread_id_flat]	   = packed.dir;

	//	luminance_back = luminance_rec709(res.color);
	//}

	// luminance_front = calc_thread_group_sum_float(luminance_front, thread_id_flat) / max(ray_count_front, 1);
	// luminance_back	= calc_thread_group_sum_float(luminance_back, thread_id_flat) / max(ray_count_back, 1);

	// if (thread_id_flat == 0)
	//{
	//	gs_blend_factor_front = ddgi_update_msme(probe.msme_front, luminance_front);
	// }
	// else if (thread_id_flat == 1)
	//{
	//	gs_blend_factor_back = ddgi_update_msme(probe.msme_back, luminance_back);
	//	gs_probe_msme_back	 = probe.msme_back;
	// }

	// group_memory_barrier_with_sync();

	// if (thread_id_flat < DDGI_IRRADIANCE_RESOLUTION * DDGI_IRRADIANCE_RESOLUTION)
	//{
	//	const uint32 tx		   = thread_id_flat % DDGI_IRRADIANCE_RESOLUTION;
	//	const uint32 ty		   = thread_id_flat / DDGI_IRRADIANCE_RESOLUTION;
	//	const float2 uv		   = (float2(tx, ty) + 0.5f) / float(DDGI_IRRADIANCE_RESOLUTION) * 2.0f - 1.0f;
	//	const float3 texel_dir = decode_octahedral(uv);


	//	float3 irradiance_sum = float3(0, 0, 0);
	//	float  weight_sum	  = 0.f;

	//	for (uint32 i = 0; i < ray_count; ++i)
	//	{
	//		const float3 ray_dir  = decode_oct_snorm(gs_dir_packed[i]);
	//		const float	 distance = gs_distance[i];

	//		// distance < 0.f == inside wall
	//		const float density_ratio = (i < ray_count_front)
	//									  ? float(ray_count) / float(max(ray_count_front, 1u))
	//									  : float(ray_count) / float(max(ray_count_back, 1u));
	//		const float weight		  = (distance > 0.f)
	//									  ? max(0.f, dot(texel_dir, ray_dir)) * density_ratio
	//									  : 0.f;

	//		const float3 radiance = decode_r11g11b10(gs_radiance_packed[i]) * DDGI_IRRADIANCE_ENERGY_CONSERVATION;

	//		irradiance_sum += radiance * weight;
	//		weight_sum	   += weight;
	//	}

	//	// irradiance_sum / weight_sum : estimation of E[irradiance] / pi
	//	irradiance_sum = irradiance_sum / weight_sum;

	//	const bool	is_front	 = dot(texel_dir, probe_normal) > 0.f;
	//	const float blend_factor = is_front ? gs_blend_factor_front : gs_blend_factor_back;

	//	texture_2d<float3>	  irradiance_src = global_resource_buffer[ddgi_data.irradiance_atlas_prev_srv_id];
	//	rw_texture_2d<float3> irradiance_dst = global_resource_buffer[ddgi_data.irradiance_atlas_uav_id];

	//	const uint32_2 atlas_coord = calc_probe_atlas_offset(probe_id, load_ddgi_tile_count_w_log2(ddgi_data), DDGI_IRRADIANCE_TILE_SIZE)
	//							   + uint32_2(tx + DDGI_BORDER, ty + DDGI_BORDER);

	//	const float3 src_irradiance = load(irradiance_src, atlas_coord, 0);

	//	if (weight_sum == 0.f or probe_state == DDGI_PROBE_STATE_NEW_BORN)
	//	{
	//		irradiance_dst[atlas_coord] = src_irradiance;
	//		// irradiance_dst[atlas_coord] = float3(0, 0, 1);
	//	}
	//	else
	//	{
	//		irradiance_dst[atlas_coord] = lerp(src_irradiance, irradiance_sum, blend_factor);
	//		// irradiance_dst[atlas_coord] = float3(1, 0, 0);
	//	}
	//}

	// group_memory_barrier_with_sync();

	// if (thread_id_flat < DDGI_VISIBILITY_RESOLUTION * DDGI_VISIBILITY_RESOLUTION)
	//{
	//	const uint32 tx		   = thread_id_flat % DDGI_VISIBILITY_RESOLUTION;
	//	const uint32 ty		   = thread_id_flat / DDGI_VISIBILITY_RESOLUTION;
	//	const float2 uv		   = (float2(tx, ty) + 0.5f) / float(DDGI_VISIBILITY_RESOLUTION) * 2.0f - 1.0f;
	//	const float3 texel_dir = decode_octahedral(uv);

	//	float2 visibility_sum = float2(0, 0);	 // (dist, dist_sq)
	//	float  weight_sum	  = 0.f;

	//	for (uint32 i = 0; i < ray_count; ++i)
	//	{
	//		const float3 ray_dir  = decode_oct_snorm(gs_dir_packed[i]);
	//		const float	 distance = gs_distance[i];

	//		const float density_ratio = (i < ray_count_front)
	//									  ? float(ray_count) / float(max(ray_count_front, 1u))
	//									  : float(ray_count) / float(max(ray_count_back, 1u));

	//		const float weight = distance > 0.f
	//							   ? pow(max(0.f, dot(texel_dir, ray_dir)), DDGI_VISIBILITY_SHARPNESS) * density_ratio
	//							   : 0.f;

	//		const float dist = clamp(distance, 0.f, max(max(ddgi_data.base_probe_spacing.x, ddgi_data.base_probe_spacing.y), ddgi_data.base_probe_spacing.z) * (1u << level));

	//		visibility_sum += float2(dist, dist * dist) * weight;
	//		weight_sum	   += weight;
	//	}

	//	visibility_sum = visibility_sum / weight_sum;

	//	texture_2d<float2>	  visibility_src = global_resource_buffer[ddgi_data.visibility_atlas_prev_srv_id];
	//	rw_texture_2d<float2> visibility_dst = global_resource_buffer[ddgi_data.visibility_atlas_uav_id];

	//	const uint32_2 atlas_coord = calc_probe_atlas_offset(probe_id, load_ddgi_tile_count_w_log2(ddgi_data), DDGI_VISIBILITY_TILE_SIZE)
	//							   + uint32_2(tx + DDGI_BORDER, ty + DDGI_BORDER);

	//	const float2 src_visibility = load(visibility_src, atlas_coord, 0);


	//	if (weight_sum == 0.f or probe_state == DDGI_PROBE_STATE_NEW_BORN)
	//	{
	//		visibility_dst[atlas_coord] = src_visibility;
	//	}
	//	else
	//	{
	//		visibility_dst[atlas_coord] = lerp(src_visibility, visibility_sum, DDGI_VISIBILITY_BLEND_FACTOR);
	//	}
	//}


	//// if (res.distance < 0)
	////{
	////	// inside wall
	////	probe.offset -= cast<half3>(dir * 0.1f);
	//// }

	// if (thread_id_flat == 0)
	//{
	//	probe.msme_back = gs_probe_msme_back;
	//	store_ddgi_probe_uav(probe_id, probe);
	// }
}
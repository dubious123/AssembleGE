#include "forward_plus_common.asli"

DECLARE_CALC_THREAD_GROUP_SUM_FLOAT(DDGI_PROBE_RAY_COUNT_NEW_BORN)

groupshared float	  gs_blend_factor_front;
groupshared float	  gs_blend_factor_back;
groupshared ddgi_msme gs_probe_msme_back;

[numthreads(DDGI_VISIBILITY_RESOLUTION, DDGI_VISIBILITY_RESOLUTION, 1)] void
main_cs(uint32_3 group_id  sv_group_id,
		uint32_3 thread_id sv_group_thread_id)

{
	const uint32 thread_id_flat = thread_id.x + thread_id.y * DDGI_VISIBILITY_RESOLUTION;
	// group = probe, thread = octahedral texel
	const ddgi_data ddgi_data	  = load_ddgi_data();
	const uint16_3	ppl_log2_axis = load_ddgi_ppl_log2_axis(ddgi_data);
	const uint32	probe_id	  = group_id.x
								  | (group_id.y << ppl_log2_axis.x)
								  | (group_id.z << (ppl_log2_axis.x + ppl_log2_axis.y));
	const uint16	level		  = cast<uint16>((probe_id >> load_ddgi_ppl_log2(ddgi_data)) & 0xff);
	const uint32	ray_offset	  = ddgi_load_ray_offset(ddgi_data, probe_id);
	// const uint32	ray_count	  = min(ddgi_load_ray_count(ddgi_data, probe_id), DDGI_PROBE_RAY_COUNT_NEW_BORN);
	const uint32 ray_count = ddgi_load_ray_count(ddgi_data, probe_id);

	attr_branch()

	if (ray_count == 0)
	{
		// todo
		return;
	}

	ddgi_probe	 probe		  = load_ddgi_probe_uav(probe_id);
	const float3 probe_normal = decode_oct_snorm(probe.normal_oct_snorm8);
	const uint16 probe_state  = probe.state_and_ray_count_ideal & 0xff;

	const float	 front_weight	 = probe.msme_front.inconsistency + probe.msme_front.relative_variance;
	const float	 back_weight	 = probe.msme_back.inconsistency + probe.msme_back.relative_variance;
	const float	 total_weight	 = front_weight + back_weight;
	const float	 ray_ratio_front = total_weight > epsilon_1e6 ? clamp(front_weight / total_weight, 0.2f, 0.8f) : 0.5f;
	const uint32 ray_count_front = cast<uint32>(ray_count * ray_ratio_front);
	const uint32 ray_count_back	 = ray_count - ray_count_front;

	float luminance_front = 0.f;
	float luminance_back  = 0.f;

	// todo, remove loop?
	for (uint32 i = thread_id_flat; i < ray_count; i += DDGI_VISIBILITY_RESOLUTION * DDGI_VISIBILITY_RESOLUTION)
	{
		const ddgi_ray_result ray_res = ddgi_load_ray_result(ddgi_data, ray_offset + i);
		const float			  lum	  = luminance_rec709(decode_r11g11b10(ray_res.radiance_r11g11b10));
		if (i < ray_count_front)
		{
			luminance_front += lum;
		}
		else
		{
			luminance_back += lum;
		}
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
			const ddgi_ray_result ray_res  = ddgi_load_ray_result(ddgi_data, ray_offset + i);
			const float3		  ray_dir  = decode_oct_snorm(cast<uint16>(ray_res.dir_oct_snorm_and_extra));
			const float			  distance = ray_res.distance;

			// distance < 0.f == inside wall
			const float density_ratio = (i < ray_count_front)
										  ? float(ray_count) / float(max(ray_count_front, 1u))
										  : float(ray_count) / float(max(ray_count_back, 1u));
			const float weight		  = (distance > 0.f)
										  ? max(0.f, dot(texel_dir, ray_dir)) * density_ratio
										  : 0.f;

			const float3 radiance = decode_r11g11b10(ray_res.radiance_r11g11b10) * DDGI_IRRADIANCE_ENERGY_CONSERVATION;

			irradiance_sum += radiance * weight;
			weight_sum	   += weight;
		}


		const bool	is_front	 = dot(texel_dir, probe_normal) > 0.f;
		const float blend_factor = probe_state == DDGI_PROBE_STATE_NEW_BORN
									 ? 1.f
								 : is_front
									 ? gs_blend_factor_front
									 : gs_blend_factor_back;

		rw_texture_2d<float3> irradiance_uav = global_resource_buffer[ddgi_data.irradiance_atlas_uav_id];

		const uint32_2 atlas_coord = calc_probe_atlas_offset(probe_id, load_ddgi_tile_count_w_log2(ddgi_data), DDGI_IRRADIANCE_TILE_SIZE)
								   + uint32_2(tx + DDGI_BORDER, ty + DDGI_BORDER);

		if (weight_sum == 0.f)
		{
			// irradiance_dst[atlas_coord] = float3(0, 0, 1);
		}
		else
		{
			// irradiance_sum / weight_sum : estimation of E[irradiance] / pi
			irradiance_sum				= irradiance_sum / weight_sum;
			irradiance_uav[atlas_coord] = lerp(irradiance_uav[atlas_coord], irradiance_sum, blend_factor);
			// irradiance_dst[atlas_coord] = float3(1, 0, 0);
		}
	}


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
			const ddgi_ray_result ray_res  = ddgi_load_ray_result(ddgi_data, ray_offset + i);
			const float3		  ray_dir  = decode_oct_snorm(cast<uint16>(ray_res.dir_oct_snorm_and_extra));
			const float			  distance = ray_res.distance;

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


		rw_texture_2d<float2> visibility_uav = global_resource_buffer[ddgi_data.visibility_atlas_uav_id];

		const uint32_2 atlas_coord = calc_probe_atlas_offset(probe_id, load_ddgi_tile_count_w_log2(ddgi_data), DDGI_VISIBILITY_TILE_SIZE)
								   + uint32_2(tx + DDGI_BORDER, ty + DDGI_BORDER);

		const float blend_factor = probe_state == DDGI_PROBE_STATE_NEW_BORN ? 1.f : DDGI_VISIBILITY_BLEND_FACTOR;

		if (weight_sum == 0.f)
		{
		}
		else
		{
			visibility_sum				= visibility_sum / weight_sum;
			visibility_uav[atlas_coord] = lerp(visibility_uav[atlas_coord], visibility_sum, blend_factor);
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
#include "forward_plus_common.asli"

#define DDGI_IRRADIANCE_HYSTERESIS			 0.97f
#define DDGI_IRRADIANCE_THRESHOLD			 1.5f
#define DDGI_IRRADIANCE_BRIGHTNESS_THRESHOLD 2.5f
#define DDGI_MAX_RADIANCE					 10.f


DECLARE_CALC_THREAD_GROUP_SUM_FLOAT(DDGI_PROBE_RAY_COUNT_NEW_BORN)

groupshared float gs_blend_factor;

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

	// if (probe_id >= ddgi_calc_probe_count(ddgi_data)) { return; }

	const uint16 level		= cast<uint16>((probe_id >> load_ddgi_ppl_log2(ddgi_data)) & 0xff);
	const uint32 ray_offset = ddgi_load_ray_offset(ddgi_data, probe_id);
	const uint32 ray_count	= ddgi_load_ray_count(ddgi_data, probe_id);

	attr_branch()

	if (ray_count == 0)
	{
		// todo
		return;
	}

	ddgi_probe probe = load_ddgi_probe_uav(probe_id);
	// const float3 probe_normal = decode_oct_snorm8(probe.normal_oct_snorm8);
	const uint16 probe_state = probe.state & 0xff;

	float luminance = 0.f;

	// float3 lum_weighted_dir = float3(0, 0, 0);

	float  back_face_dist_min = float_max;
	float3 back_face_dir	  = float3(0, 0, 0);
	uint32 back_face_count	  = 0u;
	// todo, remove loop?
	for (uint32 i = thread_id_flat; i < ray_count; i += DDGI_VISIBILITY_RESOLUTION * DDGI_VISIBILITY_RESOLUTION)
	{
		const ddgi_ray_result ray_res = ddgi_load_ray_result(ddgi_data, ray_offset + i);
		const float3		  ray_dir = decode_oct_snorm8(cast<uint16>(ray_res.dir_oct_snorm_and_extra));
		const float			  lum	  = luminance_rec709(decode_r11g11b10(ray_res.radiance_r11g11b10));

		if (ray_res.distance < 0.f)
		{
			back_face_count++;
			if (back_face_dist_min > -ray_res.distance)
			{
				back_face_dist_min = -ray_res.distance;
				back_face_dir	   = ray_dir;
			}
		}
		else
		{
			// lum_weighted_dir += ray_dir * lum * lum;

			luminance += lum;
		}
	}

	luminance = calc_thread_group_sum_float(luminance, thread_id_flat) / ray_count;

	// lum_weighted_dir.x = calc_thread_group_sum_float(lum_weighted_dir.x, thread_id_flat);
	// lum_weighted_dir.y = calc_thread_group_sum_float(lum_weighted_dir.y, thread_id_flat);
	// lum_weighted_dir.z = calc_thread_group_sum_float(lum_weighted_dir.z, thread_id_flat);

	back_face_count = cast<uint32>(calc_thread_group_sum_float(float(back_face_count), thread_id_flat));

	if (thread_id_flat == 0)
	{
		gs_blend_factor = ddgi_update_msme(probe.msme, luminance);
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
			const float3		  ray_dir  = decode_oct_snorm8(cast<uint16>(ray_res.dir_oct_snorm_and_extra));
			const float			  distance = ray_res.distance;

			const float weight = distance > 0.f
								   ? max(0.f, dot(texel_dir, ray_dir))
								   : 0.f;

			const float3 radiance = min(decode_r11g11b10(ray_res.radiance_r11g11b10), 100.f) * DDGI_IRRADIANCE_ENERGY_CONSERVATION;
			// const float3 radiance = decode_r11g11b10(ray_res.radiance_r11g11b10) * DDGI_IRRADIANCE_ENERGY_CONSERVATION;

			irradiance_sum += radiance * weight;
			weight_sum	   += weight;
		}


		const float blend_factor = probe_state == DDGI_PROBE_STATE_NEW_BORN
									 ? 1.f
									 : gs_blend_factor;

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
			// irradiance_uav[atlas_coord] = lerp(irradiance_uav[atlas_coord], irradiance_sum, blend_factor);
			// irradiance_dst[atlas_coord] = float3(1, 0, 0);
		}

		// for (uint32 i = 0; i < ray_count; ++i)
		//{
		//	const ddgi_ray_result ray_res  = ddgi_load_ray_result(ddgi_data, ray_offset + i);
		//	const float3		  ray_dir  = decode_oct_snorm8(cast<uint16>(ray_res.dir_oct_snorm_and_extra));
		//	const float			  distance = ray_res.distance;

		//	const float density_ratio = (i < ray_count_front)
		//								  ? float(ray_count) / float(max(ray_count_front, 1u))
		//								  : float(ray_count) / float(max(ray_count_back, 1u));
		//	const float weight		  = (distance > 0.f)
		//								  ? max(0.f, dot(texel_dir, ray_dir)) * density_ratio
		//								  : 0.f;

		//	const float3 radiance = min(decode_r11g11b10(ray_res.radiance_r11g11b10), DDGI_MAX_RADIANCE)
		//						  * DDGI_IRRADIANCE_ENERGY_CONSERVATION;

		//	// const float3 radiance = decode_r11g11b10(ray_res.radiance_r11g11b10)
		//	//					  * DDGI_IRRADIANCE_ENERGY_CONSERVATION;

		//	irradiance_sum += radiance * weight;
		//	weight_sum	   += weight;
		//}

		// rw_texture_2d<float3> irradiance_uav = global_resource_buffer[ddgi_data.irradiance_atlas_uav_id];

		// const uint32_2 atlas_coord = calc_probe_atlas_offset(probe_id, load_ddgi_tile_count_w_log2(ddgi_data), DDGI_IRRADIANCE_TILE_SIZE)
		//						   + uint32_2(tx + DDGI_BORDER, ty + DDGI_BORDER);

		// if (weight_sum > 0.f)
		//{
		//	const float3 curr = irradiance_sum / weight_sum;
		//	const float3 prev = irradiance_uav[atlas_coord];

		//	float hysteresis = (probe_state == DDGI_PROBE_STATE_NEW_BORN) ? 0.f : DDGI_IRRADIANCE_HYSTERESIS;

		//	const float3 abs_diff = abs(curr - prev);
		//	if (max(abs_diff) > DDGI_IRRADIANCE_THRESHOLD)
		//	{
		//		hysteresis = max(0.f, hysteresis - 0.75f);
		//	}

		//	float3		delta	   = curr - prev;
		//	const float max_change = max(abs(delta));
		//	if (max_change > DDGI_IRRADIANCE_BRIGHTNESS_THRESHOLD)
		//	{
		//		delta *= DDGI_IRRADIANCE_BRIGHTNESS_THRESHOLD / max_change;
		//	}

		//	irradiance_uav[atlas_coord] = prev + delta * (1.f - hysteresis);
		//}
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
			const float3		  ray_dir  = decode_oct_snorm8(cast<uint16>(ray_res.dir_oct_snorm_and_extra));
			const float			  distance = ray_res.distance;

			const float weight = distance > 0.f
								   ? pow(max(0.f, dot(texel_dir, ray_dir)), DDGI_VISIBILITY_SHARPNESS)
								   : 0.f;

			const float dist = clamp(distance, 0.f, max(ddgi_data.base_probe_spacing * 2) * (1u << level));

			visibility_sum += float2(dist, dist * dist) * weight;
			weight_sum	   += weight;
		}


		rw_texture_2d<float2> visibility_uav = global_resource_buffer[ddgi_data.visibility_atlas_uav_id];

		const uint32_2 atlas_coord = calc_probe_atlas_offset(probe_id, load_ddgi_tile_count_w_log2(ddgi_data), DDGI_VISIBILITY_TILE_SIZE)
								   + uint32_2(tx + DDGI_BORDER, ty + DDGI_BORDER);

		const float visibility_blend_factor = probe_state == DDGI_PROBE_STATE_NEW_BORN ? 1.f : DDGI_VISIBILITY_BLEND_FACTOR;

		if (weight_sum == 0.f)
		{
		}
		else
		{
			visibility_sum				= visibility_sum / weight_sum;
			visibility_uav[atlas_coord] = lerp(visibility_uav[atlas_coord], visibility_sum, visibility_blend_factor);
		}
	}

	if (thread_id_flat == 0)
	{
		// if (length(lum_weighted_dir) > epsilon_1e6)
		//{
		//	const float3 lum_dir	= normalize(lum_weighted_dir);
		//	const float3 new_normal = normalize(lerp(probe_normal, lum_dir, DDGI_NORMAL_BLEND));
		//	probe.normal_oct_snorm8 = encode_oct_snorm8(new_normal);
		// }

		if (probe.state == DDGI_PROBE_STATE_NEW_BORN)
		{
			ddgi_store_probe_state_and_extra(ddgi_data, probe_id, DDGI_PROBE_STATE_ACTIVE);
		}
		else if (probe.state == DDGI_PROBE_STATE_INSIDE_WALL)
		{
			if (back_face_count < 0.25f * ray_count)
			{
				probe.state = DDGI_PROBE_STATE_NEW_BORN;

				ddgi_store_probe_state_and_extra(ddgi_data, probe_id, DDGI_PROBE_STATE_NEW_BORN);
			}
		}
		else if (back_face_dist_min < float_max)
		{
			float3 spacing = ddgi_data.base_probe_spacing * (1u << level);
			float3 offset  = float3(probe.offset) + back_face_dir * back_face_dist_min;
			offset		   = clamp(offset, -0.5f * spacing, 0.5f * spacing);
			// probe.offset   = cast<half3>(offset * 0.99f);
			probe.offset = cast<half3>(lerp(probe.offset, offset, 0.99f));

			const bool at_limit = any(abs(probe.offset) >= spacing * 0.9f);
			if (at_limit or back_face_count > 0.25f * ray_count)
			{
				probe.state = DDGI_PROBE_STATE_INSIDE_WALL;

				ddgi_store_probe_state_and_extra(ddgi_data, probe_id, DDGI_PROBE_STATE_INSIDE_WALL);
			}
		}
		else
		{
			probe.offset = cast<half3>(float3(probe.offset) * 0.99f);
		}

		store_ddgi_probe_uav(probe_id, probe);
	}
}
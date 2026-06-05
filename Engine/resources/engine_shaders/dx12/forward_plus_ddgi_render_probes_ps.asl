#include "forward_plus_common.asli"

float4
main_ps(ddgi_ms_to_ps fragment) sv_target_0
{
	const uint32 probe_id  = fragment.probe_id;
	const float3 world_pos = fragment.world_pos;

	const ddgi_data ddgi_data = load_ddgi_data();
	const uint32	level	  = probe_id >> load_ddgi_ppl_log2(ddgi_data);
	const float3	probe_pos = ddgi_calc_probe_pos(ddgi_data, probe_id, level);

	const float3 normal = normalize(fragment.normal);

	if (ddgi_debug_flags_render_irradiance(ddgi_data))
	{
		const float3 irradiance = ddgi_sample_probe_irradiance(ddgi_data, probe_id, normal);

		return float4(irradiance, 1.f);
	}

	if (ddgi_debug_flags_render_visibility(ddgi_data))
	{
		const float2 visibility = ddgi_sample_probe_visibility(ddgi_data, probe_id, normal);

		if (any(visibility == 0.f))
		{
			return float4(0, 0, 1, 1);
		}

		return float4(visibility / (max(ddgi_data.base_probe_spacing) * (1u << level)), 0.f, 1.f);
	}

	if (ddgi_debug_flags_render_level(ddgi_data))
	{
		if (level == 0)
		{
			return float4(1, 0, 0, 1.f);
		}
		else if (level == 1)
		{
			return float4(0, 1, 0, 1.f);
		}
		else if (level == 2)
		{
			return float4(0, 0, 1, 1.f);
		}
		else if (level == 3)
		{
			return float4(1, 1, 0, 1.f);
		}
		else if (level == 4)
		{
			return float4(0, 1, 1, 1.f);
		}
		else if (level == 5)
		{
			return float4(1, 0, 1, 1.f);
		}
		else
		{
			return float4(1, 1, 1, 1.f);
		}
	}

	if (ddgi_debug_flags_render_weight_sum(ddgi_data))
	{
		const float probe_weight_sum = ddgi_load_probe_weight_sum(probe_id);

		if (probe_weight_sum == 0.f)
		{
			return float4(0, 0, 1, 1.f);
		}

		return float4(probe_weight_sum, 0, 0, 1);
	}

	if (ddgi_debug_flags_render_ray_count(ddgi_data))
	{
		const ddgi_probe probe = load_ddgi_probe_srv(probe_id);
		// sconst uint32	 probe_ray_count = (probe.state_and_ray_count_ideal >> 8u) & 0xff;

		const uint32 probe_ray_count = ddgi_load_ray_count(ddgi_data, probe_id);

		if (probe_ray_count <= DDGI_PROBE_RAY_COUNT_NEW_BORN * 0.25f)
		{
			return float4(1, 0, 0, 1);
		}
		else if (probe_ray_count <= DDGI_PROBE_RAY_COUNT_NEW_BORN * 0.5f)
		{
			return float4(0, 1, 0, 1);
		}
		else if (probe_ray_count <= DDGI_PROBE_RAY_COUNT_NEW_BORN * 0.75f)
		{
			return float4(0, 0, 1, 1);
		}
		else
		{
			return float4(1, 1, 1, 1);
		}

		// if (probe_ray_count > DDGI_PROBE_RAY_COUNT_NEW_BORN)
		//{
		//	return float4(0, 0, 1, 1);
		// }
		// float ratio = ddgi_load_ray_count_total(ddgi_data) / float(DDGI_RAY_BUDGET);

		// if (ddgi_load_ray_count_total(ddgi_data) == DDGI_RAY_BUDGET - 1)
		//{
		//	return float4(0, 0, 1, 1);
		// }
		// else if (ddgi_load_ray_count_total(ddgi_data) == DDGI_RAY_BUDGET)
		//{
		//	return float4(1, 0, 0, 1);
		// }
		// else if (ddgi_load_ray_count_total(ddgi_data) == 0)
		//{
		//	return float4(0, 0, 0, 1);
		// }
		// else if (ratio < 0.4f)
		//{
		//	return float4(ratio, 0, 0, 1);
		// }
		// else
		//{
		//	return float4(0, ddgi_load_ray_count_total(ddgi_data) / float(DDGI_RAY_BUDGET), 0, 1);
		// }

		if (probe_ray_count > DDGI_PROBE_RAY_COUNT_NEW_BORN * 3)
		{
			return float4(0, 0, 1, 1);
		}

		return float4(float(probe_ray_count) / DDGI_PROBE_RAY_COUNT_NEW_BORN, probe_ray_count > 0 ? 1.f : 0.f, 0, 1);
	}


	if (ddgi_debug_flags_render_state(ddgi_data))
	{
		const ddgi_probe probe		 = load_ddgi_probe_srv(probe_id);
		const uint16	 probe_state = probe.state & 0x00ff;

		if (probe_state == DDGI_PROBE_STATE_OFF)
		{
			return float4(1, 0, 0, 1.f);
		}
		else if (probe_state == DDGI_PROBE_STATE_ACTIVE)
		{
			return float4(0, 1, 0, 1.f);
		}
		else if (probe_state == DDGI_PROBE_STATE_SLEEP)
		{
			return float4(0, 0, 1, 1.f);
		}
		else if (probe_state == DDGI_PROBE_STATE_NEW_BORN)
		{
			return float4(1, 1, 1, 1.f);
		}
		else
		{
			return float4(100, 100, 0, 1.f);
		}
	}

	if (ddgi_debug_flag_render_msme(ddgi_data))
	{
		const ddgi_probe probe = load_ddgi_probe_srv(probe_id);
		return srgb_to_linear(float4(probe.msme.inconsistency, 0, 0, 1.f));
		// return srgb_to_linear(float4(probe.msme_front.variance, probe.msme_front.inconsistency, 0, 1.f));
	}

	if (ddgi_debug_flag_render_ray_factor(ddgi_data))
	{
		const ddgi_probe probe			  = load_ddgi_probe_srv(probe_id);
		const float		 probe_weight_sum = ddgi_load_probe_weight_sum(probe_id);

		const float relative_variance = probe.msme.variance
									  / max(DDGI_MEAN_SQ_THRESHOLD, probe.msme.mean_long * probe.msme.mean_long);


		const float probe_instability_raw = (relative_variance + probe.msme.inconsistency) * 0.5f;

		const float probe_instability = smoothstep(0.4f, 4.f, probe_instability_raw);
		const float probe_importance  = saturate(probe_weight_sum / 50.f);
		const float probe_recency	  = saturate(5.f / probe.frame_since_seen);

		const int32_3 probe_world_coord = ddgi_calc_probe_world_coord(ddgi_data, probe_id, level);
		const float3  probe_pos			= ddgi_calc_probe_pos(ddgi_data, probe_world_coord, level);

		// const uint32 probe_world_coord_packed_curr = (uint32(probe_world_coord.x) & 0x3ffu)
		//										   | ((uint32(probe_world_coord.y) & 0x3ffu) << 10u)
		//										   | ((uint32(probe_world_coord.z) & 0x3ffu) << 20u);
		// const uint32 probe_world_coord_packed_prev = probe.world_coord_packed;
		// const bool	 probe_reallocated			   = probe_world_coord_packed_prev != probe_world_coord_packed_curr;

		// return float4(probe_world_coord, 1.f);
		// if (probe_reallocated)
		//{
		//	return float4(1, 0, 0, 1);
		// }
		// else
		//{
		//	return float4(0, 1, 0, 1);
		// }


		if (probe.state == DDGI_PROBE_STATE_ACTIVE)
		{
			return float4(probe_importance * probe_instability * probe_recency, 0, 0, 1);
		}
		else if (probe.state == DDGI_PROBE_STATE_SLEEP)
		{
			return float4(probe_importance * probe_instability * probe_recency * 0.5f, 0, 0, 1);
		}
		else
		{
			return float4(0, 0, 0, 1);
		}
	}

	return float4(0, 0, 0, 1.0f);
}
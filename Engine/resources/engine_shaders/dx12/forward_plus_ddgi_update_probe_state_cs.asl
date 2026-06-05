#include "forward_plus_common.asli"

// helper
void
on_new_born(uint32 probe_id, inout ddgi_probe probe, out uint32 ray_count_ideal)
{
	probe.state		= DDGI_PROBE_STATE_NEW_BORN;
	ray_count_ideal = DDGI_PROBE_RAY_COUNT_NEW_BORN + DDGI_PROBE_RAY_COUNT_NEW_BORN / 2;
	probe.offset	= half3(0, 0, 0);
	// probe.normal_oct_snorm8 = encode_oct_snorm8(random_normal(probe_id, frame_index));
	ddgi_clear_probe_msme(probe);
}

wave_size(32)
[numthreads(DDGI_UPDATE_PROBE_STATE_THREAD_PER_GROUP, 1, 1)] void
main_cs(uint32_3 group_id		  sv_group_id,
		uint32 thread_id		  sv_group_thread_id,
		uint32 dispatch_thread_id sv_dispatch_thread_id)

{
	const ddgi_data ddgi_data		  = load_ddgi_data();
	const uint32	ppl				  = load_ddgi_ppl(ddgi_data);
	const uint32	level			  = group_id.z;
	const uint32	probe_offset	  = level * ppl
									  + group_id.x * DDGI_UPDATE_PROBE_STATE_THREAD_PER_GROUP * DDGI_UPDATE_PROBE_STATE_PROBE_PER_THREAD;
	const uint32	probe_count_total = ddgi_calc_probe_count(ddgi_data);
	uint32			ray_count_sum	  = 0u;

	for (uint32 i = 0; i < DDGI_UPDATE_PROBE_STATE_PROBE_PER_THREAD; ++i)
	{
		const uint32 probe_id = probe_offset + thread_id + i * DDGI_UPDATE_PROBE_STATE_THREAD_PER_GROUP;

		if (probe_id >= probe_count_total) { break; }
		if (probe_id >= (level + 1u) * ppl) { break; }

		// const uint32 level = probe_id >> load_ddgi_ppl_log2(ddgi_data);

		ddgi_probe	  probe							= load_ddgi_probe_uav(probe_id);
		const int32_3 probe_world_coord				= ddgi_calc_probe_world_coord(ddgi_data, probe_id, level);
		const float3  probe_pos						= ddgi_calc_probe_pos(ddgi_data, probe_world_coord, level);
		const float	  probe_weight_sum				= ddgi_load_probe_weight_sum(probe_id);
		const bool	  probe_seen					= probe_weight_sum > 0.f;
		const uint16  probe_state_prev				= probe.state & 0xff;
		const uint32  probe_world_coord_packed_curr = (uint32(probe_world_coord.x) & 0x3ffu)
													| ((uint32(probe_world_coord.y) & 0x3ffu) << 10u)
													| ((uint32(probe_world_coord.z) & 0x3ffu) << 20u);
		const uint32  probe_world_coord_packed_prev = probe.world_coord_packed;
		probe.world_coord_packed					= probe_world_coord_packed_curr;
		const bool probe_reallocated				= probe_world_coord_packed_prev != probe_world_coord_packed_curr;

		probe.frame_since_seen = uint16(probe_seen ? 1u : min(probe.frame_since_seen + 1u, 0xfe));

		uint32 ray_count_ideal = 0u;

		const float relative_variance = probe.msme.variance
									  / max(DDGI_MEAN_SQ_THRESHOLD, probe.msme.mean_long * probe.msme.mean_long);


		const float probe_instability_raw = (relative_variance + probe.msme.inconsistency) * 0.5f;

		const float probe_instability = smoothstep(0.4f, 4.f, probe_instability_raw);
		const float probe_importance  = saturate(probe_weight_sum / 50.f);
		const float probe_recency	  = saturate(5.f / probe.frame_since_seen);

		const bool probe_not_converged = max(probe.msme.inconsistency, probe.msme.inconsistency) > 0.2f;
		const bool probe_converged	   = probe_not_converged is_false;

		// todo consider inside wall
		if (probe_state_prev == DDGI_PROBE_STATE_OFF)
		{
			if (probe_seen)
			{
				on_new_born(probe_id, probe, ray_count_ideal);
			}
			else
			{
				probe.state		= DDGI_PROBE_STATE_OFF;
				ray_count_ideal = 0u;
			}
		}
		else if (probe_state_prev == DDGI_PROBE_STATE_NEW_BORN)
		{
			if (probe_reallocated)
			{
				on_new_born(probe_id, probe, ray_count_ideal);
			}
			else if (ddgi_is_probe_in_hole(ddgi_data, probe_pos, level))
			{
				probe.state		= DDGI_PROBE_STATE_OFF;
				ray_count_ideal = 0u;
			}
			else if (probe_seen)
			{
				probe.state		= DDGI_PROBE_STATE_ACTIVE;
				ray_count_ideal = DDGI_PROBE_RAY_COUNT_NEW_BORN;
			}
			else
			{
				probe.state		= DDGI_PROBE_STATE_SLEEP;
				ray_count_ideal = DDGI_PROBE_RAY_COUNT_NEW_BORN >> 1u;
			}
		}
		else if (probe_state_prev == DDGI_PROBE_STATE_ACTIVE)
		{
			if (probe_reallocated)
			{
				on_new_born(probe_id, probe, ray_count_ideal);
			}
			else if (ddgi_is_probe_in_hole(ddgi_data, probe_pos, level))
			{
				probe.state		= DDGI_PROBE_STATE_OFF;
				ray_count_ideal = 0u;
			}
			else if (probe_recency < 1.f)
			{
				probe.state		= DDGI_PROBE_STATE_SLEEP;
				ray_count_ideal = DDGI_PROBE_RAY_COUNT_NEW_BORN
								* probe_importance
								* probe_instability
								* probe_recency
								* 0.5f;
			}
			else
			{
				probe.state		= DDGI_PROBE_STATE_ACTIVE;
				ray_count_ideal = max(DDGI_PROBE_RAY_COUNT_NEW_BORN
										  * probe_importance
										  * probe_instability
										  * probe_recency,
									  16.f);
			}
		}
		else if (probe_state_prev == DDGI_PROBE_STATE_SLEEP)
		{
			if (probe_reallocated)
			{
				on_new_born(probe_id, probe, ray_count_ideal);
			}
			else if (ddgi_is_probe_in_hole(ddgi_data, probe_pos, level))
			{
				probe.state		= DDGI_PROBE_STATE_OFF;
				ray_count_ideal = 0u;
			}
			else if (probe_seen)
			{
				probe.state		= DDGI_PROBE_STATE_ACTIVE;
				ray_count_ideal = DDGI_PROBE_RAY_COUNT_NEW_BORN
								* probe_importance
								* probe_instability
								* probe_recency * 1.5f;
			}
			else
			{
				probe.state		= DDGI_PROBE_STATE_SLEEP;
				ray_count_ideal = DDGI_PROBE_RAY_COUNT_NEW_BORN
								* probe_importance
								* probe_instability
								* probe_recency
								* 0.8f;
			}
		}
		else if (DDGI_PROBE_STATE_INSIDE_WALL)
		{
			if (probe_reallocated /*or probe_seen*/)
			{
				on_new_born(probe_id, probe, ray_count_ideal);
			}
			else if (ddgi_is_probe_in_hole(ddgi_data, probe_pos, level))
			{
				probe.state		= DDGI_PROBE_STATE_OFF;
				ray_count_ideal = 0u;
			}
			else
			{
				probe.state		= DDGI_PROBE_STATE_INSIDE_WALL;
				ray_count_ideal = 8u;
			}
		}

		ddgi_clear_probe_weight(probe_id);
		ddgi_store_ray_count_ideal(ddgi_data, probe_id, ray_count_ideal);
		store_ddgi_probe_uav(probe_id, probe);
		ddgi_store_probe_state_and_extra(ddgi_data, probe_id, probe.state);
		ray_count_sum += ray_count_ideal;
	}

	const uint32 group_ray_sum = wave_active_sum(ray_count_sum);

	static const uint32 probe_per_group = DDGI_UPDATE_PROBE_STATE_THREAD_PER_GROUP * DDGI_UPDATE_PROBE_STATE_PROBE_PER_THREAD;
	const uint32		group_count_x	= ceil(ppl, probe_per_group);

	if (dispatch_thread_id == 0)
	{
		ddgi_clear_ray_sum_total_ideal(ddgi_data);
	}

	if (thread_id == 0)
	{
		ddgi_store_group_ray_sum_ideal(ddgi_data, group_id.x + group_count_x * group_id.z, group_ray_sum);
	}
}
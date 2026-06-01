#include "forward_plus_common.asli"

wave_size(32)
[numthreads(DDGI_UPDATE_PROBE_STATE_THREAD_PER_GROUP, 1, 1)] void
main_cs(uint32_3 group_id		  sv_group_id,
		uint32 thread_id		  sv_group_thread_id,
		uint32 dispatch_thread_id sv_dispatch_thread_id)

{
	const ddgi_data ddgi_data	 = load_ddgi_data();
	const uint32	ppl			 = load_ddgi_ppl(ddgi_data);
	const uint32	level		 = group_id.z;
	const uint32	probe_offset = level * ppl
								 + group_id.x * DDGI_UPDATE_PROBE_STATE_THREAD_PER_GROUP * DDGI_UPDATE_PROBE_STATE_PROBE_PER_THREAD;

	// ith thread
	// i , i + 32, i + 64, ...

	uint32 ray_count_sum = 0u;

	for (uint32 i = 0; i < DDGI_UPDATE_PROBE_STATE_PROBE_PER_THREAD; ++i)
	{
		const uint32 probe_id = probe_offset + thread_id + i * DDGI_UPDATE_PROBE_STATE_THREAD_PER_GROUP;

		ddgi_probe	probe			 = load_ddgi_probe_uav(probe_id);
		const float probe_weight_sum = ddgi_load_probe_weight_sum(probe_id);
		ddgi_clear_probe_weight(probe_id);


		const bool	 probe_seen = probe_weight_sum > 0.f;
		const float3 probe_pos	= ddgi_calc_probe_pos(ddgi_data, probe_id, level);

		if (ddgi_is_probe_in_hole(ddgi_data, probe_pos, level) or (probe.frame_since_seen == 0xff))
		{
			probe.state_and_ray_count_ideal = DDGI_PROBE_STATE_OFF;
			probe.frame_since_seen			= 0xff;
			store_ddgi_probe_uav(probe_id, probe);

			ddgi_store_ray_count_ideal(ddgi_data, probe_id, 0);
			continue;
		}

		// if (probe.frame_since_seen == 0xff)
		//{
		//	probe.frame_since_seen = 0xfe;
		// }

		uint16 probe_state_prev = probe.state_and_ray_count_ideal & 0xff;

		if (probe_state_prev == DDGI_PROBE_STATE_OFF)
		{
			probe.offset					 = half3(0, 0, 0);
			ray_count_sum					+= DDGI_PROBE_RAY_COUNT_NEW_BORN;
			probe.state_and_ray_count_ideal	 = DDGI_PROBE_STATE_NEW_BORN | (DDGI_PROBE_RAY_COUNT_NEW_BORN << 8);
			// new born

			probe.normal_oct_snorm8 = encode_oct_snorm8(random_normal(probe_id, frame_index));

			ddgi_clear_probe_msme(probe);

			ddgi_store_ray_count_ideal(ddgi_data, probe_id, DDGI_PROBE_RAY_COUNT_NEW_BORN);
		}
		else
		{
			float ray_factor   = probe.msme_front.relative_variance
							   + probe.msme_front.inconsistency
							   + probe.msme_back.relative_variance
							   + probe.msme_back.inconsistency;
			ray_factor		  *= 0.25f;
			uint16 next_state  = DDGI_PROBE_STATE_SLEEP;
			if (probe_state_prev == DDGI_PROBE_STATE_SLEEP)
			{
				if (probe_seen)
				{
					ray_factor *= 1.5f; /*heuristic reward*/
					next_state	= DDGI_PROBE_STATE_ACTIVE;

					probe.frame_since_seen = 1;
				}
				else
				{
					next_state = DDGI_PROBE_STATE_SLEEP;
					++probe.frame_since_seen;
				}
			}
			else
			{
				// probe_state_prev == DDGI_PROBE_STATE_ACTIVE
				if (probe_seen is_false and (probe.frame_since_seen > 5 /*heuristic*/))
				{
					ray_factor *= 0.5f; /*heuristic reward*/
					next_state	= DDGI_PROBE_STATE_SLEEP;

					++probe.frame_since_seen;
				}
				else
				{
					next_state			   = DDGI_PROBE_STATE_ACTIVE;
					probe.frame_since_seen = 1;
				}
			}

			const uint16 ray_count			 = cast<uint16>(min(probe_weight_sum, 50.f) * min(ray_factor / probe.frame_since_seen, 1.f) * DDGI_PROBE_RAY_COUNT_NEW_BORN);
			probe.state_and_ray_count_ideal	 = next_state | (ray_count << 8u);
			ray_count_sum					+= ray_count;

			ddgi_store_ray_count_ideal(ddgi_data, probe_id, ray_count);
		}

		store_ddgi_probe_uav(probe_id, probe);
	}

	const uint32 group_ray_sum = wave_active_sum(ray_count_sum);

	static const uint32 probe_per_group = DDGI_UPDATE_PROBE_STATE_THREAD_PER_GROUP * DDGI_UPDATE_PROBE_STATE_PROBE_PER_THREAD;
	const uint32		group_count_x	= (ppl + probe_per_group - 1) / probe_per_group;

	if (dispatch_thread_id == 0)
	{
		ddgi_clear_ray_sum_total_ideal(ddgi_data);
	}

	if (thread_id == 0)
	{
		ddgi_store_group_ray_sum_ideal(ddgi_data, group_id.x + group_count_x * group_id.z, group_ray_sum);
	}
}
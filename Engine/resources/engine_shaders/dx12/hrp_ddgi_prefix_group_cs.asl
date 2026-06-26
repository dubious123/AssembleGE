#include "hrp_common.asli"

DECLARE_CALC_THREAD_GROUP_PREFIX_SUM(DDGI_PREFIX_THREAD_COUNT)

wave_size(32)
[numthreads(DDGI_PREFIX_THREAD_COUNT, 1, 1)] void
main_cs(uint32 group_id	 sv_group_id,
		uint32 thread_id sv_group_thread_id)

{
	const ddgi_data data		= load_ddgi_data();
	const uint32	probe_count = ddgi_calc_probe_count(data);
	const uint32	ideal_total = max(ddgi_load_ray_sum_total_ideal(data), 1u);

	uint32 ray_count_arr[DDGI_PREFIX_ELEMENT_PER_THREAD];
	uint32 local_sum = 0u;

	expand_all()

	for (uint32 i = 0; i < DDGI_PREFIX_ELEMENT_PER_THREAD; ++i)
	{
		const uint32 probe_id = group_id * DDGI_PREFIX_ELEMENT_PER_GROUP
							  + thread_id * DDGI_PREFIX_ELEMENT_PER_THREAD
							  + i;

		uint32 ray_count_real = 0u;
		if (probe_id < probe_count)
		{
			const uint32 ideal = ddgi_load_ray_count_ideal(data, probe_id);

			ray_count_real = min(cast<uint32>(cast<uint64>(DDGI_RAY_BUDGET) * ideal / ideal_total), DDGI_PROBE_RAY_COUNT_NEW_BORN);
		}
		ray_count_arr[i]  = ray_count_real;
		local_sum		 += ray_count_real;
	}

	uint32 offset = calc_thread_group_prefix_sum(local_sum, thread_id);

	expand_all()

	for (uint32 i = 0; i < DDGI_PREFIX_ELEMENT_PER_THREAD; ++i)
	{
		const uint32 probe_id = group_id * DDGI_PREFIX_ELEMENT_PER_GROUP
							  + thread_id * DDGI_PREFIX_ELEMENT_PER_THREAD
							  + i;

		if (probe_id < probe_count)
		{
			ddgi_store_ray_offset(data, probe_id, offset);
			ddgi_store_ray_count(data, probe_id, ray_count_arr[i]);
		}
		offset += ray_count_arr[i];
	}

	if (thread_id == DDGI_PREFIX_THREAD_COUNT - 1)
	{
		ddgi_store_group_sum(data, group_id, offset);
	}
}
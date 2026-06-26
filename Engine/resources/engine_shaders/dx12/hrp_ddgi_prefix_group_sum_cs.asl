#include "hrp_common.asli"

DECLARE_CALC_THREAD_GROUP_PREFIX_SUM(DDGI_PREFIX_THREAD_COUNT)

wave_size(32)
[numthreads(DDGI_PREFIX_THREAD_COUNT, 1, 1)] void
main_cs(uint32 thread_id sv_group_thread_id)
{
	const ddgi_data data		= load_ddgi_data();
	const uint32	probe_count = ddgi_calc_probe_count(data);
	const uint32	group_count = ceil(probe_count, DDGI_PREFIX_ELEMENT_PER_GROUP);

	uint32 group_sum_arr[DDGI_PREFIX_ELEMENT_PER_THREAD];
	uint32 local_sum = 0u;

	expand_all()

	for (uint32 i = 0; i < DDGI_PREFIX_ELEMENT_PER_THREAD; ++i)
	{
		const uint32 idx  = thread_id * DDGI_PREFIX_ELEMENT_PER_THREAD + i;
		group_sum_arr[i]  = (idx < group_count) ? ddgi_load_group_sum(data, idx) : 0u;
		local_sum		 += group_sum_arr[i];
	}

	uint32 prefix = calc_thread_group_prefix_sum(local_sum, thread_id);

	expand_all()

	for (uint32 i = 0; i < DDGI_PREFIX_ELEMENT_PER_THREAD; ++i)
	{
		const uint32 idx = thread_id * DDGI_PREFIX_ELEMENT_PER_THREAD + i;
		if (idx < group_count)
		{
			ddgi_store_group_prefix(data, idx, prefix);
		}
		prefix += group_sum_arr[i];
	}

	if (thread_id == DDGI_PREFIX_THREAD_COUNT - 1)
	{
		ddgi_store_ray_count_total(data, min(prefix, DDGI_RAY_BUDGET));
	}
}
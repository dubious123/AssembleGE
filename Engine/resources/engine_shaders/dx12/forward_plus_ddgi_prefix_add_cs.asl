#include "forward_plus_common.asli"

wave_size(32)
[numthreads(DDGI_PREFIX_THREAD_COUNT, 1, 1)] void
main_cs(uint32 group_id			  sv_group_id,
		uint32 thread_id		  sv_group_thread_id,
		uint32 dispatch_thread_id sv_dispatch_thread_id)

{
	const ddgi_data data		 = load_ddgi_data();
	const uint32	probe_count	 = ddgi_calc_probe_count(data);
	const uint32	block_offset = ddgi_load_group_prefix(data, group_id);

	expand_all()

	for (uint32 i = 0; i < DDGI_PREFIX_ELEMENT_PER_THREAD; ++i)
	{
		const uint32 probe_id = group_id * DDGI_PREFIX_ELEMENT_PER_GROUP
							  + thread_id * DDGI_PREFIX_ELEMENT_PER_THREAD
							  + i;

		if (probe_id < probe_count)
		{
			ddgi_store_ray_offset(data, probe_id, block_offset + ddgi_load_ray_offset(data, probe_id));
		}
	}

	if (dispatch_thread_id == 0)
	{
		const uint32 group_x = ceil(ddgi_load_ray_count_total(data), DDGI_TRACE_THREAD_PER_GROUP);
		store_dispatch_compute_indirect_arg(uint32_3(group_x, 1, 1));
	}
}
#include "hrp_common.asli"

wave_size(GIBS_CELL_PREFIX_TPG)
[numthreads(GIBS_CELL_PREFIX_TPG, 1, 1)] void
main_cs(uint32 dispatch_thread_id sv_dispatch_thread_id,
		uint32 thread_id		  sv_group_thread_id,
		uint32 group_id			  sv_group_id)

{
	const uint32 cell_id_base = group_id * GIBS_CELL_PREFIX_EPG;

	const gibs_data data = gibs::load_data();

	uint32 local_surfel_sum = 0u;

	rw_byte_array<gibs_cell_surfel_entry> cell_surfel_entry_arr = gibs::cell::surfel_entry_rw_arr(data);

	for (uint32 i = 0; i < GIBS_CELL_PREFIX_EPT; ++i)
	{
		const uint32 cell_id = cell_id_base
							 + i * GIBS_CELL_PREFIX_TPG
							 + thread_id;

		if (cell_id >= data.cell_count_total) { break; }

		local_surfel_sum += cell_surfel_entry_arr[cell_id].surfel_count;
	}

	const uint32 local_surfel_offset = wave_prefix_sum(local_surfel_sum);

	uint32 group_surfel_offset_tmp = 0;
	if (thread_id == GIBS_CELL_PREFIX_TPG - 1)
	{
		group_surfel_offset_tmp = gibs::cell::alloc_surfel(data, local_surfel_offset + local_surfel_sum);
	}
	const uint32 group_surfel_offset = wave_read_lane_at(group_surfel_offset_tmp, GIBS_CELL_PREFIX_TPG - 1);

	uint32 offset = group_surfel_offset + local_surfel_offset;
	for (uint32 i = 0; i < GIBS_CELL_PREFIX_EPT; ++i)
	{
		const uint32 cell_id = cell_id_base
							 + i * GIBS_CELL_PREFIX_TPG
							 + thread_id;

		if (cell_id >= data.cell_count_total) { break; }

		gibs_cell_surfel_entry surfel_entry = cell_surfel_entry_arr[cell_id];

		c_auto count			  = surfel_entry.surfel_count;
		surfel_entry.offset		  = offset;
		surfel_entry.surfel_count = 0u;

		cell_surfel_entry_arr.store(cell_id, surfel_entry);
		offset += count;
	}

	if (dispatch_thread_id == 0)
	{
		const uint32_2 ray_total = gibs::ray::count_total(data);

		gibs::indirect_arg::set_ray_trace(data, uint32_3(ceil(ray_total.x + ray_total.y, 64), 1, 1));
		gibs::indirect_arg::set_ray_resolve(data, uint32_3(ceil(ray_total.x + ray_total.y, 64), 1, 1));
	}
}
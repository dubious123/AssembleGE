#include "hrp_common.asli"

wave_size(GIST_GENERAL_TPG)
[numthreads(GIST_GENERAL_TPG, 1, 1)] void
main_cs(uint32 dispatch_thread_id sv_dispatch_thread_id,
		uint32 thread_id		  sv_group_thread_id,
		uint32 group_id			  sv_group_id)

{
	const uint32 cell_id_base = group_id * GIST_GENERAL_EPG;

	const gist_data data = gist::load_data();

	uint32 local_surfel_sum = 0u;

	rw_byte_array<gist_cell_surfel_entry> cell_surfel_entry_arr = gist::cell::surfel_entry_rw_arr(data);

	for (uint32 i = 0; i < GIST_GENERAL_EPT; ++i)
	{
		const uint32 cell_id = cell_id_base
							 + i * GIST_GENERAL_TPG
							 + thread_id;

		if (cell_id >= data.cell_count_total()) { break; }

		const gist_cell_surfel_entry surfel_entry = cell_surfel_entry_arr[cell_id];

		local_surfel_sum += surfel_entry.surfel_count;
	}

	const uint32 local_surfel_offset = wave_prefix_sum(local_surfel_sum);

	uint32 group_surfel_offset_tmp = 0;
	if (thread_id == GIST_GENERAL_TPG - 1)
	{
		group_surfel_offset_tmp = gist::cell::alloc_surfel(data, local_surfel_offset + local_surfel_sum);
	}
	const uint32 group_surfel_offset = wave_read_lane_at(group_surfel_offset_tmp, GIST_GENERAL_TPG - 1);

	uint32 offset = group_surfel_offset + local_surfel_offset;
	for (uint32 i = 0; i < GIST_GENERAL_EPT; ++i)
	{
		const uint32 cell_id = cell_id_base
							 + i * GIST_GENERAL_TPG
							 + thread_id;

		if (cell_id >= data.cell_count_total()) { break; }

		gist_cell_surfel_entry surfel_entry = cell_surfel_entry_arr[cell_id];

		c_auto count			  = surfel_entry.surfel_count;
		surfel_entry.offset		  = offset;
		surfel_entry.surfel_count = 0u;

		cell_surfel_entry_arr.store(cell_id, surfel_entry);
		offset += count;
	}

	if (dispatch_thread_id == 0)
	{
		const uint32 cell_surfel_ray_total = gist::cell::ray_count_total(data);

		gist::indirect_arg::set_ray_trace(data, uint32_3(ceil(cell_surfel_ray_total, 64), 1, 1));
		gist::indirect_arg::set_ray_resolve(data, uint32_3(ceil(data.tile_count_total() + cell_surfel_ray_total + gist::adaptive::ray_count_total(data), 64), 1, 1));
	}
}
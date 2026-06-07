#include "forward_plus_common.asli"

wave_size(GIBS_CELL_SURFEL_COUNT_PREFIX_TPG)
[numthreads(GIBS_CELL_SURFEL_COUNT_PREFIX_TPG, 1, 1)] void
main_cs(uint32 thread_id sv_group_thread_id,
		uint32 group_id	 sv_group_id)

{
	const uint32 cell_id_base = group_id * GIBS_CELL_SURFEL_COUNT_PREFIX_EPG;

	const gibs_data data = gibs_load_gibs_data();

	// local sum of surfel_count_per_cell
	uint32 local_sum = 0u;

	rw_byte_array<gibs_cell_entry> cell_entry_arr = gibs_load_cell_entry_rw_arr(data);

	uint32 local_prefix_arr[GIBS_CELL_SURFEL_COUNT_PREFIX_EPT];

	expand_all()

	for (uint32 i = 0; i < GIBS_CELL_SURFEL_COUNT_PREFIX_EPT; ++i)
	{
		const uint32 cell_id = cell_id_base
							 + i * GIBS_CELL_SURFEL_COUNT_PREFIX_TPG
							 + thread_id;

		if (cell_id >= data.cell_count_total) { break; }

		const gibs_cell_entry entry = cell_entry_arr[cell_id];

		local_prefix_arr[i]	 = local_sum;
		local_sum			+= entry.count;
	}

	const uint32 local_offset = wave_prefix_sum(local_sum);

	uint32 group_offset_tmp = 0;
	if (thread_id == GIBS_CELL_SURFEL_COUNT_PREFIX_TPG - 1)
	{
		group_offset_tmp = gibs_alloc_cell_surfel_offset(data, local_offset + local_sum);
	}
	const uint32 group_offset = wave_read_lane_at(group_offset_tmp, GIBS_CELL_SURFEL_COUNT_PREFIX_TPG - 1);


	expand_all()

	for (uint32 i = 0; i < GIBS_CELL_SURFEL_COUNT_PREFIX_EPT; ++i)
	{
		const uint32 cell_id = cell_id_base
							 + i * GIBS_CELL_SURFEL_COUNT_PREFIX_TPG
							 + thread_id;

		if (cell_id >= data.cell_count_total) { break; }

		gibs_cell_entry entry = cell_entry_arr[cell_id];

		entry.offset = group_offset
					 + local_offset
					 + local_prefix_arr[i];

		entry.count = 0u;

		cell_entry_arr.store(cell_id, entry);
	}
}
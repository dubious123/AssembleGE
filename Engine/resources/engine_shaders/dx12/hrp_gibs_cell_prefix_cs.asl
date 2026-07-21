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
	uint32 local_probe_sum	= 0u;

	rw_byte_array<gibs_cell_surfel_entry> cell_surfel_entry_arr = gibs::cell::surfel_entry_rw_arr(data);
	rw_byte_array<gibs_cell_probe_entry>  cell_probe_entry_arr	= gibs::cell::probe_entry_rw_arr(data);

	uint32 local_surfel_prefix_arr[GIBS_CELL_PREFIX_EPT];
	uint32 local_probe_prefix_arr[GIBS_CELL_PREFIX_EPT];

	expand_all()

	for (uint32 i = 0; i < GIBS_CELL_PREFIX_EPT; ++i)
	{
		const uint32 cell_id = cell_id_base
							 + i * GIBS_CELL_PREFIX_TPG
							 + thread_id;

		if (cell_id >= data.cell_count_total) { break; }

		const gibs_cell_surfel_entry surfel_entry = cell_surfel_entry_arr[cell_id];

		local_surfel_prefix_arr[i]	= local_surfel_sum;
		local_surfel_sum		   += surfel_entry.surfel_count;

		const gibs_cell_probe_entry probe_entry = cell_probe_entry_arr[cell_id];

		local_probe_prefix_arr[i]  = local_probe_sum;
		local_probe_sum			  += probe_entry.probe_count;
	}

	const uint32 local_surfel_offset = wave_prefix_sum(local_surfel_sum);
	const uint32 local_probe_offset	 = wave_prefix_sum(local_probe_sum);

	uint32 group_surfel_offset_tmp = 0;
	uint32 group_probe_offset_tmp  = 0;
	if (thread_id == GIBS_CELL_PREFIX_TPG - 1)
	{
		group_surfel_offset_tmp = gibs::cell::alloc_surfel(data, local_surfel_offset + local_surfel_sum);
		group_probe_offset_tmp	= gibs::cell::alloc_probe(data, local_probe_offset + local_probe_sum);
	}
	const uint32 group_surfel_offset = wave_read_lane_at(group_surfel_offset_tmp, GIBS_CELL_PREFIX_TPG - 1);
	const uint32 group_probe_offset	 = wave_read_lane_at(group_probe_offset_tmp, GIBS_CELL_PREFIX_TPG - 1);

	expand_all()

	for (uint32 i = 0; i < GIBS_CELL_PREFIX_EPT; ++i)
	{
		const uint32 cell_id = cell_id_base
							 + i * GIBS_CELL_PREFIX_TPG
							 + thread_id;

		if (cell_id >= data.cell_count_total) { break; }

		gibs_cell_surfel_entry surfel_entry = cell_surfel_entry_arr[cell_id];

		surfel_entry.offset = group_surfel_offset
							+ local_surfel_offset
							+ local_surfel_prefix_arr[i];

		surfel_entry.surfel_count = 0u;

		cell_surfel_entry_arr.store(cell_id, surfel_entry);


		gibs_cell_probe_entry probe_entry = cell_probe_entry_arr[cell_id];

		probe_entry.offset = group_probe_offset
						   + local_probe_offset
						   + local_probe_prefix_arr[i];

		probe_entry.probe_count = 0u;

		cell_probe_entry_arr.store(cell_id, probe_entry);
	}

	if (dispatch_thread_id == 0)
	{
		const uint32_2 ray_total = gibs::ray::count_total(data);

		gibs::indirect_arg::set_ray_trace(data, uint32_3(ceil(ray_total.x + ray_total.y, 64), 1, 1));
		gibs::indirect_arg::set_ray_resolve(data, uint32_3(ceil(ray_total.x + ray_total.y, 64), 1, 1));
	}
}
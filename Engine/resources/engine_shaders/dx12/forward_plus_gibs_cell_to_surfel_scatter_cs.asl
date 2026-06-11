#include "forward_plus_common.asli"

struct fn_fill_cell_to_surfel
{
	uint32 surfel_id;

	surfel surfel;

	static fn_fill_cell_to_surfel
	init(uint32 id, const struct surfel surfel)
	{
		fn_fill_cell_to_surfel res;
		res.surfel_id = id;
		res.surfel	  = surfel;
		return res;
	}

	void
	operator()(const gibs_data data, const gibs_lut_data lut_data, int32_4 cell_idx)
	{
		if (gibs_surfel_cell_intersect(data, lut_data, surfel, cell_idx) is_false) { return; }

		const uint32				   cell_idx_flat = gibs_flatten_cell_idx(data, cell_idx);
		rw_byte_array<gibs_cell_entry> entry_arr	 = gibs_load_cell_entry_rw_arr(data);
		const gibs_cell_entry		   entry		 = entry_arr[cell_idx_flat];					  // don't read count

		const uint32		  local_offset			= entry_arr.atomic_inc(cell_idx_flat, 1u, 1u);	  // entry.count++;
		rw_byte_array<uint32> cell_to_surfel_id_arr = gibs_load_cell_to_surfel_id_rw_arr(data);
		cell_to_surfel_id_arr.store(entry.offset + local_offset, surfel_id);
	}
};

wave_size(32)
[numthreads(32, 1, 1)] void
main_cs(uint32 alive_idx sv_dispatch_thread_id)

{
	const gibs_data		   data				= gibs_load_gibs_data();
	const rw_stack<uint32> alive_stack_curr = gibs_load_alive_surfel_id_stack_curr(data);

	if (alive_idx >= alive_stack_curr.size()) { return; }

	uint32 surfel_id = alive_stack_curr[alive_idx];

	rw_array<surfel> surfel_arr = gibs_load_surfel_rw_arr(data);

	const surfel surfel = surfel_arr[surfel_id];

	// cell update
	fn_fill_cell_to_surfel fn = fn_fill_cell_to_surfel::init(surfel_id, surfel);
	gibs_foreach_neighbor_cell(fn, data, gibs_load_gibs_lut_data(), surfel.position);
}
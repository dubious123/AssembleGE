#include "hrp_common.asli"

struct fn_fill_cell_to_surfel
{
	uint32			 surfel_id;
	gibs_cell_surfel surfel;

	static fn_fill_cell_to_surfel
	init(uint32 surfel_id, const gibs_cell_surfel surfel)
	{
		fn_fill_cell_to_surfel res;
		res.surfel_id = surfel_id;
		res.surfel	  = surfel;
		return res;
	}

	void
	operator()(const gibs_data data, const gibs_lut_data lut_data, int32_4 cell_idx)
	{
		if (gibs::calc_surfel_cell_intersect(data, lut_data, surfel, cell_idx) is_false) { return; }

		const uint32 cell_id = gibs::cell::calc_id(data, cell_idx);

		gibs::cell::set_cell_to_surfel_id(data, cell_id, surfel_id);
	}
};

wave_size(AGE_WAVE_SIZE)
[numthreads(AGE_WAVE_SIZE, 1, 1)] void
main_cs(uint32 alive_id sv_dispatch_thread_id)

{
	const gibs_data						data		   = gibs::load_data();
	const byte_array<uint32>			alive_arr_curr = gibs::cell::alive_id_arr_curr(data);
	structured_buffer<gibs_cell_surfel> surfel_arr	   = global_resource_buffer[data.h_cell_surfel_buffer_srv_id];


	if (alive_id >= alive_arr_curr.size()) { return; }

	const uint32 surfel_id = alive_arr_curr[alive_id];

	const gibs_cell_surfel surfel = surfel_arr[surfel_id];

	fn_fill_cell_to_surfel fn = fn_fill_cell_to_surfel::init(surfel_id, surfel);
	gibs::foreach_neighbor_cell(fn, data, gibs::load_lut_data(), surfel.position);
}
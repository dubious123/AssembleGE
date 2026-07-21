#include "hrp_common.asli"

struct fn_cell_alloc
{
	float3 position;
	float  radius;

	static fn_cell_alloc
	init(gibs_surfel_probe probe, const gibs_data data, const gibs_lut_data lut_data)
	{
		fn_cell_alloc res;
		res.position = probe.position;
		res.radius	 = gibs::calc_cell_size(data, lut_data, probe.position);
		return res;
	}

	void
	operator()(const gibs_data data, const gibs_lut_data lut_data, int32_4 cell_idx)
	{
		if (gibs::calc_sphere_cell_intersect(data, lut_data, position, radius, cell_idx) is_false) { return; }

		const uint32 cell_id = gibs::cell::calc_id(data, cell_idx);
		const bool	 succeed = gibs::cell::add_probe(data, cell_id);

		assert(cell_id < data.cell_count_total, line);
		assert(succeed, line);
	}
};

wave_size(AGE_WAVE_SIZE)
[numthreads(AGE_WAVE_SIZE, 1, 1)] void
main_cs(uint32 alive_id sv_dispatch_thread_id)

{
	const gibs_data		data	 = gibs::load_data();
	const gibs_lut_data lut_data = gibs::load_lut_data();

	byte_array<uint32> alive_arr			  = gibs::probe::alive_id_arr_curr(data);
	const uint32	   alive_count_prev_total = alive_arr.size();

	if (alive_id >= alive_arr.size()) { return; }

	const uint32 probe_id = alive_arr[alive_id];

	structured_buffer<gibs_surfel_probe> probe_buffer = global_resource_buffer[data.h_surfel_probe_buffer_srv_id];

	const gibs_surfel_probe probe = probe_buffer[probe_id];

	fn_cell_alloc fn = fn_cell_alloc::init(probe, data, lut_data);
	gibs::foreach_neighbor_cell(fn, data, lut_data, probe.position);
}
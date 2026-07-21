#include "hrp_common.asli"

struct fn_fill_cell_to_probe
{
	uint32 probe_id;
	float3 position;
	float  radius;

	static fn_fill_cell_to_probe
	init(const gibs_data data, uint32 id, float3 position, const gibs_lut_data lut_data)
	{
		fn_fill_cell_to_probe res;
		res.probe_id = id;
		res.position = position;
		res.radius	 = gibs::calc_cell_size(data, lut_data, position);
		return res;
	}

	void
	operator()(const gibs_data data, const gibs_lut_data lut_data, int32_4 cell_idx)
	{
		if (gibs::calc_sphere_cell_intersect(data, lut_data, position, radius, cell_idx) is_false) { return; }

		const uint32 cell_id = gibs::cell::calc_id(data, cell_idx);

		gibs::cell::set_cell_to_probe_id(data, cell_id, probe_id);
	}
};

wave_size(AGE_WAVE_SIZE)
[numthreads(AGE_WAVE_SIZE, 1, 1)] void
main_cs(uint32 alive_idx sv_dispatch_thread_id)

{
	const gibs_data			 data			= gibs::load_data();
	const gibs_lut_data		 lut_data		= gibs::load_lut_data();
	const byte_array<uint32> alive_arr_curr = gibs::probe::alive_id_arr_curr(data);

	structured_buffer<gibs_surfel_probe> probe_arr = global_resource_buffer[data.h_surfel_probe_buffer_srv_id];

	if (alive_idx >= alive_arr_curr.size()) { return; }

	const uint32 probe_id = alive_arr_curr[alive_idx];

	const gibs_surfel_probe probe = probe_arr[probe_id];

	fn_fill_cell_to_probe fn = fn_fill_cell_to_probe::init(data, probe_id, probe.position, lut_data);
	gibs::foreach_neighbor_cell(fn, data, lut_data, probe.position);
}
#include "hrp_common.asli"

struct fn_cell_alloc
{
	gibs_cell_surfel surfel;

	static fn_cell_alloc
	init(const gibs_cell_surfel surfel)
	{
		fn_cell_alloc res;
		res.surfel = surfel;
		return res;
	}

	void
	operator()(const gibs_data data, const gibs_lut_data lut_data, int32_4 cell_idx)
	{
		if (gibs::calc_surfel_cell_intersect(data, lut_data, surfel, cell_idx) is_false) { return; }

		const uint32 cell_id = gibs::cell::calc_id(data, cell_idx);
		const bool	 succeed = gibs::cell::add_surfel(data, cell_id);

		assert(cell_id < data.cell_count_total, line);
		assert(succeed, line);
	}
};

wave_size(AGE_WAVE_SIZE)
[numthreads(AGE_WAVE_SIZE, 1, 1)] void
main_cs(uint32 group_id		   sv_group_id,
		uint32 group_thread_id sv_group_thread_id,
		uint32 alive_id		   sv_dispatch_thread_id)

{
	const gibs_data data = gibs::load_data();

	byte_array<uint32> alive_arr_curr = gibs::cell::alive_id_arr_curr(data);
	if (alive_id >= alive_arr_curr.size()) { return; }

	const uint32 surfel_id = alive_arr_curr[alive_id];

	structured_buffer<gibs_cell_surfel>	   surfel_arr = global_resource_buffer[data.h_cell_surfel_buffer_srv_id];
	rw_structured_buffer<gibs_surfel_msme> msme_arr	  = global_resource_buffer[data.h_cell_surfel_msme_buffer_uav_id];

	const gibs_cell_surfel surfel = surfel_arr[surfel_id];
	const gibs_surfel_msme msme	  = msme_arr[surfel_id];

	fn_cell_alloc fn = fn_cell_alloc::init(surfel);
	gibs::foreach_neighbor_cell(fn, data, gibs::load_lut_data(), surfel.position);

	// uint32 ray_count_ideal = uint32(lerp(GIBS_MIN_RAY_PER_SURFEL, GIBS_MAX_RAY_PER_SURFEL, msme.inconsistency * 4));
	// uint32 ray_count_ideal = uint32(lerp(GIBS_MIN_RAY_PER_SURFEL, GIBS_MAX_RAY_PER_SURFEL, saturate(sqrt(max(0.f, msme.incon_var)) * 10)));
	uint16 ideal_ray_count = uint16(lerp(GIBS_MIN_RAY_PER_SURFEL, GIBS_MAX_RAY_PER_SURFEL, msme.incon_var * (1.f / GIBS_MSME_INCON_BLEND)));

	if (surfel.recycle_data.frame_since_born() < GIBS_RADIANCE_CACHE_DELAY)
	{
		ideal_ray_count = GIBS_MAX_RAY_PER_SURFEL;
	}

	gibs::cell::set_ideal_ray_count(data, alive_id, ideal_ray_count);

	const uint16 ideal_ray_count_wave_sum = wave_active_sum(ideal_ray_count);

	if (wave_is_first_lane())
	{
		gibs::cell::set_ideal_ray_count_wave_sum(data, group_id, ideal_ray_count_wave_sum);
	}
}
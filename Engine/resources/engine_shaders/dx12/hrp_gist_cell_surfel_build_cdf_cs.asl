#include "hrp_common.asli"

wave_size(AGE_WAVE_SIZE)
[numthreads(32, 1, 1)] void
main_cs(uint32 group_id	 sv_group_id,
		uint32 thread_id sv_group_thread_id)

{
	const gist_data data = gist::load_data();

	byte_array<uint32> cell_alive_arr = gist::cell::alive_id_arr_curr(data);

	const uint32 alive_id  = group_id;
	const uint16 ray_count = gist::cell::ray_count_rw_arr(data)[alive_id];
	const uint32 surfel_id = cell_alive_arr[alive_id];

	if (ray_count == 0) { return; }

	rw_byte_array<half> lum_arr		= gist::cell::luminance_rw_arr(data, surfel_id);
	rw_byte_array<half> lum_cdf_arr = gist::cell::luminance_cdf_rw_arr(data, surfel_id);

	// build cdf
	{
		const float lum_33 = lum_arr[36 - 4];
		const float lum_34 = lum_arr[36 - 3];
		const float lum_35 = lum_arr[36 - 2];
		const float lum_36 = lum_arr[36 - 1];

		const float lum		   = float(lum_arr[thread_id]);
		const float lum_prefix = wave_prefix_sum(lum);

		float lum_total = 0.f;

		if (thread_id == wave_lane_count() - 1)	   // 0, 1, 2, 3
		{
			lum_total = lum_prefix + lum + lum_33 + lum_34 + lum_35 + lum_36;
		}
		lum_total = max(epsilon_1e4, wave_read_lane_at(lum_total, wave_lane_count() - 1));

		lum_cdf_arr.store(thread_id, half((lum_prefix + lum) / lum_total));

		if (thread_id == 28)
		{
			lum_cdf_arr.store(36 - 4, half((lum_total - lum_34 - lum_35 - lum_36) / lum_total));
		}
		else if (thread_id == 29)
		{
			lum_cdf_arr.store(36 - 3, half((lum_total - lum_35 - lum_36) / lum_total));
		}
		else if (thread_id == 30)
		{
			lum_cdf_arr.store(36 - 2, half((lum_total - lum_36) / lum_total));
		}
		else if (thread_id == 31)
		{
			lum_cdf_arr.store(36 - 1, half(lum_total));
		}
	}
}
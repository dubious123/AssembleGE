#include "hrp_common.asli"

wave_size(AGE_WAVE_SIZE)
[numthreads(32, 1, 1)] void
main_cs(uint32 group_id	 sv_group_id,
		uint32 thread_id sv_group_thread_id)

{
	const gibs_data data = gibs::load_data();

	byte_array<uint32> tile_alive_arr = gibs::tile::alive_id_arr_curr(data);
	byte_array<uint32> cell_alive_arr = gibs::cell::alive_id_arr_curr(data);

	const uint32 tile_surfel_count = tile_alive_arr.size();
	const uint32 cell_surfel_count = cell_alive_arr.size();

	const bool is_tile = group_id < tile_surfel_count;

	uint32 ray_count;
	uint32 surfel_id;

	if (is_tile)
	{
		const uint32 alive_id = group_id;
		ray_count			  = gibs::tile::ray_count_rw_arr(data)[alive_id];
		surfel_id			  = tile_alive_arr[alive_id];
	}
	else if (group_id - tile_surfel_count < cell_surfel_count)
	{
		const uint32 alive_id = group_id - tile_surfel_count;
		ray_count			  = gibs::cell::ray_count_rw_arr(data)[alive_id];
		surfel_id			  = cell_alive_arr[alive_id];
	}
	else
	{
		return;
	}

	if (ray_count == 0) { return; }

	rw_byte_array<half> lum_arr		= gibs::load_lum_rw_arr(data, surfel_id, is_tile);
	rw_byte_array<half> lum_cdf_arr = gibs::load_lum_cdf_rw_arr(data, surfel_id, is_tile);

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

	//
	// wave_prefix_sum is not monotonic: lanes accumulate in different orders
	// (parallel tree) and float add is non-associative, so cdf[i] <= cdf[i+1]
	// can break -> pdf = cdf[i+1] - cdf[i] can go negative in ray_trace_cs.
	//
	// fix: serial scan in build (monotonic but wastes lanes), OR clamp in consumer.
	// chose clamp: pdf = max(0, cdf[i+1] - cdf[i]).
	//
	// commented code below shows wave_prefix_sum breaking monotonicity (kept for ref):
	//
	// group_memory_barrier_with_sync();
	// if (group_id.x == 0 and thread_id == wave_lane_count() - 1)
	//{
	//	const uint32_2				atlas_offset	= gibs_calc_atlas_offset(data, surfel_id);
	//	const rw_texture_2d<float2> luminance_atlas = global_resource_buffer[data.h_irradiance_atlas_uav_id];
	//	float						prev			= -1.f;

	//	bool invalid = false;
	//	for (uint32 i = 0; i < 35; ++i)
	//	{
	//		float c = luminance_atlas[atlas_offset + uint32_2(i % 6, i / 6)].y;
	//		if (c < prev)
	//		{
	//			invalid = true;
	//			break;
	//		}
	//		prev = c;
	//	}

	//	if (invalid)
	//	{
	//		float ground_truth = 0.f;

	//		for (uint32 i = 0; i < 36; ++i)
	//		{
	//			ground_truth += luminance_atlas[atlas_offset + uint32_2(i % 6, i / 6)].x;
	//			float2 c	  = luminance_atlas[atlas_offset + uint32_2(i % 6, i / 6)];
	//			debug_log(g::fmt_forward_plus_gibs_build_cdf_cs, surfel_id, i, c, ground_truth);
	//		}
	//	}
	//}
}
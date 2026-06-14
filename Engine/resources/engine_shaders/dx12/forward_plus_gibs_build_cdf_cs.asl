#include "forward_plus_common.asli"

wave_size(32)
[numthreads(32, 1, 1)] void
main_cs(uint32_3 group_id sv_group_id,
		uint32 thread_id  sv_group_thread_id)

{
	const gibs_data				data		   = gibs_load_gibs_data();
	const rw_stack<uint32>		alive_atack	   = gibs_load_alive_surfel_id_stack_curr(data);
	const rw_byte_array<uint32> ray_offset_arr = gibs_load_surfel_ray_count_prefix_rw_arr(data);
	const rw_byte_array<uint32> ray_count_arr  = gibs_load_surfel_ray_count_ideal_rw_arr(data);
	const uint32				alive_count	   = alive_atack.size();

	uint32 alive_idx = group_id.x + group_id.y * ceil(data.max_surfel_count, 32 * 32);

	attr_branch()

	if (alive_count == 0 or alive_idx >= alive_count)
	{
		return;
	}


	const uint32 ray_offset = ray_offset_arr[alive_idx];
	const uint32 ray_count	= ray_count_arr[alive_idx];

	attr_branch()

	if (ray_count == 0)
	{
		return;
	}

	const uint32 surfel_id = alive_atack[alive_idx];

	// build cdf
	{
		const uint32_2 atlas_offset = gibs_calc_atlas_offset(data, surfel_id);

		const rw_texture_2d<float2> luminance_atlas = global_resource_buffer[data.h_irradiance_atlas_uav_id];

		uint32_2 px = atlas_offset + uint32_2(thread_id % GIBS_ATLAS_TILE_SIZE, thread_id / GIBS_ATLAS_TILE_SIZE);

		const float2 lum_33 = luminance_atlas[atlas_offset + uint32_2(2, GIBS_ATLAS_TILE_SIZE - 1)];
		const float2 lum_34 = luminance_atlas[atlas_offset + uint32_2(3, GIBS_ATLAS_TILE_SIZE - 1)];
		const float2 lum_35 = luminance_atlas[atlas_offset + uint32_2(4, GIBS_ATLAS_TILE_SIZE - 1)];
		const float2 lum_36 = luminance_atlas[atlas_offset + uint32_2(5, GIBS_ATLAS_TILE_SIZE - 1)];

		const float2 lum		= luminance_atlas[px];
		const float	 lum_prefix = wave_prefix_sum(lum.x);

		float lum_total = 0.f;

		if (thread_id == wave_lane_count() - 1)	   // 0, 1, 2, 3
		{
			lum_total = lum_prefix + lum.x + lum_33.x + lum_34.x + lum_35.x + lum_36.x;
		}
		lum_total = max(epsilon_1e4, wave_read_lane_at(lum_total, wave_lane_count() - 1));

		luminance_atlas[px] = float2(lum.x, (lum_prefix + lum.x) / lum_total);

		if (thread_id == 28)
		{
			luminance_atlas[atlas_offset + uint32_2(2, GIBS_ATLAS_TILE_SIZE - 1)] = float2(lum_33.x, (lum_total - lum_34.x - lum_35.x - lum_36.x) / lum_total);
		}
		else if (thread_id == 29)
		{
			luminance_atlas[atlas_offset + uint32_2(3, GIBS_ATLAS_TILE_SIZE - 1)] = float2(lum_34.x, (lum_total - lum_35.x - lum_36.x) / lum_total);
		}
		else if (thread_id == 30)
		{
			luminance_atlas[atlas_offset + uint32_2(4, GIBS_ATLAS_TILE_SIZE - 1)] = float2(lum_35.x, (lum_total - lum_36.x) / lum_total);
		}
		else if (thread_id == 31)
		{
			luminance_atlas[atlas_offset + uint32_2(5, GIBS_ATLAS_TILE_SIZE - 1)] = float2(lum_36.x, lum_total);
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
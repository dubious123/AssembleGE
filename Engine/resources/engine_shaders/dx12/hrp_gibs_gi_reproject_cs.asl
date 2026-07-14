#include "hrp_common.asli"

#define GIBS_GI_MAX_AGE 16

#define wave_count ((GIBS_GI_RESOLVE_BLOCK_SIZE * GIBS_GI_RESOLVE_BLOCK_SIZE + AGE_WAVE_SIZE - 1) / AGE_WAVE_SIZE)

groupshared uint32 gs_scratch[wave_count][GIBS_GI_RESOLVE_SAMPLE_PER_BLOCK];

wave_size(AGE_WAVE_SIZE)
[numthreads(GIBS_GI_RESOLVE_BLOCK_SIZE * GIBS_GI_RESOLVE_BLOCK_SIZE, 1, 1)] void
main_cs(uint32_3 group_id	   sv_group_id,
		uint32 group_thread_id sv_group_thread_id)

{
	const gibs_data data = gibs_load_gibs_data();

	const int32_2 extent = int32_2(backbuffer_size);

	bool is_thread_valid = true;

	const int32_2 px = int32_2(group_id.xy) * GIBS_GI_RESOLVE_BLOCK_SIZE
					 + int32_2(group_thread_id % GIBS_GI_RESOLVE_BLOCK_SIZE, group_thread_id / GIBS_GI_RESOLVE_BLOCK_SIZE);

	if (any(px >= extent)) { is_thread_valid = false; }

	rw_texture_2d<uint32> gi_resolve_age_curr_buffer = global_resource_buffer[data.h_gi_resolve_age_curr_buffer_uav_id];
	texture_2d<uint32>	  gi_resolve_age_prev_buffer = global_resource_buffer[data.h_gi_resolve_age_prev_buffer_srv_id];

	texture_2d<uint32_2> opaque_geo_prev_buffer = global_resource_buffer[opaque_geo_prev_buffer_srv_id];

	rw_texture_2d<float3> gi_resolve_curr_buffer = global_resource_buffer[data.h_gi_resolve_curr_buffer_uav_id];
	texture_2d<float3>	  gi_resolve_prev_buffer = global_resource_buffer[data.h_gi_resolve_prev_buffer_srv_id];

	// prev weight
	texture_2d<float> gi_resolve_weight_buffer = global_resource_buffer[data.h_gi_resolve_weight_buffer_srv_id];

	texture_2d<float>	 depth_buffer  = global_resource_buffer[opaque_depth_buffer_srv_id];
	texture_2d<uint32_2> gbuffer	   = global_resource_buffer[opaque_gbuffer_srv_id];
	texture_2d<float2>	 motion_buffer = global_resource_buffer[motion_buffer_srv_id];

	const float2 motion = is_thread_valid
							? denoise::sample_motion(motion_buffer, depth_buffer, px, extent)
							: zero<float2>();

	const float px_depth = is_thread_valid
							 ? depth_buffer[px]
							 : 0.f;

	if (px_depth == 0.f)
	{
		if (is_thread_valid)
		{
			gi_resolve_curr_buffer[px]	   = zero<float3>();
			gi_resolve_age_curr_buffer[px] = GIBS_GI_MAX_AGE;
		}

		is_thread_valid = false;
	}

	float gi_weight_sum = 0.f;

	if (is_thread_valid)
	{
		const float3 px_normal = decode_oct_snorm16(gbuffer[px].y);
		const float	 px_z_lin  = calc_linear_z_reversed(cam_near_z, cam_far_z, px_depth);

		const denoise::reproject_taps rp_taps = denoise::reproject_taps::init(opaque_geo_prev_buffer, px, extent, motion, px_depth, px_normal, px_z_lin);

		if (rp_taps.is_prev_valid)
		{
			float3 gi_sum	  = zero<float3>();
			float  gi_age_sum = 0.f;

			expand_all()

			for (uint32 i = 0; i < 4; ++i)
			{
				if (rp_taps.w[i] <= 0.f) { continue; }

				gi_sum		  += gi_resolve_prev_buffer[rp_taps.px[i]] * rp_taps.w[i];
				gi_age_sum	  += float(gi_resolve_age_prev_buffer[rp_taps.px[i]]) * rp_taps.w[i];
				gi_weight_sum += gi_resolve_weight_buffer[rp_taps.px[i]] * rp_taps.w[i];
			}

			if (rp_taps.w_sum > 0.f)
			{
				gi_resolve_curr_buffer[px] = gi_sum / rp_taps.w_sum;

				const float	 gi_age_avg	  = gi_age_sum / rp_taps.w_sum + 1.f;
				const float	 gi_age_res	  = rp_taps.quality < 1.f ? gi_age_avg * sqrt(rp_taps.quality) : gi_age_avg;
				const uint16 gi_age_res_i = clamp(uint16(round(gi_age_res)), uint16(1), uint16(GIBS_GI_MAX_AGE));

				gi_resolve_age_curr_buffer[px] = gi_age_res_i;

				gi_weight_sum /= rp_taps.w_sum;
			}
			else
			{
				gi_resolve_curr_buffer[px]	   = gi_resolve_prev_buffer[rp_taps.px_prev];
				gi_resolve_age_curr_buffer[px] = 0u;
			}
		}
		else
		{
			gi_resolve_age_curr_buffer[px] = 0u;

			const float4 gi_irradiance_sum = gibs_gather_neighbor_gi(gi_resolve_prev_buffer, opaque_geo_prev_buffer, px, px_z_lin, px_normal, extent);

			gi_resolve_curr_buffer[px] = gi_irradiance_sum.w > 0.f
										   ? gi_irradiance_sum.xyz / gi_irradiance_sum.w
										   : gi_resolve_prev_buffer[px];
		}
	}

	const float gi_weight = gi_weight_sum;

	const uint32 wave_idx = group_thread_id / AGE_WAVE_SIZE;

	static_assert((GIBS_GI_RESOLVE_BLOCK_SIZE * GIBS_GI_RESOLVE_BLOCK_SIZE) < 0xffff);

	uint32 gi_weight_packed_min = is_thread_valid
									? (as_uint(f32tof16(gi_weight)) << 16u) | uint16(group_thread_id)
									: 0xffffffffu;

	for (uint32 i = 0; i < GIBS_GI_RESOLVE_SAMPLE_PER_BLOCK; ++i)
	{
		const uint32 gi_weight_packed_wave_min = wave_active_min(gi_weight_packed_min);

		if (gi_weight_packed_wave_min == gi_weight_packed_min)
		{
			gs_scratch[wave_idx][i] = gi_weight_packed_wave_min;

			gi_weight_packed_min = 0xffffffffu;
		}
	}

	group_memory_barrier_with_sync();

	static_assert(wave_count * GIBS_GI_RESOLVE_SAMPLE_PER_BLOCK <= AGE_WAVE_SIZE);

	if (wave_idx > 0) { return; }

	const uint32   wave_lane_idx = wave_lane_index();
	const uint32_2 gs_idx		 = uint32_2(wave_lane_idx % GIBS_GI_RESOLVE_SAMPLE_PER_BLOCK, wave_lane_idx / GIBS_GI_RESOLVE_SAMPLE_PER_BLOCK);

	gi_weight_packed_min = 0xffffffffu;

	if (all(gs_idx < uint32_2(GIBS_GI_RESOLVE_SAMPLE_PER_BLOCK, wave_count)))
	{
		gi_weight_packed_min = gs_scratch[gs_idx.y][gs_idx.x];
	}

	uint32 gi_weight_packed_group_min_k = 0xffffffffu;

	for (uint32 i = 0; i < GIBS_GI_RESOLVE_SAMPLE_PER_BLOCK; ++i)
	{
		const uint32 gi_weight_packed_wave_min = wave_active_min(gi_weight_packed_min);

		if (wave_lane_index() == i) { gi_weight_packed_group_min_k = gi_weight_packed_wave_min; }

		if (gi_weight_packed_wave_min == gi_weight_packed_min)
		{
			gi_weight_packed_min = 0xffffffffu;
		}
	}

	if (wave_lane_index() < GIBS_GI_RESOLVE_SAMPLE_PER_BLOCK)
	{
		const uint32_2 block_idx = group_id.xy;
		const uint32   block_id	 = block_idx.y * data.tile_count_w * GIBS_GI_RESOLVE_BLOCK_DIM
								 + block_idx.x;

		const uint32 sample_id = block_id * GIBS_GI_RESOLVE_SAMPLE_PER_BLOCK
							   + wave_lane_index();

		rw_byte_array<uint16> sample_pos_arr = gibs_load_gi_resolve_sample_pos_rw_arr(data);

		const uint16 sample_local_pos_lin = uint32_lower_to_uint16(gi_weight_packed_group_min_k);

		sample_pos_arr.store(sample_id, sample_local_pos_lin);
	}
}
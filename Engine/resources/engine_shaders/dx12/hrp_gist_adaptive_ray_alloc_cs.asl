#include "hrp_common.asli"

groupshared uint32 gs_scratch[GIST_ADAPTIVE_RAY_TILE_SIZE * GIST_ADAPTIVE_RAY_TILE_SIZE / AGE_WAVE_SIZE];

wave_size(AGE_WAVE_SIZE)
[numthreads(GIST_ADAPTIVE_RAY_TILE_SIZE * GIST_ADAPTIVE_RAY_TILE_SIZE, 1, 1)] void
main_cs(uint32_3 group_idx sv_group_id,
		uint32 thread_id   sv_group_thread_id)

{
	static_assert(AGE_WAVE_SIZE <= 64);	   // nvidia : 32, amd : 64
	static_assert(GIST_ADAPTIVE_RAY_TILE_SIZE * GIST_ADAPTIVE_RAY_TILE_SIZE >= AGE_WAVE_SIZE);

	const gist_data data = gist::load_data();

	const int32_2 extent = int32_2(backbuffer_size);
	const int32_2 px	 = int32_2(group_idx.xy * GIST_ADAPTIVE_RAY_TILE_SIZE)
						 + int32_2(thread_id % GIST_ADAPTIVE_RAY_TILE_SIZE, thread_id / GIST_ADAPTIVE_RAY_TILE_SIZE);


	bool is_thread_valid = true;

	if (any(px >= extent)) { is_thread_valid = false; }

	texture_2d<uint32> gi_resolve_age_buffer	 = global_resource_buffer[data.h_gi_resolve_age_curr_buffer_srv_id];
	texture_2d<float2> gi_resolve_moments_buffer = global_resource_buffer[data.h_gi_resolve_moments_curr_buffer_srv_id];

	texture_2d<float> depth_buffer = global_resource_buffer[opaque_depth_buffer_srv_id];

	texture_2d<float2> mr_buffer = global_resource_buffer[opaque_mr_buffer_srv_id];

	const float px_depth = is_thread_valid ? depth_buffer[px] : 0.f;

	if (px_depth == 0.f)
	{
		is_thread_valid = false;
	}

	const uint32 age	 = is_thread_valid ? gi_resolve_age_buffer[px] : 0u;
	const float2 moments = is_thread_valid ? gi_resolve_moments_buffer[px] : zero<float2>();
	const float	 var	 = moments.y - moments.x * moments.x;

	const float	 roughness = is_thread_valid ? mr_buffer[px].g : 1.f;
	const float3 rng	   = random_pcg3d(uint32_3(px.x, px.y, frame_index + g::shader_hash));

	const bool is_new_born = is_thread_valid and age < 4u;
	const bool is_specular = is_thread_valid
						 and roughness < gist::specular_roughness_max(data)
						 and rng.x < data.specular_rpp;
	const bool is_variance = is_thread_valid
						 and age >= 4u
						 and var > 16.f * (moments.x + 0.1f) * (moments.x + 0.1f);	  // todo, add auto exposure

	const uint32 new_born_count_wave_sum = wave_active_count_bits(is_new_born);
	const uint32 specular_count_wave_sum = wave_active_count_bits(is_specular);
	const uint32 variance_count_wave_sum = wave_active_count_bits(is_variance);

	const uint32 wave_id = thread_id / AGE_WAVE_SIZE;

	if (wave_lane_index() == AGE_WAVE_SIZE - 1)
	{
		gs_scratch[wave_id] = (new_born_count_wave_sum << 0u)
							| (specular_count_wave_sum << 10u)
							| (variance_count_wave_sum << 20u);

		static_assert((GIST_ADAPTIVE_RAY_TILE_SIZE * GIST_ADAPTIVE_RAY_TILE_SIZE) <= (1u << 10u));
	}

	group_memory_barrier_with_sync();

	uint32 wave_sum_packed = 0u;
	if (wave_lane_index() < GIST_ADAPTIVE_RAY_TILE_SIZE * GIST_ADAPTIVE_RAY_TILE_SIZE / AGE_WAVE_SIZE)
	{
		wave_sum_packed = gs_scratch[wave_lane_index()];
	}

	const uint32 group_sum_packed = wave_active_sum(wave_sum_packed);

	const uint32 new_born_count_group_sum = (group_sum_packed >> 0u) & ((1u << 10u) - 1);
	const uint32 specular_count_group_sum = (group_sum_packed >> 10u) & ((1u << 10u) - 1);
	const uint32 variance_count_group_sum = (group_sum_packed >> 20u) & ((1u << 10u) - 1);

	if (thread_id == 0u)
	{
		gist::adaptive::alloc_ray_ideal(data, new_born_count_group_sum, specular_count_group_sum, variance_count_group_sum);
	}

	if (all(px < extent))
	{
		rw_texture_2d<uint32> adaptive_ray_type_buffer = global_resource_buffer[data.h_adaptive_ray_type_buffer_uav_id];
		adaptive_ray_type_buffer[px]				   = ((is_new_born ? 1u : 0u) << GIST_ADAPTIVE_RAY_TYPE_NEW_BORN)
													   | ((is_specular ? 1u : 0u) << GIST_ADAPTIVE_RAY_TYPE_SPECULAR)
													   | ((is_variance ? 1u : 0u) << GIST_ADAPTIVE_RAY_TYPE_VARIANCE);
	}
}
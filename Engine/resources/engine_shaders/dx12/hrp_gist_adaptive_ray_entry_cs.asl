#include "hrp_common.asli"

groupshared uint32 gs_scratch[GIST_ADAPTIVE_RAY_TILE_SIZE * GIST_ADAPTIVE_RAY_TILE_SIZE / AGE_WAVE_SIZE];

groupshared uint32 gs_ray_entry_offset_diffuse;
groupshared uint32 gs_ray_entry_offset_specular;

wave_size(AGE_WAVE_SIZE)
[numthreads(GIST_ADAPTIVE_RAY_TILE_SIZE * GIST_ADAPTIVE_RAY_TILE_SIZE, 1, 1)] void
main_cs(uint32_3 group_idx sv_group_id,
		uint32 thread_id   sv_group_thread_id)

{
	static_assert(AGE_WAVE_SIZE <= 64);	   // nvidia : 32, amd : 64
	static_assert(GIST_ADAPTIVE_RAY_TILE_SIZE * GIST_ADAPTIVE_RAY_TILE_SIZE >= AGE_WAVE_SIZE);
	static_assert(GIST_ADAPTIVE_RAY_TILE_SIZE * GIST_ADAPTIVE_RAY_TILE_SIZE <= 0xffff);

	const gist_data data = gist::load_data();

	const int32_2 extent = int32_2(backbuffer_size);
	const int32_2 px	 = int32_2(group_idx.xy * GIST_ADAPTIVE_RAY_TILE_SIZE)
						 + int32_2(thread_id % GIST_ADAPTIVE_RAY_TILE_SIZE, thread_id / GIST_ADAPTIVE_RAY_TILE_SIZE);

	uint32 mask = 0u;
	if (all(px < extent))
	{
		texture_2d<uint32> adaptive_ray_type_buffer = global_resource_buffer[data.h_adaptive_ray_type_buffer_srv_id];
		mask										= adaptive_ray_type_buffer[px];
	}

	const bool is_new_born = util::is_mask_set<GIST_ADAPTIVE_RAY_TYPE_NEW_BORN>(mask);
	const bool is_specular = util::is_mask_set<GIST_ADAPTIVE_RAY_TYPE_SPECULAR>(mask);
	const bool is_variance = util::is_mask_set<GIST_ADAPTIVE_RAY_TYPE_VARIANCE>(mask);

	const float3 prob = gist::adaptive::load_ray_entry_prob(data);
	const float3 rng  = random_pcg3d(uint32_3(px.x, px.y, frame_index + 97531u));

	uint32 diffuse_count = 0u;
	uint32 diffuse_type	 = 0u;
	uint32 spec_count	 = 0u;

	diffuse_type = is_new_born ? GIST_ADAPTIVE_RAY_TYPE_NEW_BORN : GIST_ADAPTIVE_RAY_TYPE_VARIANCE;

	if ((is_new_born or is_variance) and rng.y < prob[diffuse_type])
	{
		diffuse_count = 1u;
	}

	// todo, (1/spec_period) * roughness
	if (is_specular and rng.x < prob[GIST_ADAPTIVE_RAY_TYPE_SPECULAR])
	{
		spec_count = 1u;
	}

	const uint32 wave_id = thread_id / AGE_WAVE_SIZE;

	const uint32 packed_count  = diffuse_count | (spec_count << 16u);
	const uint32 packed_prefix = wave_prefix_sum(packed_count);

	if (wave_lane_index() == AGE_WAVE_SIZE - 1)
	{
		gs_scratch[wave_id] = packed_prefix + packed_count;
	}
	group_memory_barrier_with_sync();

	static_assert(GIST_ADAPTIVE_RAY_TILE_SIZE * GIST_ADAPTIVE_RAY_TILE_SIZE / AGE_WAVE_SIZE <= AGE_WAVE_SIZE);

	uint32 packed_wave_sum = 0u;
	if (wave_lane_index() < GIST_ADAPTIVE_RAY_TILE_SIZE * GIST_ADAPTIVE_RAY_TILE_SIZE / AGE_WAVE_SIZE)
	{
		packed_wave_sum = gs_scratch[wave_lane_index()];
	}

	const uint32 packed_wave_offset = wave_read_lane_at(wave_prefix_sum(packed_wave_sum), wave_id);

	if (wave_id == GIST_ADAPTIVE_RAY_TILE_SIZE * GIST_ADAPTIVE_RAY_TILE_SIZE / AGE_WAVE_SIZE - 1
		and wave_lane_index() == AGE_WAVE_SIZE - 1)
	{
		const uint32 packed_total	 = packed_wave_offset + packed_prefix + packed_count;
		gs_ray_entry_offset_diffuse	 = gist::adaptive::alloc_ray_entry_diffuse(data, packed_total & 0xffffu);
		gs_ray_entry_offset_specular = gist::adaptive::alloc_ray_entry_specular(data, packed_total >> 16u);
	}
	group_memory_barrier_with_sync();

	const uint32 packed_offset = packed_wave_offset + packed_prefix;

	const uint32_2 ray_entry_cap = gist::adaptive::load_ray_entry_cap<true>(data);
	const uint32   diffuse_cap	 = ray_entry_cap.x;
	const uint32   specular_cap	 = ray_entry_cap.y;

	if (diffuse_count != 0u)
	{
		const uint32 idx = gs_ray_entry_offset_diffuse + (packed_offset & 0xffffu);
		if (idx < diffuse_cap)
		{
			gist::adaptive::ray_entry_rw_arr(data).store(idx, gist::adaptive::pack_ray_entry(px, diffuse_type));
		}
	}

	if (spec_count != 0u)
	{
		const uint32 idx = gs_ray_entry_offset_specular + (packed_offset >> 16u);
		if (idx < specular_cap)
		{
			gist::adaptive::ray_entry_rw_arr(data).store(diffuse_cap + idx, gist::adaptive::pack_ray_entry(px, GIST_ADAPTIVE_RAY_TYPE_SPECULAR));
		}
	}
}
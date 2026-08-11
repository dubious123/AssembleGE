#include "hrp_common.asli"

groupshared uint32 gs_luminance_sum[GIST_ATLAS_TILE_SIZE * GIST_ATLAS_TILE_SIZE];
groupshared uint32 gs_count[GIST_ATLAS_TILE_SIZE * GIST_ATLAS_TILE_SIZE];

#define TPG 64

#define GIST_PX_LUMINANCE_CLAMP 1024.f
#define GIST_PX_LUMINANCE_SCALE 256.f

wave_size(AGE_WAVE_SIZE)
[numthreads(TPG, 1, 1)] void
main_cs(uint32 group_id	 sv_group_id,
		uint32 thread_id sv_group_thread_id)

{
	static_assert(GIST_ATLAS_TILE_SIZE * GIST_ATLAS_TILE_SIZE <= TPG);
	static_assert(uint32(GIST_PX_LUMINANCE_CLAMP * GIST_PX_LUMINANCE_SCALE * GIST_PX_LUMINANCE_TILE_SIZE * GIST_PX_LUMINANCE_TILE_SIZE) <= 0xffffffff);

	const gist_data data = gist::load_data();

	if (thread_id < data.atlas_texel_count())
	{
		gs_luminance_sum[thread_id] = 0u;
		gs_count[thread_id]			= 0u;
	}

	group_memory_barrier_with_sync();

	structured_buffer<gist_ray_hit_result>		ray_hit_result_buffer	   = global_resource_buffer[data.h_ray_hit_buffer_srv_id];
	structured_buffer<gist_ray_lighting_result> ray_lighting_result_buffer = global_resource_buffer[data.h_ray_lighting_buffer_srv_id];

	const int32_2 px_luminance_extent = ceil(int32_2(backbuffer_size), GIST_PX_LUMINANCE_TILE_SIZE);
	const int32_2 px_offset			  = int32_2(group_id % px_luminance_extent.x, group_id / px_luminance_extent.x)
									  * GIST_PX_LUMINANCE_TILE_SIZE;

	const uint32   tile_size	 = data.tile_size();
	const uint32   tile_per_axis = GIST_PX_LUMINANCE_TILE_SIZE / tile_size;
	const uint32   tile_count	 = tile_per_axis * tile_per_axis;
	const uint32_2 tile_idx_base = uint32_2(px_offset / tile_size);

	// threads stride the region's tiles
	for (uint32 i = thread_id; i < tile_count; i += TPG)
	{
		const uint32_2 tile_idx = tile_idx_base + uint32_2(i % tile_per_axis, i / tile_per_axis);

		if (tile_idx.x >= data.tile_count_w or tile_idx.y >= data.tile_count_h) { continue; }

		const uint32			  tile_id = gist::tile::calc_id(data, tile_idx);
		const gist_ray_hit_result ray_hit = ray_hit_result_buffer[tile_id];

		// invalid px, sky...
		if (ray_hit.pdf == 0.f) { continue; }

		// opaque back face
		if (ray_hit.distance < 0.f and ray_hit.object_id == invalid_id_uint32) { continue; }

		const float3 dir_local = decode_world_hemi_oct_snorm8(uint32_lower_to_uint16(ray_hit.dir_oct_snorm8));
		const uint32 idx	   = gist::calc_atlas_tile_local_idx(dir_local);
		const float	 cos_theta = dir_local.y;

		const float3 radiance  = decode_r11g11b10(ray_lighting_result_buffer[tile_id].radiance_r11g11b10);
		const float	 luminance = luminance_rec709(radiance);

		interlocked_add(gs_luminance_sum[idx], uint32(min(luminance * cos_theta, GIST_PX_LUMINANCE_CLAMP) * GIST_PX_LUMINANCE_SCALE));
		interlocked_add(gs_count[idx], 1u);
	}

	group_memory_barrier_with_sync();

	if (thread_id < data.atlas_texel_count() and gs_count[thread_id] > 0u)
	{
		rw_byte_array<half> lum_arr = gist::tile::px_luminance_rw_arr(data, group_id);

		const float luminance_avg = (float(gs_luminance_sum[thread_id]) / GIST_PX_LUMINANCE_SCALE) / float(gs_count[thread_id]);

		lum_arr.store(thread_id, cast<half>(lerp(float(lum_arr[thread_id]), luminance_avg, 0.1f)));
	}
}
#include "forward_plus_common.asli"

groupshared uint32 local_min_idx[Z_SLICE_COUNT];
groupshared uint32 local_max_idx[Z_SLICE_COUNT];

[numthreads(LIGHT_ZBIN_THREAD_COUNT, 1, 1)] void
main_cs(uint32 sorted_id sv_dispatch_thread_id,
		uint32 group_thread_id sv_group_thread_id)

{
	for (uint32 i = group_thread_id; i < Z_SLICE_COUNT; i += LIGHT_ZBIN_THREAD_COUNT)
	{
		local_min_idx[i] = 0xffffffff;
		local_max_idx[i] = 0;
	}

	group_memory_barrier_with_sync();


	const t_unified_light_id light_id = load_sort_value(sorted_id, sort_value_offset(false));


	if (/*light_id < MAX_VISIBLE_LIGHT_COUNT &&*/ light_id != invalid_id_uint32)
	{
		store_visible_light_count_interlocked_add(1);

		const unified_light light = load_unified_light(light_id);

		store_sorted_light(sorted_id, light);

		const float view_z = dot(light.position - camera_pos, camera_forward);
		const float min_z  = view_z - light.range;
		const float max_z  = view_z + light.range;

		const uint32 bin_begin = clamp(depth_to_bin(min_z), 0, Z_SLICE_COUNT - 1);
		const uint32 bin_end   = clamp(depth_to_bin(max_z), 0, Z_SLICE_COUNT - 1);

		for (uint32 j = bin_begin; j <= bin_end; ++j)
		{
			interlocked_min(local_min_idx[j], sorted_id);
			interlocked_max(local_max_idx[j], sorted_id);
		}


		const float3 pos   = light.position;
		const float	 range = light.range;

		const float3 cam_to_light = pos - camera_pos;
		const float	 dist		  = length(cam_to_light);

		const uint32 word_index = sorted_id / 32;
		const uint32 bit		= 1u << (sorted_id % 32);

		uint32_4 tile_aabb = uint32_4(0, light_tile_count_x - 1, 0, light_tile_count_y - 1);

		const float4 light_pos_clip = mul(view_proj, float4(pos, 1));

		if (dist > range && light_pos_clip.w > 0.1f)
		{
			const float2 light_pos_ndc = light_pos_clip.xy / light_pos_clip.w;

			const float projected_radius = range * dist / sqrt(dist * dist - range * range);

			const float4 light_right_clip = mul(view_proj, float4(pos + camera_right * projected_radius, 1));
			const float2 light_right_ndc  = light_right_clip.xy / light_right_clip.w;

			const float light_radius_ndc = length(light_right_ndc - light_pos_ndc);

			const float2 ndc_min = light_pos_ndc - float2(light_radius_ndc, light_radius_ndc);
			const float2 ndc_max = light_pos_ndc + float2(light_radius_ndc, light_radius_ndc);

			const float2 screen_a = ndc_xy_to_screen(ndc_min, backbuffer_size);
			const float2 screen_b = ndc_xy_to_screen(ndc_max, backbuffer_size);

			const float2 screen_min = float2(screen_a.x, screen_b.y);
			const float2 screen_max = float2(screen_b.x, screen_a.y);

			tile_aabb.x = clamp(int(screen_min.x) / LIGHT_TILE_SIZE, 0, int(light_tile_count_x - 1));
			tile_aabb.y = clamp(int(screen_max.x + (LIGHT_TILE_SIZE - 1)) / LIGHT_TILE_SIZE, 0, int(light_tile_count_x - 1));
			tile_aabb.z = clamp(int(screen_min.y) / LIGHT_TILE_SIZE, 0, int(light_tile_count_y - 1));
			tile_aabb.w = clamp(int(screen_max.y + (LIGHT_TILE_SIZE - 1)) / LIGHT_TILE_SIZE, 0, int(light_tile_count_y - 1));
		}

		uint32 packed_aabb = (tile_aabb.x << 24) | (tile_aabb.y << 16) | (tile_aabb.z << 8) | tile_aabb.w;

		store_packed_aabb(sorted_id, packed_aabb);
	}
	group_memory_barrier_with_sync();

	for (uint32 i = group_thread_id; i < Z_SLICE_COUNT; i += LIGHT_ZBIN_THREAD_COUNT)
	{
		if (local_min_idx[i] != 0xffffffff)
		{
			store_zbin_entry_interlocked_min(i, local_min_idx[i]);
			store_zbin_entry_interlocked_max(i, local_max_idx[i]);
		}
	}
}
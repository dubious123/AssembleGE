#include "forward_plus_common.asli"

groupshared uint32 local_min_idx[Z_SLICE_COUNT];
groupshared uint32 local_max_idx[Z_SLICE_COUNT];

[numthreads(LIGHT_ZBIN_THREAD_COUNT, 1, 1)] void
main_cs(uint32 sorted_id	   sv_dispatch_thread_id,
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

		const uint32 word_index = sorted_id / 32;
		const uint32 bit		= 1u << (sorted_id % 32);

		uint32_4 tile_aabb = uint32_4(0, light_tile_count_x - 1, 0, light_tile_count_y - 1);

		// view space center
		const float3 center = mul(view, float4(light.position, 1)).xyz;

		const float2 bound_x = project_sphere_axis(center.x, center.z, light.range, cam_near_z, proj_00);
		const float2 bound_y = project_sphere_axis(center.y, center.z, light.range, cam_near_z, proj_11);

		const float2 ndc_min = float2(bound_x.x, bound_y.x);
		const float2 ndc_max = float2(bound_x.y, bound_y.y);

		const float2 screen_a = ndc_xy_to_screen(ndc_min, backbuffer_size);
		const float2 screen_b = ndc_xy_to_screen(ndc_max, backbuffer_size);

		const float2 screen_min = float2(screen_a.x, screen_b.y);
		const float2 screen_max = float2(screen_b.x, screen_a.y);

		tile_aabb.x = clamp(int(screen_min.x) / LIGHT_TILE_SIZE, 0, int(light_tile_count_x - 1));
		tile_aabb.y = clamp(int(screen_max.x + (LIGHT_TILE_SIZE - 1)) / LIGHT_TILE_SIZE, 0, int(light_tile_count_x - 1));
		tile_aabb.z = clamp(int(screen_min.y) / LIGHT_TILE_SIZE, 0, int(light_tile_count_y - 1));
		tile_aabb.w = clamp(int(screen_max.y + (LIGHT_TILE_SIZE - 1)) / LIGHT_TILE_SIZE, 0, int(light_tile_count_y - 1));

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
#include "forward_plus_common.asli"

groupshared uint32 shared_bitmask[LIGHT_BITMASK_UINT32_COUNT];

[numthreads(SORT_THREAD_COUNT, 1, 1)] void
main_cs(uint32_3 group_id sv_group_id,
		uint32	 group_thread_id sv_group_thread_id) {
	const uint32 tile_x	 = group_id.x;
	const uint32 tile_y	 = group_id.y;
	const uint32 tile_id = tile_y * light_tile_count_x + tile_x;

	expand(LIGHT_BITMASK_UINT32_COUNT / SORT_THREAD_COUNT)

	for (uint32 i = group_thread_id; i < LIGHT_BITMASK_UINT32_COUNT; i += SORT_THREAD_COUNT)
	{
		shared_bitmask[i] = 0;
	}

	group_memory_barrier_with_sync();

	const uint32 visible_count = load_visible_light_count();
	for (uint32 sorted_id = group_thread_id; sorted_id < visible_count; sorted_id += SORT_THREAD_COUNT)
	{
		uint32 packed_aabb = load_packed_aabb(sorted_id);

		uint32 tile_min_x = (packed_aabb >> 24) & 0xff;
		uint32 tile_max_x = (packed_aabb >> 16) & 0xff;
		uint32 tile_min_y = (packed_aabb >> 8) & 0xff;
		uint32 tile_max_y = (packed_aabb) & 0xff;

		const uint32 word_index = sorted_id / 32;
		const uint32 bit		= 1u << (sorted_id % 32);

		if (tile_x >= tile_min_x
			&& tile_x <= tile_max_x
			&& tile_y >= tile_min_y
			&& tile_y <= tile_max_y)
		{
			interlocked_or(shared_bitmask[word_index], bit);
		}
	}

	group_memory_barrier_with_sync();

	expand(LIGHT_BITMASK_UINT32_COUNT / SORT_THREAD_COUNT)

	for (uint32 j = group_thread_id; j < LIGHT_BITMASK_UINT32_COUNT; j += SORT_THREAD_COUNT)
	{
		store_tile_mask(tile_id, j, shared_bitmask[j]);
	}
}
#include "forward_plus_common.asli"

[numthreads(256, 1, 1)] void
main_cs(uint32 thread_id sv_dispatch_thread_id)

{
	if (thread_id < Z_SLICE_COUNT)
	{
		zbin_entry zbin;
		zbin.max_idx = 0;
		zbin.min_idx = 0xffffffff;

		store_zbin_entry(thread_id, zbin);
	}

	// if (thread_id < light_tile_count_x * light_tile_count_y * LIGHT_BITMASK_UINT32_COUNT)
	{
		store_tile_mask_flat(thread_id, 0);
	}

	if (thread_id == 0)
	{
		store_visible_light_count(0);
	}

	// culled_light_buffer[thread_id] = invalid_id_uint32;
}
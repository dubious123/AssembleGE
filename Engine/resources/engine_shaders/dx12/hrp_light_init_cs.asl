#include "hrp_common.asli"

[numthreads(LIGHT_BIN_INIT_THREAD_COUNT, 1, 1)] void
main_cs(uint32 thread_id sv_dispatch_thread_id)

{
	static const uint32 total_thread = (X_SLICE_COUNT + Y_SLICE_COUNT + Z_SLICE_COUNT) * (1 + LIGHT_BITMASK_UINT32_COUNT);
	if (thread_id >= total_thread) { return; }

	const uint32 slice_idx = thread_id;
	zbin_entry	 zbin;
	zbin.max_idx = 0;
	zbin.min_idx = 0xffffffff;
	if (slice_idx < X_SLICE_COUNT)
	{
		store_bin_entry_x(slice_idx, zbin);
	}
	else if (slice_idx < X_SLICE_COUNT + Y_SLICE_COUNT)
	{
		store_bin_entry_y(slice_idx - X_SLICE_COUNT, zbin);
	}
	else if (slice_idx < X_SLICE_COUNT + Y_SLICE_COUNT + Z_SLICE_COUNT)
	{
		store_bin_entry_z(slice_idx - (X_SLICE_COUNT + Y_SLICE_COUNT), zbin);
	}
	else
	{
		uint32 idx = thread_id - (X_SLICE_COUNT + Y_SLICE_COUNT + Z_SLICE_COUNT);
		store_light_bin_mask_flat(idx, 0);
	}
}
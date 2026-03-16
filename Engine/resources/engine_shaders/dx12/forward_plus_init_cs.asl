#include "forward_plus_common.asli"

[numthreads(256, 1, 1)] void
main_cs(uint32 thread_id sv_dispatch_thread_id)

{
	if (thread_id < Z_SLICE_COUNT)
	{
		zbin_buffer_uav[thread_id].min_idx = 0xffffffff;
		zbin_buffer_uav[thread_id].max_idx = 0;
	}

	if (thread_id < light_tile_count_x * light_tile_count_y * LIGHT_BITMASK_UINT32_COUNT)
	{
		tile_mask_buffer_uav[thread_id] = 0;
	}

	if (thread_id == 0)
	{
		frame_data_rw_buffer_uav[0].not_culled_light_count				= 0;
		frame_data_rw_buffer_uav[0].generic_counter						= 0;
		frame_data_rw_buffer_uav[0].z_min								= 0xffffffff;
		frame_data_rw_buffer_uav[0].z_max								= 0;
		frame_data_rw_buffer_uav[0].not_culled_transparent_object_count = 0;

		debug_buffer[0].invalid_count = 0;
		debug_buffer[0].visible_count = 0;
	}

	// culled_light_buffer[thread_id] = invalid_id_uint32;
}
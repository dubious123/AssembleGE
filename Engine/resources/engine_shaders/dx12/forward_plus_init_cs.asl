#include "forward_plus_common.asli"

[numthreads(1, 1, 1)] void
main_cs(uint32 thread_id sv_dispatch_thread_id)

{
	frame_data_rw_buffer_uav[0].generic_counter						= 0;
	frame_data_rw_buffer_uav[0].z_min								= 0xffffffff;
	frame_data_rw_buffer_uav[0].z_max								= 0;
	frame_data_rw_buffer_uav[0].not_culled_transparent_object_count = 0;

	debug_buffer[0].invalid_count = 0;
	debug_buffer[0].visible_count = 0;

	// culled_light_buffer[thread_id] = invalid_id_uint32;
}
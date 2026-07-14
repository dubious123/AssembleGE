#include "hrp_common.asli"

[numthreads(16, 16, 1)] void
main_cs(uint32_3 thread_id sv_dispatch_thread_id)

{
	const int32_2 px = thread_id.xy;

	if (any(px >= int32_2(backbuffer_size))) { return; }

	texture_2d<float>	 depth_buffer = global_resource_buffer[opaque_depth_buffer_srv_id];
	texture_2d<uint32_2> gbuffer	  = global_resource_buffer[opaque_gbuffer_srv_id];

	const float z_depth = depth_buffer[px];

	const uint32 normal_snorm16 = gbuffer[px].y;

	rw_texture_2d<uint32_2> opaque_geo_prev_buffer = global_resource_buffer[opaque_geo_prev_buffer_uav_id];

	opaque_geo_prev_buffer[px] = uint32_2(as_uint(z_depth), normal_snorm16);
}

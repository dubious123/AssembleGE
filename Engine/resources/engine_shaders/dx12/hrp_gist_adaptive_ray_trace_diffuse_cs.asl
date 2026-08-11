#include "hrp_common.asli"

wave_size(AGE_WAVE_SIZE)
[numthreads(64, 1, 1)] void
main_cs(uint32 thread_id sv_dispatch_thread_id)

{
	const gist_data data = gist::load_data();

	if (thread_id >= gist::adaptive::diffuse_ray_count_total(data)) { return; }


	rw_structured_buffer<gist_ray_hit_result> ray_hit_buffer = global_resource_buffer[data.h_ray_hit_buffer_uav_id];

	uint32		  type = 0u;
	const int32_2 px   = gist::adaptive::unpack_ray_entry(gist::adaptive::ray_entry_arr(data)[thread_id], type);

	texture_2d<float> depth_buffer = global_resource_buffer[opaque_depth_buffer_srv_id];
	const float		  z_depth	   = depth_buffer[px];

	assert(z_depth != 0.f and (type == GIST_ADAPTIVE_RAY_TYPE_NEW_BORN or type == GIST_ADAPTIVE_RAY_TYPE_VARIANCE));

	ray_hit_buffer[gist::adaptive::ray_hit_diffuse_id(data, thread_id)] = gist::px_trace_ray_diffuse(data, px, z_depth, random_pcg4d(uint32_4(px.x, px.y, frame_index, g::shader_hash)));
}
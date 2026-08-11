#include "hrp_common.asli"

wave_size(AGE_WAVE_SIZE)
[numthreads(16, 16, 1)] void
main_cs(uint32_3 tile_idx sv_dispatch_thread_id)

{
	const gist_data data = gist::load_data();

	if (tile_idx.x >= data.tile_count_w or tile_idx.y >= data.tile_count_h) { return; }

	texture_2d<float>						  depth_buffer	 = global_resource_buffer[opaque_depth_buffer_srv_id];
	texture_2d<uint32_2>					  gbuffer		 = global_resource_buffer[opaque_gbuffer_srv_id];
	rw_structured_buffer<gist_ray_hit_result> ray_hit_buffer = global_resource_buffer[data.h_ray_hit_buffer_uav_id];

	const uint32  tile_id = gist::tile::calc_id(data, tile_idx.xy);
	const int32_2 px	  = gist::tile::calc_px(data, tile_idx.xy, tile_id);

	gist_ray_hit_result res = zero<gist_ray_hit_result>();
	res.object_id			= invalid_id_uint32;
	res.distance			= float_max;

	const float z_depth = depth_buffer[px];

	if (z_depth != 0.f)
	{
		res = gist::px_trace_ray_diffuse(data, px, z_depth, random_pcg4d(uint32_4(px.x, px.y, frame_index, g::shader_hash)));
	}

	ray_hit_buffer[tile_id] = res;
}
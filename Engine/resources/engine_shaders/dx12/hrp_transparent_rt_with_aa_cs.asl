#include "hrp_common.asli"

[numthreads(8, 8, 1)] void
main_cs(uint32_3 dispatch_thread_id sv_dispatch_thread_id)

{
	if (dispatch_thread_id.x >= (uint32)backbuffer_size.x || dispatch_thread_id.y >= (uint32)backbuffer_size.y)
	{
		return;
	}

	const int32_2 px = dispatch_thread_id.xy;

	const texture_2d<float> opaque_depth_tex	  = global_resource_buffer[opaque_depth_buffer_srv_id];
	const texture_2d<float> transparent_depth_tex = global_resource_buffer[transparent_depth_buffer_srv_id];

	const float opaque_z_depth		= opaque_depth_tex[px];
	const float transparent_z_depth = transparent_depth_tex[px];

	if (transparent_z_depth == 0.f) { return; }

	const float3 world_far = ndc_to_world(view_proj_inv, screen_px_to_ndc(px, opaque_z_depth, inv_backbuffer_size));

	const float3 rel	 = world_far - camera_pos;
	const float	 t_max	 = length(rel);
	const float3 ray_dir = rel / t_max;

	ray_query<RAY_FLAG_CULL_BACK_FACING_TRIANGLES> query;

	ray_desc desc;
	desc.Origin	   = camera_pos;
	desc.Direction = ray_dir;
	desc.TMin	   = 0.f;
	desc.TMax	   = t_max;

	const float4 res = rt_calc_transparent_color(rt_arg::init_gibs(true), desc, query);

	rw_texture_2d<float4> res_tex  = global_resource_buffer[blend_buffer_uav_id];
	res_tex[dispatch_thread_id.xy] = res;
}

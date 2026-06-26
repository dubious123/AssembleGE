#include "hrp_common.asli"

[numthreads(8, 8, 1)] void
main_cs(uint32_3 dispatch_thread_id sv_dispatch_thread_id)

{
	if (dispatch_thread_id.x >= (uint32)backbuffer_size.x || dispatch_thread_id.y >= (uint32)backbuffer_size.y)
	{
		return;
	}

	const texture_2d<float> depth_tex = global_resource_buffer[depth_buffer_texture_id];
	const float				z_depth	  = load(depth_tex, dispatch_thread_id.x, dispatch_thread_id.y, 0);

	const float2 screen_pos = float2(dispatch_thread_id.x + .5f, dispatch_thread_id.y + .5f);

	const float2 ndc = screen_to_ndc(screen_pos, inv_backbuffer_size);

	float4 world_far  = mul(view_proj_inv, float4(ndc, z_depth, 1.0));
	world_far.xyz	 /= world_far.w;

	const float3 ray_dir = normalize(world_far.xyz - camera_pos);
	const float	 t_max	 = length(world_far.xyz - camera_pos);

	ray_query<RAY_FLAG_CULL_BACK_FACING_TRIANGLES> query;

	ray_desc desc;
	desc.Origin	   = camera_pos;
	desc.Direction = ray_dir;
	desc.TMin	   = 0.001;
	desc.TMax	   = t_max;

	const float4 res = rt_calc_transparent_color(rt_arg::init_gibs(true), desc, query);

	rw_texture_2d<float4> res_tex = global_resource_buffer[rt_transparent_buffer_uav_texture_id];

	res_tex[dispatch_thread_id.xy] = res;
}
#include "hrp_common.asli"

[numthreads(32, 1, 1)] void
main_cs(uint32 dispatch_thread_id sv_dispatch_thread_id,
		uint32 group_thread_id	  sv_group_thread_id)

{
	const aa_data data = load_aa_data();

	byte_address_buffer ray_buffer = global_resource_buffer[data.h_ray_buffer_srv_id];

	const uint32_2 scratch				 = load<uint32_2>(ray_buffer, sizeof(uint16_2) * data.px_headroom + sizeof(uint32_2));
	const uint32   transparent_rpp		 = scratch.x;
	const uint32   transparent_ray_count = scratch.y;

	if (dispatch_thread_id >= transparent_ray_count)
	{
		return;
	}

	// rpp is pow of 2
	// rpp < 32

	// entry_id = tid / rpp
	const uint32 entry_id = dispatch_thread_id >> first_bit_high(transparent_rpp);

	// local_id = tid % rpp
	const uint16 local_id = cast<uint16>(dispatch_thread_id & (transparent_rpp - 1u));

	const uint16_2 px = load<uint16_2>(ray_buffer, sizeof(uint16_2) * entry_id);

	const texture_2d<float> opaque_depth_tex	  = global_resource_buffer[opaque_depth_buffer_srv_id];
	const texture_2d<float> transparent_depth_tex = global_resource_buffer[transparent_depth_buffer_srv_id];
	const float				opaque_z_depth		  = opaque_depth_tex[px];
	const float				transparent_z_depth	  = transparent_depth_tex[px];

	// px + 0.5 + rng( [0, 1] ) - 0.5
	const float2 screen_pos = px + random_r2_sequence(1 + local_id);

	const float3 world_far = ndc_to_world(view_proj_inv, screen_to_ndc(screen_pos, opaque_z_depth, inv_backbuffer_size));

	const float	 t_max	 = length(world_far - camera_pos);
	const float3 ray_dir = (world_far - camera_pos) / t_max;

	ray_query<RAY_FLAG_CULL_BACK_FACING_TRIANGLES> query;

	ray_desc desc;
	desc.Origin	   = camera_pos;
	desc.Direction = ray_dir;
	desc.TMin	   = 0.f;
	desc.TMax	   = t_max;

	const float4 col = rt_calc_transparent_color(rt_arg::init_gibs(true), desc, query);

	const float4 col_prefix = wave_prefix_sum(col);

	// if (local_id == transparent_rpp - 1)
	//{
	const float4 col_prefix_prev = group_thread_id >= transparent_rpp ? wave_read_lane_at(col_prefix, group_thread_id - transparent_rpp + 1) : zero<float4>();
	const float4 col_sum		 = col_prefix - col_prefix_prev + col;


	if (local_id == transparent_rpp - 1)
	{
		rw_texture_2d<float4> res_tex = global_resource_buffer[blend_buffer_uav_id];

		const float4 center_color = transparent_z_depth != 0.f ? res_tex[px] : zero<float4>();
		res_tex[px]				  = (center_color + col_sum) / (transparent_rpp + 1);
	}
}

#include "hrp_common.asli"

struct ps_out
{
	float4 color sv_target_0;
	// float2 motion sv_target_1;
};

ps_out
main_ps(float4 pos sv_position)
{
	const texture_2d<float> depth_tex			  = global_resource_buffer[opaque_depth_buffer_srv_id];
	const texture_2d<float> transparent_depth_tex = global_resource_buffer[transparent_depth_buffer_srv_id];

	uint32_2	px					= uint32_2(pos.xy);
	const float opaque_z_depth		= depth_tex[px];
	const float transparent_z_depth = transparent_depth_tex[px];

	if (transparent_z_depth == 0.f)
	{
		discard;
		return zero<ps_out>();
	}

	const float3 world_far = ndc_to_world(view_proj_inv, screen_to_ndc(pos.xy, opaque_z_depth, inv_backbuffer_size));

	const float3 rel	 = world_far - camera_pos;
	const float	 t_max	 = length(rel);
	const float3 ray_dir = rel / t_max;

	ps_out res;
	res.color = lighting::calc_transparent_color<false>(camera_pos, ray_dir, 0.f, t_max);
	return res;
}
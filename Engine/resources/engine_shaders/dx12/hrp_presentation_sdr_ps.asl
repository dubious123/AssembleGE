#include "hrp_common.asli"

float4
main_ps(float4 pos sv_position) sv_target_0
{
	texture_2d<float4> post_buffer_tex = global_resource_buffer[post_buffer_texture_id];

	float3 color = load(post_buffer_tex, pos.x, pos.y, 0).rgb;

	return float4(color, 1.0);
}
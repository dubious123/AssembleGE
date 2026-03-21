#include "forward_plus_common.asli"

struct vertex
{
	float4 pos sv_position;
};

float4
main_ps(vertex input) sv_target_0
{
	texture_2d<float4> blend_tex = global_resource_buffer[rt_transparent_buffer_srv_texture_id];

	float4 col = load(blend_tex, input.pos.x, input.pos.y, 0);

	return col;
}
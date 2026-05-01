#include "forward_plus_common.asli"

struct vertex
{
	float4 pos sv_position;
};

float4
main_ps(vertex input) sv_target_0
{
	texture_2d<float4> main_tex = global_resource_buffer[main_buffer_texture_id];

	float3 col = load(main_tex, input.pos.x, input.pos.y, 0).rgb;

	col = max(col, 0.0);

	// col = tonemap_reinhard_luminance(col, 4.0);
	col = tonemap_aces_hill_hdr(col, 4.0);

	return float4(col, 1.0);
}
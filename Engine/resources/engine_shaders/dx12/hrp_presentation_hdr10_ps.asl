#include "hrp_common.asli"

float4
main_ps(float4 pos sv_position) sv_target_0
{
	// float2 uv = pos.xy * inv_backbuffer_size;

	texture_2d<float4> post_buffer_tex = global_resource_buffer[post_buffer_texture_id];

	float3 color = load(post_buffer_tex, pos.x, pos.y, 0).rgb;
	// float3 color = sample(post_buffer_tex, get_linear_clamp_sampler(), uv).rgb;
	color = max(color, 0.0);
	color = color * sdr_to_pq_scale;

	color = rec709_to_rec2020(color);
	color = linear_to_pq(color);

	return float4(color, 1.0);
}
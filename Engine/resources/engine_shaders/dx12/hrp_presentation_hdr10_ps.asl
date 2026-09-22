#include "hrp_common.asli"

float4
main_ps(float4 pos sv_position) sv_target_0
{
	float2 uv = pos.xy * inv_backbuffer_size;

	texture_2d<float4> main_buffer_tex = global_resource_buffer[post_buffer_texture_id];

	float3 color = sample(main_buffer_tex, get_linear_clamp_sampler(), uv).rgb;
	color		 = max(color, 0.0);
	color		 = color * (100.0 / 10000.0);	 // (sdr_peak / pq_peak)

	color = rec709_to_rec2020(color);
	color = linear_to_pq(color);

	return float4(color, 1.0);
}
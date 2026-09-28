#include "hrp_common.asli"

float4
main_ps(float4 pos sv_position) sv_target_0
{
	texture_2d<float4> main_tex = global_resource_buffer[main_buffer_texture_id];

	float3 col = load(main_tex, pos.x, pos.y, 0).rgb;

	col = max(col, 0.0);

	c_auto bloom = load_bloom();

	attr_branch()

	if (bloom.srv_texture_id != invalid_id_uint32)
	{
		texture_2d<float3> bloom_tex	= global_resource_buffer[bloom.srv_texture_id];
		float3			   bloom_color	= sample_level(bloom_tex, get_linear_clamp_sampler(), pos.xy * inv_backbuffer_size, 0).rgb;
		col							   += bloom_color * bloom.intensity * bloom.tint;
	}

	col = tonemap_aces_hill_hdr(col, hdr_headroom);

	return float4(col, 1.0);
}
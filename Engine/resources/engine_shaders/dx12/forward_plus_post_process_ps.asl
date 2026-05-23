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

	const bloom bloom = load_bloom();

	attr_branch()

	if (bloom.srv_texture_id != invalid_id_uint32)
	{
		texture_2d<float3> bloom_tex	= global_resource_buffer[bloom.srv_texture_id];
		float3			   bloom_color	= sample_level(bloom_tex, get_linear_clamp_sampler(), input.pos.xy * inv_backbuffer_size, 0).rgb;
		col							   += bloom_color * bloom.intensity * bloom.tint;
	}

	// col = tonemap_reinhard_luminance(col, 4.0);
	col = tonemap_aces_hill_hdr(col, 4.0);

	return float4(col, 1.0);
}
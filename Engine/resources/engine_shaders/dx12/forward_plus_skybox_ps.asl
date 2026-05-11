#include "forward_plus_common.asli"

struct vertex
{
	float4 pos sv_position;
	float3 dir semantics(direction);
};

float4
main_ps(vertex v) sv_target_0
{
	float3 dir = normalize(v.dir);

	float3 res = float3(0, 0, 0);

	expand(MAX_ENV_LIGHT)

	for (uint32 i = 0; i < env_light_count; ++i)
	{
		const env_light env = load_env_light(i);

		texture_cube<float4> radiance = global_resource_buffer[env.radiance_tex_id];

		res += sample_level(radiance, get_linear_clamp_sampler(), mul(env.rotation_mat, dir), 0).rgb * float(env.intensity) * env.tint;
	}

	return float4(res, 1.0);
}
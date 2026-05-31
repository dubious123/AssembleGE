#include "forward_plus_common.asli"

struct vertex
{
	float4 pos sv_position;
	float3 dir semantics(direction);
};

float4
main_ps(vertex v) sv_target_0
{
	return float4(calc_skybox_color(normalize(v.dir)), 1.f);
}
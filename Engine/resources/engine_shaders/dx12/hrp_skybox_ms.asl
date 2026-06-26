#include "hrp_common.asli"

struct vertex
{
	float4 pos sv_position;
	float3 dir semantics(direction);
};


[numthreads(1, 1, 1)][output_topology("triangle")] void
main_ms(
	out vertices vertex	 vertex_arr[3],
	out indices uint32_3 triangle_arr[1])

{
	set_mesh_output_counts(3, 1);

	static const float3x4 pos_arr = float3x4(float4(-1.f, -1.f, 0.f, 1.f),
											 float4(-1.f, 3.f, 0.f, 1.f),
											 float4(3.f, -1.f, 0.f, 1.f));
	expand(3)

	for (uint32 i = 0; i < 3; ++i)
	{
		vertex_arr[i].pos = pos_arr[i];

		vertex_arr[i].dir = ndc_to_world(view_proj_inv, pos_arr[i].xyz) - camera_pos;
	}

	triangle_arr[0] = uint32_3(0, 1, 2);
}
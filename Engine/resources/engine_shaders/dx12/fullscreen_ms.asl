#include "forward_plus_common.asli"

struct vertex
{
	float4 pos sv_position;
};

[numthreads(1, 1, 1)][output_topology("triangle")] void
main_ms(
	out vertices vertex	 vertex_arr[3],
	out indices uint32_3 triangle_arr[1])

{
	set_mesh_output_counts(3, 1);

	vertex_arr[0].pos = float4(-1.f, -1.f, 0.f, 1.f);
	vertex_arr[1].pos = float4(-1.f, 3.f, 0.f, 1.f);
	vertex_arr[2].pos = float4(3.f, -1.f, 0.f, 1.f);

	triangle_arr[0] = uint32_3(0, 1, 2);
}
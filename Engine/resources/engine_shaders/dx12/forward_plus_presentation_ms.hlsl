#define SHADER_STAGE_MS

#include "forward_plus_common.hlsli"

#undef SHADER_STAGE_MS

struct vertex
{
    float4 pos : SV_Position;
};

[numthreads(1, 1, 1)]
[outputtopology("triangle")]
void
main_ms(
		out vertices vertex vertex_arr[3],
		out indices uint3 triangle_arr[1])
{
    SetMeshOutputCounts(3, 1);
    
    vertex_arr[0].pos = float4(-1.f, -1.f, 0.f, 1.f);
    vertex_arr[1].pos = float4(-1.f, 3.f, 0.f, 1.f);
    vertex_arr[2].pos = float4(3.f, -1.f, 0.f, 1.f);
    
    triangle_arr[0] = uint3(0, 1, 2);
}
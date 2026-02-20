struct vs_output
{
    noperspective float4 position : SV_Position;
    noperspective float2 uv : TEXCOORD;
};

vs_output main_vs(in uint vertex_idx : SV_VertexID)
{
    vs_output output;
    const float2 tex = float2(uint2(vertex_idx, vertex_idx << 1) & 2);

    output.position = float4(lerp(float2(-1, 1), float2(1, -1), tex), 0, 1);
    output.uv = tex;
    
    return output;
}
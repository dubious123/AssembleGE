Texture2D src : register(t0);

static const float exposure = 1.0f;

float3 tonemap_reinhard(float3 c)
{
    return c / (1.0 + c);
}

float4 main_ps(in noperspective float4 position : SV_Position,
                     in noperspective float2 uv : TEXCOORD) : SV_Target0
{
    float3 hdr = src.Load(int3((int2)position.xy, 0)).rgb;

    hdr *= exposure;

    float3 ldr = tonemap_reinhard(hdr);
    
    return float4(ldr, 1.0);
}
#define PRESENTATION_PS
#include "forward_plus_common.hlsli"
#undef PRESENTATION_PS

float4
main_ps(float4 pos : SV_Position) : SV_Target0
{
    //1. calculate uv
    //2. sample color from main buffer 
    //3. color -> hdr10
    return float4(1.0f, 0.0f, 0.0f, 1.0f);
}
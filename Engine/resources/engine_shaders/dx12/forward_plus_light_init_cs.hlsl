#define SHADER_STAGE_CS
#include "forward_plus_common.hlsli"
#undef SHADER_STAGE_CS

[numthreads(32, 1, 1)]
void main_cs(uint32 light_id : SV_DispatchThreadID)
{
    culled_light_buffer[light_id] = invalid_id_uint32;
}
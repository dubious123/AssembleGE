#include "hrp_common.asli"

float4
main_ps(float4 pos sv_position) sv_target_0
{
	texture_2d<float4> blend_tex = global_resource_buffer[blend_buffer_srv_id];

	return blend_tex[int32_2(pos.xy)];
}
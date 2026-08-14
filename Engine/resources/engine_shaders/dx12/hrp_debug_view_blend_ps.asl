#include "hrp_common.asli"

float4
main_ps(float4 pos sv_position) sv_target_0
{
	texture_2d<float4> debug_view_buffer = global_resource_buffer[debug_view::load_data().h_debug_view_buffer_srv_id];

	return debug_view_buffer[int32_2(pos.xy)];
}
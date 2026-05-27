#include "forward_plus_common.asli"

float4
main_ps(debug_ms_to_ps fragment) sv_target_0
{
	texture_2d<float> depth_buffer = global_resource_buffer[depth_buffer_texture_id];
	const float		  z_depth	   = load(depth_buffer, fragment.v.pos.xy, 0);
	if (fragment.v.pos.z < z_depth)
	{
		discard;
		return float4(0, 0, 0, 0);
	}

	const debug_object_data debug_obj_data = load_debug_object_data(fragment.debug_obj_id);

	float3 lighting = debug_obj_data.color;
	return float4(lighting, 1.0f);
}
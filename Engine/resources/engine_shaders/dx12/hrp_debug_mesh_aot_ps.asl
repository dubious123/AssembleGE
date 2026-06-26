#include "hrp_common.asli"

float4
main_ps(debug_ms_to_ps fragment) sv_target_0
{
	const debug_object_data debug_obj_data = load_debug_object_data(fragment.debug_obj_id);

	float3 lighting = debug_obj_data.color;

	return float4(lighting, 1.0f);
}
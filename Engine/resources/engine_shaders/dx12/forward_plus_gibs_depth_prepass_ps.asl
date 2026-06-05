#include "forward_plus_common.asli"

uint32_2
main_ps(
	float4 pos		 sv_position,
	float3 normal	 semantics(normal),
	uint32 object_id semantics(object_id)) sv_target_0
{
	uint32_2 res;

	res.x = object_id;
	res.y = encode_oct_snorm16(normalize(normal));

	return res;
}
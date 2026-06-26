#include "hrp_common.asli"

[numthreads(LIGHT_CULL_THREAD_COUNT, 1, 1)] void
main_cs(uint32 light_id sv_dispatch_thread_id)

{
	if (light_id < unified_light_count)
	{
		const unified_light light = load_unified_light(light_id);
		store_sort_key(light_id, morton_3d(light.position - camera_pos), sort_key_offset(false));

		// const float			min_z = light.position.z - light.range;
		// store_sort_key(light_id, float_to_sortable(min_z), sort_key_offset(false));


		store_sort_value(light_id, light_id, sort_value_offset(false));
	}
	else
	{
		store_sort_key(light_id, invalid_id_uint32, sort_key_offset(false));
		store_sort_value(light_id, invalid_id_uint32, sort_value_offset(false));
	}
}
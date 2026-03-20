#include "forward_plus_common.asli"

[numthreads(LIGHT_CULL_THREAD_COUNT, 1, 1)] void
main_cs(uint32 light_id sv_dispatch_thread_id)

{
	if (light_id < unified_light_count)
	{
		const unified_light light = load_unified_light(light_id);

		if (sphere_frustum_test(light.position, light.range, frustum_planes))
		{
			const float view_z = dot(light.position - camera_pos, camera_forward);
			const float min_z  = view_z - light.range;

			store_sort_key(light_id, float_to_sortable(min_z), sort_key_offset(false));
			store_sort_value(light_id, light_id, sort_value_offset(false));

			return;
		}
	}

	store_sort_key(light_id, invalid_id_uint32, sort_key_offset(false));
	store_sort_value(light_id, invalid_id_uint32, sort_value_offset(false));
}
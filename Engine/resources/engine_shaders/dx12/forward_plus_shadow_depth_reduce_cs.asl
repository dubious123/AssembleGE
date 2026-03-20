#include "forward_plus_common.asli"

DECLARE_CALC_THREAD_GROUP_MIN_MAX((16 * 16))

[numthreads(16, 16, 1)] void
main_cs(uint32_2 local_thread_id sv_group_thread_id,
		uint32_2 thread_id sv_dispatch_thread_id)

{
	uint32_2 z_min_max = uint32_2(0xffffffff, 0);

	if (all(thread_id.xy < backbuffer_size))
	{
		texture_2d<float> depth_buffer_tex = global_resource_buffer[depth_buffer_texture_id];

		const float z = depth_buffer_tex[thread_id];

		if (z > epsilon_1e4)
		{
			uint32 linear_z = as_uint(linearize_reverse_z(z, cam_near_z, cam_far_z));

			z_min_max = uint32_2(linear_z, linear_z);
		}
	}

	const uint32_2 group_min_max = calc_thread_group_min_max(z_min_max.x, z_min_max.y, local_thread_id.y * 16 + local_thread_id.x);

	if (all(local_thread_id == uint32_2(0, 0)))
	{
		store_z_min_interlocked_min(group_min_max.x);
		store_z_max_interlocked_max(group_min_max.y);
	}
}
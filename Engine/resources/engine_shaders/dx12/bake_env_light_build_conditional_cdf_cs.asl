#include "bake_common.asli"

DECLARE_CALC_THREAD_GROUP_PREFIX_SUM_FLOAT(1024)

groupshared float row_total;

[numthreads(1024, 1, 1)] void
main_cs(uint32_3 dispatch_thread_id sv_dispatch_thread_id,
		uint32 thread_id			sv_group_thread_id)

{
	const uint32	   row		= dispatch_thread_id.y;
	texture_2d<float4> equirect = global_resource_buffer[env_light_input_texture_srv_id];

	const float sin_theta = sin(pi * (float(row) + 0.5) / float(env_light_input_texture_height));

	float thread_local_sum = 0.f;

	for (uint32 i = 0; i < 32; ++i)
	{
		const uint32 offset = thread_id * 32 + i;

		if (env_light_input_texture_width <= offset) { break; }

		float luminance = luminance_rec709(equirect[uint32_2(offset, row)].rgb) * sin_theta;

		env_light_radiance_luminance_uav[row * env_light_input_texture_width + offset] = luminance;

		thread_local_sum += luminance;
	}

	const float prefix_sum = calc_thread_group_prefix_sum_float(thread_local_sum, thread_id);

	if (thread_id == 1024 - 1)
	{
		row_total = prefix_sum + thread_local_sum;
		store_luminance_row_sum(row, row_total);
	}

	group_memory_barrier_with_sync();

	if (row_total == 0.f) { return; }

	float acc = prefix_sum;
	for (uint32 i = 0; i < 32; ++i)
	{
		const uint32 offset = thread_id * 32 + i;
		if (offset < env_light_input_texture_width)
		{
			acc += env_light_radiance_luminance_uav[row * env_light_input_texture_width + offset];

			env_light_conditional_cdf_uav[row * env_light_input_texture_width + offset] = acc / row_total;
		}
	}
}

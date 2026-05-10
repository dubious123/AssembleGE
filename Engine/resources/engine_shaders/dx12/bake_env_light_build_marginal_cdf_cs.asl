#include "bake_common.asli"

DECLARE_CALC_THREAD_GROUP_PREFIX_SUM_FLOAT(1024)

groupshared float total;

[numthreads(1024, 1, 1)] void
main_cs(uint32 thread_id sv_dispatch_thread_id) {
	float local_sum = 0.f;
	for (uint32 i = 0; i < 32; ++i)
	{
		const uint32 row = thread_id * 32 + i;
		if (row >= env_light_input_texture_height) { break; }

		local_sum += load_luminance_row_sum(row);
	}

	const float prefix_sum = calc_thread_group_prefix_sum_float(local_sum, thread_id);

	if (thread_id == 1024 - 1)
	{
		total = prefix_sum + local_sum;
	}

	group_memory_barrier_with_sync();

	if (total <= 0.0) return;

	float acc = prefix_sum;
	for (uint32 i = 0; i < 32; ++i)
	{
		const uint32 row = thread_id * 32 + i;
		if (row >= env_light_input_texture_height) break;

		acc += load_luminance_row_sum(row);

		env_light_marginal_cdf_uav[row] = acc / total;
	}
}
#include "forward_plus_common.asli"

groupshared uint32 local_min_idx[LIGHT_AXIS_SLICE_MAX];
groupshared uint32 local_max_idx[LIGHT_AXIS_SLICE_MAX];

[numthreads(LIGHT_ZBIN_THREAD_COUNT, 1, 1)] void
main_cs(uint32 sorted_id	   sv_dispatch_thread_id,
		uint32 group_thread_id sv_group_thread_id)

{
	const t_unified_light_id light_id = load_sort_value(sorted_id, sort_value_offset(false));

	unified_light light = (unified_light)0;

	if (light_id != invalid_id_uint32)
	{
		light = load_unified_light(light_id);

		store_sorted_light(sorted_id, light);
	}


	// todo, better aabb

	const uint32 word_idx = sorted_id / 32u;
	const uint32 mask	  = 1u << (sorted_id % 32u);

	{
		for (uint32 i = group_thread_id; i < X_SLICE_COUNT; i += LIGHT_ZBIN_THREAD_COUNT)
		{
			local_min_idx[i] = 0xffffffff;
			local_max_idx[i] = 0;
		}

		group_memory_barrier_with_sync();

		if (light_id != invalid_id_uint32)
		{
			const float x_min = light.position.x - light.range;
			const float x_max = light.position.x + light.range;

			const uint32 bin_x_begin = world_to_bin_x(x_min);
			const uint32 bin_x_end	 = world_to_bin_x(x_max);

			for (uint32 i = bin_x_begin; i <= bin_x_end; ++i)
			{
				interlocked_min(local_min_idx[i], sorted_id);
				interlocked_max(local_max_idx[i], sorted_id);

				store_bin_mask_x_atomic(i, word_idx, mask);
			}
		}

		group_memory_barrier_with_sync();

		for (uint32 i = group_thread_id; i < X_SLICE_COUNT; i += LIGHT_ZBIN_THREAD_COUNT)
		{
			if (local_min_idx[i] != 0xffffffff)
			{
				store_bin_entry_x_atomic(i, local_min_idx[i], local_max_idx[i]);
			}
		}
	}

	{
		for (uint32 i = group_thread_id; i < Y_SLICE_COUNT; i += LIGHT_ZBIN_THREAD_COUNT)
		{
			local_min_idx[i] = 0xffffffff;
			local_max_idx[i] = 0;
		}

		group_memory_barrier_with_sync();

		if (light_id != invalid_id_uint32)
		{
			const float y_min = light.position.y - light.range;
			const float y_max = light.position.y + light.range;

			const uint32 bin_y_begin = world_to_bin_y(y_min);
			const uint32 bin_y_end	 = world_to_bin_y(y_max);

			for (uint32 i = bin_y_begin; i <= bin_y_end; ++i)
			{
				interlocked_min(local_min_idx[i], sorted_id);
				interlocked_max(local_max_idx[i], sorted_id);

				store_bin_mask_y_atomic(i, word_idx, mask);
			}
		}

		group_memory_barrier_with_sync();

		for (uint32 i = group_thread_id; i < Y_SLICE_COUNT; i += LIGHT_ZBIN_THREAD_COUNT)
		{
			if (local_min_idx[i] != 0xffffffff)
			{
				store_bin_entry_y_atomic(i, local_min_idx[i], local_max_idx[i]);
			}
		}
	}

	{
		for (uint32 i = group_thread_id; i < Z_SLICE_COUNT; i += LIGHT_ZBIN_THREAD_COUNT)
		{
			local_min_idx[i] = 0xffffffff;
			local_max_idx[i] = 0;
		}

		group_memory_barrier_with_sync();

		if (light_id != invalid_id_uint32)
		{
			const float z_min = light.position.z - light.range;
			const float z_max = light.position.z + light.range;

			const uint32 bin_z_begin = world_to_bin_z(z_min);
			const uint32 bin_z_end	 = world_to_bin_z(z_max);

			for (uint32 i = bin_z_begin; i <= bin_z_end; ++i)
			{
				interlocked_min(local_min_idx[i], sorted_id);
				interlocked_max(local_max_idx[i], sorted_id);

				store_bin_mask_z_atomic(i, word_idx, mask);
			}
		}

		group_memory_barrier_with_sync();

		for (uint32 i = group_thread_id; i < Z_SLICE_COUNT; i += LIGHT_ZBIN_THREAD_COUNT)
		{
			if (local_min_idx[i] != 0xffffffff)
			{
				store_bin_entry_z_atomic(i, local_min_idx[i], local_max_idx[i]);
			}
		}
	}
}
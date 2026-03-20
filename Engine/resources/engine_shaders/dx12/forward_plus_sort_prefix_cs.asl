#include "forward_plus_common.asli"

DECLARE_CALC_THREAD_GROUP_PREFIX_SUM(SORT_THREAD_COUNT)

groupshared uint32 bin_prefix_table[SORT_ELEMENT_COUNT_PER_THREAD][SORT_THREAD_COUNT];

[numthreads(SORT_THREAD_COUNT, 1, 1)] void
main_cs(uint32 bin_entry sv_group_id,
		uint32 thread_id sv_group_thread_id)

{
	expand(SORT_ELEMENT_COUNT_PER_THREAD)

	for (uint32 i = 0; i < SORT_ELEMENT_COUNT_PER_THREAD; ++i)
	{
		// block_size == thread_count * element_count_per_thread
		//  group_count <= block_size
		// histogram_uav : uint32[bin_count][group_count]
		const uint32 index = i * SORT_THREAD_COUNT + thread_id;
		const uint32 col   = index / SORT_ELEMENT_COUNT_PER_THREAD;
		const uint32 row   = index % SORT_ELEMENT_COUNT_PER_THREAD;

		// transpose data
		bin_prefix_table[row][col] = index < SORT_GROUP_COUNT
									   ? load_sort_histogram(bin_entry * SORT_GROUP_COUNT + index)
									   : 0;
	}

	group_memory_barrier_with_sync();

	uint32 thread_local_sum = 0;
	for (uint32 i = 0; i < SORT_ELEMENT_COUNT_PER_THREAD; ++i)
	{
		uint32 temp						= bin_prefix_table[i][thread_id];
		bin_prefix_table[i][thread_id]	= thread_local_sum;
		thread_local_sum			   += temp;
	}

	const uint32 prefix = calc_thread_group_prefix_sum(thread_local_sum, thread_id);

	for (uint32 i = 0; i < SORT_ELEMENT_COUNT_PER_THREAD; ++i)
	{
		bin_prefix_table[i][thread_id] += prefix;
	}
	group_memory_barrier_with_sync();

	expand(SORT_ELEMENT_COUNT_PER_THREAD)

	for (uint32 i = 0; i < SORT_ELEMENT_COUNT_PER_THREAD; ++i)
	{
		// block_size == thread_count * element_count_per_thread
		// group_count <= block_size
		// histogram_uav : uint32[bin_count][group_count]
		const uint32 index = i * SORT_THREAD_COUNT + thread_id;
		const uint32 col   = index / SORT_ELEMENT_COUNT_PER_THREAD;
		const uint32 row   = index % SORT_ELEMENT_COUNT_PER_THREAD;

		if (index < SORT_GROUP_COUNT)
		{
			store_sort_histogram(bin_entry * SORT_GROUP_COUNT + index, bin_prefix_table[row][col]);
		}
	}

	if (thread_id == SORT_THREAD_COUNT - 1)
	{
		store_sort_bin_count(bin_entry, prefix + thread_local_sum);
	}
}

#include "forward_plus_common.asli"

// 16 * 128 = 8k byte
groupshared uint32
	local_histogram[SORT_BIN_COUNT][SORT_THREAD_COUNT];

[numthreads(SORT_THREAD_COUNT, 1, 1)] void
main_cs(uint32 visible_light_id sv_dispatch_thread_id,
		uint32 group_id sv_group_id,
		uint32 thread_id sv_group_thread_id)

{
	for (uint32 bin = 0; bin < SORT_BIN_COUNT; ++bin)
	{
		local_histogram[bin][thread_id] = 0;
	}

	group_memory_barrier_with_sync();

	const uint32 src_keys_offset = sort_key_offset(radix_sort_pass % 2 == 1);

	for (uint32 block_index = 0; block_index < SORT_BLOCK_COUNT_PER_GROUP; ++block_index)
	{
		// expect compiler to prefetch all elements, hiding memory latency
		expand(SORT_ELEMENT_COUNT_PER_THREAD)

		for (uint32 i = 0; i < SORT_ELEMENT_COUNT_PER_THREAD; ++i)
		{
			const uint32 key_index = group_id * SORT_BLOCK_COUNT_PER_GROUP * SORT_BLOCK_SIZE
								   + block_index * SORT_BLOCK_SIZE
								   + i * SORT_THREAD_COUNT
								   + thread_id;

			const uint32 key = load_sort_key(key_index, src_keys_offset);

			static const uint32 bin_bit_mask = SORT_BIN_COUNT - 1;
			const uint32		bin			 = (key >> (radix_sort_pass * SORT_BIN_BIT_WIDTH)) & bin_bit_mask;

			++local_histogram[bin][thread_id];
		}
	}

	group_memory_barrier_with_sync();

	if (thread_id < SORT_BIN_COUNT)
	{
		const uint32 bin			 = thread_id;
		uint32		 block_histogram = 0;

		for (uint32 row = 0; row < SORT_THREAD_COUNT; ++row)
		{
			block_histogram += local_histogram[bin][row];
		}
		// histogram[bin][group_id] = block_histogram
		// sort_buffer[SORT_HISTOGRAM_OFFSET + bin * (SORT_GROUP_COUNT) + group_id] = block_histogram;

		store_sort_histogram(bin * (SORT_GROUP_COUNT) + group_id, block_histogram);
	}
}

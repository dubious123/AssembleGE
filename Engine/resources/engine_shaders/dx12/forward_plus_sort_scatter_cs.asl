#include "forward_plus_common.asli"

DECLARE_CALC_THREAD_GROUP_PREFIX_SUM(SORT_THREAD_COUNT)


groupshared uint32 bin_offset_arr[SORT_BIN_COUNT];
groupshared uint32 gs_generic[SORT_THREAD_COUNT];

groupshared uint32 local_histogram[SORT_BIN_COUNT];

[numthreads(SORT_THREAD_COUNT, 1, 1)] void
main_cs(uint32 group_id sv_group_id,
		uint32 thread_id sv_group_thread_id)

{
	if (thread_id < SORT_BIN_COUNT)
	{
		const uint32 group_local_bin_offset = load_sort_histogram(thread_id * SORT_GROUP_COUNT + group_id);
		const uint32 offset					= load_sort_bin_count(thread_id);

		bin_offset_arr[thread_id] = wave_prefix_sum(offset) + group_local_bin_offset;
	}

	group_memory_barrier_with_sync();

	const uint32 src_keys_offset   = sort_key_offset(radix_sort_pass % 2 == 1);
	const uint32 dst_keys_offset   = sort_key_offset(radix_sort_pass % 2 == 0);
	const uint32 src_values_offset = sort_value_offset(radix_sort_pass % 2 == 1);
	const uint32 dst_values_offset = sort_value_offset(radix_sort_pass % 2 == 0);

	for (uint32 block_index = 0; block_index < SORT_BLOCK_COUNT_PER_GROUP; ++block_index)
	{
		// expect compiler to prefetch all elements, hiding memory latency
		expand(SORT_ELEMENT_COUNT_PER_THREAD)

		for (uint32 i = 0; i < SORT_ELEMENT_COUNT_PER_THREAD; ++i)
		{
			static const uint32 bin_bit_mask = SORT_BIN_COUNT - 1;

			const uint32 index = group_id * SORT_BLOCK_COUNT_PER_GROUP * SORT_BLOCK_SIZE
							   + block_index * SORT_BLOCK_SIZE
							   + i * SORT_THREAD_COUNT
							   + thread_id;

			uint32 key	 = load_sort_key(index, src_keys_offset);
			uint32 value = load_sort_value(index, src_values_offset);

			// sort thread_count(s) keys
			for (uint32 bit_shift = 0; bit_shift < SORT_BIN_BIT_WIDTH; bit_shift += 2)
			{
				const uint32 bin	 = (key >> (radix_sort_pass * SORT_BIN_BIT_WIDTH)) & bin_bit_mask;
				const uint32 bit_bin = (bin >> bit_shift) & ((1 << 2) - 1);

				const uint32 bit_bin_shifted = (1u << (bit_bin * 8));

				const uint32 histogram_prefix_sum = calc_thread_group_prefix_sum(bit_bin_shifted, thread_id);

				if (thread_id == SORT_THREAD_COUNT - 1)
				{
					gs_generic[0] = histogram_prefix_sum + bit_bin_shifted;
				}

				group_memory_barrier_with_sync();

				// histogram : uint8[4]
				// histogram[2] == count of bit_bin #2
				const uint32 histogram = gs_generic[0];

				group_memory_barrier_with_sync();

				// base_offset : uint8[4]
				// base_offset[2] == base_offset of bit_bin #2
				// [ #2 + #1 + #0 | #1 + #0 | #0 | 0 ]
				const uint32 packed_prefix_offset = (histogram << 8) + (histogram << 16) + (histogram << 24);

				// packed_bit_bin_offset : uint8[4]
				// packed_bit_bin_offset[2] == offset of bit_bin #2
				// [ #2 + #1 + #0 | #1 + #0 + local_offset_of_bin_2 | #0 | 0 ]
				const uint32 packed_bit_bin_offset = packed_prefix_offset + histogram_prefix_sum;

				const uint32 bit_bin_offset = (packed_bit_bin_offset >> (bit_bin * 8)) & (SORT_THREAD_COUNT - 1);

				// swap key and value
				gs_generic[bit_bin_offset] = key;

				group_memory_barrier_with_sync();

				key = gs_generic[thread_id];

				group_memory_barrier_with_sync();

				gs_generic[bit_bin_offset] = value;

				group_memory_barrier_with_sync();

				value = gs_generic[thread_id];

				group_memory_barrier_with_sync();
			}

			const uint32 bin = (key >> (radix_sort_pass * SORT_BIN_BIT_WIDTH)) & bin_bit_mask;

			if (thread_id < SORT_BIN_COUNT)
			{
				local_histogram[thread_id] = 0;
			}

			group_memory_barrier_with_sync();

			interlocked_add(local_histogram[bin], 1);

			group_memory_barrier_with_sync();

			if (thread_id < SORT_BIN_COUNT)
			{
				const uint32 histogram_prefix = wave_prefix_sum(local_histogram[thread_id]);
				gs_generic[thread_id]		  = histogram_prefix;
			}

			group_memory_barrier_with_sync();

			// gs_generic[bin] : prefix of bin offset
			// thread_id : prefix of bin offset + local index of bin
			// thread_id - gs_generic[bin] : local_index of bin
			const uint32 bin_offset = bin_offset_arr[bin] + (thread_id - gs_generic[bin]);

			store_sort_key(bin_offset, key, dst_keys_offset);
			store_sort_value(bin_offset, value, dst_values_offset);

			if (thread_id < SORT_BIN_COUNT)
			{
				bin_offset_arr[thread_id] += local_histogram[thread_id];
			}

			// group_memory_barrier_with_sync();
		}
	}
}

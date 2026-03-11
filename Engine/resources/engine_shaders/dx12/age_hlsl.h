#pragma once

#define AGE_HLSL
#define SYS_VAL(name) : name
#define REG(...) : register(__VA_ARGS__)
#define UNROLL [[unroll]]

#define uint64 uint64_t
#define uint32 uint
#define uint16 uint16_t
// #define uint8  uint

#define int64 int64_t
#define int32 int
#define int16 int16_t
// #define int8  uint

#define int32_2 int2
#define int32_3 int3
#define int32_4 int4

#define uint32_2 uint2
#define uint32_3 uint3
#define uint32_4 uint4

#define int16_2 int16_t2
#define int16_3 int16_t3
#define int16_4 int16_t4

#define uint16_2 uint16_t2
#define uint16_3 uint16_t3
#define uint16_4 uint16_t4

#define int8_2 int8_t2
#define int8_3 int8_t3
#define int8_4 int8_t4

#define uint8_2 uint8_t2
#define uint8_3 uint8_t3
#define uint8_4 uint8_t4

#include "age_hlsl_math.h"

#define DECLARE_CALC_THREAD_GROUP_PREFIX_SUM(thread_count)                                                  \
	groupshared uint32 __age_hlsl_prefix_sum_arr__[thread_count];                                           \
	uint32 calc_thread_group_prefix_sum(uint32 thread_local, uint32 thread_id)                              \
	{                                                                                                       \
		uint32		 wave_prefix = WavePrefixSum(thread_local);                                             \
		const uint32 wave_id	 = thread_id / WaveGetLaneCount();                                          \
		if (WaveGetLaneIndex() == WaveGetLaneCount() - 1)                                                   \
		{                                                                                                   \
			__age_hlsl_prefix_sum_arr__[wave_id] = wave_prefix + thread_local;                              \
		}                                                                                                   \
		GroupMemoryBarrierWithGroupSync();                                                                  \
		if (wave_id == 0)                                                                                   \
		{                                                                                                   \
			__age_hlsl_prefix_sum_arr__[thread_id] = WavePrefixSum(__age_hlsl_prefix_sum_arr__[thread_id]); \
		}                                                                                                   \
		GroupMemoryBarrierWithGroupSync();                                                                  \
		return __age_hlsl_prefix_sum_arr__[wave_id] + wave_prefix;                                          \
	}

#define DECLARE_CALC_THREAD_GROUP_MIN_MAX(thread_count)                                                    \
	groupshared uint32 __age_hlsl_min_arr__[thread_count];                                                 \
	groupshared uint32 __age_hlsl_max_arr__[thread_count];                                                 \
                                                                                                           \
	uint32_2 calc_thread_group_min_max(uint32 thread_local_min, uint32 thread_local_max, uint32 thread_id) \
	{                                                                                                      \
		uint32		 wave_min	= WaveActiveMin(thread_local_min);                                         \
		uint32		 wave_max	= WaveActiveMax(thread_local_max);                                         \
		const uint32 wave_count = (thread_count + WaveGetLaneCount() - 1) / WaveGetLaneCount();            \
		const uint32 wave_id	= thread_id / WaveGetLaneCount();                                          \
                                                                                                           \
		if (WaveIsFirstLane())                                                                             \
		{                                                                                                  \
			__age_hlsl_min_arr__[wave_id] = wave_min;                                                      \
			__age_hlsl_max_arr__[wave_id] = wave_max;                                                      \
		}                                                                                                  \
		GroupMemoryBarrierWithGroupSync();                                                                 \
                                                                                                           \
		if (wave_id == 0 && thread_id < wave_count)                                                        \
		{                                                                                                  \
			__age_hlsl_min_arr__[thread_id] = WaveActiveMin(__age_hlsl_min_arr__[thread_id]);              \
			__age_hlsl_max_arr__[thread_id] = WaveActiveMax(__age_hlsl_max_arr__[thread_id]);              \
		}                                                                                                  \
		GroupMemoryBarrierWithGroupSync();                                                                 \
		return uint32_2(__age_hlsl_min_arr__[wave_id], __age_hlsl_max_arr__[wave_id]);                     \
	}
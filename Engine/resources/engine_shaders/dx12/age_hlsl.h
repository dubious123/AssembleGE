#pragma once
#define AGE_HLSL
#define SYS_VAL(name) : name
#define REG(...) : register(__VA_ARGS__)

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
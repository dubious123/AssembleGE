#pragma once
#include <cstdint>
#if defined(_WIN32)
	#include <DirectXMath.h>
#else
#endif

#if defined(_WIN64)
using float2	= DirectX::XMFLOAT2;
using float2a	= DirectX::XMFLOAT2A;
using float3	= DirectX::XMFLOAT3;
using float3a	= DirectX::XMFLOAT3A;
using float4	= DirectX::XMFLOAT4;
using float4a	= DirectX::XMFLOAT4A;
using uint2		= DirectX::XMUINT2;
using uint3		= DirectX::XMUINT3;
using uint4		= DirectX::XMUINT4;
using int2		= DirectX::XMINT2;
using int3		= DirectX::XMINT3;
using int4		= DirectX::XMINT4;
using float3x3	= DirectX::XMFLOAT3X3;
using float4x4	= DirectX::XMFLOAT4X4;
using float4x4a = DirectX::XMFLOAT4X4A;
#endif

using uint64 = uint64_t;
using uint32 = uint32_t;
using uint16 = uint16_t;
using uint8	 = uint8_t;

using int64 = int64_t;
using int32 = int32_t;
using int16 = int16_t;
using int8	= int8_t;

using float32  = float;
using double64 = double;

static constexpr uint64 invalid_id_uint64 = 0xffff'ffff'ffff'ffffui64;
static constexpr uint32 invalid_id_uint32 = 0xffff'ffffui32;
static constexpr uint16 invalid_id_uint16 = 0xffffui16;

static constexpr uint64 invalid_idx_uint64 = 0xffff'ffff'ffff'ffffui64;
static constexpr uint32 invalid_idx_uint32 = 0xffff'ffffui32;
static constexpr uint16 invalid_idx_uint16 = 0xffffui16;
static constexpr uint16 invalid_idx_uint8  = 0xffui8;
#pragma once

namespace age::inline math
{
	template <std::integral t>
	requires(not std::is_same_v<t, bool>)
	struct extent_2d
	{
		t width;
		t height;

		constexpr auto
		operator<=>(const extent_2d<t>&) const noexcept = default;
	};
}	 // namespace age::inline math

#if defined(_WIN64)
using int2 = DirectX::XMINT2;
using int3 = DirectX::XMINT3;
using int4 = DirectX::XMINT4;

using uint2 = DirectX::XMUINT2;
using uint3 = DirectX::XMUINT3;
using uint4 = DirectX::XMUINT4;

using float2  = DirectX::XMFLOAT2;
using float2a = DirectX::XMFLOAT2A;
using float3  = DirectX::XMFLOAT3;
using float3a = DirectX::XMFLOAT3A;
using float4  = DirectX::XMFLOAT4;
using float4a = DirectX::XMFLOAT4A;

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

inline constexpr uint64 invalid_id_uint64 = 0xffff'ffff'ffff'ffffui64;
inline constexpr uint32 invalid_id_uint32 = 0xffff'ffffui32;
inline constexpr uint16 invalid_id_uint16 = 0xffffui16;

inline constexpr uint64 invalid_idx_uint64 = 0xffff'ffff'ffff'ffffui64;
inline constexpr uint32 invalid_idx_uint32 = 0xffff'ffffui32;
inline constexpr uint16 invalid_idx_uint16 = 0xffffui16;
inline constexpr uint16 invalid_idx_uint8  = 0xffui8;

enum e_primitive_type
{
	primitive_type_int2,
	primitive_type_int3,
	primitive_type_int4,

	primitive_type_uint2,
	primitive_type_uint3,
	primitive_type_uint4,

	primitive_type_float2,
	primitive_type_float2a,
	primitive_type_float3,
	primitive_type_float3a,
	primitive_type_float4,
	primitive_type_float4a,

	primitive_type_float3x3,
	primitive_type_float4x4,
	primitive_type_float4x4a,

	primitive_type_uint64,
	primitive_type_uint32,
	primitive_type_uint16,
	primitive_type_uint8,

	primitive_type_int64,
	primitive_type_int32,
	primitive_type_int16,
	primitive_type_int8,

	primitive_type_float32,
	primitive_type_double64,

	primitive_type_count,
	//----------------------------------------------

	primitive_type_Int2 = primitive_type_int2,
	primitive_type_Int3 = primitive_type_int3,
	primitive_type_Int4 = primitive_type_int4,

	primitive_type_Uint2 = primitive_type_uint2,
	primitive_type_Uint3 = primitive_type_uint3,
	primitive_type_Uint4 = primitive_type_uint4,

	primitive_type_Float2  = primitive_type_float2,
	primitive_type_Float2a = primitive_type_float2a,
	primitive_type_Float3  = primitive_type_float3,
	primitive_type_Float3a = primitive_type_float3a,
	primitive_type_Float4  = primitive_type_float4,
	primitive_type_Float4a = primitive_type_float4a,

	primitive_type_Float3x3	 = primitive_type_float3x3,
	primitive_type_Float4x4	 = primitive_type_float4x4,
	primitive_type_Float4x4a = primitive_type_float4x4a,

	primitive_type_Uint64 = primitive_type_uint64,
	primitive_type_Uint32 = primitive_type_uint32,
	primitive_type_Uint16 = primitive_type_uint16,
	primitive_type_Uint8  = primitive_type_uint8,

	primitive_type_Int64 = primitive_type_int64,
	primitive_type_Int32 = primitive_type_int32,
	primitive_type_Int16 = primitive_type_int16,
	primitive_type_Int8	 = primitive_type_int8,

	primitive_type_Float32	= primitive_type_float32,
	primitive_type_Double64 = primitive_type_double64,

	primitive_type_Count = primitive_type_count
};

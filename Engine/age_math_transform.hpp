#pragma once

namespace age::inline math
{
	FORCE_INLINE constexpr float
	cvt_to_radian(float degrees) noexcept
	{
		return degrees * g::degree_to_radian;
	}

	FORCE_INLINE constexpr float
	cvt_to_degree(float radians) noexcept
	{
		return radians * g::radian_to_degree;
	}
}	 // namespace age::inline math

// quaternoin
namespace age::inline math
{
	FORCE_INLINE bool AGE_SIMD_CALL
	is_quaternoin_normalized(fxm_vec quaternion) noexcept
	{
		c_auto len_sq = quaternion | simd::length_square4() | simd::get_x();

		return std::abs(len_sq - 1.0f) <= age::math::g::epsilon_1e4;
	}

	FORCE_INLINE bool
	is_quaternoin_normalized(float4 quaternion) noexcept
	{
		return is_quaternoin_normalized(simd::load(quaternion));
	}

	FORCE_INLINE uint32
	quaternion_encode(float4 q) noexcept
	{
		using namespace age::math::simd;
		auto xm_q = simd::load(q);

		AGE_ASSERT(is_quaternoin_normalized(xm_q));

		auto q_abs = xm_q | simd::abs() | simd::to<float4>();

		auto missing_idx = 0u;
		auto missing	 = q_abs.x;
		if (q_abs.y > missing)
		{
			missing		= q_abs.y;
			missing_idx = 1;
		}
		if (q_abs.z > missing)
		{
			missing		= q_abs.z;
			missing_idx = 2;
		}
		if (q_abs.w > missing)
		{
			missing		= q_abs.w;
			missing_idx = 3;
		}

		auto xm_q_flipped = simd::sign_mask(q[missing_idx]) | simd::bit_xor(xm_q);

		auto quantized_u4 = xm_q_flipped
						  | simd::add(simd::g::xm_sqrt2_inv_f4)
						  | simd::mul(simd::g::xm_sqrt2_inv_f4)
						  | simd::saturate()
						  | simd::mul(simd::g::xm_1023_f4)
						  | simd::round()
						  | simd::to<uint32_4>();

		constexpr const uint32 perm_table[4][3] = {
			{ 1, 2, 3 },	// missing=x : c=(y,z,w)
			{ 0, 2, 3 },	// missing=y : c=(x,z,w)
			{ 0, 1, 3 },	// missing=z : c=(x,y,w)
			{ 0, 1, 2 },	// missing=w : c=(x,y,z)
		};

		return (missing_idx << 30)
			 | ((quantized_u4[perm_table[missing_idx][2]] & 0x3ffu) << 20)
			 | ((quantized_u4[perm_table[missing_idx][1]] & 0x3ffu) << 10)
			 | ((quantized_u4[perm_table[missing_idx][0]] & 0x3ffu) << 0);
	}

	FORCE_INLINE float4
	quaternion_decode(uint32 encoded_q) noexcept
	{
		auto index = (encoded_q >> 30) & 0x3;
		auto c	   = float3{
			(float)((encoded_q >> 0) & 0x3ff),	   // x
			(float)((encoded_q >> 10) & 0x3ff),	   // y
			(float)((encoded_q >> 20) & 0x3ff)
		};	  // z

		c = c / 1023.f * g::sqrt_2 - g::sqrt_2_inv;

		float missing = std::sqrtf(std::clamp(1.f - dot(c, c), 0.f, 1.f));

		switch (index)
		{
		case 0:
			return float4(missing, c.x, c.y, c.z);
		case 1:
			return float4(c.x, missing, c.y, c.z);
		case 2:
			return float4(c.x, c.y, missing, c.z);
		default:	// case 3
			return float4(c.x, c.y, c.z, missing);
		}
	}
}	 // namespace age::inline math
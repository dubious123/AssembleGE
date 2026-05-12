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

	FORCE_INLINE float3
	normalize(float3 v) noexcept
	{
		return v | simd::load() | simd::normalize3() | simd::to<float3>();
	}

	FORCE_INLINE float4
	normalize(float4 v) noexcept
	{
		return v | simd::load() | simd::normalize4() | simd::to<float4>();
	}
}	 // namespace age::inline math

// 2d
namespace age::inline math
{
	FORCE_INLINE float4
	intersect_2d(const float4& aabb_l, const float4& aabb_r) noexcept
	{
		auto&& [xm_l, xm_r] = simd::load(aabb_l, aabb_r);
		c_auto xm_select	= fxm_vec{ std::bit_cast<float>(0), std::bit_cast<float>(0), std::bit_cast<float>(0xFFFFFFFF), std::bit_cast<float>(0xFFFFFFFF) };
		c_auto xm_max		= simd::max(xm_l, xm_r);
		c_auto xm_min		= simd::min(xm_l, xm_r);
		return simd::select(xm_max, xm_min, xm_select) | simd::to<float4>();
	}

	FORCE_INLINE bool
	contains_2d(const float4& rect, const float2& point)
	{
		return point.x >= rect.x and point.x <= rect.z
		   and point.y >= rect.y and point.y <= rect.w;
	}
}	 // namespace age::inline math

// quaternoin
namespace age::inline math
{
	FORCE_INLINE float4
	euler_rad_to_quat(float3 euler_radian) noexcept
	{
		return euler_radian | simd::load() | simd::euler_to_quat() | simd::to<float4>();
	}

	FORCE_INLINE float4
	euler_deg_to_quat(float3 euler_deg) noexcept
	{
		return euler_deg | simd::load() | simd::mul(simd::replicate(g::degree_to_radian)) | simd::euler_to_quat() | simd::to<float4>();
	}

	FORCE_INLINE float3
	quat_to_euler_rad(float4 quat) noexcept
	{
		c_auto x = quat.x, y = quat.y, z = quat.z, w = quat.w;

		// pitch (X)
		c_auto sinp	 = 2.0f * (w * x + y * z);
		c_auto cosp	 = 1.0f - 2.0f * (x * x + y * y);
		c_auto pitch = std::atan2(sinp, cosp);

		// yaw (Y)
		c_auto siny = 2.0f * (w * y - z * x);
		c_auto yaw	= std::abs(siny) >= 1.0f
						? std::copysign(g::pi_div_2, siny)	  // gimbal lock
						: std::asin(siny);

		// roll (Z)
		c_auto sinr = 2.0f * (w * z + x * y);
		c_auto cosr = 1.0f - 2.0f * (z * z + y * y);
		c_auto roll = std::atan2(sinr, cosr);


		return float3(pitch, yaw, roll);
	}

	FORCE_INLINE float3
	quat_to_euler_deg(float4 quat) noexcept
	{
		return quat_to_euler_rad(quat) * float3{ g::radian_to_degree };
	}

	FORCE_INLINE float3x3
	euler_deg_to_mat3x3(float3 euler_deg) noexcept
	{
		return euler_deg | simd::load() | simd::mul(simd::replicate(g::degree_to_radian)) | simd::euler_to_mat4x4() | simd::to<float3x3>();
	}

	FORCE_INLINE float3x4
	trs(float3 pos, float4 quat, float3 scale) noexcept
	{
		return simd::transformation(simd::load(scale), simd::g::xm_zero_f4, simd::load(quat), simd::load(pos))
			 | simd::to<float3x4>();
	}

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
			(float)((encoded_q >> 20) & 0x3ff)	   // z
		};

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

// color
namespace age::inline math
{
	FORCE_INLINE constexpr float
	srgb_to_linear_channel(float c) noexcept
	{
		return c <= 0.04045f
				 ? c / 12.92f
				 : std::pow((c + 0.055f) / 1.055f, 2.4f);
	}

	FORCE_INLINE constexpr float3
	srgb_to_linear(float3 c) noexcept
	{
		return {
			srgb_to_linear_channel(c.x),
			srgb_to_linear_channel(c.y),
			srgb_to_linear_channel(c.z),
		};
	}

	FORCE_INLINE constexpr float
	linear_to_srgb_channel(float c) noexcept
	{
		return c <= 0.0031308f
				 ? 12.92f * c
				 : 1.055f * std::pow(c, 1.0f / 2.4f) - 0.055f;
	}

	FORCE_INLINE constexpr float3
	linear_to_srgb(float3 c) noexcept
	{
		return {
			linear_to_srgb_channel(c.x),
			linear_to_srgb_channel(c.y),
			linear_to_srgb_channel(c.z),
		};
	}

	FORCE_INLINE constexpr float3
	srgb_to_linear_approx(float3 c) noexcept
	{
		return { std::pow(c.x, 2.2f), std::pow(c.y, 2.2f), std::pow(c.z, 2.2f) };
	}

	FORCE_INLINE constexpr float3
	linear_to_srgb_approx(float3 c) noexcept
	{
		return { std::pow(c.x, 1.0f / 2.2f), std::pow(c.y, 1.0f / 2.2f), std::pow(c.z, 1.0f / 2.2f) };
	}
}	 // namespace age::inline math
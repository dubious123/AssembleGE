#pragma once

namespace age::inline math
{
#define AGE_VEC_FUNC(func_name)                                                                    \
	template <template <typename> typename v, typename t>                                          \
	FORCE_INLINE constexpr decltype(auto)                                                          \
	func_name(const v<t>& lhs) noexcept                                                            \
	{                                                                                              \
		if constexpr (std::is_same_v<v<t>, float2> or std::is_same_v<v<t>, float2a>)               \
		{                                                                                          \
			return v<t>{ func_name(lhs.x), func_name(lhs.y) };                                     \
		}                                                                                          \
		else if constexpr (std::is_same_v<v<t>, float3> or std::is_same_v<v<t>, float3a>)          \
		{                                                                                          \
			return v<t>{ func_name(lhs.x), func_name(lhs.y), func_name(lhs.z) };                   \
		}                                                                                          \
		else if constexpr (std::is_same_v<v<t>, float4> or std::is_same_v<v<t>, float4a>)          \
		{                                                                                          \
			return v<t>{ func_name(lhs.x), func_name(lhs.y), func_name(lhs.z), func_name(lhs.w) }; \
		}                                                                                          \
		else                                                                                       \
		{                                                                                          \
			static_assert(false, "unsupported type for pow");                                      \
		}                                                                                          \
	}

	FORCE_INLINE float
	as_float(auto&& x) noexcept
	{
		return std::bit_cast<float>(FWD(x));
	}

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

	FORCE_INLINE float2
	normalize(const float2& v) noexcept
	{
		return v | simd::load() | simd::normalize2() | simd::to<float2>();
	}

	FORCE_INLINE float3
	normalize(const float3& v) noexcept
	{
		return v | simd::load() | simd::normalize3() | simd::to<float3>();
	}

	FORCE_INLINE float4
	normalize(const float4& v) noexcept
	{
		return v | simd::load() | simd::normalize4() | simd::to<float4>();
	}

	FORCE_INLINE float4
	mul(const float4x4& mat, const float4& v) noexcept
	{
		return simd::transform4(simd::load(mat), simd::load(v)) | simd::to<float4>();
	}

	FORCE_INLINE float
	dot(const float3& lhs, const float3& rhs) noexcept
	{
		return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z;
	}

	FORCE_INLINE float
	dot(const float4& lhs, const float4& rhs) noexcept
	{
		return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z + lhs.w * rhs.w;
	}

	FORCE_INLINE float3
	cross(const float3& lhs, const float3& rhs) noexcept
	{
		return float3{
			lhs.y * rhs.z - lhs.z * rhs.y,
			lhs.z * rhs.x - lhs.x * rhs.z,
			lhs.x * rhs.y - lhs.y * rhs.x,
		};
	}

	FORCE_INLINE decltype(auto)
	abs(float f) noexcept
	{
		return std::abs(f);
	}

	FORCE_INLINE decltype(auto)
	abs(const float2& v) noexcept
	{
		return float2{ std::abs(v.x), std::abs(v.y) };
	}

	FORCE_INLINE decltype(auto)
	abs(const float3& v) noexcept
	{
		return float3{ std::abs(v.x), std::abs(v.y), std::abs(v.z) };
	}

	FORCE_INLINE decltype(auto)
	abs(const float4& v) noexcept
	{
		return float4{ std::abs(v.x), std::abs(v.y), std::abs(v.z), std::abs(v.w) };
	}

	FORCE_INLINE decltype(auto)
	length_sq(const float2& v) noexcept
	{
		return v.x * v.x + v.y * v.y;
	}

	FORCE_INLINE decltype(auto)
	length_sq(const float3& v) noexcept
	{
		return v.x * v.x + v.y * v.y + v.z * v.z;
	}

	FORCE_INLINE decltype(auto)
	length_sq(const float4& v) noexcept
	{
		return v.x * v.x + v.y * v.y + v.z * v.z + v.w * v.w;
	}

	FORCE_INLINE decltype(auto)
	length(const float2& v) noexcept
	{
		return std::sqrt(length_sq(v));
	}

	FORCE_INLINE decltype(auto)
	length(const float3& v) noexcept
	{
		return std::sqrt(length_sq(v));
	}

	FORCE_INLINE decltype(auto)
	length(const float4& v) noexcept
	{
		return std::sqrt(length_sq(v));
	}

	FORCE_INLINE constexpr decltype(auto)
	ceil(auto&& vec) noexcept
	{
		if constexpr (std::is_same_v<BARE_OF(vec), float2> or std::is_same_v<BARE_OF(vec), float2a>)
		{
			return float2{ std::ceil(vec.x), std::ceil(vec.y) };
		}
		else if constexpr (std::is_same_v<BARE_OF(vec), float3> or std::is_same_v<BARE_OF(vec), float3a>)
		{
			return float3{ std::ceil(vec.x), std::ceil(vec.y), std::ceil(vec.z) };
		}
		else if constexpr (std::is_same_v<BARE_OF(vec), float4> or std::is_same_v<BARE_OF(vec), float4a>)
		{
			return float4{ std::ceil(vec.x), std::ceil(vec.y), std::ceil(vec.z), std::ceil(vec.w) };
		}
		else
		{
			static_assert(false, "unsupported type for ceil");
		}
	}

	FORCE_INLINE constexpr decltype(auto)
	floor(auto&& vec) noexcept
	{
		if constexpr (std::is_same_v<BARE_OF(vec), float2> or std::is_same_v<BARE_OF(vec), float2a>)
		{
			return float2{ std::floor(vec.x), std::floor(vec.y) };
		}
		else if constexpr (std::is_same_v<BARE_OF(vec), float3> or std::is_same_v<BARE_OF(vec), float3a>)
		{
			return float3{ std::floor(vec.x), std::floor(vec.y), std::floor(vec.z) };
		}
		else if constexpr (std::is_same_v<BARE_OF(vec), float4> or std::is_same_v<BARE_OF(vec), float4a>)
		{
			return float4{ std::floor(vec.x), std::floor(vec.y), std::floor(vec.z), std::floor(vec.w) };
		}
		else
		{
			static_assert(false, "unsupported type for floor");
		}
	}

	template <template <typename> typename v, typename t>
	FORCE_INLINE constexpr decltype(auto)
	min(const v<t>& lhs, const v<t>& rhs) noexcept
	{
		if constexpr (std::is_same_v<v<t>, float2> or std::is_same_v<v<t>, float2a>)
		{
			return v<t>{ std::min(lhs.x, rhs.x), std::min(lhs.y, rhs.y) };
		}
		else if constexpr (std::is_same_v<v<t>, float3> or std::is_same_v<v<t>, float3a>)
		{
			return v<t>{ std::min(lhs.x, rhs.x), std::min(lhs.y, rhs.y), std::min(lhs.z, rhs.z) };
		}
		else if constexpr (std::is_same_v<v<t>, float4> or std::is_same_v<v<t>, float4a>)
		{
			return v<t>{ std::min(lhs.x, rhs.x), std::min(lhs.y, rhs.y), std::min(lhs.z, rhs.z), std::min(lhs.w, rhs.w) };
		}
		else
		{
			static_assert(false, "unsupported type for min");
		}
	}

	template <template <typename> typename v, typename t>
	FORCE_INLINE constexpr decltype(auto)
	min(const v<t>& lhs, t rhs) noexcept
	{
		if constexpr (std::is_same_v<v<t>, float2> or std::is_same_v<v<t>, float2a>)
		{
			return v<t>{ std::min(lhs.x, rhs), std::min(lhs.y, rhs) };
		}
		else if constexpr (std::is_same_v<v<t>, float3> or std::is_same_v<v<t>, float3a>)
		{
			return v<t>{ std::min(lhs.x, rhs), std::min(lhs.y, rhs), std::min(lhs.z, rhs) };
		}
		else if constexpr (std::is_same_v<v<t>, float4> or std::is_same_v<v<t>, float4a>)
		{
			return v<t>{ std::min(lhs.x, rhs), std::min(lhs.y, rhs), std::min(lhs.z, rhs), std::min(lhs.w, rhs) };
		}
		else
		{
			static_assert(false, "unsupported type for min");
		}
	}

	FORCE_INLINE constexpr decltype(auto)
	min(float l, float r) noexcept
	{
		return std::min(l, r);
	}

	template <template <typename> typename v, typename t>
	FORCE_INLINE constexpr decltype(auto)
	max(const v<t>& lhs, const v<t>& rhs) noexcept
	{
		if constexpr (std::is_same_v<v<t>, float2> or std::is_same_v<v<t>, float2a>)
		{
			return v<t>{ std::max(lhs.x, rhs.x), std::max(lhs.y, rhs.y) };
		}
		else if constexpr (std::is_same_v<v<t>, float3> or std::is_same_v<v<t>, float3a>)
		{
			return v<t>{ std::max(lhs.x, rhs.x), std::max(lhs.y, rhs.y), std::max(lhs.z, rhs.z) };
		}
		else if constexpr (std::is_same_v<v<t>, float4> or std::is_same_v<v<t>, float4a>)
		{
			return v<t>{ std::max(lhs.x, rhs.x), std::max(lhs.y, rhs.y), std::max(lhs.z, rhs.z), std::max(lhs.w, rhs.w) };
		}
		else
		{
			static_assert(false, "unsupported type for max");
		}
	}

	template <template <typename> typename v, typename t>
	FORCE_INLINE constexpr decltype(auto)
	max(const v<t>& lhs, t rhs) noexcept
	{
		if constexpr (std::is_same_v<v<t>, float2> or std::is_same_v<v<t>, float2a>)
		{
			return v<t>{ std::max(lhs.x, rhs), std::max(lhs.y, rhs) };
		}
		else if constexpr (std::is_same_v<v<t>, float3> or std::is_same_v<v<t>, float3a>)
		{
			return v<t>{ std::max(lhs.x, rhs), std::max(lhs.y, rhs), std::max(lhs.z, rhs) };
		}
		else if constexpr (std::is_same_v<v<t>, float4> or std::is_same_v<v<t>, float4a>)
		{
			return v<t>{ std::max(lhs.x, rhs), std::max(lhs.y, rhs), std::max(lhs.z, rhs), std::max(lhs.w, rhs) };
		}
		else
		{
			static_assert(false, "unsupported type for max");
		}
	}

	FORCE_INLINE constexpr decltype(auto)
	max(float l, float r) noexcept
	{
		return std::max(l, r);
	}

	template <template <typename> typename v, typename t_>
	FORCE_INLINE constexpr decltype(auto)
	lerp(const v<t_>& a, const v<t_>& b, float t) noexcept
	{
		if constexpr (std::is_same_v<v<t_>, float2> or std::is_same_v<v<t_>, float2a>)
		{
			return v<t_>{ std::lerp(a.x, b.x, t), std::lerp(a.y, b.y, t) };
		}
		else if constexpr (std::is_same_v<v<t_>, float3> or std::is_same_v<v<t_>, float3a>)
		{
			return v<t_>{ std::lerp(a.x, b.x, t), std::lerp(a.y, b.y, t), std::lerp(a.z, b.z, t) };
		}
		else if constexpr (std::is_same_v<v<t_>, float4> or std::is_same_v<v<t_>, float4a>)
		{
			return v<t_>{ std::lerp(a.x, b.x, t), std::lerp(a.y, b.y, t), std::lerp(a.z, b.z, t), std::lerp(a.w, b.w, t) };
		}
		else
		{
			static_assert(false, "unsupported type for lerp");
		}
	}

	FORCE_INLINE constexpr decltype(auto)
	lerp(float a, float b, float t) noexcept
	{
		return a + t * (b - a);
	}

	FORCE_INLINE constexpr decltype(auto)
	sqrt(float f) noexcept
	{
		return std::sqrt(f);
	}

	FORCE_INLINE constexpr decltype(auto)
	sign(float f) noexcept
	{
		if (f > 0) { return 1.f; }
		if (f < 0) { return -1.f; }
		return 0.f;
	}

	AGE_VEC_FUNC(sign);

	FORCE_INLINE constexpr decltype(auto)
	sin(float f) noexcept
	{
		return std::sin(f);
	}

	FORCE_INLINE constexpr decltype(auto)
	cos(float f) noexcept
	{
		return std::cos(f);
	}

	FORCE_INLINE constexpr decltype(auto)
	pow(float a, float b)
	{
		return std::pow(a, b);
	}

	template <template <typename> typename v, typename t>
	FORCE_INLINE constexpr decltype(auto)
	pow(const v<t>& lhs, t rhs) noexcept
	{
		if constexpr (std::is_same_v<v<t>, float2> or std::is_same_v<v<t>, float2a>)
		{
			return v<t>{ std::pow(lhs.x, rhs), std::pow(lhs.y, rhs) };
		}
		else if constexpr (std::is_same_v<v<t>, float3> or std::is_same_v<v<t>, float3a>)
		{
			return v<t>{ std::pow(lhs.x, rhs), std::pow(lhs.y, rhs), std::pow(lhs.z, rhs) };
		}
		else if constexpr (std::is_same_v<v<t>, float4> or std::is_same_v<v<t>, float4a>)
		{
			return v<t>{ std::pow(lhs.x, rhs), std::pow(lhs.y, rhs), std::pow(lhs.z, rhs), std::pow(lhs.w, rhs) };
		}
		else
		{
			static_assert(false, "unsupported type for pow");
		}
	}

	template <template <typename> typename v, typename t>
	FORCE_INLINE constexpr decltype(auto)
	pow(const v<t>& lhs, const v<t>& rhs) noexcept
	{
		if constexpr (std::is_same_v<v<t>, float2> or std::is_same_v<v<t>, float2a>)
		{
			return v<t>{ std::pow(lhs.x, rhs.x), std::pow(lhs.y, rhs.y) };
		}
		else if constexpr (std::is_same_v<v<t>, float3> or std::is_same_v<v<t>, float3a>)
		{
			return v<t>{ std::pow(lhs.x, rhs.x), std::pow(lhs.y, rhs.y), std::pow(lhs.z, rhs.z) };
		}
		else if constexpr (std::is_same_v<v<t>, float4> or std::is_same_v<v<t>, float4a>)
		{
			return v<t>{ std::pow(lhs.x, rhs.x), std::pow(lhs.y, rhs.y), std::pow(lhs.z, rhs.z), std::pow(lhs.w, rhs.w) };
		}
		else
		{
			static_assert(false, "unsupported type for max");
		}
	}

	FORCE_INLINE constexpr decltype(auto)
	clamp(auto x, auto min, auto max) noexcept
	{
		return std::clamp(x, min, max);
	}

	FORCE_INLINE decltype(auto)
	clamp(const float2& v, auto min, auto max) noexcept
	{
		return float2{ std::clamp(v.x, min, max), std::clamp(v.y, min, max) };
	}

	FORCE_INLINE decltype(auto)
	clamp(const float3& v, auto min, auto max) noexcept
	{
		return float3{ std::clamp(v.x, min, max), std::clamp(v.y, min, max), std::clamp(v.z, min, max) };
	}

	FORCE_INLINE decltype(auto)
	clamp(const float4& v, auto min, auto max) noexcept
	{
		return float4{ std::clamp(v.x, min, max), std::clamp(v.y, min, max), std::clamp(v.z, min, max), std::clamp(v.w, min, max) };
	}

#undef AGE_VEC_FUNC
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

namespace age::inline math
{
	FORCE_INLINE float2
	screen_to_ndc(const float2& screen_size, const float2& screen_pos) noexcept
	{
		return float2{ screen_pos.x / screen_size.x * 2.f - 1.f, 1.f - screen_pos.y / screen_size.y * 2.f };
	}

	FORCE_INLINE float3
	ndc_to_world(const float4x4& view_proj_inv, const float3& ndc) noexcept
	{
		c_auto temp = mul(view_proj_inv, float4(ndc, 1.f));

		return temp.xyz / temp.w;
	}

	FORCE_INLINE float3
	ndc_to_world(const float4x4& view_proj_inv, const float4& ndc) noexcept
	{
		c_auto temp = mul(view_proj_inv, ndc);

		return temp.xyz / temp.w;
	}
}	 // namespace age::inline math

// quaternoin
namespace age::inline math
{
	FORCE_INLINE float3
	rotate(const float4& quaternion, const float3& v) noexcept
	{
		return simd::rotate3(simd::load(quaternion), simd::load(v)) | simd::to<float3>();
	}

	FORCE_INLINE float2
	rotate(const float2& v, float radian)
	{
		return float2((v.x * cos(radian) - v.y * sin(radian)), v.x * sin(radian) + v.y * cos(radian));
	}

	FORCE_INLINE float3
	rotate_around(const float4& quat, const float3& point, const float3& pivot) noexcept
	{
		return pivot + math::rotate(quat, point - pivot);
	}

	FORCE_INLINE float4
	quat_rotation_normal(const float3& dir /*normalized*/, const float rad) noexcept
	{
		return simd::quat_rotation_normal(simd::load(dir), rad) | simd::to<float4>();
	}

	FORCE_INLINE float4
	quat_look_to(const float3& dir /*normalized*/, const float roll_rad) noexcept
	{
		c_auto xm_look = simd::load(dir);

		c_auto dot = simd::dot3(xm_look, simd::g::xm_forward_f4) | simd::get_x();

		c_auto w = std::sqrt(2.f * (1.f + dot));

		c_auto base = dot > (1.f - g::epsilon_1e4)
						? simd::g::xm_quat_identity_f4
					: dot < (g::epsilon_1e4 - 1.f)
						? simd::g::xm_quat_back_f4
						: simd::cross3(simd::g::xm_forward_f4, xm_look)
							  | simd::scale(1.f / w)
							  | simd::set_w(w * 0.5f)
							  | simd::normalize4();

		return roll_rad != 0.f
				 ? simd::quat_rotation_normal(xm_look, roll_rad)
					   | simd::quat_mul(base)
					   | simd::to<float4>()
				 : base
					   | simd::to<float4>();
	}

	FORCE_INLINE float4
	quat_look_to(const float3& dir /*normalized*/, const float3& up) noexcept
	{
		const auto&& [xm_look, xm_up] = simd::load(dir, up);

		return simd::view_look_to(simd::g::xm_zero_f4, xm_look, xm_up)
			 | simd::rotation_mat_to_quat()
			 | simd::to<float4>();
	}

	FORCE_INLINE float4
	quat_look_to(const float3& dir /*normalized*/) noexcept
	{
		return simd::view_look_to(simd::g::xm_zero_f4, simd::load(dir), simd::g::xm_up_f4)
			 | simd::rotation_mat_to_quat()
			 | simd::to<float4>();
	}

	// rotate q1 and q0
	FORCE_INLINE float4
	quat_mul(const float4& q0, const float4& q1) noexcept
	{
		return simd::quat_mul(simd::load(q0), simd::load(q1)) | simd::normalize4() | simd::to<float4>();
	}

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

		return std::abs(len_sq - 1.0f) <= age::math::g::epsilon_1e6;
	}

	FORCE_INLINE bool
	is_quaternoin_normalized(float4 quaternion) noexcept
	{
		return is_quaternoin_normalized(simd::load(quaternion));
	}

	// FORCE_INLINE uint32
	// quaternion_encode(float4 q) noexcept
	//{
	//	using namespace age::math::simd;
	//	auto xm_q = simd::load(q);

	//	AGE_ASSERT(is_quaternoin_normalized(xm_q));

	//	auto q_abs = xm_q | simd::abs() | simd::to<float4>();

	//	auto missing_idx = 0u;
	//	auto missing	 = q_abs.x;
	//	if (q_abs.y > missing)
	//	{
	//		missing		= q_abs.y;
	//		missing_idx = 1;
	//	}
	//	if (q_abs.z > missing)
	//	{
	//		missing		= q_abs.z;
	//		missing_idx = 2;
	//	}
	//	if (q_abs.w > missing)
	//	{
	//		missing		= q_abs.w;
	//		missing_idx = 3;
	//	}

	//	auto xm_q_flipped = simd::sign_mask(q[missing_idx]) | simd::bit_xor(xm_q);

	//	auto quantized_u4 = xm_q_flipped
	//					  | simd::add(simd::g::xm_sqrt2_inv_f4)
	//					  | simd::mul(simd::g::xm_sqrt2_inv_f4)
	//					  | simd::saturate()
	//					  | simd::mul(simd::g::xm_1023_f4)
	//					  | simd::round()
	//					  | simd::to<uint32_4>();

	//	constexpr const uint32 perm_table[4][3] = {
	//		{ 1, 2, 3 },	// missing=x : c=(y,z,w)
	//		{ 0, 2, 3 },	// missing=y : c=(x,z,w)
	//		{ 0, 1, 3 },	// missing=z : c=(x,y,w)
	//		{ 0, 1, 2 },	// missing=w : c=(x,y,z)
	//	};

	//	return (missing_idx << 30)
	//		 | ((quantized_u4[perm_table[missing_idx][2]] & 0x3ffu) << 20)
	//		 | ((quantized_u4[perm_table[missing_idx][1]] & 0x3ffu) << 10)
	//		 | ((quantized_u4[perm_table[missing_idx][0]] & 0x3ffu) << 0);
	//}

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

	FORCE_INLINE uint32_2
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
						  | simd::mul(simd::g::xm_65535_f4)
						  | simd::round()
						  | simd::to<uint32_4>();

		constexpr const uint32 perm_table[4][3] = {
			{ 1, 2, 3 },	// missing=x : c=(y,z,w)
			{ 0, 2, 3 },	// missing=y : c=(x,z,w)
			{ 0, 1, 3 },	// missing=z : c=(x,y,w)
			{ 0, 1, 2 },	// missing=w : c=(x,y,z)
		};

		c_auto a = quantized_u4[perm_table[missing_idx][0]] & 0xffffu;
		c_auto b = quantized_u4[perm_table[missing_idx][1]] & 0xffffu;
		c_auto c = quantized_u4[perm_table[missing_idx][2]] & 0xffffu;

		return uint32_2{
			(b << 16) | a,
			(missing_idx << 16) | c
		};
	}

	FORCE_INLINE float4
	quaternion_decode(uint32_2 encoded_q) noexcept
	{
		auto index = (encoded_q.y >> 16) & 0x3;
		auto c	   = float3{
			(float)(encoded_q.x & 0xffff),			  // x
			(float)((encoded_q.x >> 16) & 0xffff),	  // y
			(float)(encoded_q.y & 0xffff)			  // z
		};

		c = c / 65535.f * g::sqrt_2 - g::sqrt_2_inv;

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

	FORCE_INLINE constexpr float4
	srgb_to_linear(float4 c) noexcept
	{
		return {
			srgb_to_linear_channel(c.x),
			srgb_to_linear_channel(c.y),
			srgb_to_linear_channel(c.z),
			c.w
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
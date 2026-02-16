#pragma once

using xm_vec = DirectX::XMVECTOR;
using xm_mat = DirectX::XMMATRIX;

using fxm_vec = const xm_vec;
using cxm_vec = const xm_vec&;

using fxm_mat = const xm_mat;
using cxm_mat = const xm_mat&;

namespace age::math::simd::g
{
	inline constexpr auto ones_i4 = DirectX::XMVECTORU32{ { { uint32{ 0xffff'ffff }, uint32{ 0xffff'ffff }, uint32{ 0xffff'ffff }, uint32{ 0xffff'ffff } } } };
	inline constexpr auto ones_f4 = DirectX::XMVECTORF32{ { { 1.0f, 1.0f, 1.0f, 1.0f } } };

	FORCE_INLINE xm_vec XM_CALLCONV
	vec_zero() noexcept
	{
		DirectX::XMVectorZero();
	}

}	 // namespace age::math::simd::g

namespace age::math::simd
{
	template <typename t_val, typename t_func>
	requires std::is_invocable_v<t_func, t_val>
	FORCE_INLINE auto
	operator|(t_val&& val, t_func&& func) noexcept
	{
		return std::invoke(FWD(func), FWD(val));
	}
}	 // namespace age::math::simd

// store, load
namespace age::math::simd
{
	FORCE_INLINE void XM_CALLCONV
	store(uint32_3& out, fxm_vec v) noexcept
	{
		DirectX::XMStoreUInt3(reinterpret_cast<DirectX::XMUINT3*>(&out), v);
	}

	FORCE_INLINE void XM_CALLCONV
	store(float2& out, fxm_vec v) noexcept
	{
		DirectX::XMStoreFloat2(reinterpret_cast<DirectX::XMFLOAT2*>(&out), v);
	}

	FORCE_INLINE void XM_CALLCONV
	store(float3& out, fxm_vec v) noexcept
	{
		DirectX::XMStoreFloat3(reinterpret_cast<DirectX::XMFLOAT3*>(&out), v);
	}

	FORCE_INLINE void XM_CALLCONV
	store(float4& out, fxm_vec v) noexcept
	{
		DirectX::XMStoreFloat4(reinterpret_cast<DirectX::XMFLOAT4*>(&out), v);
	}

	FORCE_INLINE void XM_CALLCONV
	store(float2a& out, fxm_vec v) noexcept
	{
		DirectX::XMStoreFloat2A(reinterpret_cast<DirectX::XMFLOAT2A*>(&out), v);
	}

	FORCE_INLINE void XM_CALLCONV
	store(float3a& out, fxm_vec v) noexcept
	{
		DirectX::XMStoreFloat3A(reinterpret_cast<DirectX::XMFLOAT3A*>(&out), v);
	}

	FORCE_INLINE void XM_CALLCONV
	store(float4a& out, fxm_vec v) noexcept
	{
		DirectX::XMStoreFloat4A(reinterpret_cast<DirectX::XMFLOAT4A*>(&out), v);
	}

	FORCE_INLINE void XM_CALLCONV
	store(half2& out, fxm_vec v) noexcept
	{
		DirectX::PackedVector::XMStoreHalf2(reinterpret_cast<DirectX::PackedVector::XMHALF2*>(&out), v);
	}

	FORCE_INLINE void XM_CALLCONV
	store(half4& out, fxm_vec v) noexcept
	{
		DirectX::PackedVector::XMStoreHalf4(reinterpret_cast<DirectX::PackedVector::XMHALF4*>(&out), v);
	}

#define AGE_SIMD_LOAD(name, input_type, func)                                \
	struct __##name##input_type##__                                          \
	{                                                                        \
		FORCE_INLINE decltype(auto) XM_CALLCONV                              \
		operator()(const input_type& v) const noexcept                       \
		{                                                                    \
			using t_arg = meta::function_traits_of<DirectX::func>::t_arg<0>; \
			return DirectX::func(reinterpret_cast<t_arg>(v.data()));         \
		}                                                                    \
	};                                                                       \
	FORCE_INLINE decltype(auto) XM_CALLCONV                                  \
	name(const input_type& v) noexcept                                       \
	{                                                                        \
		using t_arg = meta::function_traits_of<DirectX::func>::t_arg<0>;     \
		return DirectX::func(reinterpret_cast<t_arg>(v.data()));             \
	}

	AGE_SIMD_LOAD(load, float2, XMLoadFloat2);
	AGE_SIMD_LOAD(load, float3, XMLoadFloat3);
	AGE_SIMD_LOAD(load, float4, XMLoadFloat4);
	AGE_SIMD_LOAD(load, float2a, XMLoadFloat2A);
	AGE_SIMD_LOAD(load, float3a, XMLoadFloat3A);
	AGE_SIMD_LOAD(load, float4a, XMLoadFloat4A);
	AGE_SIMD_LOAD(load, float3x3, XMLoadFloat3x3);
	AGE_SIMD_LOAD(load, float4x4, XMLoadFloat4x4);
	AGE_SIMD_LOAD(load, float3x3a, XMLoadFloat4x4A);
	AGE_SIMD_LOAD(load, float4x4a, XMLoadFloat4x4A);
	AGE_SIMD_LOAD(load, half2, PackedVector::XMLoadHalf2);
	AGE_SIMD_LOAD(load, half4, PackedVector::XMLoadHalf4);

#undef AGE_SIMD_LOAD

	FORCE_INLINE decltype(auto) XM_CALLCONV
	load(auto&&... arg) noexcept
		requires(sizeof...(arg) > 1)
	{
		return std::tuple<decltype(age::math::simd::load(FWD(arg)))...>{
			age::math::simd::load(FWD(arg))...
		};
	}

	struct __load__
	{
		FORCE_INLINE decltype(auto) XM_CALLCONV
		operator()(const auto& v) const noexcept
		{
			return load(v);
		}
	};

	FORCE_INLINE decltype(auto) XM_CALLCONV
	load() noexcept
	{
		return __load__{};
	}

	FORCE_INLINE
	xm_vec XM_CALLCONV
	load_vec_replicate(const float& f) noexcept
	{
		return DirectX::XMVectorReplicate(f);
	}
}	 // namespace age::math::simd

// compare
namespace age::math::simd
{
	FORCE_INLINE
	xm_vec XM_CALLCONV
	cmp_less(xm_vec v0, xm_vec v1) noexcept
	{
		return DirectX::XMVectorLess(v0, v1);
	}

	FORCE_INLINE
	xm_vec XM_CALLCONV
	cmp_greater_equal(xm_vec v0, xm_vec v1) noexcept
	{
		return DirectX::XMVectorGreaterOrEqual(v0, v1);
	}

	FORCE_INLINE decltype(auto) XM_CALLCONV
	cmp_greater_equal_rmask(xm_vec v0, xm_vec v1) noexcept
	{
		auto rmask	= uint32{};
		auto xm_vec = DirectX::XMVectorGreaterOrEqualR(&rmask, v0, v1);
		return std::tuple{ rmask, xm_vec };
	}
}	 // namespace age::math::simd

#define AGE_SIMD_VEC_UNARY_OP(name, func)       \
	struct __##name##__                         \
	{                                           \
		FORCE_INLINE decltype(auto) XM_CALLCONV \
		operator()(fxm_vec v) const noexcept    \
		{                                       \
			return DirectX::func(v);            \
		}                                       \
	};                                          \
	FORCE_INLINE decltype(auto) XM_CALLCONV     \
	name(fxm_vec v) noexcept                    \
	{                                           \
		return DirectX::func(v);                \
	}                                           \
	FORCE_INLINE decltype(auto) XM_CALLCONV     \
	name() noexcept                             \
	{                                           \
		return __##name##__{};                  \
	}

#define AGE_SIMD_VEC_BINARY_OP(name, func, rhs_type) \
	struct __##name##__                              \
	{                                                \
		rhs_type __rhs__;                            \
		FORCE_INLINE decltype(auto) XM_CALLCONV      \
		operator()(fxm_vec v) const noexcept         \
		{                                            \
			return DirectX::func(v, __rhs__);        \
		}                                            \
	};                                               \
	FORCE_INLINE decltype(auto) XM_CALLCONV          \
	name(rhs_type rhs) noexcept                      \
	{                                                \
		return __##name##__{ rhs };                  \
	}                                                \
	FORCE_INLINE decltype(auto) XM_CALLCONV          \
	name(fxm_vec lhs, rhs_type rhs) noexcept         \
	{                                                \
		return DirectX::func(lhs, rhs);              \
	}

#define AGE_SIMD_VEC_GETTER_OP(name, func1, func2)        \
	template <typename t = float>                         \
	struct __##name##__                                   \
	{                                                     \
		FORCE_INLINE decltype(auto) XM_CALLCONV           \
		operator()(fxm_vec v) const noexcept              \
		{                                                 \
			if constexpr (std::is_floating_point_v<t>)    \
			{                                             \
				return static_cast<t>(DirectX::func1(v)); \
			}                                             \
			else if constexpr (std::is_integral_v<t>)     \
			{                                             \
				return static_cast<t>(DirectX::func2(v)); \
			}                                             \
			else                                          \
			{                                             \
				static_assert(false, "wrong type");       \
			}                                             \
		}                                                 \
	};                                                    \
                                                          \
	template <typename t = float>                         \
	FORCE_INLINE decltype(auto) XM_CALLCONV               \
	name(fxm_vec v) noexcept                              \
	{                                                     \
		return __##name##__<t>{}(v);                      \
	}                                                     \
                                                          \
	template <typename t = float>                         \
	FORCE_INLINE decltype(auto) XM_CALLCONV               \
	name() noexcept                                       \
	{                                                     \
		return __##name##__<t>{};                         \
	}

namespace age::math::simd
{
	AGE_SIMD_VEC_UNARY_OP(abs, XMVectorAbs);
	AGE_SIMD_VEC_UNARY_OP(normalize3, XMVector3Normalize);
	AGE_SIMD_VEC_UNARY_OP(reciprocal_length3, XMVector3ReciprocalLength);
	AGE_SIMD_VEC_UNARY_OP(reciprocal_length_est3, XMVector3ReciprocalLengthEst);
	AGE_SIMD_VEC_UNARY_OP(acos, XMVectorACos);
	AGE_SIMD_VEC_UNARY_OP(acos_est, XMVectorACosEst);

	AGE_SIMD_VEC_GETTER_OP(get_x, XMVectorGetX, XMVectorGetIntX);
	AGE_SIMD_VEC_GETTER_OP(get_y, XMVectorGetY, XMVectorGetIntY);
	AGE_SIMD_VEC_GETTER_OP(get_z, XMVectorGetZ, XMVectorGetIntZ);
	AGE_SIMD_VEC_GETTER_OP(get_w, XMVectorGetW, XMVectorGetIntW);

	template <typename t>
	struct __to__
	{
		static FORCE_INLINE t XM_CALLCONV
		operator()(fxm_vec v) noexcept
		{
			t res{};
			store(res, v);
			return res;
		}
	};

	template <typename t>
	FORCE_INLINE decltype(auto) XM_CALLCONV
	to() noexcept
	{
		return __to__<t>{};
	}

	template <typename t>
	FORCE_INLINE decltype(auto) XM_CALLCONV
	to(fxm_vec v) noexcept
	{
		return __to__<t>{}(v);
	}

	template <auto i0, auto i1, auto i2, auto i3>
	struct __swizzle__
	{
		static FORCE_INLINE xm_vec XM_CALLCONV
		operator()(fxm_vec v) noexcept
		{
			return DirectX::XMVectorSwizzle<i0, i1, i2, i3>(v);
		}
	};

	template <auto i0, auto i1, auto i2, auto i3>
	FORCE_INLINE decltype(auto) XM_CALLCONV
	swizzle() noexcept
	{
		return __swizzle__<i0, i1, i2, i3>{};
	}

	template <auto i0, auto i1, auto i2, auto i3>
	FORCE_INLINE decltype(auto) XM_CALLCONV
	swizzle(fxm_vec v) noexcept
	{
		return DirectX::XMVectorSwizzle<i0, i1, i2, i3>(v);
	}
}	 // namespace age::math::simd

namespace age::math::simd
{
	AGE_SIMD_VEC_BINARY_OP(dot3, XMVector3Dot, fxm_vec);
	AGE_SIMD_VEC_BINARY_OP(cross3, XMVector3Cross, fxm_vec);
	AGE_SIMD_VEC_BINARY_OP(mul, XMVectorMultiply, fxm_vec);
	AGE_SIMD_VEC_BINARY_OP(sub, XMVectorSubtract, fxm_vec);
	AGE_SIMD_VEC_BINARY_OP(setw, XMVectorSetW, float);
	AGE_SIMD_VEC_BINARY_OP(transform_coord3, XMVector3TransformCoord, fxm_mat);
	AGE_SIMD_VEC_BINARY_OP(max, XMVectorMax, fxm_vec);
	AGE_SIMD_VEC_BINARY_OP(cmp_equal, XMVectorEqual, fxm_vec);
	AGE_SIMD_VEC_BINARY_OP(bit_and, XMVectorAndInt, fxm_vec);
	AGE_SIMD_VEC_BINARY_OP(bit_and_not, XMVectorAndCInt, fxm_vec);	  // lhs & (~rhs)

	template <typename t>
	struct __store_to__
	{
		t __dst__;

		FORCE_INLINE void XM_CALLCONV
		operator()(fxm_vec v) noexcept
		{
			store(__dst__, v);
		}
	};

	FORCE_INLINE decltype(auto) XM_CALLCONV
	store_to(auto& dst) noexcept
	{
		static_assert(std::is_lvalue_reference_v<decltype(dst)>);
		static_assert(std::is_const_v<std::remove_reference_t<decltype(dst)>> is_false);
		return __store_to__<decltype(dst)>{ dst };
	}

	struct __cmp_equal_r1__
	{
		fxm_vec __rhs__;

		FORCE_INLINE uint32 XM_CALLCONV
		operator()(fxm_vec __lhs__) noexcept
		{
			auto cmp_r = uint32{};
			DirectX::XMVectorEqualR(&cmp_r, __lhs__, __rhs__);
			return cmp_r;
		}
	};

	struct __cmp_equal_r2__
	{
		fxm_vec __rhs__;
		uint32& cmp_r;

		FORCE_INLINE xm_vec XM_CALLCONV
		operator()(fxm_vec __lhs__) noexcept
		{
			return DirectX::XMVectorEqualR(&cmp_r, __lhs__, __rhs__);
		}
	};

	FORCE_INLINE decltype(auto) XM_CALLCONV
	cmp_equal_r(fxm_vec rhs) noexcept
	{
		return __cmp_equal_r1__{ rhs };
	}

	FORCE_INLINE decltype(auto) XM_CALLCONV
	cmp_equal_r(fxm_vec rhs, uint32& cmp_r) noexcept
	{
		return __cmp_equal_r2__{ rhs, cmp_r };
	}
}	 // namespace age::math::simd

FORCE_INLINE xm_vec XM_CALLCONV
operator-(fxm_vec v1, fxm_vec v2) noexcept
{
	return DirectX::XMVectorSubtract(v1, v2);
}

FORCE_INLINE xm_vec XM_CALLCONV
operator+(fxm_vec v1, fxm_vec v2) noexcept
{
	return DirectX::XMVectorAdd(v1, v2);
}

// packed
namespace age::math
{
	class cvt_unorm_tag { };

	class cvt_snorm_tag { };

	class cvt_cast_tag { };

	template <typename t>
	concept cx_cvt_tag = std::is_same_v<t, cvt_unorm_tag>
					  or std::is_same_v<t, cvt_snorm_tag>
					  or std::is_same_v<t, cvt_cast_tag>;

	namespace detail
	{
		template <typename t_dst, typename t_src, typename t_tag>
		requires false
		FORCE_INLINE t_dst
		cvt_to_impl(const t_src& v, t_tag) noexcept;

		template <typename t_dst>
		requires meta::variadic_contains_v<t_dst, uint8, uint16>
		FORCE_INLINE t_dst
		cvt_to_impl(const float& f, cvt_unorm_tag) noexcept
		{
			return static_cast<t_dst>(
				std::clamp(f, 0.f, 1.f) * std::numeric_limits<t_dst>::max() + 0.5f);
		}

		template <typename t_dst>
		requires meta::variadic_contains_v<t_dst, int8, int16>
		FORCE_INLINE t_dst
		cvt_to_impl(const float& f, cvt_snorm_tag) noexcept
		{
			return static_cast<t_dst>(
				std::clamp(f, -1.f, 1.f) * (std::numeric_limits<t_dst>::max()) + std::copysignf(0.5f, f));
		}

		template <typename t_dst>
		requires std::is_same_v<t_dst, half>
		FORCE_INLINE half
		cvt_to_impl(const float& f, cvt_cast_tag) noexcept
		{
			return DirectX::PackedVector::XMConvertFloatToHalf(f);
		}

		template <typename t_dst = float, typename t_src>
		requires meta::variadic_contains_v<t_src, uint8, uint16>
			 and meta::variadic_contains_v<t_dst, float, double, half>
		FORCE_INLINE t_dst
		cvt_to_impl(const t_src& u, cvt_unorm_tag) noexcept
		{
			if constexpr (std::is_same_v<t_dst, half>)
			{
				cvt_to_impl<half>(static_cast<float>(u) / std::numeric_limits<t_src>::max());
			}
			else
			{
				return static_cast<t_dst>(u) / std::numeric_limits<t_src>::max();
			}
		}

		template <typename t_dst = float, typename t_src>
		requires meta::variadic_contains_v<t_src, int8, int16>
			 and meta::variadic_contains_v<t_dst, float, double, half>
		FORCE_INLINE float
		cvt_to_impl(const t_src& i, cvt_snorm_tag) noexcept
		{
			if constexpr (std::is_same_v<t_dst, half>)
			{
				cvt_to_impl<half>(static_cast<float>(i) / std::numeric_limits<t_src>::max());
			}
			else
			{
				return static_cast<t_dst>(i) / std::numeric_limits<t_src>::max();
			}
		}

		template <typename t_dst = float>
		requires meta::variadic_contains_v<t_dst, float, double, half>
		FORCE_INLINE t_dst
		cvt_to_impl(const half& h, cvt_cast_tag) noexcept
		{
			if constexpr (std::is_same_v<t_dst, float>)
			{
				return DirectX::PackedVector::XMConvertHalfToFloat(h);
			}
			else if constexpr (std::is_same_v<t_dst, double>)
			{
				return static_cast<double>(DirectX::PackedVector::XMConvertHalfToFloat(h));
			}
			else
			{
				return h;
			}
		}

		template <typename t_dst, template <typename> typename t_vec, typename t_src>
		requires(std::is_same_v<t_src, float> and std::is_same_v<t_dst, t_vec<half>>)
			 or (std::is_same_v<t_src, half> and std::is_same_v<t_dst, t_vec<float>>)
		FORCE_INLINE t_dst
		cvt_to_impl(const t_vec<t_src>& src, cvt_cast_tag) noexcept
		{
			return simd::to<t_dst>(simd::load(src));
		}

		template <typename t_dst>
		requires std::is_same_v<t_dst, oct<uint8>>
		FORCE_INLINE t_dst
		cvt_to_impl(const float3& f, cvt_cast_tag) noexcept
		{
			auto l1_norm = std::abs(f.x) + std::abs(f.y) + std::abs(f.z);
			auto x		 = f.x / l1_norm;
			auto y		 = f.y / l1_norm;
			if (f.z < 0.f)
			{
				auto old_x = x;
				auto old_y = y;

				x = std::copysignf(1.f - std::abs(old_y), old_x);
				y = std::copysignf(1.f - std::abs(old_x), old_y);
			}

			return t_dst{
				.x = cvt_to_impl<typename t_dst::t_value>(0.5f + x * 0.5f, cvt_unorm_tag{}),
				.y = cvt_to_impl<typename t_dst::t_value>(0.5f + y * 0.5f, cvt_unorm_tag{})
			};
		}

		template <typename t_dst>
		requires std::is_same_v<t_dst, oct<int8>>
		FORCE_INLINE t_dst
		cvt_to_impl(const float3& f, cvt_cast_tag) noexcept
		{
			auto l1_norm = std::abs(f.x) + std::abs(f.y) + std::abs(f.z);
			auto x		 = f.x / l1_norm;
			auto y		 = f.y / l1_norm;
			if (f.z < 0.f)
			{
				auto old_x = x;
				auto old_y = y;

				x = std::copysignf(1.f - std::abs(old_y), old_x);
				y = std::copysignf(1.f - std::abs(old_x), old_y);
			}

			return t_dst{
				.x = cvt_to_impl<typename t_dst::t_value>(x, cvt_snorm_tag{}),
				.y = cvt_to_impl<typename t_dst::t_value>(y, cvt_snorm_tag{})
			};
		}

		template <typename t_dst, typename t_src>
		requires std::is_same_v<t_dst, float3>
			 and meta::variadic_contains_v<t_src, uint8, uint16>
		FORCE_INLINE float3
		cvt_to_impl(const oct<t_src>& o, cvt_cast_tag) noexcept
		{
			auto res = float3(
				cvt_to_impl<float>(o.x, cvt_unorm_tag{}) * 2.f - 1.f,
				cvt_to_impl<float>(o.y, cvt_unorm_tag{}) * 2.f - 1.f,
				0.f);
			res.z = 1.f - std::abs(res.x) - std::abs(res.y);


			if (res.z < 0.f)
			{
				auto old_x = res.x;
				auto old_y = res.y;

				res.x = std::copysignf(1.f - std::abs(old_y), old_x);
				res.y = std::copysignf(1.f - std::abs(old_x), old_y);
			}

			return simd::load(res)
				 | simd::normalize3()
				 | simd::to<float3>();
		}

		template <typename t_dst, typename t_src>
		requires std::is_same_v<t_dst, float3>
			 and meta::variadic_contains_v<t_src, int8, int16>
		FORCE_INLINE float3
		cvt_to_impl(const oct<t_src>& o, cvt_cast_tag) noexcept
		{
			auto res = float3(
				cvt_to_impl(o.x, cvt_snorm_tag{}),
				cvt_to_impl(o.y, cvt_snorm_tag{}),
				0.f);
			res.z = 1.f - std::abs(res.x) - std::abs(res.y);


			if (res.z < 0.f)
			{
				auto old_x = res.x;
				auto old_y = res.y;

				res.x = std::copysignf(1.f - std::abs(old_y), old_x);
				res.y = std::copysignf(1.f - std::abs(old_x), old_y);
			}

			return simd::load(res)
				 | simd::normalize3()
				 | simd::to<float3>();
		}

		template <typename t_dst, typename t_tag>
		requires meta::variadic_contains_v<t_dst, vec3<uint8>, vec3<uint16>, vec3<int8>, vec3<int16>>
			 and meta::variadic_contains_v<t_tag, cvt_unorm_tag, cvt_snorm_tag>
		FORCE_INLINE t_dst
		cvt_to_impl(const float3& f3, t_tag) noexcept
		{
			return t_dst{
				cvt_to_impl<typename t_dst::t_value>(f3.x, t_tag{}),
				cvt_to_impl<typename t_dst::t_value>(f3.y, t_tag{}),
				cvt_to_impl<typename t_dst::t_value>(f3.z, t_tag{})
			};
		}
	}	 // namespace detail

	namespace detail
	{
		template <typename t_src, typename t_dst, typename t_tag = cvt_cast_tag>
		requires cx_cvt_tag<t_tag>
		FORCE_INLINE void
		cvt_to_stream_impl(const t_src* p_src, t_dst* p_dst, std::size_t n, t_tag)
		{
			static_assert(std::is_same_v<t_src, t_dst>, "cvt_to_impl: unsupported conversion");
		}

		template <>
		FORCE_INLINE void
		cvt_to_stream_impl(const half* p_src, float* p_dst, std::size_t n, cvt_cast_tag)
		{
			DirectX::PackedVector::XMConvertHalfToFloatStream(p_dst, sizeof(float), p_src, sizeof(half), n);
		}

		template <>
		FORCE_INLINE void
		cvt_to_stream_impl(const float* p_src, half* p_dst, std::size_t n, cvt_cast_tag)
		{
			DirectX::PackedVector::XMConvertFloatToHalfStream(p_dst, sizeof(half), p_src, sizeof(float), n);
		}
	}	 // namespace detail

	template <typename t_dst, typename t_src, typename t_tag = cvt_cast_tag>
	requires cx_cvt_tag<t_tag>
	FORCE_INLINE decltype(auto)
	cvt_to(const t_src& src, t_tag tag = {}) noexcept
	{
		return age::math::detail::cvt_to_impl<t_dst>(src, tag);
	}

	template <typename t_src, typename t_dst, typename t_tag = cvt_cast_tag>
	requires cx_cvt_tag<t_tag>
	FORCE_INLINE decltype(auto)
	cvt_to(const t_src* p_src, t_dst* p_dst, std::size_t n, t_tag tag = {})
	{
		return math::detail::cvt_to_stream_impl(p_src, p_dst, n, tag);
	}
}	 // namespace age::math

namespace age::math::simd
{
	FORCE_INLINE
	xm_vec XM_CALLCONV
	mul_add(fxm_vec v0,
			fxm_vec v1,
			fxm_vec v2) noexcept
	{
		return DirectX::XMVectorMultiplyAdd(v0, v1, v2);
	}
}	 // namespace age::math::simd

namespace age::math::simd
{
	FORCE_INLINE
	xm_vec XM_CALLCONV
	clamp(fxm_vec v,
		  fxm_vec v_min = DirectX::g_XMNegativeOne.v,
		  fxm_vec v_max = DirectX::g_XMOne.v) noexcept
	{
		return DirectX::XMVectorClamp(v, v_min, v_max);
	}
}	 // namespace age::math::simd

namespace age::math::simd
{
	FORCE_INLINE
	xm_vec XM_CALLCONV
	angle_between_vec3(fxm_vec v0, fxm_vec v1) noexcept
	{
		//[0,pi]
		return DirectX::XMVector3AngleBetweenVectors(v0, v1);
	}
}	 // namespace age::math::simd

namespace age::math::simd
{
	template <typename t>
	FORCE_INLINE bool XM_CALLCONV
	is_orthogonal_basis(t& mat) noexcept
	{
		using namespace DirectX;
		auto xm_mat = load(mat);
		xm_mat.r[3] = g_XMIdentityR3;

		const auto xm_mat_trans	   = XMMatrixTranspose(xm_mat);
		const auto xm_mat_prod	   = XMMatrixMultiply(xm_mat, xm_mat_trans);
		const auto xm_mat_identity = XMMatrixIdentity();
		const auto xm_vec_epsilon  = XMVectorReplicate(std::numeric_limits<float>::epsilon());
		return XMVector4NearEqual(xm_mat_prod.r[0], xm_mat_identity.r[0], xm_vec_epsilon)
		   and XMVector4NearEqual(xm_mat_prod.r[1], xm_mat_identity.r[1], xm_vec_epsilon)
		   and XMVector4NearEqual(xm_mat_prod.r[2], xm_mat_identity.r[2], xm_vec_epsilon)
		   and XMVector4NearEqual(xm_mat_prod.r[3], xm_mat_identity.r[3], xm_vec_epsilon);
	}
}	 // namespace age::math::simd

namespace age::math::simd
{
	FORCE_INLINE float3 XM_CALLCONV
	calc_plane_normal_normalized(const auto& p0, const auto& p1, const auto& p2)
	{
		auto&& [v0, v1, v2] = simd::load(p0, p1, p2);
		return (v2 - v0)
			 | simd::cross3(v1 - v0)
			 | simd::normalize3()
			 | simd::to<float3>();
	}

	FORCE_INLINE float3 XM_CALLCONV
	transform_basis(const float3& old, const float3x3& old_basis, const float3x3& new_basis)
	{
		auto&& [v_old, m_basis_old, m_basis_new] = simd::load(old, old_basis, new_basis);
		return v_old
			 | simd::setw(1.f)
			 | simd::transform_coord3(m_basis_old)
			 | simd::transform_coord3(DirectX::XMMatrixInverse(nullptr, m_basis_new))
			 | simd::to<float3>();
	}
}	 // namespace age::math::simd

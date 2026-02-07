#pragma once

// store, load
namespace age::math::simd
{
	FORCE_INLINE void XM_CALLCONV
	store(float2& out, DirectX::FXMVECTOR v) noexcept
	{
		DirectX::XMStoreFloat2(reinterpret_cast<DirectX::XMFLOAT2*>(&out), v);
	}

	FORCE_INLINE void XM_CALLCONV
	store(float3& out, DirectX::FXMVECTOR v) noexcept
	{
		DirectX::XMStoreFloat3(reinterpret_cast<DirectX::XMFLOAT3*>(&out), v);
	}

	FORCE_INLINE void XM_CALLCONV
	store(float4& out, DirectX::FXMVECTOR v) noexcept
	{
		DirectX::XMStoreFloat4(reinterpret_cast<DirectX::XMFLOAT4*>(&out), v);
	}

	FORCE_INLINE void XM_CALLCONV
	store(float2a& out, DirectX::FXMVECTOR v) noexcept
	{
		DirectX::XMStoreFloat2A(reinterpret_cast<DirectX::XMFLOAT2A*>(&out), v);
	}

	FORCE_INLINE void XM_CALLCONV
	store(float3a& out, DirectX::FXMVECTOR v) noexcept
	{
		DirectX::XMStoreFloat3A(reinterpret_cast<DirectX::XMFLOAT3A*>(&out), v);
	}

	FORCE_INLINE void XM_CALLCONV
	store(float4a& out, DirectX::FXMVECTOR v) noexcept
	{
		DirectX::XMStoreFloat4A(reinterpret_cast<DirectX::XMFLOAT4A*>(&out), v);
	}

	template <typename t>
	FORCE_INLINE t XM_CALLCONV
	to(DirectX::FXMVECTOR v) noexcept
	{
		t res{};
		store(res, v);
		return res;
	}

	FORCE_INLINE
	DirectX::XMVECTOR XM_CALLCONV
	load(const float2& vec) noexcept
	{
		return DirectX::XMLoadFloat2(reinterpret_cast<DirectX::XMFLOAT2*>((void*)vec.data()));
	}

	FORCE_INLINE
	DirectX::XMVECTOR XM_CALLCONV
	load(const float3& vec) noexcept
	{
		return DirectX::XMLoadFloat3(reinterpret_cast<DirectX::XMFLOAT3*>((void*)vec.data()));
	}

	FORCE_INLINE
	DirectX::XMVECTOR XM_CALLCONV
	load(const float4& vec) noexcept
	{
		return DirectX::XMLoadFloat4(reinterpret_cast<DirectX::XMFLOAT4*>((void*)vec.data()));
	}

	FORCE_INLINE
	DirectX::XMVECTOR XM_CALLCONV
	load(const float2a& vec) noexcept
	{
		return DirectX::XMLoadFloat2A(reinterpret_cast<DirectX::XMFLOAT2A*>((void*)vec.data()));
	}

	FORCE_INLINE
	DirectX::XMVECTOR XM_CALLCONV
	load(const float3a& vec) noexcept
	{
		return DirectX::XMLoadFloat3A(reinterpret_cast<DirectX::XMFLOAT3A*>((void*)vec.data()));
	}

	FORCE_INLINE
	DirectX::XMVECTOR XM_CALLCONV
	load(const float4a& vec) noexcept
	{
		return DirectX::XMLoadFloat4A(reinterpret_cast<DirectX::XMFLOAT4A*>((void*)vec.data()));
	}

	FORCE_INLINE
	DirectX::XMMATRIX XM_CALLCONV
	load(const float3x3& mat) noexcept
	{
		return DirectX::XMLoadFloat3x3(reinterpret_cast<DirectX::XMFLOAT3X3*>((void*)mat.data()));
	}

	FORCE_INLINE
	DirectX::XMMATRIX XM_CALLCONV
	load(const float3x3a& mat) noexcept
	{
		return DirectX::XMLoadFloat3x3(reinterpret_cast<DirectX::XMFLOAT3X3*>((void*)mat.data()));
	}

	FORCE_INLINE
	DirectX::XMMATRIX XM_CALLCONV
	load(const float4x4& mat) noexcept
	{
		return DirectX::XMLoadFloat4x4(reinterpret_cast<DirectX::XMFLOAT4X4*>((void*)mat.data()));
	}

	FORCE_INLINE
	DirectX::XMMATRIX XM_CALLCONV
	load(const float4x4a& mat) noexcept
	{
		return DirectX::XMLoadFloat4x4(reinterpret_cast<DirectX::XMFLOAT4X4A*>((void*)mat.data()));
	}

	FORCE_INLINE
	DirectX::XMVECTOR XM_CALLCONV
	load_vec_replicate(const float& f) noexcept
	{
		return DirectX::XMVectorReplicate(f);
	}

	FORCE_INLINE
	decltype(auto) XM_CALLCONV
	load(auto&&... arg) noexcept
		requires(sizeof...(arg) > 1)
	{
		return std::array{
			age::math::simd::load(FWD(arg))...
		};
	}
}	 // namespace age::math::simd

// access
namespace age::math::simd
{
	FORCE_INLINE
	float XM_CALLCONV
	get_x(DirectX::XMVECTOR v) noexcept
	{
		return DirectX::XMVectorGetX(v);
	}
}	 // namespace age::math::simd

// compare
namespace age::math::simd
{
	FORCE_INLINE
	DirectX::XMVECTOR XM_CALLCONV
	cmp_less(DirectX::XMVECTOR v0, DirectX::XMVECTOR v1) noexcept
	{
		return DirectX::XMVectorLess(v0, v1);
	}

	FORCE_INLINE
	DirectX::XMVECTOR XM_CALLCONV
	cmp_greater_equal(DirectX::XMVECTOR v0, DirectX::XMVECTOR v1) noexcept
	{
		return DirectX::XMVectorGreaterOrEqual(v0, v1);
	}

	FORCE_INLINE
	decltype(auto) XM_CALLCONV
	cmp_greater_equal_rmask(DirectX::XMVECTOR v0, DirectX::XMVECTOR v1) noexcept
	{
		auto rmask	= uint32{};
		auto xm_vec = DirectX::XMVectorGreaterOrEqualR(&rmask, v0, v1);
		return std::tuple{ rmask, xm_vec };
	}
}	 // namespace age::math::simd

namespace age::math::simd
{
	FORCE_INLINE
	DirectX::XMVECTOR XM_CALLCONV
	normalize3(DirectX::FXMVECTOR v) noexcept
	{
		return DirectX::XMVector3Normalize(v);
	}

	FORCE_INLINE
	DirectX::XMVECTOR XM_CALLCONV
	reciprocal_length3(DirectX::FXMVECTOR v) noexcept
	{
		return DirectX::XMVector3ReciprocalLength(v);
	}

	FORCE_INLINE
	DirectX::XMVECTOR XM_CALLCONV
	reciprocal_length_est3(DirectX::FXMVECTOR v) noexcept
	{
		return DirectX::XMVector3ReciprocalLengthEst(v);
	}

	FORCE_INLINE
	DirectX::XMVECTOR XM_CALLCONV
	acos(DirectX::FXMVECTOR v) noexcept
	{
		return DirectX::XMVectorACos(v);
	}

	FORCE_INLINE
	DirectX::XMVECTOR XM_CALLCONV
	acos_esc(DirectX::FXMVECTOR v) noexcept
	{
		return DirectX::XMVectorACosEst(v);
	}
}	 // namespace age::math::simd

namespace age::math::simd
{
	FORCE_INLINE
	DirectX::XMVECTOR XM_CALLCONV
	dot3(DirectX::FXMVECTOR v0, DirectX::FXMVECTOR v1) noexcept
	{
		return DirectX::XMVector3Dot(v0, v1);
	}

	FORCE_INLINE
	DirectX::XMVECTOR XM_CALLCONV
	cross3(DirectX::FXMVECTOR v0, DirectX::FXMVECTOR v1) noexcept
	{
		return DirectX::XMVector3Cross(v0, v1);
	}

	FORCE_INLINE
	DirectX::XMVECTOR XM_CALLCONV
	mul3(DirectX::FXMVECTOR v0, DirectX::FXMVECTOR v1) noexcept
	{
		return DirectX::XMVectorMultiply(v0, v1);
	}

	FORCE_INLINE
	DirectX::XMVECTOR XM_CALLCONV
	sub(DirectX::FXMVECTOR v0, DirectX::FXMVECTOR v1) noexcept
	{
		return DirectX::XMVectorSubtract(v0, v1);
	}
}	 // namespace age::math::simd

namespace age::math::simd
{
	FORCE_INLINE
	DirectX::XMVECTOR XM_CALLCONV
	mul_add(DirectX::FXMVECTOR v0,
			DirectX::FXMVECTOR v1,
			DirectX::FXMVECTOR v2) noexcept
	{
		return DirectX::XMVectorMultiplyAdd(v0, v1, v2);
	}
}	 // namespace age::math::simd

namespace age::math::simd
{
	FORCE_INLINE
	DirectX::XMVECTOR XM_CALLCONV
	clamp(DirectX::FXMVECTOR v,
		  DirectX::FXMVECTOR v_min = DirectX::g_XMNegativeOne.v,
		  DirectX::FXMVECTOR v_max = DirectX::g_XMOne.v) noexcept
	{
		return DirectX::XMVectorClamp(v, v_min, v_max);
	}
}	 // namespace age::math::simd

namespace age::math::simd
{
	FORCE_INLINE
	DirectX::XMVECTOR XM_CALLCONV
	angle_between_vec3(DirectX::FXMVECTOR v0, DirectX::FXMVECTOR v1) noexcept
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
		using namespace DirectX;
		auto	   res = float3{};
		const auto v0  = load(p0);
		const auto v1  = load(p1);
		const auto v2  = load(p2);

		store(&res,
			  XMVector3Normalize(
				  XMVector3Cross(
					  XMVectorSubtract(v1, v0),
					  XMVectorSubtract(v2, v0))));
		return res;
	}

	FORCE_INLINE float3 XM_CALLCONV
	transform_basis(const float3& old, const float3x3& old_basis, const float3x3& new_basis)
	{
		const auto m_basis_old = load(old_basis);
		const auto m_basis_new = load(new_basis);

		const auto v_pos_old = DirectX::XMVectorSet(old.x, old.y, old.z, 1.f);

		const auto v_pos_world = DirectX::XMVector3TransformCoord(v_pos_old, m_basis_old);

		const auto m_inv_to	 = DirectX::XMMatrixInverse(nullptr, m_basis_new);
		const auto v_pos_new = DirectX::XMVector3TransformCoord(v_pos_world, m_inv_to);

		return simd::to<float3>(v_pos_new);
	}
}	 // namespace age::math::simd

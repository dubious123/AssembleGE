#pragma once
#include "age.hpp"

namespace age::asset::detail
{
	template <std::size_t uv_set_count>
	struct alignas(4) vertex_p_uv
	{
		uint16_3						pos{};
		uint16							extra{};
		std::array<half2, uv_set_count> uv_set{};
	};

	template <std::size_t uv_set_count>
	struct alignas(4) vertex_pn_uv
	{
		uint16_3						pos{};
		oct<int8>						normal_oct{};
		std::array<half2, uv_set_count> uv_set{};
	};

	template <std::size_t uv_set_count>
	struct alignas(4) vertex_pnt_uv
	{
		uint16_3  pos{};
		oct<int8> normal_oct{};

		oct<int8> tangent_oct{};
		uint8_2	  extra{};

		std::array<half2, uv_set_count> uv_set{};
	};

	template <>
	struct alignas(4) vertex_p_uv<0>
	{
		uint16_3 pos{};
		uint16	 extra{};
	};

	template <>
	struct alignas(4) vertex_pn_uv<0>
	{
		uint16_3  pos{};
		oct<int8> normal_oct{};
	};

	template <>
	struct alignas(4) vertex_pnt_uv<0>
	{
		uint16_3  pos{};
		oct<int8> normal_oct{};

		oct<int8> tangent_oct{};
		uint16	  extra{};	  // first bit is tangent handedness;
	};

	template <std::size_t uv_set_count>
	consteval bool
	validate()
	{
		static_assert(sizeof(vertex_p_uv<uv_set_count>)
						  == sizeof(vertex_p_uv<uv_set_count>{}.pos)
								 + sizeof(vertex_p_uv<uv_set_count>{}.extra)
								 + sizeof(half2) * uv_set_count,
					  "vertex_p_uv<uv_set_count> failed");

		static_assert(sizeof(vertex_pn_uv<uv_set_count>)
						  == sizeof(vertex_pn_uv<uv_set_count>{}.pos)
								 + sizeof(vertex_pn_uv<uv_set_count>{}.normal_oct)
								 + sizeof(half2) * uv_set_count,
					  "vertex_pn_uv<uv_set_count> failed");

		static_assert(sizeof(vertex_pnt_uv<uv_set_count>)
						  == sizeof(vertex_pnt_uv<uv_set_count>{}.pos)
								 + sizeof(vertex_pnt_uv<uv_set_count>{}.normal_oct)
								 + sizeof(vertex_pnt_uv<uv_set_count>{}.tangent_oct)
								 + sizeof(vertex_pnt_uv<uv_set_count>{}.extra)
								 + sizeof(half2) * uv_set_count,
					  "vertex_pnt_uv<uv_set_count> failed");
		return true;
	}

	static_assert(
		validate<0>()
		and validate<1>()
		and validate<2>()
		and validate<3>()
		and validate<4>());

	template <typename t>
	struct uv_count;

	template <template <auto> typename t, auto n>
	struct uv_count<t<n>>
	{
		static constexpr auto value = n;
	};

	template <typename t>

	inline constexpr auto uv_count_v = uv_count<t>::value;
}	 // namespace age::asset::detail

namespace age::asset
{
	using vertex_p	   = detail::vertex_p_uv<0>;
	using vertex_p_uv0 = detail::vertex_p_uv<1>;
	using vertex_p_uv1 = detail::vertex_p_uv<2>;
	using vertex_p_uv2 = detail::vertex_p_uv<3>;
	using vertex_p_uv3 = detail::vertex_p_uv<4>;

	using vertex_pn		= detail::vertex_pn_uv<0>;
	using vertex_pn_uv0 = detail::vertex_pn_uv<1>;
	using vertex_pn_uv1 = detail::vertex_pn_uv<2>;
	using vertex_pn_uv2 = detail::vertex_pn_uv<3>;
	using vertex_pn_uv3 = detail::vertex_pn_uv<4>;

	using vertex_pnt	 = detail::vertex_pnt_uv<0>;
	using vertex_pnt_uv0 = detail::vertex_pnt_uv<1>;
	using vertex_pnt_uv1 = detail::vertex_pnt_uv<2>;
	using vertex_pnt_uv2 = detail::vertex_pnt_uv<3>;
	using vertex_pnt_uv3 = detail::vertex_pnt_uv<4>;

	template <typename t_vertex>
	constexpr e::vertex_kind
	get_vertex_kind() noexcept
	{
		if constexpr (std::same_as<t_vertex, vertex_p_uv0>) { return e::vertex_kind::p_uv0; }
		else if constexpr (std::same_as<t_vertex, vertex_pn_uv0>) { return e::vertex_kind::pn_uv0; }
		else if constexpr (std::same_as<t_vertex, vertex_pnt_uv0>) { return e::vertex_kind::pnt_uv0; }
		else if constexpr (std::same_as<t_vertex, vertex_p_uv1>) { return e::vertex_kind::p_uv1; }
		else if constexpr (std::same_as<t_vertex, vertex_pn_uv1>) { return e::vertex_kind::pn_uv1; }
		else if constexpr (std::same_as<t_vertex, vertex_pnt_uv1>) { return e::vertex_kind::pnt_uv1; }
		else if constexpr (std::same_as<t_vertex, vertex_p_uv2>) { return e::vertex_kind::p_uv2; }
		else if constexpr (std::same_as<t_vertex, vertex_pn_uv2>) { return e::vertex_kind::pn_uv2; }
		else if constexpr (std::same_as<t_vertex, vertex_pnt_uv2>) { return e::vertex_kind::pnt_uv2; }
		else if constexpr (std::same_as<t_vertex, vertex_p_uv3>) { return e::vertex_kind::p_uv3; }
		else if constexpr (std::same_as<t_vertex, vertex_pn_uv3>) { return e::vertex_kind::pn_uv3; }
		else if constexpr (std::same_as<t_vertex, vertex_pnt_uv3>) { return e::vertex_kind::pnt_uv3; }
		else
		{
			static_assert(false, "invalid type");
			AGE_UNREACHABLE();
			return -1;
		}
	}

	namespace detail
	{
		template <e::vertex_kind e_kind>
		consteval auto
		get_vetex_type_helper() noexcept
		{
			if constexpr (e_kind == e::vertex_kind::p_uv0) { return vertex_p_uv0{}; }
			else if constexpr (e_kind == e::vertex_kind::pn_uv0) { return vertex_pn_uv0{}; }
			else if constexpr (e_kind == e::vertex_kind::pnt_uv0) { return vertex_pnt_uv0{}; }

			else if constexpr (e_kind == e::vertex_kind::p_uv1) { return vertex_p_uv1{}; }
			else if constexpr (e_kind == e::vertex_kind::pn_uv1) { return vertex_pn_uv1{}; }
			else if constexpr (e_kind == e::vertex_kind::pnt_uv1) { return vertex_pnt_uv1{}; }

			else if constexpr (e_kind == e::vertex_kind::p_uv2) { return vertex_p_uv2{}; }
			else if constexpr (e_kind == e::vertex_kind::pn_uv2) { return vertex_pn_uv2{}; }
			else if constexpr (e_kind == e::vertex_kind::pnt_uv2) { return vertex_pnt_uv2{}; }

			else if constexpr (e_kind == e::vertex_kind::p_uv3) { return vertex_p_uv3{}; }
			else if constexpr (e_kind == e::vertex_kind::pn_uv3) { return vertex_pn_uv3{}; }
			else if constexpr (e_kind == e::vertex_kind::pnt_uv3) { return vertex_pnt_uv3{}; }
			else
			{
				static_assert(false);
			}
		}
	}	 // namespace detail

	template <e::vertex_kind e_kind>
	using t_vertex_kind = BARE_OF(detail::get_vetex_type_helper<e_kind>());

	template <typename t>
	concept cx_baked_vertex = meta::is_specialization_of_nttp_v<t, detail::vertex_p_uv>
						   or meta::is_specialization_of_nttp_v<t, detail::vertex_pn_uv>
						   or meta::is_specialization_of_nttp_v<t, detail::vertex_pnt_uv>;
}	 // namespace age::asset

namespace age::asset
{
	template <typename t_vertex_to>
	FORCE_INLINE t_vertex_to
	cvt_vertex_to(const vertex_fat& v_fat, const float3& aabb_min, const float3& aabb_size) noexcept
	{
		constexpr auto uv_count = detail::uv_count_v<t_vertex_to>;

		auto res = t_vertex_to{};

		auto aabb_size_inv = float3::one() / aabb_size;

		res.pos = age::cvt_to<uint16_3>((v_fat.pos - aabb_min) * aabb_size_inv, age::cvt_unorm_tag{});
		auto p	= res.pos;
		if constexpr (uv_count > 0)
		{
			age::cvt_to(v_fat.uv_set[0].data(), res.uv_set[0].data(), uv_count * 2);
		}

		if constexpr (meta::is_specialization_of_nttp_v<t_vertex_to, detail::vertex_p_uv>)
		{
		}
		else if constexpr (meta::is_specialization_of_nttp_v<t_vertex_to, detail::vertex_pn_uv>)
		{
			res.normal_oct = age::cvt_to<oct<int8>>(v_fat.normal, age::cvt_cast_tag{});
		}
		else if constexpr (meta::is_specialization_of_nttp_v<t_vertex_to, detail::vertex_pnt_uv>)
		{
			res.normal_oct	= age::cvt_to<oct<int8>>(v_fat.normal, age::cvt_cast_tag{});
			res.tangent_oct = age::cvt_to<oct<int8>>(v_fat.tangent.xyz, age::cvt_cast_tag{});
			res.extra		= v_fat.tangent.w > 0.f ? 1 : 0;
		}
		else
		{
			static_assert(false, "invalid type conversion");
		}

		return res;
	}

	template <e::vertex_kind e_kind>
	FORCE_INLINE decltype(auto)
	cvt_vertex_to(const vertex_fat& v_fat, const float3& aabb_min, const float3& aabb_size) noexcept
	{
		return cvt_vertex_to<t_vertex_kind<e_kind>>(v_fat, aabb_min, aabb_size);
	}

	template <typename t_vertex_to>
	requires std::is_same_v<t_vertex_to, vertex_fat>
	FORCE_INLINE vertex_fat
	cvt_vertex_to(const mesh_editable& mesh_edit, const mesh_editable::vertex& v) noexcept
	{
		return {
			.pos	 = mesh_edit.position_vec[v.pos_idx],
			.normal	 = mesh_edit.vertex_attr_vec[v.attribute_idx].normal,
			.tangent = mesh_edit.vertex_attr_vec[v.attribute_idx].tangent,
			.uv_set	 = mesh_edit.vertex_attr_vec[v.attribute_idx].uv_set
		};
	}

	template <typename t_vertex_to, cx_baked_vertex t_vertex_baked>
	requires std::is_same_v<t_vertex_to, vertex_fat>
	FORCE_INLINE vertex_fat
	cvt_vertex_to(const t_vertex_baked& v, const float3& aabb_min, const float3& aabb_size) noexcept
	{
		constexpr auto uv_count = detail::uv_count_v<t_vertex_baked>;

		auto res = vertex_fat{};

		res.pos = float3{ static_cast<float>(v.pos.x), static_cast<float>(v.pos.y), static_cast<float>(v.pos.z) } / 65535.f * aabb_size + aabb_min;

		if constexpr (uv_count > 0)
		{
			age::cvt_to(v.uv_set[0].data(), res.uv_set[0].data(), uv_count * 2);
		}

		if constexpr (meta::is_specialization_of_nttp_v<t_vertex_baked, detail::vertex_p_uv>)
		{
		}
		else if constexpr (meta::is_specialization_of_nttp_v<t_vertex_baked, detail::vertex_pn_uv>)
		{
			res.normal = age::cvt_to<float3>(v.normal_oct, age::cvt_cast_tag{});
		}
		else if constexpr (meta::is_specialization_of_nttp_v<t_vertex_baked, detail::vertex_pnt_uv>)
		{
			res.normal		= age::cvt_to<float3>(v.normal_oct, age::cvt_cast_tag{});
			res.tangent.xyz = age::cvt_to<float3>(v.tangent_oct, age::cvt_cast_tag{});
			res.tangent.w	= 2.0f * float(v.extra.x & 1u) - 1.f;
		}
		else
		{
			static_assert(false, "invalid type conversion");
		}


		return res;
	}

	template <typename t_vertex_to>
	mesh_triangulated<t_vertex_to>
	triangulate(const mesh_editable& m) noexcept
	{
		return {
			.vertex_vec = m.vertex_vec
						| std::views::transform([&m](auto& v) { return cvt_vertex_to<t_vertex_to>(m, v); })
						| std::ranges::to<age::vector<t_vertex_to>>(),
			.v_idx_vec = external::earcut::perform(m)
		};
	}
}	 // namespace age::asset

namespace age::asset
{
	struct lod_group_baked_header
	{
		uint8 lod_count{};
	};

	struct lod_group_baked
	{
		lod_group_baked_header header{};

		age::asset::handle h_asset{};
	};

	lod_group_baked
	load_lod_group_baked(const std::string_view& file_name) noexcept;

	void
	upload_lod_group_baked(const lod_group_baked&) noexcept;

	void
	debug_validate(const lod_group_baked&) noexcept;
}	 // namespace age::asset
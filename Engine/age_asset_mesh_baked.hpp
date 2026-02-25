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
	e::vertex_kind
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
}	 // namespace age::asset

namespace age::asset
{
	template <typename t_vertex_to>
	FORCE_INLINE t_vertex_to
	cvt_vertex_to(const vertex_fat& v_fat, float3 aabb_min, float3 aabb_size) noexcept
	{
		constexpr auto uv_count = detail::uv_count_v<t_vertex_to>;

		auto res = t_vertex_to{};

		auto aabb_size_inv = float3::one() / aabb_size;

		res.pos = age::cvt_to<uint16_3>((v_fat.pos - aabb_min) * aabb_size_inv, age::cvt_unorm_tag{});
		auto p	= res.pos;
		if constexpr (uv_count > 0)
		{
			age::cvt_to(
				v_fat.uv_set[0].data(),
				reinterpret_cast<half*>(res.uv_set.data() + uv_count), uv_count);
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
		}
		else
		{
			static_assert(false, "invalid type conversion");
		}

		return res;
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
	struct mesh_baked_header
	{
		// uint32 vertex_offset = sizeof(mesh_baked_header)
		uint32 global_vertex_index_buffer_offset;
		uint32 local_vertex_index_buffer_offset;
		uint32 meshlet_header_buffer_offset;
		uint32 meshlet_buffer_offset;
		uint32 meshlet_count;
		float3 aabb_min;
		float3 aabb_size;
	};

	// [mesh_baked_header]
	// [vertex_buffer]
	// [global_vertex_index_buffer]
	// [local_vertex_index_buffer]
	// [meshlet_header_buffer]
	// [meshlet_buffer]
	struct mesh_baked
	{
		age::dynamic_array<std::byte> buffer{};

		mesh_baked_header
		get_header() const noexcept
		{
			AGE_ASSERT(buffer.size() >= sizeof(mesh_baked_header));

			auto res = mesh_baked_header{};
			std::memcpy(&res, buffer.data(), sizeof(mesh_baked_header));
			return res;
		}
	};

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

namespace age::asset
{
	template <typename t_vertex>
	mesh_baked
	bake_mesh(const asset::mesh_editable& mesh_edit) noexcept
	{
		auto m = asset::triangulate<vertex_fat>(mesh_edit);
		AGE_ASSERT(m.v_idx_vec.size() % 3 == 0);
		for (auto [nth, idx] : m.v_idx_vec | std::views::enumerate)
		{
			AGE_ASSERT(idx < m.vertex_vec.size());
		}

		auto&& [index_buffer, vertex_buffer] = external::meshopt::gen_remap(m.v_idx_vec, m.vertex_vec);

		external::meshopt::opt_reorder_buffers(index_buffer, vertex_buffer);

		auto&& [meshlet_global_index_buffer, meshlet_local_index_buffer, meshlet_header_vec, meshlet_vec] =
			external::meshopt::gen_meshlets(
				index_buffer,
				vertex_buffer);

		// print meshlets

		// for (auto&& [mshlt_idx, mshlt] : meshlet_vec | std::views::enumerate)
		//{
		//	std::println("meshlet[{}] begin", mshlt_idx);
		//	for (auto&& [idx, v] : std::views::iota(mshlt.global_index_offset)
		//							   | std::views::take(mshlt.vertex_count)
		//							   | std::views::transform([&meshlet_global_index_buffer](auto i) { return meshlet_global_index_buffer[i]; })
		//							   | std::views::transform([&vertex_buffer](auto i) -> decltype(auto) { return vertex_buffer[i]; })
		//							   | std::views::enumerate)
		//	{
		//		std::println("vertex[{}] : [{}, {}, {}]", idx, v.pos.x, v.pos.y, v.pos.z);
		//	}

		//	for (auto&& [idx, tpl] : std::views::iota(0)
		//								 | std::views::take(mshlt.primitive_count)
		//								 | std::views::transform([&meshlet_local_index_buffer, &mshlt](auto i) { return uint32_3{ meshlet_local_index_buffer[mshlt.primitive_offset + i * 3], meshlet_local_index_buffer[mshlt.primitive_offset + i * 3 + 1], meshlet_local_index_buffer[mshlt.primitive_offset + i * 3 + 2] }; })
		//								 | std::views::transform([&meshlet_global_index_buffer, &mshlt](auto u3) { return uint32_3{ meshlet_global_index_buffer[mshlt.global_index_offset + u3.x], meshlet_global_index_buffer[mshlt.global_index_offset + u3.y], meshlet_global_index_buffer[mshlt.global_index_offset + u3.z] }; })
		//								 | std::views::transform([&vertex_buffer](auto u3) -> decltype(auto) { return std::tuple{ vertex_buffer[u3.x], vertex_buffer[u3.y], vertex_buffer[u3.z] }; })
		//								 | std::views::enumerate)
		//	{
		//		std::println("triangle [{}] : [{}, {}, {}], [{}, {}, {}], [{}, {}, {}]", idx,
		//					 std::get<0>(tpl).pos.x, std::get<0>(tpl).pos.y, std::get<0>(tpl).pos.z,
		//					 std::get<1>(tpl).pos.x, std::get<1>(tpl).pos.y, std::get<1>(tpl).pos.z,
		//					 std::get<2>(tpl).pos.x, std::get<2>(tpl).pos.y, std::get<2>(tpl).pos.z

		//		);
		//	}

		//	std::println("meshlet[{}] end", mshlt_idx);
		//}

		// auto aabb_min = int16_3{ std::numeric_limits<int16>::max() },
		//	 aabb_max = int16_3{ std::numeric_limits<int16>::lowest() };

		auto aabb_min = float3{ std::numeric_limits<float>::max() };
		auto aabb_max = float3{ std::numeric_limits<float>::lowest() };
		for (c_auto& v : vertex_buffer)
		{
			aabb_min = age::min(aabb_min, v.pos);
			aabb_max = age::max(aabb_max, v.pos);
		}

		c_auto aabb_size = age::max(aabb_max - aabb_min, float3{ age::g::epsilon_1e6 });

		c_auto vertex_buffer_quantized_buffer = vertex_buffer
											  | std::views::transform([aabb_min, aabb_size](c_auto& v) {
													return cvt_vertex_to<t_vertex>(v, aabb_min, aabb_size);
												})
											  | std::ranges::to<age::vector<t_vertex>>();

		auto header = mesh_baked_header{};
		{
			c_auto header_vertex_buffer_offset = static_cast<uint32>(sizeof(mesh_baked_header));

			header.meshlet_header_buffer_offset		 = header_vertex_buffer_offset + vertex_buffer_quantized_buffer.byte_size<uint32>();
			header.meshlet_buffer_offset			 = header.meshlet_header_buffer_offset + meshlet_header_vec.byte_size<uint32>();
			header.global_vertex_index_buffer_offset = header.meshlet_buffer_offset + meshlet_vec.byte_size<uint32>();
			header.local_vertex_index_buffer_offset	 = header.global_vertex_index_buffer_offset + meshlet_global_index_buffer.byte_size<uint32>();
			header.meshlet_count					 = static_cast<uint32>(meshlet_vec.size());
			header.aabb_min							 = aabb_min;
			header.aabb_size						 = aabb_size;

			AGE_ASSERT(header.meshlet_header_buffer_offset % 4 == 0);
			AGE_ASSERT(header.meshlet_buffer_offset % 4 == 0);
			AGE_ASSERT(header.meshlet_header_buffer_offset % 4 == 0);
			AGE_ASSERT(header.global_vertex_index_buffer_offset % 4 == 0);
			AGE_ASSERT(header.local_vertex_index_buffer_offset % 4 == 0);
		};

		auto buffer = age::buffer::write_bytes<alignof(uint32)>(header,
																vertex_buffer_quantized_buffer,
																meshlet_header_vec,
																meshlet_vec,
																meshlet_global_index_buffer,
																meshlet_local_index_buffer);

		return { .buffer = std::move(buffer) };
	}
}	 // namespace age::asset
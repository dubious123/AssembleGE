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
}	 // namespace age::asset

namespace age::asset
{
	struct vertex_fat
	{
		float3				  pos	  = {};
		float3				  normal  = {};
		float4				  tangent = {};
		std::array<float2, 4> uv_set  = {};
	};

	template <typename t_vertex>
	struct mesh_triangulated
	{
		data_structure::vector<t_vertex> vertex_vec{};
		data_structure::vector<uint32>	 v_idx_vec{};
	};

	template <typename t_vertex>
	t_vertex
	gather_vertex(const mesh_editable& mesh_edit, const mesh_editable::vertex& v) noexcept
	{
		constexpr auto uv_count = detail::uv_count_v<t_vertex>;

		auto	res	 = t_vertex{};
		c_auto& pos	 = mesh_edit.position_vec[v.pos_idx];
		c_auto& attr = mesh_edit.vertex_attr_vec[v.attribute_idx];

		res.pos = pos;
		if constexpr (uv_count > 0)
		{
			age::math::cvt_to<uv_count>(
				attr.uv_set[0].data(),
				reinterpret_cast<half*>(res.uv_set.data() + uv_count));
		}

		if constexpr (meta::is_specialization_of_nttp_v<t_vertex, detail::vertex_p_uv>)
		{
		}
		else if constexpr (meta::is_specialization_of_nttp_v<t_vertex, detail::vertex_pn_uv>)
		{
			res.normal_oct = cvt_to<oct<int8>>(attr.normal);
		}
		else if constexpr (meta::is_specialization_of_nttp_v<t_vertex, detail::vertex_pnt_uv>)
		{
			res.normal_oct	= cvt_to<oct<int8>>(attr.normal);
			res.tangent_oct = cvt_to<oct<int8>>(attr.tangent.xyz);
		}
		else
		{
			static_assert(false, "invalid vertex type");
		}

		return res;
	}

	template <>
	FORCE_INLINE vertex_fat
	gather_vertex<vertex_fat>(const mesh_editable& mesh_edit, const mesh_editable::vertex& v) noexcept
	{
		return {
			.pos	 = mesh_edit.position_vec[v.pos_idx],
			.normal	 = mesh_edit.vertex_attr_vec[v.attribute_idx].normal,
			.tangent = mesh_edit.vertex_attr_vec[v.attribute_idx].tangent,
			.uv_set	 = mesh_edit.vertex_attr_vec[v.attribute_idx].uv_set
		};
	}

	template <typename t_vertex>
	FORCE_INLINE t_vertex
	gather_vertex(const mesh_editable& mesh_edit, uint32 v_idx) noexcept
	{
		return gather_vertex<t_vertex>(mesh_edit, mesh_edit.vertex_vec[v_idx]);
	}

	template <typename t_vertex>
	mesh_triangulated<t_vertex>
	triangulate(const mesh_editable& m) noexcept
	{
		return {
			.vertex_vec = m.vertex_vec
						| std::views::transform([&m](auto& v) { return gather_vertex<t_vertex>(m, v); })
						| std::ranges::to<data_structure::vector<t_vertex>>(),
			.v_idx_vec = external::earcut::perform(m)
		};
	}
}	 // namespace age::asset

namespace age::asset
{
	struct meshlet_cull_data
	{
		float4 bounding_sphere{};	 // xyz = center, w = radius
		uint8  normal_cone[4]{};	 // xyz = axis, w = -cos(a + 90)
		float  apex_offset{};		 // apex = center - axis * offset
	};	  // Source: Microsoft DirectX-Graphics-Samples (D3D12MeshShaders)

	struct meshlet
	{
		// local_idx = local_idx_buffer[ nth primitive * i ]
		// global_idx = global_idx_buffer[local_idx]
		// vertex = vertex_buffer[global_idx]

		uint32 global_index_offset{};
		uint32 local_index_offset{};

		uint8  vertex_count{};
		uint8  primitive_count{};
		uint16 padding{};
	};

	struct mesh_baked_header
	{
		uint32			 vertex_count{};
		uint32			 global_vertex_count{};
		uint32			 primitive_count{};
		uint32			 meshlet_count{};
		e::vertex_kind	 vertex_kind{};
		e::topology_kind topology_kind{};
	};

	// [mesh_baked_header]
	// [vertex_buffer]
	// [global_vertex_index_buffer]
	// [local_vertex_index_buffer]
	// [meshlet_array]
	struct mesh_baked
	{
		mesh_baked_header header{};

		age::asset::handle h_asset{};
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
	mesh_baked
	bake_mesh(const asset::mesh_editable&) noexcept;
}
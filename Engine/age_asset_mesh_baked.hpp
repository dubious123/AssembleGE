#pragma once

namespace age::asset
{
	namespace detail
	{
		template <std::size_t uv_set_count>
		struct vertex_p_uv
		{
			float3							 position{};
			std::array<half_2, uv_set_count> uv_set{};
		};

		template <std::size_t uv_set_count>
		struct vertex_pn_uv
		{
			float3							 position{};
			half_2							 normal{};
			std::array<half_2, uv_set_count> uv_set{};
		};

		template <std::size_t uv_set_count>
		struct vertex_pnt_uv
		{
			float3							 position{};
			int8							 __internal_padding__[3];
			int8							 tangent_handedness{};	  // -1 or 1
			half_2							 normal_oct{};
			half_2							 tangent_oct{};
			std::array<half_2, uv_set_count> uv_set{};
		};

		template <>
		struct vertex_p_uv<0>
		{
			float3 position{};
		};

		template <>
		struct vertex_pn_uv<0>
		{
			float3 position{};
			half_2 normal{};
		};

		template <>
		struct vertex_pnt_uv<0>
		{
			float3 position{};
			int8   __internal_padding__[3];
			int8   tangent_handedness{};	// -1 or 1
			half_2 normal_oct{};
			half_2 tangent_oct{};
		};
	}	 // namespace detail

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
	bake_mesh(mesh_editable&) noexcept;
}
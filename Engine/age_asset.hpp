#pragma once

namespace age::asset
{
	AGE_DEFINE_ENUM(
		type,
		uint8,
		mesh_editable,
		lod_group_editable,
		scene_editable,
		mesh_baked,
		lod_group_baked,
		count);
}

namespace age::asset
{
	using t_asset_id = uint32;

	struct handle
	{
		t_asset_id id;
	};

	struct data
	{
		asset::type			 type{};
		std::span<std::byte> blob{};
	};

	handle
	load_from_file(const std::string_view& file_path) noexcept;

	void unload(handle) noexcept;

	void
	deinit() noexcept;
}	 // namespace age::asset

namespace age::asset::g
{
	inline constexpr uint32 uv_set_max = 4;

	inline constexpr auto mashlet_thread_count		  = 32ul;
	inline constexpr auto mashlet_max_vertex_count	  = 64ul;
	inline constexpr auto mashlet_max_primitive_count = 126ul;
}	 // namespace age::asset::g

namespace age::asset
{
	struct mesh_editable
	{
		struct vertex_attribute
		{
			float3							  normal{};
			float4							  tangent{};
			std::array<float2, g::uv_set_max> uv_set{};
			uint8							  uv_count{};
		};

		struct vertex
		{
			uint32 pos_idx{};
			uint32 attribute_idx{};
		};

		struct face
		{
			uint32 vertex_id_begin_idx{};
			uint32 vertex_count{};
		};

		data_structure::vector<float3>			 position_vec{};
		data_structure::vector<vertex_attribute> vertex_attr_vec{};
		data_structure::vector<vertex>			 vertex_vec{};

		data_structure::vector<uint32> vertex_id_vec{};

		data_structure::vector<face> face_vec{};
	};

	struct lod_group_editable
	{
		std::string							  name{};
		data_structure::vector<mesh_editable> mesh_vec{};
	};

	struct scene_editable
	{
		std::string								   name{};
		data_structure::vector<lod_group_editable> lod_group_vec{};
	};
}	 // namespace age::asset

namespace age::asset
{
	AGE_DEFINE_ENUM(
		vertex_type,
		uint8,
		p_uv0,
		pn_uv0,
		pnt_uv0,

		p_uv1,
		pn_uv1,
		pnt_uv1,

		p_uv2,
		pn_uv2,
		pnt_uv2,

		p_uv3,
		pn_uv3,
		pnt_uv3,

		count);

	AGE_DEFINE_ENUM(
		topology_type,
		uint8,
		triangle,
		count);

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
		uint32		  vertex_count{};
		uint32		  global_vertex_count{};
		uint32		  primitive_count{};
		uint32		  meshlet_count{};
		vertex_type	  vertex_type{};
		topology_type topology_type{};
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

namespace age::asset::g
{
	inline auto asset_data_vec = age::data_structure::stable_dense_vector<asset::data>{ 2 };
}
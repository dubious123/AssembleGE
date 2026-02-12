#pragma once
#include "age.hpp"

namespace age::asset::e
{
	AGE_DEFINE_ENUM(
		kind,
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
		e::kind				 asset_kind{};
		std::span<std::byte> blob{};
		std::align_val_t	 alignment{};
	};

	struct file_header
	{
		uint32	  magic;
		uint32	  header_size;
		uint64	  file_size;
		uint8	  version_major;
		uint8	  version_minor;
		e::kind	  asset_kind;
		std::byte reserve[5];
	};

	bool
	validate(const file_header&, const std::ifstream&) noexcept;

	asset::handle
	load_from_file(const std::string_view& file_path) noexcept;

	void
	write_to_file(const std::string_view& file_path, const file_header&, const auto& asset_data) noexcept;

	void unload(asset::handle) noexcept;

	void
	deinit() noexcept;
}	 // namespace age::asset

namespace age::asset::g
{
	inline constexpr uint32 uv_set_max = 4;

	inline constexpr auto mashlet_thread_count		  = 32ul;
	inline constexpr auto mashlet_max_vertex_count	  = 64ul;
	inline constexpr auto mashlet_max_primitive_count = 126ul;

	inline constexpr auto asset_header_magic = uint32{ 'AGEA' };
}	 // namespace age::asset::g

// mesh editable
namespace age::asset::e
{
	AGE_DEFINE_ENUM(
		primitive_mesh_kind,
		uint8,
		plane,
		cube,
		capsule,
		cylinder,
		ico_sphere,
		uv_phere,
		count);

	AGE_DEFINE_ENUM(
		winding_kind,
		uint8,
		clockwise,
		counter_clockwise,
		count);

	AGE_DEFINE_ENUM(
		handedness_kind,
		uint8,
		right,
		left,
		count);

	AGE_DEFINE_ENUM(
		vertex_kind,
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

	enum class bake_flags : uint8
	{
		front_outer = 1 << 0,
		front_hole	= 1 << 1,
		back_outer	= 1 << 2,
		back_hole	= 1 << 3
	};

	AGE_ENUM_FLAG_OPERATORS(bake_flags);
}	 // namespace age::asset::e

namespace age::asset
{
	struct mesh_editable;

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
	struct primitive_desc
	{
		float3				   size{ 1.f, 1.f, 1.f };
		uint32				   seg_u{ 1 };
		uint32				   seg_v{ 1 };
		float3x3			   local_basis = float3x3::identity();
		e::primitive_mesh_kind mesh_kind{};
		uint8				   padding[3];
	};

	struct normal_calc_desc
	{
		AGE_DEFINE_ENUM_MEMBER(
			mode,
			uint8,
			area,
			angle,
			area_angle);

		mode   calc_mode;
		bool   smoothing_include_hole;
		float  smoothing_angle_rad;
		float3 fallback{ 0.f, 1.f, 0.f };
	};

	struct tangent_calc_desc
	{
	};

	mesh_editable
	create_primitive(const primitive_desc& desc) noexcept;

	void
	calculate_normal(mesh_editable&, const normal_calc_desc&) noexcept;

	void
	calculate_tangent(mesh_editable&, const tangent_calc_desc&) noexcept;
}	 // namespace age::asset

// mesh baked
namespace age::asset::e
{
	AGE_DEFINE_ENUM(
		topology_kind,
		uint8,
		triangle,
		count);
}

namespace age::asset::g
{
	inline auto asset_data_vec = age::data_structure::stable_dense_vector<asset::data>{ 2 };
}
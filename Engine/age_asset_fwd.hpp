#pragma once

namespace age::asset::e
{
	AGE_DEFINE_ENUM(
		kind,
		uint8,
		asset_registry,
		font,
		mesh_editable,
		lod_group_editable,
		scene_editable,
		mesh_baked,
		lod_group_baked,
		editor_game,
		editor_entity_storage);

	// mesh editable
	AGE_DEFINE_ENUM(
		primitive_mesh_kind,
		uint8,
		plane,
		cube,
		capsule,
		cylinder,
		ico_sphere,
		uv_sphere,
		cube_sphere);

	AGE_DEFINE_ENUM(
		winding_kind,
		uint8,
		clockwise,
		counter_clockwise);

	AGE_DEFINE_ENUM(
		handedness_kind,
		uint8,
		right,
		left);

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
		pnt_uv3);

	AGE_DEFINE_ENUM_WITH_VALUE(
		mesh_bake_flags, uint8,
		(front_outer, 1 << 0),
		(front_hole, 1 << 1),
		(back_outer, 1 << 2),
		(back_hole, 1 << 3))

	AGE_ENUM_FLAG_OPERATORS(mesh_bake_flags);

	AGE_DEFINE_ENUM(
		topology_kind,
		uint8,
		triangle);

	AGE_DEFINE_ENUM(
		normal_calc_mode_kind,
		uint8,
		area,
		angle,
		area_angle);

	AGE_DEFINE_ENUM_WITH_VALUE(
		font_charset_flag,
		uint64,
		(ascii, 1 << 0),
		(hangul, 1 << 1));

	AGE_ENUM_FLAG_OPERATORS(font_charset_flag);
}	 // namespace age::asset::e

// version 2
namespace age::asset
{
	template <e::kind>
	struct entry;
}	 // namespace age::asset

namespace age::asset
{
	struct data
	{
		e::kind				 asset_kind{};
		uint8_3				 extra_1;
		uint8_4				 extra_2;
		std::span<std::byte> blob{};
		std::align_val_t	 alignment{};
		std::string			 path;

		std::byte*
		get_payload() noexcept;

		read_buf
		get_payload_read_buf() const noexcept;
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

	using t_asset_id = uint32;

	struct handle
	{
		t_asset_id id;

		FORCE_INLINE data*
		operator->() const noexcept;

		template <e::kind e_kind>
		FORCE_INLINE static handle
		make(std::unsigned_integral auto idx) noexcept
		{
			AGE_ASSERT(idx < 0x00ff'ffff);

			return handle{ .id = (to_idx(e_kind) << 24) | (static_cast<uint32>(idx) & 0x00ff'ffff) };
		}

		FORCE_INLINE e::kind
		get_kind() const noexcept
		{
			return static_cast<e::kind>(id >> 24);
		}

		FORCE_INLINE uint32
		get_idx() const noexcept
		{
			return id & 0x00ff'ffff;
		}

		template <e::kind>
		auto&
		get_entry() const noexcept;

		inline std::array<char, config::max_asset_path_len>&
		get_path() const noexcept;
	};
}	 // namespace age::asset

// mesh gen primitives
namespace age::asset
{
	struct primitive_desc
	{
		float3				   pos{ 0, 0, 0 };
		float3				   size{ 1.f, 1.f, 1.f };
		uint32				   seg_u{ 1 };
		uint32				   seg_v{ 1 };
		float3x3			   local_basis = float3x3::identity();
		e::primitive_mesh_kind mesh_kind{};
		uint8				   padding[3];
	};

	struct normal_calc_desc
	{
		e::normal_calc_mode_kind calc_mode;
		bool					 smoothing_include_hole;
		float					 smoothing_angle_rad;
		float3					 fallback{ 0.f, 1.f, 0.f };
	};

	struct tangent_calc_desc
	{
	};
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
		age::vector<t_vertex> vertex_vec{};
		age::vector<uint32>	  v_idx_vec{};
	};

	struct meshlet_header
	{
		oct<int8> cone_axis_oct;
		int8	  cone_cull_cutoff;
		uint8	  padding;	   // apex = center - axis * offset;

		int16_3	 aabb_min;	   // 6byte
		uint16_3 aabb_size;	   // 6byte
	};

	struct meshlet
	{
		uint32 global_index_offset{};
		uint32 primitive_offset{};

		uint8  vertex_count{};
		uint8  primitive_count{};
		uint16 padding{};
	};
}	 // namespace age::asset

namespace age::asset
{
	struct mesh_editable;

	struct lod_group_editable
	{
		std::string				   name{};
		age::vector<mesh_editable> mesh_vec{};
	};

	struct scene_editable
	{
		std::string						name{};
		age::vector<lod_group_editable> lod_group_vec{};
	};
}	 // namespace age::asset

namespace age::asset::font
{
	struct glyph_data
	{
		float  advance;
		float2 offset;
		float2 size;
		float2 atlas_uv_min;
		float2 atlas_uv_max;
	};

	struct asset_header
	{
		e::font_charset_flag charset_flag;
		uint64				 glyph_data_offset;
		uint64				 atlas_offset;

		float ascent;
		float descent;
		float space_advance;
		float line_height;
		float em_size;
		float px_range;

		uint32 atlas_width;
		uint32 atlas_height;

		uint32 extra_unicode_offset;

		uint16 glyph_count;
		uint16 extra_unicode_count;

		uint8	atlas_channel_count;
		uint8_3 _;


		std::span<uint16>
		get_extra_unicode() noexcept;

		std::span<glyph_data>
		get_glyph() noexcept;

		std::span<const uint8>
		get_atlas() const noexcept;

		const glyph_data&
		get_glyph_data(uint16 unicode) noexcept;
	};
}	 // namespace age::asset::font

namespace age::asset
{
	template <>
	struct entry<e::kind::font>
	{
		void* p_blob;	 // glyph + extra unicode

		e::font_charset_flag charset_flag;
		uint64				 glyph_data_offset;
		uint64				 atlas_offset;

		float ascent;
		float descent;
		float space_advance;
		float line_height;
		float em_size;
		float px_range;

		uint32 atlas_width;
		uint32 atlas_height;

		uint32 extra_unicode_offset;

		uint16 glyph_count;
		uint16 extra_unicode_count;

		uint8	atlas_channel_count;
		uint8_3 _;

		uint32 atlas_id = age::get_invalid_id<uint32>();
		uint32 path_id;


		std::span<uint16>
		get_extra_unicode() noexcept;

		std::span<font::glyph_data>
		get_glyph() noexcept;

		const font::glyph_data&
		get_glyph_data(uint16 unicode) noexcept;

		std::array<char, config::max_asset_path_len>&
		get_path() const noexcept;

		// bool
		// loaded() noexcept
		//{ return AGE_IS_INVALID_ID(atlas_id); }
	};
}	 // namespace age::asset

namespace age::asset::g
{
	inline auto path_vec = age::sparse_vector<std::array<char, config::max_asset_path_len>>{};

	template <e::kind e_kind>
	inline auto entry_pool = age::sparse_vector<entry<e_kind>>{};

	inline std::filesystem::path						 registry_path;
	inline std::array<age::vector<handle>, e::kind_size> registry_map;
	inline std::array<age::unordered_map<age::array<char, config::max_asset_path_len>, handle>, e::kind_size>
		registry_path_to_handle_map;
}	 // namespace age::asset::g

namespace age::asset
{
	template <e::kind e_kind>
	FORCE_INLINE auto&
	pool_of() noexcept
	{
		return g::entry_pool<e_kind>;
	}
}	 // namespace age::asset

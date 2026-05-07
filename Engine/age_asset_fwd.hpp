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
		editor_entity_storage,
		material,
		texture,
		env_light);

	// mesh editable
	AGE_DEFINE_ENUM(
		primitive_mesh_kind,
		uint8,
		plane,
		cube,
		cube_sphere,
		capsule,
		cylinder,
		ico_sphere,
		uv_sphere);

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

	AGE_DEFINE_ENUM(alpha_mode_kind, uint8, opaque, mask, blend);

	AGE_DEFINE_ENUM(mip_filter_kind, uint8, point, linear, cubic, box, triangle);
	AGE_DEFINE_ENUM(wrap_mode_kind, uint8, wrap, mirror, clamp);
}	 // namespace age::asset::e

namespace age::asset
{
	template <e::kind>
	struct entry;
}	 // namespace age::asset

namespace age::asset
{
	struct file_header
	{
		uint32	  magic;
		uint32	  header_size;
		uint64	  file_size;
		uint8	  version_major;
		uint8	  version_minor;
		e::kind	  asset_kind;
		uint8	  blob_alignment_log2;
		std::byte reserve[4];
	};

	using t_asset_id = uint32;

	struct handle
	{
		t_asset_id id = age::get_invalid_id<t_asset_id>();

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

		template <e::kind>
		std::array<char, config::max_asset_path_len>&
		get_path() const noexcept;

		inline std::array<char, config::max_asset_path_len>&
		get_path() const noexcept;

		template <e::kind>
		std::array<char, config::max_asset_display_name_len>
		get_display_name() const noexcept;

		inline std::array<char, config::max_asset_display_name_len>
		get_display_name() const noexcept;

		bool
		operator==(const handle&) const noexcept = default;
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

	struct mesh_baked_header
	{
		// uint32 vertex_offset = sizeof(mesh_baked_header)
		uint32 vertex_kind_and_extra;	 // [0:7] kind, [8:31] flags

		uint32 global_vertex_index_buffer_offset;
		uint32 local_vertex_index_buffer_offset;
		uint32 meshlet_header_buffer_offset;
		uint32 meshlet_buffer_offset;
		uint32 meshlet_count;
		float3 aabb_min;
		float3 aabb_size;

		uint32 reserved[4];
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
}	 // namespace age::asset::font

namespace age::asset
{
	template <>
	struct entry<e::kind::font>
	{
		std::byte* p_blob;	  // glyph + extra unicode

		e::font_charset_flag charset_flag;

		float ascent;
		float descent;
		float space_advance;
		float line_height;
		float em_size;
		float px_range;

		uint32 atlas_width;
		uint32 atlas_height;

		uint16 glyph_count;
		uint16 extra_unicode_count;

		uint8	atlas_channel_count;
		uint8_3 _;

		uint32 atlas_id = age::get_invalid_id<uint32>();
		uint32 path_id;

		std::span<const font::glyph_data>
		get_glyph() const noexcept;

		std::span<const uint16>
		get_extra_unicode() const noexcept;

		const font::glyph_data&
		get_glyph_data(uint16 unicode) const noexcept;

		std::array<char, config::max_asset_path_len>&
		get_path() const noexcept;

		bool
		is_loaded() const noexcept;
	};

	template <>
	struct entry<e::kind::mesh_baked>
	{
		// asset_header - meshlet - index buffer (uint32) - pos buffer (float3)
		struct header
		{
			uint64 meshlet_buffer_byte_size;
			uint32 index_count;
			uint32 pos_count;
		};

		static_assert(std::is_implicit_lifetime_v<header>);
		static_assert(std::is_trivially_copyable_v<header>);
		static_assert(sizeof(header) == 16);

		static_assert(std::is_implicit_lifetime_v<mesh_baked_header>);
		static_assert(std::is_trivially_copyable_v<mesh_baked_header>);


		using allocator_type = aligned_byte_allocator;

		uint32 path_id;
		uint32 render_id = age::get_invalid_id<uint32>();

		std::byte* p_blob = nullptr;

		uint32 ref_counter = 0u;
		uint32 _;

		std::array<char, config::max_asset_path_len>&
		get_path() const noexcept;

		bool
		is_cpu_loaded() const noexcept;

		bool
		is_gpu_loaded() const noexcept;

		const header&
		get_header() const noexcept;

		const mesh_baked_header&
		get_mesh_header() const noexcept;

		const void*
		meshlet_buffer_data() const noexcept;

		uint64
		index_buffer_byte_offset() const noexcept;

		uint64
		pos_buffer_byte_offset() const noexcept;

		const void*
		index_buffer_data() const noexcept;

		const void*
		pos_buffer_data() const noexcept;
	};

	template <>
	struct entry<e::kind::material>
	{
		uint32 path_id;
		uint32 render_id = age::get_invalid_id<uint32>();

		float4			   base_color_factor;
		float			   metallic_factor;
		float			   roughness_factor;
		float3			   emissive_factor;
		float			   normal_scale;
		float			   occlusion_strength;
		float			   alpha_cutoff;
		e::alpha_mode_kind alpha_mode;
		uint8_3			   _;

		handle h_tex_base_color;
		handle h_tex_metallic_roughness;
		handle h_tex_normal;
		handle h_tex_occlusion;
		handle h_tex_emissive;

		uint32 ref_counter = 0u;

		std::array<char, config::max_asset_path_len>&
		get_path() const noexcept;

		bool
		is_loaded() const noexcept;

		std::array<const handle*, 5>
		all_textures() const noexcept;

		std::array<handle*, 5>
		all_textures() noexcept;
	};

	template <>
	struct entry<e::kind::texture>
	{
		struct header
		{
			extent_2d<uint32>			extent;
			uint16						tex_depth_or_array_size;
			graphics::e::texture_format format;
			uint8						mip_count;
			uint8						flags;	  // [0] is_cubemap, [1] is_3d,
			uint16						extra;
		};

		static_assert(std::is_implicit_lifetime_v<header>);
		static_assert(std::is_trivially_copyable_v<header>);
		static_assert(sizeof(header) == 16);

		using allocator_type = aligned_byte_allocator;

		uint32 path_id;
		uint32 render_id = age::get_invalid_id<uint32>();


		std::byte* p_blob;

		uint32 ref_counter = 0u;

		std::array<char, config::max_asset_path_len>&
		get_path() const noexcept;

		bool
		is_cpu_loaded() const noexcept;

		bool
		is_gpu_loaded() const noexcept;

		const header&
		get_header() const noexcept;

		const void*
		get_texture_buffer() const noexcept;

		bool
		is_cube_map() const noexcept;

		bool
		is_tex3d() const noexcept;
	};

	template <>
	struct entry<e::kind::env_light>
	{
		struct info_bake
		{
			uint32						cubemap_size;
			uint16						prefilter_size;
			uint16						prefilter_mip_count;
			uint16						irradiance_size;
			graphics::e::texture_format format;
			float						max_luminance;
			float						min_luminance;
			float						mean_luminance;
			float3						dominant_light_direction;
			float3						dominant_light_color;
			uint32_4					reserved;
		};

		struct info_runtime
		{
			float	 intensity;
			float3	 tint;
			float3	 rotation_deg;	  // look_at, normalized
			uint32_4 reserved;
		};

		struct header
		{
			info_bake	 bake_info;
			info_runtime runtime_info;
		};

		static_assert(std::is_implicit_lifetime_v<header>);
		static_assert(std::is_trivially_copyable_v<header>);
		static_assert(sizeof(header) == 108);

		using allocator_type = aligned_byte_allocator;

		uint32 path_id;
		uint32 render_id = age::get_invalid_id<uint32>();


		std::byte* p_blob;

		uint32 ref_counter = 0u;

		std::array<char, config::max_asset_path_len>&
		get_path() const noexcept;

		bool
		is_cpu_loaded() const noexcept;

		bool
		is_gpu_loaded() const noexcept;

		const header&
		get_header() const noexcept;

		header&
		get_header() noexcept;

		const void*
		get_texture_buffer() const noexcept;
	};
}	 // namespace age::asset

namespace age::asset
{
	struct env_light_desc
	{
		graphics::e::texture_format format				= graphics::e::texture_format::bc6h_uf16;
		uint32						cubemap_size		= 1024;
		uint16						prefilter_size		= 256;
		uint16						prefilter_mip_count = 7;
		uint16						irradiance_size		= 32;

		bool invert_y = false;
	};

	struct material_desc
	{
		float4			   base_color_factor  = float4::one();
		float			   metallic_factor	  = 1.f;
		float			   roughness_factor	  = 1.f;
		float3			   emissive_factor	  = float3::zero();
		float			   normal_scale		  = 1.f;
		float			   occlusion_strength = 1.f;
		float			   alpha_cutoff		  = 0.f;
		e::alpha_mode_kind alpha_mode		  = e::alpha_mode_kind::opaque;

		handle h_tex_base_color;
		handle h_tex_metallic_roughness;
		handle h_tex_normal;
		handle h_tex_occlusion;
		handle h_tex_emissive;
	};

	struct texture_bake_option
	{
		graphics::e::texture_format format = graphics::e::texture_format::bc7_unorm_srgb;

		bool   is_cube				= false;
		bool   is_3d				= false;
		uint32 array_or_depth_count = 1;

		const char* output_filename = nullptr;

		uint32 width	= 0;				 // 0 = source
		uint32 height	= 0;				 // 0 = source
		bool   fit_pow2 = false;

		uint32			   mip_count = 0;	 // 0 = full chain, 1 = none
		e::mip_filter_kind filter	 = e::mip_filter_kind::linear;
		e::wrap_mode_kind  wrap		 = e::wrap_mode_kind::clamp;

		bool hflip = false;
		bool vflip = false;

		bool invert_y = false;			  // for gltf normal map

		bool  separate_alpha  = false;
		float alpha_threshold = -1.0f;	  // -1 = unset
		float keep_coverage	  = -1.0f;
	};
}	 // namespace age::asset

namespace age::asset::g
{
	inline constexpr auto uv_set_max = 4u;

	inline constexpr auto mashlet_max_vertex_count	  = 64ul;
	inline constexpr auto mashlet_max_primitive_count = 126ul;

	inline constexpr auto asset_header_magic = uint32{ 'AGEA' };

	inline auto path_vec = age::sparse_vector<std::array<char, config::max_asset_path_len>>{};

	template <e::kind e_kind>
	inline auto entry_pool = age::sparse_vector<entry<e_kind>>{};
	inline std::array<age::unordered_map<age::array<char, config::max_asset_path_len>, handle>, e::kind_size>
		path_to_handle_map;

	inline std::filesystem::path						 registry_path;
	inline std::array<age::vector<handle>, e::kind_size> registry_map;
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

#pragma once
#define MAX_UV_COUNT 2

// static buffer offset
#define MAX_OPAQUE_MESHLET_RENDER_DATA_COUNT (1u << 20)
#define MAX_OBJECT_DATA_COUNT				 1024u
#define MAX_DIRECTIONAL_LIGHT_COUNT			 2
#define MAX_LIGHT_COUNT						 (512 * 512)

// shadow
// todo, measure shadow rt performance
#define MAX_SHADOW_LIGHT_COUNT 100

// light cull
#define LIGHT_CULL_THREAD_COUNT 256
#define LIGHT_ZBIN_THREAD_COUNT 256

#define MAX_VISIBLE_LIGHT_COUNT (16 * 1024)
#define Z_SLICE_COUNT			(512)

#define LIGHT_TILE_SIZE			   32
#define LIGHT_BITMASK_UINT32_COUNT (MAX_VISIBLE_LIGHT_COUNT / 32)

#define LIGHT_KIND_DIRECTIONAL 0
#define LIGHT_KIND_POINT	   1
#define LIGHT_KIND_SPOT		   2
#define LIGHT_KIND_AREA		   3
#define LIGHT_KIND_VOLUMN	   4

// sort
#define SORT_THREAD_COUNT			  128
#define SORT_ELEMENT_COUNT_PER_THREAD 4

#define MAX_SORT_COUNT (512 * 512)

#define SORT_BLOCK_SIZE (SORT_THREAD_COUNT * SORT_ELEMENT_COUNT_PER_THREAD)

#define SORT_BLOCK_COUNT ((MAX_SORT_COUNT + SORT_BLOCK_SIZE - 1) / SORT_BLOCK_SIZE)

#if defined(AGE_SHADER)
	#define SORT_GROUP_COUNT (min(SORT_BLOCK_SIZE, SORT_BLOCK_COUNT))
#else
	#define SORT_GROUP_COUNT (std::min(SORT_BLOCK_SIZE, SORT_BLOCK_COUNT))
#endif

#define SORT_BLOCK_COUNT_PER_GROUP ((SORT_BLOCK_COUNT + SORT_GROUP_COUNT - 1) / SORT_GROUP_COUNT)

#define SORT_BIN_BIT_WIDTH 4
#define SORT_BIN_COUNT	   (1 << SORT_BIN_BIT_WIDTH)

#define SORT_HISTOGRAM_TABLE_SIZE (SORT_BIN_COUNT * SORT_GROUP_COUNT)


// transparent

#if !defined(AGE_SHADER)
	#define SHARED_TYPE shared_type::
#else
	#define SHARED_TYPE
#endif

// static
#define OPAQUE_MSHLT_OBJECT_DATA_OFFSET (0)
#define OBJECT_DATA_OFFSET				(OPAQUE_MSHLT_OBJECT_DATA_OFFSET + sizeof(SHARED_TYPE opaque_meshlet_render_data) * MAX_OPAQUE_MESHLET_RENDER_DATA_COUNT)
#define DIRECTIONAL_LIGHT_OFFSET		(OBJECT_DATA_OFFSET + sizeof(SHARED_TYPE object_data) * MAX_OBJECT_DATA_COUNT)
#define UNIFIED_LIGHT_OFFSET			(DIRECTIONAL_LIGHT_OFFSET + sizeof(SHARED_TYPE directional_light) * MAX_DIRECTIONAL_LIGHT_COUNT)
#define STATIC_BUFFER_SIZE				(UNIFIED_LIGHT_OFFSET + sizeof(SHARED_TYPE unified_light) * MAX_LIGHT_COUNT)

// scratch
#define SCRATCH_SORT_BUFFER_OFFSET (0)

#define SORT_KEYS_OFFSET	   (SCRATCH_SORT_BUFFER_OFFSET)
#define SORT_KEYS_ALT_OFFSET   (SORT_KEYS_OFFSET + MAX_SORT_COUNT * sizeof(uint32))
#define SORT_VALUES_OFFSET	   (SORT_KEYS_ALT_OFFSET + MAX_SORT_COUNT * sizeof(uint32))
#define SORT_VALUES_ALT_OFFSET (SORT_VALUES_OFFSET + MAX_SORT_COUNT * sizeof(uint32))
#define SORT_HISTOGRAM_OFFSET  (SORT_VALUES_ALT_OFFSET + MAX_SORT_COUNT * sizeof(uint32))
#define SORT_BIN_COUNT_OFFSET  (SORT_HISTOGRAM_OFFSET + SORT_HISTOGRAM_TABLE_SIZE * sizeof(uint32))

#define LIGHT_CULL_PACKED_AABB_OFFSET (SORT_BIN_COUNT_OFFSET + SORT_BIN_COUNT * sizeof(uint32))
#define VISIBLE_LIGHT_COUNT_OFFSET	  (LIGHT_CULL_PACKED_AABB_OFFSET + MAX_VISIBLE_LIGHT_COUNT * sizeof(uint32))
#define SCRATCH_BUFFER_TOTAL_SIZE	  (VISIBLE_LIGHT_COUNT_OFFSET + sizeof(uint32))

// light cull
#define LIGHT_CULL_ZBIN_OFFSET		(0)
#define LIGHT_CULL_TILE_MASK_OFFSET (LIGHT_CULL_ZBIN_OFFSET + sizeof(SHARED_TYPE zbin_entry) * Z_SLICE_COUNT)


#define RT_MASK_OPAQUE		0x01
#define RT_MASK_TRANSPARENT 0x02
#define RT_MASK_MASK		0x04
#define RT_MASK_ALL			0xff

// ui
#define UI_SHAPE_KIND_RECT		   0
#define UI_SHAPE_KIND_CIRCLE	   1
#define UI_SHAPE_KIND_ARROW_RIGHT  2
#define UI_SHAPE_KIND_TEXT		   3
#define UI_SHAPE_KIND_CHECK		   4
#define UI_SHAPE_KIND_ROUNDED_RECT 5
#define UI_SHAPE_KIND_TRIANGLE	   6
#define UI_SHAPE_KIND_CROSS		   7

#define UI_BRUSH_KIND_COLOR 0

// mesh enum

#define VERTEX_KIND_P_UV0	0
#define VERTEX_KIND_PN_UV0	1
#define VERTEX_KIND_PNT_UV0 2
#define VERTEX_KIND_P_UV1	3
#define VERTEX_KIND_PN_UV1	4
#define VERTEX_KIND_PNT_UV1 5
#define VERTEX_KIND_P_UV2	6
#define VERTEX_KIND_PN_UV2	7
#define VERTEX_KIND_PNT_UV2 8
#define VERTEX_KIND_P_UV3	9
#define VERTEX_KIND_PN_UV3	10
#define VERTEX_KIND_PNT_UV3 11

#define VERTEX_SIZE_P_UV0	12
#define VERTEX_SIZE_PN_UV0	12
#define VERTEX_SIZE_PNT_UV0 16
#define VERTEX_SIZE_P_UV1	16
#define VERTEX_SIZE_PN_UV1	16
#define VERTEX_SIZE_PNT_UV1 20
#define VERTEX_SIZE_P_UV2	20
#define VERTEX_SIZE_PN_UV2	20
#define VERTEX_SIZE_PNT_UV2 24
#define VERTEX_SIZE_P_UV3	24
#define VERTEX_SIZE_PN_UV3	24
#define VERTEX_SIZE_PNT_UV3 28

// material enum
#define MATERIAL_ALPHA_BLEND_OPAQUE 0
#define MATERIAL_ALPHA_BLEND_MASK	1
#define MATERIAL_ALPHA_BLEND_BLEND	2


#if !defined(AGE_SHADER)
	#include "age.hpp"

	#define reg(...)
	#define cbuffer struct
	#define row_major
	#define semantics(...)

namespace age::graphics::render_pipeline::forward_plus
{
#endif
	// shared type
	using t_object_id			 = uint32;
	using t_mesh_id				 = uint32;
	using t_camera_id			 = uint32;
	using t_directional_light_id = uint16;
	using t_unified_light_id	 = uint32;
	using t_texture_id			 = uint32;
	using t_material_id			 = uint32;
#if !defined(AGE_SHADER)

}	 // namespace age::graphics::render_pipeline::forward_plus

namespace age::graphics::render_pipeline::forward_plus::shared_type
{
#endif
	//---[ ui ]------------------------------------------------------------
	struct ui_shape_data
	{
		uint32 data[5];			// TBD, corner radius, circle radius, ...
	};

	struct ui_brush_data
	{
		uint32_4 data;			// TBD, texture_id, static_color, calculate from shadow with delta time...
	};

	struct ui_data
	{
		float2 pivot_pos;		// screen pos of pivot
		float2 pivot_uv;
		float2 size;			// pixel size
		float  rotation;		// z rotation, radian
		float  border_thickness;

		float4 clip_rect;

		uint32 packed_enums;	// [shape_kind(8bit)][body_brush_kind(8bit)][border_brush_kind(8bit)][extra(8bit)]

		ui_shape_data shape_data;
		ui_brush_data body_brush_data;
		ui_brush_data border_brush_data;
	};

	struct light_cull_data
	{
		uint32 not_culled_light_count;
	};

	struct zbin_entry
	{
		uint32 min_idx;
		uint32 max_idx;
	};

	//---[ lights ]------------------------------------------------------------
	struct directional_light
	{
		float3 direction;				 // 12
		float  intensity;				 // 4
		float3 color;					 // 12
		uint32 cast_shadow_and_extra;	 // 4
	};	  // 32 bytes

	struct unified_light
	{
		float3 position;				 // 12
		float  range;					 // 4
		half3  color;					 // 6
		half   intensity;				 // 2
		half3  direction;				 // 6
		half   cos_inner;				 // 2
		half   cos_outer;				 // 2
		uint16 cast_shadow_and_extra;	 // 2
	};	  // total: 36 bytes

	//---[ mesh ]------------------------------------------------------------
	// data only used by amplification shader
	struct meshlet_header
	{
		uint16 cone_axis_oct;

		uint16 cone_cull_cutoff_and_extra;

		int16_3	 aabb_min;						 // 6byte
		uint16_3 aabb_size;						 // 6byte
	};

	struct meshlet
	{
		uint32 global_index_offset;				 // 4 bytes
		uint32 primitive_offset;				 // 4 bytes

		uint32 vertex_count_prim_count_extra;	 // [vertex_count(8bit)][primitive_count(8bit)][extra(16bit)]
	};

	struct object_data
	{
		float3	 pos;							 // 12
		uint32	 quaternion;					 // 4 | 10 10 10 2
		half3	 scale;							 // 6
		uint16_t extra;							 // 2
	};	  // total: 24 bytes

	struct opaque_meshlet_render_data
	{
		uint32 object_id;
		uint32 mesh_byte_offset;
		uint32 mesh_chunk_srv_id;
		uint32 meshlet_id;
		uint32 material_id;
	};

	struct rt_instance_render_data
	{
		uint32 object_id;
		uint32 mesh_byte_offset;
		uint32 mesh_chunk_srv_id;
		uint32 rt_index_buffer_offset;
		uint32 material_id;
		// todo mat_id
	};

	struct mesh_header
	{
		// uint32 vertex_offset = sizeof(mesh_baked_header), sizeof(mesh_baked_header) == 20
#if defined(AGE_SHADER)
		uint32				vertex_buffer_offset;	 // not from gpu, calculated from read_mesh_header function
		byte_address_buffer mesh_chunk_srv;
#endif

		uint32 vertex_kind_and_extra;
		uint32 global_vertex_index_buffer_offset;
		uint32 local_vertex_index_buffer_offset;
		uint32 meshlet_header_buffer_offset;
		uint32 meshlet_buffer_offset;
		uint32 meshlet_count;
		float3 aabb_min;
		float3 aabb_size;
	};

	struct material
	{
		float4	 base_color_factor;
		float	 metallic_factor;
		float	 roughness_factor;
		float3	 emissive_factor;
		float	 normal_scale;
		float	 occlusion_strength;
		float	 alpha_cutoff;
		uint32	 alpha_mode_and_extra;
		uint32_2 _;

		uint32 base_color_texture_id;
		uint32 metallic_roughness_texture_id;
		uint32 normal_texture_id;
		uint32 occlusion_texture_id;
		uint32 emissive_texture_id;
	};

	cbuffer frame_data reg(b0)
	{
		row_major float4x4 view_proj;								// 64
		row_major float4x4 view_proj_inv;							// 64
		float3			   camera_pos;								// 12
		float			   time;									// 4
		float4			   frustum_planes[6];						// 96
		float2			   inv_backbuffer_size;						// 8
		float2			   backbuffer_size;							// 8
		float3			   camera_forward;							// 12
		uint32			   frame_index;								// 4
		float3			   camera_right;							// 12
		uint32			   main_buffer_texture_id;					// 4
		uint32			   post_buffer_texture_id;					// 4
		uint32			   depth_buffer_texture_id;					// 4
		uint32			   rt_tlas_buffer_id;						// 4
		uint32			   rt_transparent_buffer_srv_texture_id;	// 4
		uint32			   rt_transparent_buffer_uav_texture_id;	// 4
		uint32			   _0;
		uint32			   _1;
		uint32			   _2;

		uint32_4 extra[12];
		// total: 256 * 2 bytes
	};

	cbuffer root_constants reg(b1)
	{
		uint32			   opaque_meshlet_render_data_count;	 // 4 bytes
		uint32			   directional_light_count_and_extra;	 // 4 bytes
		t_unified_light_id unified_light_count;					 // 4 btyes
		// uint32			   transparent_object_render_data_count;

		uint32 light_tile_count_x;
		uint32 light_tile_count_y;
		float  cam_near_z;
		float  cam_far_z;
		float  cam_log_far_near_ratio;
		// uint32 shadow_atlas_id;		  // bindless index for shadow atlas
		uint32 radix_sort_pass;
		// uint32 shadow_light_index;	  // shadow mapping
		uint32 ui_data_id_offset;
		uint32 ui_data_count;
	};


#if !defined(AGE_SHADER)
}	 // namespace age::graphics::render_pipeline::forward_plus::shared_type

namespace age::graphics::e
{
	AGE_DEFINE_ENUM_WITH_VALUE(rt_mask, uint32,
							   (opaque, RT_MASK_OPAQUE),
							   (transparent, RT_MASK_TRANSPARENT),
							   (mask, RT_MASK_MASK),
							   (all, RT_MASK_ALL));
}

namespace age::graphics::render_pipeline::forward_plus::g
{
	// config
	inline constexpr uint32 max_sort_count			= MAX_SORT_COUNT;
	inline constexpr uint32 max_light_count			= MAX_LIGHT_COUNT;
	inline constexpr uint32 max_visible_light_count = MAX_VISIBLE_LIGHT_COUNT;
	inline constexpr uint32 light_tile_size			= LIGHT_TILE_SIZE;

	inline constexpr auto max_shadow_light_count = MAX_SHADOW_LIGHT_COUNT;

	// buffer offsets and size, texture size
	inline constexpr auto opaque_mshlt_object_data_offset = OPAQUE_MSHLT_OBJECT_DATA_OFFSET;
	inline constexpr auto object_data_offset			  = OBJECT_DATA_OFFSET;
	inline constexpr auto directional_light_offset		  = DIRECTIONAL_LIGHT_OFFSET;
	inline constexpr auto unified_light_offset			  = UNIFIED_LIGHT_OFFSET;
	inline constexpr auto static_buffer_size			  = STATIC_BUFFER_SIZE;

	inline constexpr uint32 scratch_buffer_total_size	= SCRATCH_BUFFER_TOTAL_SIZE;
	inline constexpr uint32 light_cull_tile_mask_offset = LIGHT_CULL_TILE_MASK_OFFSET;


	// sort
	inline constexpr uint32 sort_thread_count	 = SORT_THREAD_COUNT;
	inline constexpr uint32 sort_bin_count		 = SORT_BIN_COUNT;
	inline constexpr uint32 sort_iteration_count = sizeof(float) * 8 / SORT_BIN_BIT_WIDTH;
	inline constexpr uint32 sort_group_count	 = SORT_GROUP_COUNT;

	// light
	inline constexpr uint32 light_cull_thread_count	   = LIGHT_CULL_THREAD_COUNT;
	inline constexpr uint32 light_bitmask_uint32_count = LIGHT_BITMASK_UINT32_COUNT;
	inline constexpr uint32 zbin_thread_count		   = LIGHT_ZBIN_THREAD_COUNT;

	// ui
	inline constexpr auto max_ui_z_count = 128;

	static_assert(sizeof(age::ui::render_data) == sizeof(shared_type::ui_data));

	static_assert(MAX_LIGHT_COUNT <= MAX_SORT_COUNT);
	static_assert(MAX_SORT_COUNT % SORT_THREAD_COUNT == 0);
	static_assert(MAX_SORT_COUNT <= SORT_GROUP_COUNT * SORT_BLOCK_COUNT_PER_GROUP * SORT_BLOCK_SIZE);
	static_assert(SORT_GROUP_COUNT <= SORT_BLOCK_SIZE);
	static_assert(SORT_BIN_BIT_WIDTH == 4);
	static_assert(SORT_THREAD_COUNT <= 0xff);
	static_assert(SORT_THREAD_COUNT > 0);
	static_assert(std::popcount<uint32>(SORT_THREAD_COUNT) == 1);

	static_assert(UI_SHAPE_KIND_RECT == to_idx(age::ui::e::shape_kind::rect));
	static_assert(UI_SHAPE_KIND_CIRCLE == to_idx(age::ui::e::shape_kind::circle));
	static_assert(UI_SHAPE_KIND_ARROW_RIGHT == to_idx(age::ui::e::shape_kind::arrow_right));
	static_assert(UI_SHAPE_KIND_TEXT == to_idx(age::ui::e::shape_kind::text));
	static_assert(UI_SHAPE_KIND_CHECK == to_idx(age::ui::e::shape_kind::check));
	static_assert(UI_SHAPE_KIND_ROUNDED_RECT == to_idx(age::ui::e::shape_kind::rounded_rect));
	static_assert(UI_SHAPE_KIND_TRIANGLE == to_idx(age::ui::e::shape_kind::triangle));
	static_assert(UI_SHAPE_KIND_CROSS == to_idx(age::ui::e::shape_kind::cross));
	static_assert(UI_BRUSH_KIND_COLOR == to_idx(age::ui::e::brush_kind::color));

	static_assert(VERTEX_KIND_P_UV0 == to_idx(age::asset::e::vertex_kind::p_uv0));
	static_assert(VERTEX_KIND_PN_UV0 == to_idx(age::asset::e::vertex_kind::pn_uv0));
	static_assert(VERTEX_KIND_PNT_UV0 == to_idx(age::asset::e::vertex_kind::pnt_uv0));
	static_assert(VERTEX_KIND_P_UV1 == to_idx(age::asset::e::vertex_kind::p_uv1));
	static_assert(VERTEX_KIND_PN_UV1 == to_idx(age::asset::e::vertex_kind::pn_uv1));
	static_assert(VERTEX_KIND_PNT_UV1 == to_idx(age::asset::e::vertex_kind::pnt_uv1));
	static_assert(VERTEX_KIND_P_UV2 == to_idx(age::asset::e::vertex_kind::p_uv2));
	static_assert(VERTEX_KIND_PN_UV2 == to_idx(age::asset::e::vertex_kind::pn_uv2));
	static_assert(VERTEX_KIND_PNT_UV2 == to_idx(age::asset::e::vertex_kind::pnt_uv2));
	static_assert(VERTEX_KIND_P_UV3 == to_idx(age::asset::e::vertex_kind::p_uv3));
	static_assert(VERTEX_KIND_PN_UV3 == to_idx(age::asset::e::vertex_kind::pn_uv3));
	static_assert(VERTEX_KIND_PNT_UV3 == to_idx(age::asset::e::vertex_kind::pnt_uv3));

	static_assert(VERTEX_SIZE_P_UV0 == sizeof(age::asset::t_vertex_kind<age::asset::e::vertex_kind::p_uv0>));
	static_assert(VERTEX_SIZE_PN_UV0 == sizeof(age::asset::t_vertex_kind<age::asset::e::vertex_kind::pn_uv0>));
	static_assert(VERTEX_SIZE_PNT_UV0 == sizeof(age::asset::t_vertex_kind<age::asset::e::vertex_kind::pnt_uv0>));
	static_assert(VERTEX_SIZE_P_UV1 == sizeof(age::asset::t_vertex_kind<age::asset::e::vertex_kind::p_uv1>));
	static_assert(VERTEX_SIZE_PN_UV1 == sizeof(age::asset::t_vertex_kind<age::asset::e::vertex_kind::pn_uv1>));
	static_assert(VERTEX_SIZE_PNT_UV1 == sizeof(age::asset::t_vertex_kind<age::asset::e::vertex_kind::pnt_uv1>));
	static_assert(VERTEX_SIZE_P_UV2 == sizeof(age::asset::t_vertex_kind<age::asset::e::vertex_kind::p_uv2>));
	static_assert(VERTEX_SIZE_PN_UV2 == sizeof(age::asset::t_vertex_kind<age::asset::e::vertex_kind::pn_uv2>));
	static_assert(VERTEX_SIZE_PNT_UV2 == sizeof(age::asset::t_vertex_kind<age::asset::e::vertex_kind::pnt_uv2>));
	static_assert(VERTEX_SIZE_P_UV3 == sizeof(age::asset::t_vertex_kind<age::asset::e::vertex_kind::p_uv3>));
	static_assert(VERTEX_SIZE_PN_UV3 == sizeof(age::asset::t_vertex_kind<age::asset::e::vertex_kind::pn_uv3>));
	static_assert(VERTEX_SIZE_PNT_UV3 == sizeof(age::asset::t_vertex_kind<age::asset::e::vertex_kind::pnt_uv3>));

	static_assert(MATERIAL_ALPHA_BLEND_OPAQUE == to_idx(age::asset::e::alpha_mode_kind::opaque));
	static_assert(MATERIAL_ALPHA_BLEND_MASK == to_idx(age::asset::e::alpha_mode_kind::mask));
	static_assert(MATERIAL_ALPHA_BLEND_BLEND == to_idx(age::asset::e::alpha_mode_kind::blend));


	#undef reg
	#undef cbuffer
	#undef row_major
	#undef semantics

	#undef MAX_UV_COUNT

	// static buffer offset
	#undef MAX_OPAQUE_MESHLET_RENDER_DATA_COUNT
	#undef MAX_OBJECT_DATA_COUNT
	#undef MAX_DIRECTIONAL_LIGHT_COUNT
	#undef MAX_LIGHT_COUNT

	#undef MAX_SHADOW_LIGHT_COUNT

	// light cull
	#undef LIGHT_CULL_THREAD_COUNT
	#undef LIGHT_ZBIN_THREAD_COUNT

	#undef MAX_VISIBLE_LIGHT_COUNT
	#undef Z_SLICE_COUNT

	#undef LIGHT_TILE_SIZE
	#undef LIGHT_BITMASK_UINT32_COUNT

	#undef LIGHT_KIND_DIRECTIONAL
	#undef LIGHT_KIND_POINT
	#undef LIGHT_KIND_SPOT
	#undef LIGHT_KIND_AREA
	#undef LIGHT_KIND_VOLUMN

	// scratch buffer

	// sort
	#undef SORT_THREAD_COUNT
	#undef SORT_ELEMENT_COUNT_PER_THREAD

	#undef MAX_SORT_COUNT

	#undef SORT_BLOCK_SIZE

	#undef SORT_BLOCK_COUNT

	#if defined(AGE_SHADER)
		#undef SORT_GROUP_COUNT
	#else
		#undef SORT_GROUP_COUNT
	#endif

	#undef SORT_BLOCK_COUNT_PER_GROUP

	#undef SORT_BIN_BIT_WIDTH
	#undef SORT_BIN_COUNT

	#undef SORT_HISTOGRAM_TABLE_SIZE


	// transparent

	#if !defined(AGE_SHADER)
		#undef SHARED_TYPE
	#else
		#undef SHARED_TYPE
	#endif

	// static
	#undef OPAQUE_MSHLT_OBJECT_DATA_OFFSET
	#undef OBJECT_DATA_OFFSET
	#undef DIRECTIONAL_LIGHT_OFFSET
	#undef UNIFIED_LIGHT_OFFSET
	#undef STATIC_BUFFER_SIZE

	// scratch
	#undef SCRATCH_SORT_BUFFER_OFFSET

	#undef SORT_KEYS_OFFSET
	#undef SORT_KEYS_ALT_OFFSET
	#undef SORT_VALUES_OFFSET
	#undef SORT_VALUES_ALT_OFFSET
	#undef SORT_HISTOGRAM_OFFSET
	#undef SORT_BIN_COUNT_OFFSET

	#undef LIGHT_CULL_PACKED_AABB_OFFSET
	#undef VISIBLE_LIGHT_COUNT_OFFSET
	#undef SCRATCH_BUFFER_TOTAL_SIZE

	// light cull
	#undef LIGHT_CULL_ZBIN_OFFSET
	#undef LIGHT_CULL_TILE_MASK_OFFSET


	#undef RT_MASK_OPAQUE
	#undef RT_MASK_TRANSPARENT
	#undef RT_MASK_ALL

	#undef UI_SHAPE_KIND_RECT
	#undef UI_SHAPE_KIND_CIRCLE

	#undef UI_BRUSH_KIND_COLOR

	#undef VERTEX_KIND_P_UV0
	#undef VERTEX_KIND_PN_UV0
	#undef VERTEX_KIND_PNT_UV0
	#undef VERTEX_KIND_P_UV1
	#undef VERTEX_KIND_PN_UV1
	#undef VERTEX_KIND_PNT_UV1
	#undef VERTEX_KIND_P_UV2
	#undef VERTEX_KIND_PN_UV2
	#undef VERTEX_KIND_PNT_UV2
	#undef VERTEX_KIND_P_UV3
	#undef VERTEX_KIND_PN_UV3
	#undef VERTEX_KIND_PNT_UV3


	#undef VERTEX_SIZE_P_UV0
	#undef VERTEX_SIZE_PN_UV0
	#undef VERTEX_SIZE_PNT_UV0
	#undef VERTEX_SIZE_P_UV1
	#undef VERTEX_SIZE_PN_UV1
	#undef VERTEX_SIZE_PNT_UV1
	#undef VERTEX_SIZE_P_UV2
	#undef VERTEX_SIZE_PN_UV2
	#undef VERTEX_SIZE_PNT_UV2
	#undef VERTEX_SIZE_P_UV3
	#undef VERTEX_SIZE_PN_UV3
	#undef VERTEX_SIZE_PNT_UV3

	#undef MATERIAL_ALPHA_BLEND_OPAQUE
	#undef MATERIAL_ALPHA_BLEND_MASK
	#undef MATERIAL_ALPHA_BLEND_BLEND
}	 // namespace age::graphics::render_pipeline::forward_plus::g

#else

#endif
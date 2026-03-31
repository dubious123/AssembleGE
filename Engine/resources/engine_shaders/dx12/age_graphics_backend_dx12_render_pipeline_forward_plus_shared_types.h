#pragma once

#define UV_COUNT 2

// static buffer offset
#define MAX_OPAQUE_MESHLET_RENDER_DATA_COUNT (1u << 20)
#define MAX_OBJECT_DATA_COUNT				 1024u
#define MAX_DIRECTIONAL_LIGHT_COUNT			 2
#define MAX_LIGHT_COUNT						 (512 * 512)

// shadow
#define SHADOW_DEPTH_REDUCE_THREAD_COUNT 16

#define SHADOW_MAP_WIDTH  (2048 * 2)
#define SHADOW_MAP_HEIGHT (2048 * 2)

#define SHADOW_ATLAS_WIDTH	(SHADOW_MAP_WIDTH * SHADOW_ATLAS_SEG_U)
#define SHADOW_ATLAS_HEIGHT (SHADOW_MAP_HEIGHT * SHADOW_ATLAS_SEG_V)

#define SHADOW_DEPTH_BIAS 1000
#define SHADOW_SLOPE_BIAS 5.f

#define SHADOW_CASCADE_SPLIT_FACTOR 0.5f
#define DIRECTIONAL_SHADOW_BACKOFF	50.f

#define SHADOW_ATLAS_SEG_U 4
#define SHADOW_ATLAS_SEG_V 4

#define MAX_SHADOW_LIGHT_COUNT (SHADOW_ATLAS_SEG_U * SHADOW_ATLAS_SEG_V)

#define SHADOW_CASCADE_COUNT 4

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

// scratch buffer

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
#define SHADOW_LIGHT_HEADER_OFFSET		(UNIFIED_LIGHT_OFFSET + sizeof(SHARED_TYPE unified_light) * MAX_LIGHT_COUNT)
#define STATIC_BUFFER_SIZE				(SHADOW_LIGHT_HEADER_OFFSET + sizeof(SHARED_TYPE shadow_light_header) * MAX_SHADOW_LIGHT_COUNT)

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

// shadow
#define SHADOW_STAGE_BUFFER_Z_MIN_OFFSET   (0)
#define SHADOW_STAGE_BUFFER_Z_MAX_OFFSET   (SHADOW_STAGE_BUFFER_Z_MIN_OFFSET + sizeof(uint32))
#define SHADOW_STAGE_BUFFER_CASCADE_OFFSET (SHADOW_STAGE_BUFFER_Z_MAX_OFFSET + sizeof(uint32))
#define SHADOW_STAGE_BUFFER_SIZE		   (SHADOW_STAGE_BUFFER_CASCADE_OFFSET + sizeof(float) * SHADOW_CASCADE_COUNT)

// light cull
#define LIGHT_CULL_ZBIN_OFFSET		(0)
#define LIGHT_CULL_TILE_MASK_OFFSET (LIGHT_CULL_ZBIN_OFFSET + sizeof(SHARED_TYPE zbin_entry) * Z_SLICE_COUNT)


#define RT_MASK_OPAQUE		0x01
#define RT_MASK_TRANSPARENT 0x02
#define RT_MASK_ALL			0xff

// ui
#define UI_SHAPE_KIND_RECT		  0
#define UI_SHAPE_KIND_CIRCLE	  1
#define UI_SHAPE_KIND_ARROW_RIGHT 2

#define UI_BRUSH_KIND_COLOR 0


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
	using t_shadow_light_id		 = uint16;
#if !defined(AGE_SHADER)

}	 // namespace age::graphics::render_pipeline::forward_plus

namespace age::graphics::render_pipeline::forward_plus::shared_type
{
#endif
	//---[ ui ]------------------------------------------------------------
	struct ui_shape_data
	{
		uint32_4 data;			// TBD, corner radius, circle radius, ...
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
	struct shadow_light
	{
		row_major float4x4 view_proj;			 // 64
		float4			   frustum_planes[6];	 // 96
	};	  // 160 bytes

	struct shadow_light_header
	{
		uint32 light_id;
		uint16 light_kind;
		uint16 shadow_id;
	};

	struct directional_light
	{
		float3 direction;			   // 12
		float  intensity;			   // 4
		float3 color;				   // 12
		uint32 shadow_id_and_extra;	   // 4
	};	  // 32 bytes

	struct unified_light
	{
		float3 position;			   // 12
		float  range;				   // 4
		half3  color;				   // 6
		half   intensity;			   // 2
		half3  direction;			   // 6
		half   cos_inner;			   // 2
		half   cos_outer;			   // 2
		uint16 shadow_id_and_extra;	   // 2
	};	  // total: 36 bytes

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

	struct vertex_encoded
	{
		uint16_3 pos;
		uint16	 normal_oct;
		uint16	 tangent_oct;
		uint16	 extra;
#if UV_COUNT > 0
		half2 uv_set[UV_COUNT];
#endif
	};

	struct vertex_decoded
	{
		float4 pos	   semantics(SV_Position);
		float3 normal  semantics(NORMAL);
		float4 tangent semantics(TANGENT);

		// clang-format off
#if UV_COUNT >= 1
		half2 uv0 semantics(TEXCOORD0);
#endif
#if UV_COUNT >= 2
		half2 uv1 semantics(TEXCOORD1);
#endif
#if UV_COUNT >= 3
		half2 uv2 semantics(TEXCOORD2);
#endif
#if UV_COUNT >= 4
		half2 uv3 semantics(TEXCOORD3);
#endif
		// clang-format on
	};

	struct object_data
	{
		float3	 pos;			// 12
		uint32	 quaternion;	// 4 | 10 10 10 2
		half3	 scale;			// 6
		uint16_t extra;			// 2
	};	  // total: 24 bytes

	struct opaque_meshlet_render_data
	{
		uint32 object_id;
		uint32 mesh_byte_offset;
		uint32 meshlet_id;
	};

	struct rt_instance_render_data
	{
		uint32 object_id;
		uint32 mesh_byte_offset;
		uint32 rt_index_buffer_offset;
		// todo mat_id
	};

	struct mesh_header
	{
		// uint32 vertex_offset = sizeof(mesh_baked_header), sizeof(mesh_baked_header) == 20
#if !defined(AGE_SHADER)
#else
	uint32 vertex_buffer_offset;	// not from gpu, calculated from read_mesh_header function
#endif

		uint32 global_vertex_index_buffer_offset;
		uint32 local_vertex_index_buffer_offset;
		uint32 meshlet_header_buffer_offset;
		uint32 meshlet_buffer_offset;
		uint32 meshlet_count;
		float3 aabb_min;
		float3 aabb_size;
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
		uint32			   depth_buffer_texture_id;					// 4
		uint32			   rt_tlas_buffer_id;						// 4
		uint32			   rt_transparent_buffer_srv_texture_id;	// 4
		uint32			   rt_transparent_buffer_uav_texture_id;	// 4

		uint32_4 extra[13];
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
		uint32 shadow_atlas_id;		  // bindless index for shadow atlas
		uint32 radix_sort_pass;
		uint32 shadow_light_index;	  // shadow mapping
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
							   (all, RT_MASK_ALL));
}

namespace age::graphics::render_pipeline::forward_plus::g
{
	// config
	inline constexpr uint32 max_sort_count			= MAX_SORT_COUNT;
	inline constexpr uint32 max_light_count			= MAX_LIGHT_COUNT;
	inline constexpr uint32 max_visible_light_count = MAX_VISIBLE_LIGHT_COUNT;
	inline constexpr uint32 light_tile_size			= LIGHT_TILE_SIZE;

	inline constexpr auto directional_shadow_cascade_count = SHADOW_CASCADE_COUNT;
	inline constexpr auto shadow_map_width				   = SHADOW_MAP_WIDTH;
	inline constexpr auto shadow_map_height				   = SHADOW_MAP_HEIGHT;
	inline constexpr auto shadow_atlas_seg_u			   = SHADOW_ATLAS_SEG_U;
	inline constexpr auto shadow_atlas_seg_v			   = SHADOW_ATLAS_SEG_V;
	inline constexpr auto max_shadow_light_count		   = MAX_SHADOW_LIGHT_COUNT;

	inline constexpr auto shadow_depth_bias = SHADOW_DEPTH_BIAS;
	inline constexpr auto shadow_slope_bias = SHADOW_SLOPE_BIAS;


	// buffer offsets and size, texture size
	inline constexpr auto opaque_mshlt_object_data_offset = OPAQUE_MSHLT_OBJECT_DATA_OFFSET;
	inline constexpr auto object_data_offset			  = OBJECT_DATA_OFFSET;
	inline constexpr auto directional_light_offset		  = DIRECTIONAL_LIGHT_OFFSET;
	inline constexpr auto unified_light_offset			  = UNIFIED_LIGHT_OFFSET;
	inline constexpr auto shadow_light_header_offset	  = SHADOW_LIGHT_HEADER_OFFSET;
	inline constexpr auto static_buffer_size			  = STATIC_BUFFER_SIZE;

	inline constexpr uint32 scratch_buffer_total_size	= SCRATCH_BUFFER_TOTAL_SIZE;
	inline constexpr uint32 light_cull_tile_mask_offset = LIGHT_CULL_TILE_MASK_OFFSET;
	inline constexpr uint32 shadow_stage_buffer_size	= SHADOW_STAGE_BUFFER_SIZE;


	// sort
	inline constexpr uint32 sort_thread_count	 = SORT_THREAD_COUNT;
	inline constexpr uint32 sort_bin_count		 = SORT_BIN_COUNT;
	inline constexpr uint32 sort_iteration_count = sizeof(float) * 8 / SORT_BIN_BIT_WIDTH;
	inline constexpr uint32 sort_group_count	 = SORT_GROUP_COUNT;

	// light
	inline constexpr uint32 light_cull_thread_count	   = LIGHT_CULL_THREAD_COUNT;
	inline constexpr uint32 light_bitmask_uint32_count = LIGHT_BITMASK_UINT32_COUNT;
	inline constexpr uint32 zbin_thread_count		   = LIGHT_ZBIN_THREAD_COUNT;

	// shadow
	inline constexpr auto	shadow_depth_reduce_thread_count = SHADOW_DEPTH_REDUCE_THREAD_COUNT;
	inline constexpr uint32 shadow_atlas_width				 = SHADOW_ATLAS_WIDTH;
	inline constexpr uint32 shadow_atlas_height				 = SHADOW_ATLAS_HEIGHT;

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

	#undef reg
	#undef cbuffer
	#undef row_major
	#undef semantics

	#undef UV_COUNT

	// static buffer offset
	#undef MAX_OPAQUE_MESHLET_RENDER_DATA_COUNT
	#undef MAX_OBJECT_DATA_COUNT
	#undef MAX_DIRECTIONAL_LIGHT_COUNT
	#undef MAX_LIGHT_COUNT

	// shadow
	#undef SHADOW_DEPTH_REDUCE_THREAD_COUNT

	#undef SHADOW_MAP_WIDTH
	#undef SHADOW_MAP_HEIGHT

	#undef SHADOW_ATLAS_WIDTH
	#undef SHADOW_ATLAS_HEIGHT

	#undef SHADOW_DEPTH_BIAS
	#undef SHADOW_SLOPE_BIAS

	#undef SHADOW_CASCADE_SPLIT_FACTOR
	#undef DIRECTIONAL_SHADOW_BACKOFF

	#undef SHADOW_ATLAS_SEG_U
	#undef SHADOW_ATLAS_SEG_V

	#undef MAX_SHADOW_LIGHT_COUNT

	#undef SHADOW_CASCADE_COUNT

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
	#undef SHADOW_LIGHT_HEADER_OFFSET
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

	// shadow
	#undef SHADOW_STAGE_BUFFER_Z_MIN_OFFSET
	#undef SHADOW_STAGE_BUFFER_Z_MAX_OFFSET
	#undef SHADOW_STAGE_BUFFER_CASCADE_OFFSET
	#undef SHADOW_STAGE_BUFFER_SIZE

	// light cull
	#undef LIGHT_CULL_ZBIN_OFFSET
	#undef LIGHT_CULL_TILE_MASK_OFFSET


	#undef RT_MASK_OPAQUE
	#undef RT_MASK_TRANSPARENT
	#undef RT_MASK_ALL

	#undef UI_SHAPE_KIND_RECT
	#undef UI_SHAPE_KIND_CIRCLE

	#undef UI_BRUSH_KIND_COLOR
}	 // namespace age::graphics::render_pipeline::forward_plus::g

#else

#endif
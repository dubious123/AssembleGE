#pragma once
#define UV_COUNT		2
#define LIGHT_TILE_SIZE 32

#define MAX_LIGHT_COUNT (1024 * 1024)

#define MAX_VISIBLE_LIGHT_COUNT (16 * 1024)
#define LIGHT_CULL_THREAD_COUNT 256

#define LIGHT_SORT_THREAD_COUNT				128
#define LIGHT_SORT_ELEMENT_COUNT_PER_THREAD 4
#define LIGHT_SORT_BLOCK_SIZE				(LIGHT_SORT_THREAD_COUNT * LIGHT_SORT_ELEMENT_COUNT_PER_THREAD)

#define LIGHT_ZBIN_THREAD_COUNT 256

#define LIGHT_SORT_BLOCK_COUNT ((MAX_VISIBLE_LIGHT_COUNT + LIGHT_SORT_BLOCK_SIZE - 1) / LIGHT_SORT_BLOCK_SIZE)

#if defined(AGE_HLSL)
	#define LIGHT_SORT_GROUP_COUNT (min(LIGHT_SORT_BLOCK_SIZE, LIGHT_SORT_BLOCK_COUNT))
#else
	#define LIGHT_SORT_GROUP_COUNT (std::min(LIGHT_SORT_BLOCK_SIZE, LIGHT_SORT_BLOCK_COUNT))
#endif

#define LIGHT_SORT_BLOCK_COUNT_PER_GROUP ((LIGHT_SORT_BLOCK_COUNT + LIGHT_SORT_GROUP_COUNT - 1) / LIGHT_SORT_GROUP_COUNT)


#define LIGHT_SORT_BIN_BIT_WIDTH 4
#define LIGHT_SORT_BIN_COUNT	 (1 << LIGHT_SORT_BIN_BIT_WIDTH)

#define LIGHT_SORT_HISTOGRAM_TABLE_SIZE (LIGHT_SORT_BIN_COUNT * LIGHT_SORT_GROUP_COUNT)

#define LIGHT_SORT_SORT_KEYS_OFFSET		  0																   // 0
#define LIGHT_SORT_SORT_KEYS_ALT_OFFSET	  (LIGHT_SORT_SORT_KEYS_OFFSET + MAX_VISIBLE_LIGHT_COUNT)		   // 16384
#define LIGHT_SORT_SORT_VALUES_OFFSET	  (LIGHT_SORT_SORT_KEYS_ALT_OFFSET + MAX_VISIBLE_LIGHT_COUNT)	   // 32768
#define LIGHT_SORT_SORT_VALUES_ALT_OFFSET (LIGHT_SORT_SORT_VALUES_OFFSET + MAX_VISIBLE_LIGHT_COUNT)		   // 49152
#define LIGHT_SORT_HISTOGRAM_OFFSET		  (LIGHT_SORT_SORT_VALUES_ALT_OFFSET + MAX_VISIBLE_LIGHT_COUNT)	   // 65536
#define LIGHT_SORT_BIN_COUNT_OFFSET		  (LIGHT_SORT_HISTOGRAM_OFFSET + LIGHT_SORT_HISTOGRAM_TABLE_SIZE)
#define LIGHT_SORT_SORT_BUFFER_TOTAL_SIZE (LIGHT_SORT_BIN_COUNT_OFFSET + LIGHT_SORT_BIN_COUNT)			   //

#define Z_SLICE_COUNT (512)

#define LIGHT_BITMASK_UINT32_COUNT (MAX_VISIBLE_LIGHT_COUNT / 32)


#define SHADOW_MAP_WIDTH  (2048 * 2)
#define SHADOW_MAP_HEIGHT (2048 * 2)

#define SHADOW_ATLAS_SEG_U 4
#define SHADOW_ATLAS_SEG_V 4

#define MAX_SHADOW_LIGHT_COUNT (SHADOW_ATLAS_SEG_U * SHADOW_ATLAS_SEG_V)

#define SHADOW_ATLAS_WIDTH	(SHADOW_MAP_WIDTH * SHADOW_ATLAS_SEG_U)
#define SHADOW_ATLAS_HEIGHT (SHADOW_MAP_HEIGHT * SHADOW_ATLAS_SEG_V)

#define SHADOW_DEPTH_BIAS 100
#define SHADOW_SLOPE_BIAS 2.f

#define DIRECTIONAL_SHADOW_CASCADE_COUNT 4

#if !defined(AGE_HLSL)
	#include "age.hpp"

namespace age::graphics::render_pipeline::forward_plus
{
	using t_object_id = uint32;
	using t_mesh_id	  = uint32;
	using t_camera_id = uint32;

	using t_directional_light_id = uint8;
	using t_unified_light_id	 = uint32;
	using t_shadow_light_id		 = uint8;

	using t_global_light_index = uint32;
}	 // namespace age::graphics::render_pipeline::forward_plus

namespace age::graphics::render_pipeline::forward_plus::g
{
	inline constexpr uint8	uv_count		= UV_COUNT;
	inline constexpr uint32 max_light_count = MAX_LIGHT_COUNT;

	inline constexpr uint32 max_visible_light_count = MAX_VISIBLE_LIGHT_COUNT;

	inline constexpr uint32 light_sort_bin_count	   = LIGHT_SORT_BIN_COUNT;
	inline constexpr uint32 light_sort_iteration_count = sizeof(float) * 8 / LIGHT_SORT_BIN_BIT_WIDTH;
	inline constexpr uint32 light_sort_group_count	   = LIGHT_SORT_GROUP_COUNT;

	// light culling

	inline constexpr uint8 light_tile_size = (uint8)LIGHT_TILE_SIZE;

	inline constexpr uint32 light_cull_cs_thread_count = LIGHT_CULL_THREAD_COUNT;
	inline constexpr uint32 light_sort_cs_thread_count = LIGHT_SORT_THREAD_COUNT;
	inline constexpr uint32 light_bitmask_uint32_count = LIGHT_BITMASK_UINT32_COUNT;

	inline constexpr uint32 sort_buffer_total_byte_size = LIGHT_SORT_SORT_BUFFER_TOTAL_SIZE * sizeof(uint32);

	inline constexpr uint32 z_slice_count = Z_SLICE_COUNT;


	inline constexpr auto shadow_map_width	= SHADOW_MAP_WIDTH;
	inline constexpr auto shadow_map_height = SHADOW_MAP_HEIGHT;

	inline constexpr auto shadow_atlas_seg_u = SHADOW_ATLAS_SEG_U;
	inline constexpr auto shadow_atlas_seg_v = SHADOW_ATLAS_SEG_V;

	inline constexpr auto max_shadow_light_count = MAX_SHADOW_LIGHT_COUNT;

	inline constexpr uint32 shadow_atlas_width	= SHADOW_ATLAS_WIDTH;
	inline constexpr uint32 shadow_atlas_height = SHADOW_ATLAS_HEIGHT;

	inline constexpr auto shadow_depth_bias = SHADOW_DEPTH_BIAS;
	inline constexpr auto shadow_slope_bias = SHADOW_SLOPE_BIAS;

	inline constexpr uint32 directional_shadow_cascade_count = DIRECTIONAL_SHADOW_CASCADE_COUNT;

	static_assert(MAX_VISIBLE_LIGHT_COUNT % g::light_sort_cs_thread_count == 0);

	static_assert(MAX_VISIBLE_LIGHT_COUNT <= LIGHT_SORT_GROUP_COUNT * LIGHT_SORT_BLOCK_COUNT_PER_GROUP * LIGHT_SORT_BLOCK_SIZE);
	static_assert(LIGHT_SORT_GROUP_COUNT <= LIGHT_SORT_BLOCK_SIZE);

	static_assert(LIGHT_SORT_BIN_BIT_WIDTH == 4);
	static_assert(LIGHT_SORT_THREAD_COUNT <= 0xff);
	static_assert(LIGHT_SORT_THREAD_COUNT > 0);
	static_assert(std::popcount<uint32>(LIGHT_SORT_THREAD_COUNT) == 1);
}	 // namespace age::graphics::render_pipeline::forward_plus::g

namespace age::graphics::render_pipeline::forward_plus::shared_type
{
	#define SYS_VAL(name)
	#define cbuffer struct
	#define REG(...)
	#define row_major

#else
	#define t_object_id uint32
	#define t_mesh_id	uint32
	#define t_camera_id uint32

	#define t_directional_light_id uint8
	#define t_unified_light_id	   uint32
	#define t_shadow_light_id	   uint8

	#define UV_COUNT 2
#endif
	struct debug_77
	{
		uint32 tile_min_x;
		uint32 tile_max_x;
		uint32 tile_min_y;
		uint32 tile_max_y;

		float2 screen_min;
		float2 screen_max;

		float2 backbuffer_size;
		uint32 visible_count;
		uint32 invalid_count;

		uint32 tile_bit_mask_arr[100];
	};

	struct frame_data_rw
	{
		uint32 generic_counter;
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

	struct directional_light
	{
		float3 direction;	 // 12
		float  intensity;	 // 4
		float3 color;		 // 12
		uint32 shadow_id;	 // 4
	};	  // 32 bytes

	struct point_light
	{
		float3 position;	 // 12
		float  range;		 // 4
		float3 color;		 // 12
		float  intensity;	 // 4
	};	  // 32 bytes

	struct spot_light
	{
		float3 position;	 // 12
		float  range;		 // 4
		float3 direction;	 // 12
		float  intensity;	 // 4
		float3 color;		 // 12
		float  cos_inner;	 // 4  (falloff begin, cosine)
		float  cos_outer;	 // 4  (cosine)
	};	  // 52 bytes (1 cache line)

	// struct unified_light
	//{
	//	float3 position;	 // 12
	//	float  range;		 // 4
	//	float3 color;		 // 12
	//	float  intensity;	 // 4
	//	float3 direction;	 // 12
	//	float  cos_inner;	 // 4
	//	float  cos_outer;	 // 4
	//	uint32 padding;		 // 4
	// };	  // total: 56 bytes

	struct unified_light
	{
		float3 position;	 // 12
		float  range;		 // 4
		half3  color;		 // 6
		half   intensity;	 // 2
		half3  direction;	 // 6
		half   cos_inner;	 // 2
		half   cos_outer;	 // 2
		uint16 padding;		 // 2
	};	  // total: 36 bytes

	//---[ light culling ]------------------------------------------------------------

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
		float4 pos	   SYS_VAL(SV_Position);
		float3 normal  SYS_VAL(NORMAL);
		float4 tangent SYS_VAL(TANGENT);

		// clang-format off
#if UV_COUNT >= 1
		half2 uv0 SYS_VAL(TEXCOORD0);
#endif
#if UV_COUNT >= 2
		half2 uv1 SYS_VAL(TEXCOORD1);
#endif
#if UV_COUNT >= 3
		half2 uv2 SYS_VAL(TEXCOORD2);
#endif
#if UV_COUNT >= 4
		half2 uv3 SYS_VAL(TEXCOORD3);
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

	struct job_data
	{
		uint32 object_id;
		uint32 mesh_byte_offset;
		uint32 meshlet_id;
	};

	struct mesh_header
	{
		// uint32 vertex_offset = sizeof(mesh_baked_header), sizeof(mesh_baked_header) == 20
#if !defined(AGE_HLSL)
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

	cbuffer frame_data REG(b0)
	{
		row_major float4x4 view_proj;										// 64 bytes
		row_major float4x4 view_proj_inv;									// 64 bytes
		float3			   camera_pos;										// 12
		float			   time;											// 4
		float4			   frustum_planes[6];								// 96
		float2			   inv_backbuffer_size;								// 8
		float2			   backbuffer_size;									// 8
		float3			   camera_forward;									// 12
		uint32			   frame_index;										// 4
		float3			   camera_right;									// 12
		uint32			   main_buffer_texture_id;							// 4
		float4			   cascade_splits[(DIRECTIONAL_SHADOW_CASCADE_COUNT + 3) / 4];

		uint32_4 extra[14 - (DIRECTIONAL_SHADOW_CASCADE_COUNT + 3) / 4];	//
																			// total: 256 * 2 bytes
	};

	cbuffer root_constants REG(b1)
	{
		uint32			   job_count;										// 4 bytes
		uint32			   directional_light_count_and_extra;				// 4 bytes
		t_unified_light_id unified_light_count;								// 4 btyes

		uint32 cluster_tile_count_x;
		uint32 cluster_tile_count_y;
		float  cluster_near_z;
		float  cluster_far_z;
		float  cluster_log_far_near_ratio;
		uint32 shadow_atlas_id;	   // bindless index for shadow atlas
		uint32 light_radix_sort_pass;
		uint32 shadow_light_index;
	};

#if !defined(AGE_HLSL)
	#undef SYS_VAL
	#undef cbuffer
	#undef REG
	#undef row_major

	#undef UV_COUNT
	#undef MAX_LIGHT_COUNT
	#undef LIGHT_TILE_SIZE
	#undef LIGHT_CULL_THREAD_COUNT
	#undef LIGHT_SORT_THREAD_COUNT
	#undef MAX_VISIBLE_LIGHT_COUNT
	#undef SORT_GROUP_COUNT
	#undef HISTOGRAM_SIZE
	#undef LIGHT_SORT_SORT_KEYS_OFFSET
	#undef LIGHT_SORT_SORT_KEYS_ALT_OFFSET
	#undef LIGHT_SORT_SORT_VALUES_OFFSET
	#undef LIGHT_SORT_SORT_VALUES_ALT_OFFSET
	#undef LIGHT_SORT_HISTOGRAM_OFFSET
	#undef LIGHT_SORT_SORT_BUFFER_TOTAL_SIZE
	#undef Z_SLICE_COUNT
	#undef LIGHT_BITMASK_UINT32_COUNT
	#undef LIGHT_TYPE_POINT
	#undef LIGHT_TYPE_SPOT
	#undef LIGHT_TYPE_BITS
	#undef LIGHT_INDEX_MASK

}	 // namespace age::graphics::render_pipeline::forward_plus
#else

#endif
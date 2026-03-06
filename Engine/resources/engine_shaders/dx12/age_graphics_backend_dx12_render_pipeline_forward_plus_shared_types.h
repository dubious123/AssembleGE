#pragma once
#define UV_COUNT							2
#define CLUSTER_TILE_SIZE					16
#define CLUSTER_DEPTH_SLICE_COUNT			24
#define CLUSTER_MAX_LIGHT_COUNT_PER_CLUSTER 1000
#define MAX_GLOBAL_LIGHT_INDEX_COUNT		(256 * 1024 * 1024)


#define LIGHT_CULL_CS_THREAD_COUNT		   256
#define LIGHT_CULL_CS_MAX_CULL_LIGHT_COUNT (1024 * 1024)

#define LIGHT_SORT_CS_THREAD_COUNT			  256
#define LIGHT_SORT_CS_MAX_VISIBLE_LIGHT_COUNT (16 * 1024)

#define SORT_GROUP_COUNT					 (LIGHT_SORT_CS_MAX_VISIBLE_LIGHT_COUNT / LIGHT_SORT_CS_THREAD_COUNT)
#define HISTOGRAM_SIZE						 (256 * SORT_GROUP_COUNT)
#define LIGHT_SORT_CS_SORT_KEYS_OFFSET		 0																				   // 0
#define LIGHT_SORT_CS_SORT_KEYS_ALT_OFFSET	 (LIGHT_SORT_CS_SORT_KEYS_OFFSET + LIGHT_SORT_CS_MAX_VISIBLE_LIGHT_COUNT)		   // 16384
#define LIGHT_SORT_CS_SORT_VALUES_OFFSET	 (LIGHT_SORT_CS_SORT_KEYS_ALT_OFFSET + LIGHT_SORT_CS_MAX_VISIBLE_LIGHT_COUNT)	   // 32768
#define LIGHT_SORT_CS_SORT_VALUES_ALT_OFFSET (LIGHT_SORT_CS_SORT_VALUES_OFFSET + LIGHT_SORT_CS_MAX_VISIBLE_LIGHT_COUNT)		   // 49152
#define LIGHT_SORT_CS_HISTOGRAM_OFFSET		 (LIGHT_SORT_CS_SORT_VALUES_ALT_OFFSET + LIGHT_SORT_CS_MAX_VISIBLE_LIGHT_COUNT)	   // 65536
#define LIGHT_SORT_CS_SORT_BUFFER_TOTAL_SIZE (LIGHT_SORT_CS_HISTOGRAM_OFFSET + HISTOGRAM_SIZE)								   // 65536 + 16384 = 81920

#define Z_SLICE_COUNT (4 * 1024)

#define LIGHT_BITMASK_UINT32_COUNT (LIGHT_SORT_CS_MAX_VISIBLE_LIGHT_COUNT / 32)

#define LIGHT_TYPE_POINT 0
#define LIGHT_TYPE_SPOT	 1
#define LIGHT_TYPE_BITS	 3
#define LIGHT_INDEX_MASK ((1u << (32 - LIGHT_TYPE_BITS)) - 1)

#if !defined(AGE_HLSL)
	#include "age.hpp"

namespace age::graphics::render_pipeline::forward_plus
{
	using t_object_id = uint32;
	using t_mesh_id	  = uint32;
	using t_camera_id = uint32;

	using t_directional_light_id = uint8;
	using t_point_light_id		 = uint32;
	using t_spot_light_id		 = uint32;

	using t_global_light_index = uint32;
}	 // namespace age::graphics::render_pipeline::forward_plus

namespace age::graphics::render_pipeline::forward_plus::g
{
	inline constexpr uint8 uv_count = UV_COUNT;

	// light culling
	inline constexpr uint8	light_culling_cluster_tile_size					  = (uint8)CLUSTER_TILE_SIZE;
	inline constexpr uint8	light_culling_cluster_depth_slice_count			  = (uint8)CLUSTER_DEPTH_SLICE_COUNT;
	inline constexpr uint8	light_culling_cluster_max_light_count_per_cluster = (uint8)CLUSTER_MAX_LIGHT_COUNT_PER_CLUSTER;
	inline constexpr uint32 light_culling_max_global_light_index_count		  = MAX_GLOBAL_LIGHT_INDEX_COUNT;	 // 4M
	inline constexpr uint8	light_culling_depth_slice_count					  = 24u;

	inline constexpr uint32 light_cull_cs_thread_count		   = LIGHT_CULL_CS_THREAD_COUNT;
	inline constexpr uint32 light_cull_cs_max_cull_light_count = LIGHT_CULL_CS_MAX_CULL_LIGHT_COUNT;

	inline constexpr uint32 light_sort_cs_thread_count			  = LIGHT_SORT_CS_THREAD_COUNT;
	inline constexpr uint32 light_sort_cs_max_visible_light_count = LIGHT_SORT_CS_MAX_VISIBLE_LIGHT_COUNT;
	inline constexpr uint32 light_bitmask_uint32_count			  = LIGHT_BITMASK_UINT32_COUNT;

	inline constexpr uint32 sort_group_count = SORT_GROUP_COUNT;
	inline constexpr uint32 histogram_size	 = HISTOGRAM_SIZE;

	inline constexpr uint32 sort_keys_offset			= LIGHT_SORT_CS_SORT_KEYS_OFFSET;
	inline constexpr uint32 sort_keys_alt_offset		= LIGHT_SORT_CS_SORT_KEYS_ALT_OFFSET;
	inline constexpr uint32 sort_values_offset			= LIGHT_SORT_CS_SORT_VALUES_OFFSET;
	inline constexpr uint32 sort_values_alt_offset		= LIGHT_SORT_CS_SORT_VALUES_ALT_OFFSET;
	inline constexpr uint32 histogram_offset			= LIGHT_SORT_CS_HISTOGRAM_OFFSET;
	inline constexpr uint32 sort_buffer_total_size		= LIGHT_SORT_CS_SORT_BUFFER_TOTAL_SIZE;
	inline constexpr uint32 sort_buffer_total_byte_size = sort_buffer_total_size * sizeof(uint32);

	inline constexpr uint32 z_slice_count = Z_SLICE_COUNT;


	static_assert(g::light_cull_cs_max_cull_light_count % g::light_sort_cs_thread_count == 0);
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
	#define t_point_light_id	   uint32
	#define t_spot_light_id		   uint32

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
	struct directional_light
	{
		float3 direction;		// 12
		float  intensity;		// 4
		float3 color;			// 12
		uint32 shadow_index;	// 4
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

	//---[ light culling ]------------------------------------------------------------

	struct cluster_light_info
	{
		uint32 offset;	  // start index in global_light_index_buffer
		uint32 count;	  // number of lights affecting this cluster
	};

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
		row_major float4x4 view_proj;						   // 64 bytes
		row_major float4x4 view_proj_inv;					   // 64 bytes
		float3			   camera_pos;						   // 12
		float			   time;							   // 4
		float4			   frustum_planes[6];				   // 96
		float2			   inv_backbuffer_size;				   // 8
		float2			   backbuffer_size;					   // 8
		float3			   camera_forward;					   // 12
		uint32			   frame_index;						   // 4
		float3			   camera_right;					   // 12
		uint32			   main_buffer_texture_id;			   // 4

		uint32 extra[56];									   // 4 * 56
															   // total: 256 * 2 bytes
	};

	cbuffer root_constants REG(b1)
	{
		uint32			 job_count;							   // 4 bytes
		uint32			 directional_light_count_and_extra;	   // 4 bytes
		t_point_light_id point_light_count;					   // 4 btyes
		t_spot_light_id	 spot_light_count;					   // 4 bytes

		uint32 cluster_tile_count_x;
		uint32 cluster_tile_count_y;
		float  cluster_near_z;
		float  cluster_far_z;
		float  cluster_log_far_near_ratio;
		uint32 depth_buffer_texture_id;	   // bindless index for depth buffer
		uint32 light_radix_sort_pass;
	};

#if !defined(AGE_HLSL)
	#undef UV_COUNT

	#undef SYS_VAL
	#undef cbuffer
	#undef REG
	#undef row_major

	#undef CLUSTER_TILE_SIZE
	#undef CLUSTER_DEPTH_SLICE_COUNT
	#undef CLUSTER_MAX_LIGHT_COUNT_PER_CLUSTER
	#undef MAX_GLOBAL_LIGHT_INDEX_COUNT

	#undef LIGHT_TYPE_POINT
	#undef LIGHT_TYPE_SPOT
	#undef LIGHT_TYPE_BITS
	#undef LIGHT_INDEX_MASK
}	 // namespace age::graphics::render_pipeline::forward_plus
#else

#endif
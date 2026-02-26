#pragma once
#define UV_COUNT 2

#if !defined(AGE_HLSL)
	#include "age.hpp"

namespace age::graphics::render_pipeline::forward_plus
{
	using t_object_id = uint32;
	using t_mesh_id	  = uint32;
	using t_camera_id = uint32;
}	 // namespace age::graphics::render_pipeline::forward_plus

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
#endif

	// data only used by amplification shader
	struct meshlet_header
	{
		uint16 cone_axis_oct;
		uint16 cone_cull_cutoff_and_offset;

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
		row_major float4x4 view_proj;				  // 64 bytes
		row_major float4x4 view_proj_inv;			  // 64 bytes
		float3			   camera_pos;				  // 12
		float			   time;					  // 4
		float4			   frustum_planes[6];		  // 96
		uint32			   frame_index;				  // 4
		float2			   inv_backbuffer_size;		  // 8
		uint32			   main_buffer_texture_id;	  // 4
													  // total: 256 bytes
	};

	cbuffer root_constants REG(b1)
	{
		uint32 job_count;
	};

#if !defined(AGE_HLSL)
	#undef UV_COUNT

	#undef SYS_VAL
	#undef cbuffer
	#undef REG
	#undef row_major
}	 // namespace age::graphics::render_pipeline::forward_plus
#else

#endif
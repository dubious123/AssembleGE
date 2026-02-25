#pragma once
#include "age.hpp"

// shader shared types
namespace age::graphics::render_pipeline::forward_plus
{
	inline constexpr auto max_mesh_count			= 1024u;
	inline constexpr auto max_job_count_per_thread	= 1024u * 256u;
	inline constexpr auto max_job_data				= max_job_count_per_thread * g::thread_count;
	inline constexpr auto max_object_data_count		= 1024u;
	inline constexpr auto max_mesh_buffer_byte_size = static_cast<uint32>(std::numeric_limits<uint32>::max() * 0.5f);

	using t_object_id = uint32;
	using t_mesh_id	  = uint32;
	using t_camera_id = uint32;

	struct frame_data
	{
		float4x4			  view_proj;				 // 64 bytes
		float4x4			  view_proj_inv;			 // 64 bytes
		float3				  camera_pos;				 // 12
		float				  time;						 // 4
		std::array<float4, 6> frustum_planes;			 // 96
		uint32				  frame_index;				 // 4
		float2				  inv_backbuffer_size;		 // 8
		uint32				  main_buffer_texture_id;	 // 4
														 // total: 256 bytes
	};

	struct job_data
	{
		uint32 object_id;
		uint32 mesh_byte_offset;
		uint32 meshlet_idx;
	};

	struct object_data
	{
		float3 pos;			  // 12
		uint32 quaternion;	  // 4  | 10 10 10 2
		half3  scale;		  // 6

		uint16 extra;		  // 2
	};	  // total: 24 bytes
}	 // namespace age::graphics::render_pipeline::forward_plus

// descriptors
namespace age::graphics::render_pipeline::forward_plus
{
	struct camera_desc
	{
		e::camera_kind kind;
		float3		   pos;
		float4		   quaternion;

		union
		{
			struct
			{
				float near_z;
				float far_z;
				float fov_y;
				float aspect_ratio;
			} perspective;

			struct
			{
				float view_width;
				float view_height;
				float near_z;
				float far_z;
			} orthographic;
		};
	};
}	 // namespace age::graphics::render_pipeline::forward_plus
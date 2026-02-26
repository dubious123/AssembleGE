#pragma once
#include "age.hpp"
#include "age_graphics_backend_dx12_render_pipeline_forward_plus_shared_types.h"

// shader shared types
namespace age::graphics::render_pipeline::forward_plus
{
	inline constexpr auto max_mesh_count			= 1024u;
	inline constexpr auto max_job_count_per_frame	= (1u << 20);	 // 1M
	inline constexpr auto max_job_count_per_thread	= max_job_count_per_frame / g::thread_count;
	inline constexpr auto max_object_data_count		= 1024u;
	inline constexpr auto max_mesh_buffer_byte_size = static_cast<uint32>(std::numeric_limits<uint32>::max() * 0.5f);

																	 // struct frame_data
	//{
	//	float4x4			  view_proj;				 // 64 bytes
	//	float4x4			  view_proj_inv;			 // 64 bytes
	//	float3				  camera_pos;				 // 12
	//	float				  time;						 // 4
	//	std::array<float4, 6> frustum_planes;			 // 96
	//	uint32				  frame_index;				 // 4
	//	float2				  inv_backbuffer_size;		 // 8
	//	uint32				  main_buffer_texture_id;	 // 4
	//													 // total: 256 bytes
	// };

	// struct job_data
	//{
	//	uint32 object_id;
	//	uint32 mesh_byte_offset;
	//	uint32 meshlet_idx;
	// };

	// struct object_data
	//{
	//	float3 pos;			  // 12
	//	uint32 quaternion;	  // 4  | 10 10 10 2
	//	half3  scale;		  // 6

	//	uint16 extra;		  // 2
	//};	  // total: 24 bytes
}	 // namespace age::graphics::render_pipeline::forward_plus

// root signatures
namespace age::graphics::render_pipeline::forward_plus
{
	using binding_config_t = binding_slot_config<
		binding_slot<
			"frame_data_buffer",
			D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC,
			D3D12_SHADER_VISIBILITY_ALL,
			what::constant_buffer_array<shared_type::frame_data>,
			how::root_descriptor,
			where::b<0>>,

		binding_slot<
			"job_data_buffer",
			D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC,
			D3D12_SHADER_VISIBILITY_ALL,
			what::structured_buffer_array<shared_type::job_data>,
			how::root_descriptor,
			where::t<0>>,

		binding_slot<
			"object_data_buffer",
			D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC,
			D3D12_SHADER_VISIBILITY_ALL,
			what::structured_buffer<shared_type::object_data>,
			how::root_descriptor,
			where::t<1>>,

		binding_slot<
			"mesh_data_buffer",
			D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC,
			D3D12_SHADER_VISIBILITY_ALL,
			what::byte_address_buffer,
			how::root_descriptor,
			where::t<2>>,

		binding_slot<
			"linear_clamp_sampler",
			D3D12_SAMPLER_FLAG_NONE,
			D3D12_SHADER_VISIBILITY_PIXEL,
			what::sampler<defaults::static_sampler_desc::linear_clamp>,
			how::static_sampler,
			where::s<0>>>;
}

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
#pragma once
#include "age.hpp"
#include "age_graphics_backend_dx12_render_pipeline_forward_plus_shared_types.h"

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
			where::b<0, 0>>,

		binding_slot<
			"root_constants",
			D3D12_ROOT_DESCRIPTOR_FLAG_NONE,
			D3D12_SHADER_VISIBILITY_ALL,
			what::constant_buffer_array<shared_type::root_constants>,
			how::root_constant,
			where::b<1, 0>>,

		binding_slot<
			"static_buffer",
			D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC,
			D3D12_SHADER_VISIBILITY_ALL,
			what::byte_address_buffer_array<>,
			how::root_descriptor,
			where::t<0, 0>>,

		binding_slot<
			"mesh_data_buffer",
			D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC,
			D3D12_SHADER_VISIBILITY_ALL,
			what::byte_address_buffer,
			how::root_descriptor,
			where::t<1, 0>>,

		binding_slot<
			"scratch_buffer_uav",
			D3D12_ROOT_DESCRIPTOR_FLAG_DATA_VOLATILE,
			D3D12_SHADER_VISIBILITY_ALL,
			what::rw_byte_address_buffer,
			how::root_descriptor,
			where::u<0, 0>>,

		binding_slot<
			"light_cull_stage_buffer_srv",
			D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE,
			D3D12_SHADER_VISIBILITY_ALL,
			what::byte_address_buffer,
			how::root_descriptor,
			where::t<0, 1>>,

		binding_slot<
			"light_cull_stage_buffer_uav",
			D3D12_ROOT_DESCRIPTOR_FLAG_DATA_VOLATILE,
			D3D12_SHADER_VISIBILITY_ALL,
			what::rw_byte_address_buffer,
			how::root_descriptor,
			where::u<0, 1>>,

		binding_slot<
			"sorted_light_buffer_srv",
			D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE,
			D3D12_SHADER_VISIBILITY_ALL,
			what::structured_buffer<shared_type::unified_light>,
			how::root_descriptor,
			where::t<1, 1>>,

		binding_slot<
			"sorted_light_buffer_uav",
			D3D12_ROOT_DESCRIPTOR_FLAG_DATA_VOLATILE,
			D3D12_SHADER_VISIBILITY_ALL,
			what::rw_structured_buffer<shared_type::unified_light>,
			how::root_descriptor,
			where::u<1, 1>>,

		binding_slot<
			"shadow_stage_buffer_srv",
			D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE,
			D3D12_SHADER_VISIBILITY_ALL,
			what::byte_address_buffer,
			how::root_descriptor,
			where::t<0, 2>>,

		binding_slot<
			"shadow_stage_buffer_uav",
			D3D12_ROOT_DESCRIPTOR_FLAG_DATA_VOLATILE,
			D3D12_SHADER_VISIBILITY_ALL,
			what::rw_byte_address_buffer,
			how::root_descriptor,
			where::u<0, 2>>,

		binding_slot<
			"shadow_light_buffer_srv",
			D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE,
			D3D12_SHADER_VISIBILITY_ALL,
			what::structured_buffer<shared_type::shadow_light>,
			how::root_descriptor,
			where::t<1, 2>>,

		binding_slot<
			"shadow_light_buffer_uav",
			D3D12_ROOT_DESCRIPTOR_FLAG_DATA_VOLATILE,
			D3D12_SHADER_VISIBILITY_ALL,
			what::rw_structured_buffer<shared_type::shadow_light>,
			how::root_descriptor,
			where::u<1, 2>>,

		binding_slot<
			"rt_instance_render_data_buffer_srv",
			D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC,
			D3D12_SHADER_VISIBILITY_ALL,
			what::structured_buffer_array<shared_type::rt_instance_render_data>,
			how::root_descriptor,
			where::t<0, 3>>,

		binding_slot<
			"rt_index_buffer",
			D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC,
			D3D12_SHADER_VISIBILITY_ALL,
			what::structured_buffer<uint32>,
			how::root_descriptor,
			where::t<1, 3>>,

		binding_slot<
			"ui_data_buffer",
			D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC,
			D3D12_SHADER_VISIBILITY_ALL,
			what::structured_buffer_array<shared_type::ui_data>,
			how::root_descriptor,
			where::t<0, 4>>,


		binding_slot<
			"linear_clamp_sampler",
			D3D12_SAMPLER_FLAG_NONE,
			D3D12_SHADER_VISIBILITY_ALL,
			what::sampler<defaults::static_sampler_desc::linear_clamp>,
			how::static_sampler,
			where::s<0>>,

		binding_slot<
			"shadow_sampler",
			D3D12_SAMPLER_FLAG_NONE,
			D3D12_SHADER_VISIBILITY_ALL,
			what::sampler<defaults::static_sampler_desc::shadow_cmp>,
			how::static_sampler,
			where::s<1>>>;
}	 // namespace age::graphics::render_pipeline::forward_plus

// descriptors
namespace age::graphics::render_pipeline::forward_plus
{
	struct camera_desc
	{
		graphics::e::camera_kind kind;
		float3					 pos;
		float4					 quaternion;

		float near_z;
		float far_z;

		union
		{
			struct
			{
				float fov_y;
				float aspect_ratio;
			} perspective;

			struct
			{
				float view_width;
				float view_height;
			} orthographic;
		};
	};

	struct camera_data
	{
		float3				  pos;
		float3				  forward;
		float3				  right;
		float4x4			  view_proj;
		float4x4			  view_proj_inv;
		std::array<float4, 6> frustum_plane_arr;
	};

	struct mesh_data
	{
		t_mesh_id				  id;
		uint32					  offset;
		uint32					  byte_size;
		uint32					  meshlet_count;
		uint32					  rt_index_buffer_elem_offset;
		graphics::rt::blas_handle h_blas;
	};

	struct point_light_desc
	{
		float3 position;	 // 12
		float  range;		 // 4
		float3 color;		 // 12
		float  intensity;	 // 4
	};	  // 32 bytes

	struct spot_light_desc
	{
		float3 position;	 // 12
		float  range;		 // 4
		float3 direction;	 // 12
		float  intensity;	 // 4
		float3 color;		 // 12
		float  cos_inner;	 // 4  (falloff begin, cosine)
		float  cos_outer;	 // 4  (cosine)
	};

	struct directional_light_desc
	{
		float3 direction;	 // 12
		float  intensity;	 // 4
		float3 color;		 // 12
	};	  // 28 bytes

	struct shadow_light_header
	{
		uint32			  light_id;
		e::light_kind	  light_kind;
		t_shadow_light_id shadow_id;
	};

	struct ui_desc_brush_data
	{
		union
		{
			struct
			{
				float3 value;
			} color;
		};
	};

	struct ui_desc
	{
		float2 pivot_pos;	 // screen pos of pivot
		float2 pivot_uv;
		float2 size;		 // pixel size
		float  rotation;	 // z rotation, radian
		float  border_thickness;

		uint8 z_order;

		age::ui::e::shape_kind shape_kind;

		union
		{
		};

		age::ui::e::brush_kind body_brush_kind;

		ui_desc_brush_data body_brush_data;

		age::ui::e::brush_kind border_brush_kind;

		ui_desc_brush_data border_brush_data;
	};

	struct ui_header
	{
		uint32 idx;
		uint8  z_order;
	};
}	 // namespace age::graphics::render_pipeline::forward_plus
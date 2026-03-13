#pragma once
#include "age.hpp"
#include "age_graphics_backend_dx12_render_pipeline_forward_plus_shared_types.h"

// shader shared types
namespace age::graphics::render_pipeline::forward_plus::g
{
	inline constexpr auto max_mesh_count					   = 1024u;
	inline constexpr auto max_opaque_meshlet_render_data_count = (1u << 20);
	inline constexpr auto max_opaque_meshlet_per_thread		   = max_opaque_meshlet_render_data_count / age::graphics::g::thread_count;
	inline constexpr auto max_object_data_count				   = 1024u;
	inline constexpr auto max_mesh_buffer_byte_size			   = static_cast<uint32>(std::numeric_limits<uint32>::max() * 0.5f);

	inline constexpr auto max_directional_light_count = 2;
}	 // namespace age::graphics::render_pipeline::forward_plus::g

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
			"opaque_meshlet_render_data_buffer",
			D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC,
			D3D12_SHADER_VISIBILITY_ALL,
			what::structured_buffer_array<shared_type::opaque_meshlet_render_data>,
			how::root_descriptor,
			where::t<0, 0>>,

		binding_slot<
			"object_data_buffer",
			D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC,
			D3D12_SHADER_VISIBILITY_ALL,
			what::structured_buffer_array<shared_type::object_data>,
			how::root_descriptor,
			where::t<1, 0>>,

		binding_slot<
			"mesh_data_buffer",
			D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC,
			D3D12_SHADER_VISIBILITY_ALL,
			what::byte_address_buffer,
			how::root_descriptor,
			where::t<2, 0>>,

		binding_slot<
			"directional_light_buffer",
			D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC,
			D3D12_SHADER_VISIBILITY_ALL,
			what::structured_buffer_array<shared_type::directional_light>,
			how::root_descriptor,
			where::t<3, 0>>,

		binding_slot<
			"unified_light_buffer",
			D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC,
			D3D12_SHADER_VISIBILITY_ALL,
			what::structured_buffer_array<shared_type::unified_light>,
			how::root_descriptor,
			where::t<4, 0>>,

		binding_slot<
			"frame_data_rw_buffer_srv",
			D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE,
			D3D12_SHADER_VISIBILITY_ALL,
			what::structured_buffer_array<shared_type::frame_data_rw>,
			how::root_descriptor,
			where::t<5, 0>>,

		binding_slot<
			"frame_data_rw_buffer_uav",
			D3D12_ROOT_DESCRIPTOR_FLAG_DATA_VOLATILE,
			D3D12_SHADER_VISIBILITY_ALL,
			what::rw_structured_buffer_array<shared_type::frame_data_rw>,
			how::root_descriptor,
			where::u<5, 0>>,

		binding_slot<
			"shadow_light_header_buffer",
			D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC,
			D3D12_SHADER_VISIBILITY_ALL,
			what::structured_buffer_array<shared_type::shadow_light_header>,
			how::root_descriptor,
			where::t<0, 1>>,

		binding_slot<
			"shadow_light_buffer_srv",
			D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE,
			D3D12_SHADER_VISIBILITY_ALL,
			what::structured_buffer<shared_type::shadow_light>,
			how::root_descriptor,
			where::t<1, 1>>,


		binding_slot<
			"shadow_light_buffer_uav",
			D3D12_ROOT_DESCRIPTOR_FLAG_DATA_VOLATILE,
			D3D12_SHADER_VISIBILITY_ALL,
			what::rw_structured_buffer<shared_type::shadow_light>,
			how::root_descriptor,
			where::u<1, 1>>,

		binding_slot<
			"light_sort_buffer_srv",
			D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE,
			D3D12_SHADER_VISIBILITY_ALL,
			what::structured_buffer<uint32>,
			how::root_descriptor,
			where::t<0, 2>>,


		binding_slot<
			"light_sort_buffer_uav",
			D3D12_ROOT_DESCRIPTOR_FLAG_DATA_VOLATILE,
			D3D12_SHADER_VISIBILITY_ALL,
			what::rw_structured_buffer<uint32>,
			how::root_descriptor,
			where::u<0, 2>>,

		binding_slot<
			"zbin_buffer_srv",
			D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE,
			D3D12_SHADER_VISIBILITY_ALL,
			what::structured_buffer<shared_type::zbin_entry>,
			how::root_descriptor,
			where::t<1, 2>>,


		binding_slot<
			"zbin_buffer_uav",
			D3D12_ROOT_DESCRIPTOR_FLAG_DATA_VOLATILE,
			D3D12_SHADER_VISIBILITY_ALL,
			what::rw_structured_buffer<shared_type::zbin_entry>,
			how::root_descriptor,
			where::u<1, 2>>,

		binding_slot<
			"tile_mask_buffer_srv",
			D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE,
			D3D12_SHADER_VISIBILITY_PIXEL,
			what::structured_buffer<uint32>,
			how::root_descriptor,
			where::t<2, 2>>,

		binding_slot<
			"tile_mask_buffer_uav",
			D3D12_ROOT_DESCRIPTOR_FLAG_DATA_VOLATILE,
			D3D12_SHADER_VISIBILITY_ALL,
			what::rw_structured_buffer<uint32>,
			how::root_descriptor,
			where::u<2, 2>>,

		binding_slot<
			"unified_sorted_light_buffer_srv",
			D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE,
			D3D12_SHADER_VISIBILITY_PIXEL,
			what::structured_buffer<shared_type::unified_light>,
			how::root_descriptor,
			where::t<3, 2>>,

		binding_slot<
			"unified_sorted_light_buffer_uav",
			D3D12_ROOT_DESCRIPTOR_FLAG_DATA_VOLATILE,
			D3D12_SHADER_VISIBILITY_ALL,
			what::rw_structured_buffer<shared_type::unified_light>,
			how::root_descriptor,
			where::u<3, 2>>,

		binding_slot<
			"debug_uav",
			D3D12_ROOT_DESCRIPTOR_FLAG_DATA_VOLATILE,
			D3D12_SHADER_VISIBILITY_ALL,
			what::rw_structured_buffer_array<shared_type::debug_77>,
			how::root_descriptor,
			where::u<7, 7>>,

		binding_slot<
			"linear_clamp_sampler",
			D3D12_SAMPLER_FLAG_NONE,
			D3D12_SHADER_VISIBILITY_PIXEL,
			what::sampler<defaults::static_sampler_desc::linear_clamp>,
			how::static_sampler,
			where::s<0>>,

		binding_slot<
			"shadow_sampler",
			D3D12_SAMPLER_FLAG_NONE,
			D3D12_SHADER_VISIBILITY_PIXEL,
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
		t_mesh_id id;
		uint32	  offset;
		uint32	  byte_size;
		uint32	  meshlet_count;
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
}	 // namespace age::graphics::render_pipeline::forward_plus
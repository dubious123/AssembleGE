#pragma once
#include "age.hpp"

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
			what::constant_buffer<shared_type::root_constants>,
			how::root_constant,
			where::b<1, 0>>,

		binding_slot<
			"indirect_arg_buffer",
			D3D12_ROOT_DESCRIPTOR_FLAG_DATA_VOLATILE,
			D3D12_SHADER_VISIBILITY_ALL,
			what::rw_byte_address_buffer,
			how::root_descriptor,
			where::u<2, 0>>,

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
			"light_bin_stage_buffer_srv",
			D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE,
			D3D12_SHADER_VISIBILITY_ALL,
			what::byte_address_buffer,
			how::root_descriptor,
			where::t<0, 1>>,

		binding_slot<
			"light_bin_stage_buffer_uav",
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
			"material_buffer",
			D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC,
			D3D12_SHADER_VISIBILITY_ALL,
			what::structured_buffer<shared_type::material>,
			how::root_descriptor,
			where::t<0, 2>>,

		binding_slot<
			"env_light_buffer",
			D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC,
			D3D12_SHADER_VISIBILITY_ALL,
			what::structured_buffer_array<shared_type::env_light>,
			how::root_descriptor,
			where::t<1, 2>>,

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
			"rt_raycast_request_buffer_srv",
			D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC,
			D3D12_SHADER_VISIBILITY_ALL,
			what::structured_buffer_array<shared_type::raycast_request>,
			how::root_descriptor,
			where::t<2, 3>>,

		binding_slot<
			"rt_raycast_result_buffer_uav",
			D3D12_ROOT_DESCRIPTOR_FLAG_DATA_VOLATILE,
			D3D12_SHADER_VISIBILITY_ALL,
			what::rw_structured_buffer<shared_type::raycast_result>,
			how::root_descriptor,
			where::u<3, 3>>,

		binding_slot<
			"selection_outline_meshlet_render_data_buffer",
			D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC,
			D3D12_SHADER_VISIBILITY_ALL,
			what::structured_buffer_array<shared_type::selection_outline_meshlet_render_data>,
			how::root_descriptor,
			where::t<0, 4>>,

		binding_slot<
			"selection_outline_data_buffer",
			D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC,
			D3D12_SHADER_VISIBILITY_PIXEL,
			what::structured_buffer_array<shared_type::selection_outline_data>,
			how::root_descriptor,
			where::t<1, 4>>,

		binding_slot<
			"ui_root_data_buffer",
			D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC,
			D3D12_SHADER_VISIBILITY_ALL,
			what::structured_buffer_array<shared_type::ui_root_data>,
			how::root_descriptor,
			where::t<0, 5>>,

		binding_slot<
			"ui_data_buffer",
			D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC,
			D3D12_SHADER_VISIBILITY_ALL,
			what::structured_buffer_array<shared_type::ui_data>,
			how::root_descriptor,
			where::t<1, 5>>,

		binding_slot<
			"ddgi_probe_buffer_srv",
			D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE,
			D3D12_SHADER_VISIBILITY_ALL,
			what::structured_buffer<shared_type::ddgi_probe>,
			how::root_descriptor,
			where::t<1, 7>>,

		binding_slot<
			"ddgi_probe_buffer_uav",
			D3D12_ROOT_DESCRIPTOR_FLAG_DATA_VOLATILE,
			D3D12_SHADER_VISIBILITY_ALL,
			what::rw_structured_buffer<shared_type::ddgi_probe>,
			how::root_descriptor,
			where::u<1, 7>>,

		binding_slot<
			"ddgi_scratch_buffer",
			D3D12_ROOT_DESCRIPTOR_FLAG_DATA_VOLATILE,
			D3D12_SHADER_VISIBILITY_ALL,
			what::rw_byte_address_buffer,
			how::root_descriptor,
			where::u<2, 7>>,

		binding_slot<
			"debug_meshlet_render_data_buffer",
			D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC,
			D3D12_SHADER_VISIBILITY_ALL,
			what::structured_buffer_array<shared_type::debug_meshlet_render_data>,
			how::root_descriptor,
			where::t<0, 77>>,

		binding_slot<
			"debug_meshlet_aot_render_data_buffer",
			D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC,
			D3D12_SHADER_VISIBILITY_ALL,
			what::structured_buffer_array<shared_type::debug_meshlet_render_data>,
			how::root_descriptor,
			where::t<1, 77>>,

		binding_slot<
			"crash_assert_buffer",
			D3D12_ROOT_DESCRIPTOR_FLAG_DATA_VOLATILE,
			D3D12_SHADER_VISIBILITY_ALL,
			what::rw_byte_address_buffer,
			how::root_descriptor,
			where::u<666, 666>>,

		binding_slot<
			"linear_clamp_sampler",
			D3D12_SAMPLER_FLAG_NONE,
			D3D12_SHADER_VISIBILITY_ALL,
			what::sampler<defaults::static_sampler_desc::linear_clamp>,
			how::static_sampler,
			where::s<0>>,

		binding_slot<
			"linear_wrap_sampler",
			D3D12_SAMPLER_FLAG_NONE,
			D3D12_SHADER_VISIBILITY_ALL,
			what::sampler<defaults::static_sampler_desc::linear_wrap>,
			how::static_sampler,
			where::s<1>>,

		binding_slot<
			"point_clamp_sampler",
			D3D12_SAMPLER_FLAG_NONE,
			D3D12_SHADER_VISIBILITY_ALL,
			what::sampler<defaults::static_sampler_desc::point_clamp>,
			how::static_sampler,
			where::s<2>>>;
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
		float3				  up;
		float4x4			  view;
		float4x4			  proj;
		float4x4			  view_proj;
		float4x4			  view_proj_inv;
		std::array<float4, 6> frustum_plane_arr;
	};

	struct mesh_data
	{
		t_mesh_id id;
		uint32	  offset;
		uint32	  chunk_srv_id;
		uint32	  byte_size;
		uint32	  meshlet_count;
		// uint32			rt_index_buffer_elem_offset;
		uint32			rt_idx_chunk_srv_id;
		uint32			rt_idx_offset;
		uint32			rt_idx_size;
		resource_handle h_blas;
	};

	struct point_light_desc
	{
		float3	position;	  // 12
		float	range;		  // 4
		float3	color;		  // 12
		float	intensity;	  // 4
		bool	cast_shadow;
		uint8_3 _;
	};

	struct spot_light_desc
	{
		float3	position;	  // 12
		float	range;		  // 4
		float3	direction;	  // 12
		float	intensity;	  // 4
		float3	color;		  // 12
		float	cos_inner;	  // 4  (falloff begin, cosine)
		float	cos_outer;	  // 4  (cosine)
		bool	cast_shadow;
		uint8_3 _;
	};

	struct directional_light_desc
	{
		float3	direction;	  // 12
		float	intensity;	  // 4
		float3	color;		  // 12
		bool	cast_shadow;
		uint8_3 _;
	};

	struct texture_data
	{
		srv_desc_handle h_srv_desc;
		resource_handle h_resource;
	};

	struct material_data
	{
		asset::handle h_mat;
	};

	struct env_light_data
	{
		uint32			gpu_id;
		resource_handle h_radiance;
		resource_handle h_prefilter;
		resource_handle h_irradiance;
		srv_desc_handle h_radiance_srv_desc;
		srv_desc_handle h_prefilter_srv_desc;
		srv_desc_handle h_irradiance_srv_desc;
	};

	struct bloom_desc
	{
		float  threshold = 1.0f;
		float  knee		 = 0.5f;
		float  intensity = 0.05f;
		float  radius	 = 1.0f;
		float3 tint		 = float3::one();
	};

	struct ddgi_desc
	{
		uint32_3					  probe_per_level_axis;
		float3						  base_probe_spacing;
		uint32						  level_count;
		graphics::e::ddgi_debug_flags debug_flags;
		bool						  lock_origin;
		uint8_3						  _;
	};

	struct ddgi_data
	{
		shared_type::ddgi_data ddgi_data_gpu;
		bool				   enabled = false;
		bool				   need_cleanup;
		bool				   render_probes = true;
		bool				   lock_origin;

		extent_2d<uint32> irradiance_atlas_extent;
		extent_2d<uint32> visibility_atlas_extent;

		resource_handle h_irradiance_atlas;
		resource_handle h_visibility_atlas;
		resource_handle h_probe_buffer;
		resource_handle h_ddgi_scratch_buffer;
		srv_desc_handle h_irradiance_srv_desc;
		uav_desc_handle h_irradiance_uav_desc;
		srv_desc_handle h_visibility_srv_desc;
		uav_desc_handle h_visibility_uav_desc;

		clear_uav_desc_handle h_irradiance_clear_uav_desc;
		clear_uav_desc_handle h_visibility_clear_uav_desc;
		clear_uav_desc_handle h_probe_buffer_clear_uav_desc;

		float3 origin;
	};

	struct gibs_desc
	{
		uint32						  max_surfel_count;
		graphics::e::gibs_debug_flags debug_flags;
		bool						  lock_origin;
		uint8						  cell_count;	 // base cell count per axis, pow of 2
		uint8						  outer_layer_count;
		uint8						  _;
		float						  cell_size;	 // meter
		float						  outer_cell_size_factor;


													 // total cell count == cell_count ^ 3 + 6 *(cell_count ^ 2) * outer_layer_count
		// inner extent == cell_count * cell_size
		// outer cell size == cell_size * (outer_cell_size_factor ^ k)
		// outer extent = inner extent + sum(outer cell size per k)
	};

	struct gibs_data
	{
		shared_type::gibs_data	   gibs_data_gpu;
		shared_type::gibs_lut_data gibs_lut_data_gpu;

		bool enabled;
		bool need_cleanup;
		bool render_surfels;
		bool lock_origin;

		float3 origin;

		resource_handle		  h_surfel_buffer;		 // surfel, geometry, msme, visibility, luminance
		srv_desc_handle		  h_surfel_buffer_srv_desc;
		uav_desc_handle		  h_surfel_buffer_uav_desc;
		clear_uav_desc_handle h_surfel_buffer_clear_uav_desc;

		resource_handle h_surfel_id_stack_buffer;	 // dead, alive prev, alive curr
		srv_desc_handle h_surfel_id_stack_buffer_srv_desc;
		uav_desc_handle h_surfel_id_stack_buffer_uav_desc;

		resource_handle		  h_scratch_buffer;		 // prefix, sum, ...
		uav_desc_handle		  h_scratch_buffer_uav_desc;
		clear_uav_desc_handle h_scratch_buffer_clear_uav_desc;

		resource_handle h_ray_buffer;				 // ray count, ray entry, ray result
		srv_desc_handle h_ray_buffer_srv_desc;
		uav_desc_handle h_ray_buffer_uav_desc;

		resource_handle		  h_tile_buffer;		 // tile -> surfel, surfel_gt_id, cell -> surfel, surfel_gt_id
		srv_desc_handle		  h_tile_buffer_srv_desc;
		uav_desc_handle		  h_tile_buffer_uav_desc;
		clear_uav_desc_handle h_tile_buffer_clear_uav_desc;

		resource_handle		  h_tile_spawn_kill_buffer;
		srv_desc_handle		  h_tile_spawn_kill_buffer_srv_desc;
		uav_desc_handle		  h_tile_spawn_kill_buffer_uav_desc;
		clear_uav_desc_handle h_tile_spawn_kill_buffer_clear_uav_desc;

		resource_handle		  h_cell_buffer;	// cell -> surfel, surfel_gt_id,
		srv_desc_handle		  h_cell_buffer_srv_desc;
		uav_desc_handle		  h_cell_buffer_uav_desc;
		clear_uav_desc_handle h_cell_buffer_clear_uav_desc;

		resource_handle		  h_cell_spawn_kill_buffer;
		srv_desc_handle		  h_cell_spawn_kill_buffer_srv_desc;
		uav_desc_handle		  h_cell_spawn_kill_buffer_uav_desc;
		clear_uav_desc_handle h_cell_spawn_kill_buffer_clear_uav_desc;

		resource_handle h_gbuffer;
		rtv_desc_handle h_gbuffer_rtv_desc;
		srv_desc_handle h_gbuffer_srv_desc;

		resource_handle		  h_gi_resolve_buffer;
		srv_desc_handle		  h_gi_resolve_buffer_srv_desc;
		uav_desc_handle		  h_gi_resolve_buffer_uav_desc;
		clear_uav_desc_handle h_gi_resolve_buffer_clear_uav_desc;


		resource_handle h_indirect_arg_buffer;
		uav_desc_handle h_indirect_arg_buffer_uav_desc;
	};
}	 // namespace age::graphics::render_pipeline::forward_plus
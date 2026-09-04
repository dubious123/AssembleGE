#pragma once
#include "age.hpp"

// root signatures
namespace age::graphics::render_pipeline
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
			"meshlet_render_data_buffer",
			D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC,
			D3D12_SHADER_VISIBILITY_ALL,
			what::structured_buffer_array<shared_type::meshlet_render_data>,
			how::root_descriptor,
			where::t<0, 6>>,

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
			"debug_assert_buffer",
			D3D12_ROOT_DESCRIPTOR_FLAG_DATA_VOLATILE,
			D3D12_SHADER_VISIBILITY_ALL,
			what::rw_byte_address_buffer,
			how::root_descriptor,
			where::u<666, 666>>,

		binding_slot<
			"linear_wrap_sampler",
			D3D12_SAMPLER_FLAG_NONE,
			D3D12_SHADER_VISIBILITY_ALL,
			what::sampler<defaults::static_sampler_desc::linear_wrap>,
			how::static_sampler,
			where::s<0>>,

		binding_slot<
			"linear_clamp_sampler",
			D3D12_SAMPLER_FLAG_NONE,
			D3D12_SHADER_VISIBILITY_ALL,
			what::sampler<defaults::static_sampler_desc::linear_clamp>,
			how::static_sampler,
			where::s<1>>,

		binding_slot<
			"linear_mirror_sampler",
			D3D12_SAMPLER_FLAG_NONE,
			D3D12_SHADER_VISIBILITY_ALL,
			what::sampler<defaults::static_sampler_desc::linear_mirror>,
			how::static_sampler,
			where::s<2>>,

		binding_slot<
			"point_wrap_sampler",
			D3D12_SAMPLER_FLAG_NONE,
			D3D12_SHADER_VISIBILITY_ALL,
			what::sampler<defaults::static_sampler_desc::point_wrap>,
			how::static_sampler,
			where::s<3>>,

		binding_slot<
			"point_clamp_sampler",
			D3D12_SAMPLER_FLAG_NONE,
			D3D12_SHADER_VISIBILITY_ALL,
			what::sampler<defaults::static_sampler_desc::point_clamp>,
			how::static_sampler,
			where::s<4>>,

		binding_slot<
			"point_mirror_sampler",
			D3D12_SAMPLER_FLAG_NONE,
			D3D12_SHADER_VISIBILITY_ALL,
			what::sampler<defaults::static_sampler_desc::point_mirror>,
			how::static_sampler,
			where::s<5>>

		>;
}	 // namespace age::graphics::render_pipeline

// descriptors
namespace age::graphics::render_pipeline
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
		age::array<float4, 6> frustum_plane_arr;
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

	struct model_render_option
	{
		graphics::e::mesh_raster_override_kind		  raster_override_kind		  = graphics::e::mesh_raster_override_kind::none;
		graphics::e::mesh_rt_alpha_test_override_kind rt_alpha_test_override_kind = graphics::e::mesh_rt_alpha_test_override_kind::none;
		graphics::e::model_render_option_flags		  option_flags				  = graphics::e::model_render_option_flags::none;
		uint8										  fade_unorm8				  = 255u;
	};
}	 // namespace age::graphics::render_pipeline

namespace age::graphics::render_pipeline
{
	struct raycast_result
	{
		uint32 object_id;	 // invalid_id == no hit
		float  t_hit;
		float3 world_pos;
		// object_deleted == true means the ray did hit <frame_buffer_count> frames ago,
		// but the object is gone now. whether to still use the result is up to the caller.
		// one possible use case for using the result even when object_deleted is true:
		// debug_mesh hit tests, where debug objects are cleared every frame
		bool	object_deleted;
		uint8_3 _;
	};
}	 // namespace age::graphics::render_pipeline

// system desc, data
namespace age::graphics::render_pipeline
{
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
		float  outer_cell_size_factor;

		resource_handle h_tile_surfel_buffer;
		srv_desc_handle h_tile_surfel_buffer_srv_desc;
		uav_desc_handle h_tile_surfel_buffer_uav_desc;

		resource_handle h_tile_surfel_geo_buffer;
		srv_desc_handle h_tile_surfel_geo_buffer_srv_desc;
		uav_desc_handle h_tile_surfel_geo_buffer_uav_desc;

		resource_handle h_tile_surfel_msme_buffer;
		srv_desc_handle h_tile_surfel_msme_buffer_srv_desc;
		uav_desc_handle h_tile_surfel_msme_buffer_uav_desc;

		resource_handle h_tile_surfel_visibility_buffer;
		srv_desc_handle h_tile_surfel_visibility_buffer_srv_desc;
		uav_desc_handle h_tile_surfel_visibility_buffer_uav_desc;

		resource_handle h_tile_surfel_luminance_buffer;
		srv_desc_handle h_tile_surfel_luminance_buffer_srv_desc;
		uav_desc_handle h_tile_surfel_luminance_buffer_uav_desc;

		resource_handle h_cell_surfel_buffer;
		srv_desc_handle h_cell_surfel_buffer_srv_desc;
		uav_desc_handle h_cell_surfel_buffer_uav_desc;

		resource_handle h_cell_surfel_geo_buffer;
		srv_desc_handle h_cell_surfel_geo_buffer_srv_desc;
		uav_desc_handle h_cell_surfel_geo_buffer_uav_desc;

		resource_handle h_cell_surfel_msme_buffer;
		srv_desc_handle h_cell_surfel_msme_buffer_srv_desc;
		uav_desc_handle h_cell_surfel_msme_buffer_uav_desc;

		resource_handle h_cell_surfel_visibility_buffer;
		srv_desc_handle h_cell_surfel_visibility_buffer_srv_desc;
		uav_desc_handle h_cell_surfel_visibility_buffer_uav_desc;

		resource_handle h_cell_surfel_luminance_buffer;
		srv_desc_handle h_cell_surfel_luminance_buffer_srv_desc;
		uav_desc_handle h_cell_surfel_luminance_buffer_uav_desc;

		resource_handle h_tile_surfel_id_stack_buffer;	  // dead, alive prev, alive curr
		srv_desc_handle h_tile_surfel_id_stack_buffer_srv_desc;
		uav_desc_handle h_tile_surfel_id_stack_buffer_uav_desc;

		resource_handle h_cell_surfel_id_stack_buffer;	  // dead, alive prev, alive curr
		srv_desc_handle h_cell_surfel_id_stack_buffer_srv_desc;
		uav_desc_handle h_cell_surfel_id_stack_buffer_uav_desc;

		resource_handle		  h_scratch_buffer;			  // prefix, sum, ...
		uav_desc_handle		  h_scratch_buffer_uav_desc;
		clear_uav_desc_handle h_scratch_buffer_clear_uav_desc;

		resource_handle h_ray_entry_buffer;				  // ray count, ray entry
		srv_desc_handle h_ray_entry_buffer_srv_desc;
		uav_desc_handle h_ray_entry_buffer_uav_desc;

		resource_handle h_ray_hit_buffer;
		srv_desc_handle h_ray_hit_buffer_srv_desc;
		uav_desc_handle h_ray_hit_buffer_uav_desc;

		resource_handle h_ray_lighting_buffer;
		srv_desc_handle h_ray_lighting_buffer_srv_desc;
		uav_desc_handle h_ray_lighting_buffer_uav_desc;

		resource_handle		  h_tile_buffer;	// tile -> surfel, surfel_gt_id, cell -> surfel, surfel_gt_id
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

		AGE_DECL_PING_PONG_BUFFER(gi_resolve_age, gibs_data_gpu.is_alt(), srv, uav, clear_uav)

		resource_handle		  h_gi_resolve_prev_buffer;
		srv_desc_handle		  h_gi_resolve_prev_buffer_srv_desc;
		uav_desc_handle		  h_gi_resolve_prev_buffer_uav_desc;
		clear_uav_desc_handle h_gi_resolve_prev_buffer_clear_uav_desc;

		resource_handle		  h_gi_resolve_curr_buffer;
		srv_desc_handle		  h_gi_resolve_curr_buffer_srv_desc;
		uav_desc_handle		  h_gi_resolve_curr_buffer_uav_desc;
		clear_uav_desc_handle h_gi_resolve_curr_buffer_clear_uav_desc;

		resource_handle h_gi_resolve_scratch_buffer;
		srv_desc_handle h_gi_resolve_scratch_buffer_srv_desc;
		uav_desc_handle h_gi_resolve_scratch_buffer_uav_desc;

		resource_handle		  h_gi_resolve_weight_buffer;
		srv_desc_handle		  h_gi_resolve_weight_buffer_srv_desc;
		uav_desc_handle		  h_gi_resolve_weight_buffer_uav_desc;
		clear_uav_desc_handle h_gi_resolve_weight_buffer_clear_uav_desc;

		resource_handle		  h_gi_resolve_sample_pos_buffer;
		srv_desc_handle		  h_gi_resolve_sample_pos_buffer_srv_desc;
		uav_desc_handle		  h_gi_resolve_sample_pos_buffer_uav_desc;
		clear_uav_desc_handle h_gi_resolve_sample_pos_buffer_clear_uav_desc;

		resource_handle h_gi_resolve_sample_res_buffer;
		srv_desc_handle h_gi_resolve_sample_res_buffer_srv_desc;
		uav_desc_handle h_gi_resolve_sample_res_buffer_uav_desc;

		resource_handle h_indirect_arg_buffer;
		uav_desc_handle h_indirect_arg_buffer_uav_desc;
	};

	struct gist_desc
	{
		uint8						  diffuse_ray_period;				// pow_of_2
		uint8						  specular_ray_period;				// pow_of_2
		uint8						  cell_surfel_ray_count_min;
		uint8						  cell_surfel_ray_count_max;
		uint32						  max_cell_surfel_count;
		float						  cell_surfel_ray_budget_factor;	// cell_surfel_ray_count : surfel_count * min_ray * factor
		graphics::e::gist_debug_flags debug_flags;
		bool						  lock_origin;
		uint8						  cell_count_per_axis;				// base cell count per axis, pow of 2
		uint8						  outer_layer_count;
		uint8						  _;
		float						  cell_size;
		float						  outer_cell_size_factor;


		// total cell count == cell_count ^ 3 + 6 *(cell_count ^ 2) * outer_layer_count
		// inner extent == cell_count * cell_size
		// outer cell size == cell_size * (outer_cell_size_factor ^ k)
		// outer extent = inner extent + sum(outer cell size per k)
	};

	struct gist_data
	{
		shared_type::gist_data	   gpu_data;
		shared_type::gist_lut_data gpu_lut_data;

		bool enabled		   = false;
		bool need_cleanup	   = false;
		bool render_debug_view = false;
		bool lock_origin	   = false;

		bool  is_alt;
		uint8 diffuse_ray_period;	  // pow_of_2
		uint8 specular_ray_period;	  // pow_of_2
		uint8 _;

		float3 origin;
		float  cell_surfel_ray_budget_factor;
		float  outer_cell_size_factor;

		resource_handle h_cell_surfel_buffer;
		srv_desc_handle h_cell_surfel_buffer_srv_desc;
		uav_desc_handle h_cell_surfel_buffer_uav_desc;

		resource_handle h_cell_surfel_geo_buffer;
		srv_desc_handle h_cell_surfel_geo_buffer_srv_desc;
		uav_desc_handle h_cell_surfel_geo_buffer_uav_desc;

		resource_handle h_cell_surfel_msme_buffer;
		srv_desc_handle h_cell_surfel_msme_buffer_srv_desc;
		uav_desc_handle h_cell_surfel_msme_buffer_uav_desc;

		resource_handle h_cell_surfel_visibility_buffer;
		srv_desc_handle h_cell_surfel_visibility_buffer_srv_desc;
		uav_desc_handle h_cell_surfel_visibility_buffer_uav_desc;

		resource_handle h_cell_surfel_luminance_buffer;
		srv_desc_handle h_cell_surfel_luminance_buffer_srv_desc;
		uav_desc_handle h_cell_surfel_luminance_buffer_uav_desc;

		resource_handle h_px_luminance_buffer;
		srv_desc_handle h_px_luminance_buffer_srv_desc;
		uav_desc_handle h_px_luminance_buffer_uav_desc;

		resource_handle h_cell_surfel_dead_id_stack_buffer;
		srv_desc_handle h_cell_surfel_dead_id_stack_buffer_srv_desc;
		uav_desc_handle h_cell_surfel_dead_id_stack_buffer_uav_desc;

		AGE_DECL_PING_PONG_BUFFER(cell_surfel_alive_id_stack, is_alt, srv, uav)

		resource_handle		  h_scratch_buffer;	   // prefix, sum, ...
		uav_desc_handle		  h_scratch_buffer_uav_desc;
		clear_uav_desc_handle h_scratch_buffer_clear_uav_desc;

		resource_handle h_ray_entry_buffer;		   // ray count, ray entry
		srv_desc_handle h_ray_entry_buffer_srv_desc;
		uav_desc_handle h_ray_entry_buffer_uav_desc;

		resource_handle h_ray_hit_buffer;
		srv_desc_handle h_ray_hit_buffer_srv_desc;
		uav_desc_handle h_ray_hit_buffer_uav_desc;

		resource_handle h_ray_lighting_buffer;
		srv_desc_handle h_ray_lighting_buffer_srv_desc;
		uav_desc_handle h_ray_lighting_buffer_uav_desc;

		resource_handle		  h_cell_buffer;	// cell -> surfel, surfel_gt_id,
		srv_desc_handle		  h_cell_buffer_srv_desc;
		uav_desc_handle		  h_cell_buffer_uav_desc;
		clear_uav_desc_handle h_cell_buffer_clear_uav_desc;

		resource_handle		  h_cell_spawn_kill_buffer;
		srv_desc_handle		  h_cell_spawn_kill_buffer_srv_desc;
		uav_desc_handle		  h_cell_spawn_kill_buffer_uav_desc;
		clear_uav_desc_handle h_cell_spawn_kill_buffer_clear_uav_desc;

		AGE_DECL_PING_PONG_BUFFER(gi_resolve_age, is_alt, srv, uav, clear_uav)
		AGE_DECL_PING_PONG_BUFFER(gi_resolve_moments, is_alt, srv, uav, clear_uav)

		resource_handle		  h_gi_resolve_prev_buffer;
		srv_desc_handle		  h_gi_resolve_prev_buffer_srv_desc;
		uav_desc_handle		  h_gi_resolve_prev_buffer_uav_desc;
		clear_uav_desc_handle h_gi_resolve_prev_buffer_clear_uav_desc;

		resource_handle		  h_gi_resolve_curr_buffer;
		srv_desc_handle		  h_gi_resolve_curr_buffer_srv_desc;
		uav_desc_handle		  h_gi_resolve_curr_buffer_uav_desc;
		clear_uav_desc_handle h_gi_resolve_curr_buffer_clear_uav_desc;

		resource_handle h_gi_resolve_scratch_buffer;
		srv_desc_handle h_gi_resolve_scratch_buffer_srv_desc;
		uav_desc_handle h_gi_resolve_scratch_buffer_uav_desc;

		AGE_DECL_PING_PONG_BUFFER(gi_resolve_specular, is_alt, srv, uav, clear_uav)
		AGE_DECL_PING_PONG_BUFFER(gi_resolve_specular_age, is_alt, srv, uav, clear_uav)

		resource_handle		  h_gi_resolve_specular_final_buffer;
		srv_desc_handle		  h_gi_resolve_specular_final_buffer_srv_desc;
		uav_desc_handle		  h_gi_resolve_specular_final_buffer_uav_desc;
		clear_uav_desc_handle h_gi_resolve_specular_final_buffer_clear_uav_desc;

		resource_handle h_adaptive_ray_type_buffer;
		srv_desc_handle h_adaptive_ray_type_buffer_srv_desc;
		uav_desc_handle h_adaptive_ray_type_buffer_uav_desc;

		resource_handle h_adaptive_ray_entry_buffer;
		srv_desc_handle h_adaptive_ray_entry_buffer_srv_desc;
		uav_desc_handle h_adaptive_ray_entry_buffer_uav_desc;

		resource_handle h_indirect_arg_buffer;
		uav_desc_handle h_indirect_arg_buffer_uav_desc;
	};

	struct ao_desc
	{
		uint8	slice_count;
		uint8	offset_count;
		uint8_2 _;
		float	radius;			  // world
		float	max_px_radius;	  // px_radius = min(max_px_radius, world_to_screen(radius))
		float	intensity;		  // lerp(1.f, ao, intensity);
		float	power;			  // pow(ao, power);
		float	thickness;
		float	fade_distance;	  //
		float	fade_range;		  // 1 - smoothstep(fade_range - fade_distance, fade_range, d)

		graphics::e::ao_debug_flags debug_flags;
	};

	struct ao_data
	{
		shared_type::ao_data ao_data_gpu;

		bool  enabled;
		bool  need_cleanup;
		bool  is_alt;
		uint8 _;

		resource_handle h_ao_buffer;
		srv_desc_handle h_ao_buffer_srv_desc;
		uav_desc_handle h_ao_buffer_uav_desc;

		AGE_DECL_PING_PONG_BUFFER(ao_raw, is_alt, srv, uav, clear_uav)			  // r16_float
		AGE_DECL_PING_PONG_BUFFER(ao_bent_normal, is_alt, srv, uav, clear_uav)	  // rg16_snorm
		AGE_DECL_PING_PONG_BUFFER(ao_age, is_alt, srv, uav, clear_uav)			  // r8_uint
	};

	struct segment_data
	{
		shared_type::segment_data segment_data_gpu;

		resource_handle h_segment_buffer;
		srv_desc_handle h_segment_buffer_srv_desc;
		uav_desc_handle h_segment_buffer_uav_desc;


		resource_handle h_transparent_segment_buffer;
		srv_desc_handle h_transparent_segment_buffer_srv_desc;
		uav_desc_handle h_transparent_segment_buffer_uav_desc;
	};

	struct aa_desc
	{
		bool fxaa_on_offscreen = true;

		// 0 (disabled), 2, 4, 8, 16
		uint8 opaque_aa_ray_per_px = 8;

		// 0 (disabled), 2, 4, 8, 16
		uint8 transparent_aa_ray_per_px = 8;

		// max_aa_ray_budget = screen_px_count * aa_px_cap * (opaque_aa_rpp + transparent_aa_rpp)
		// (0,1]
		float aa_px_cap = 0.05f;

		// max_aa_px_count = screen_px_count * aa_px_cap * aa_px_headroom;
		// (1, 1/aa_px_cap]
		float aa_px_headroom			   = 4.f;
		float edge_plane_dist_tolerance_px = 0.05f;
		float edge_normal_threshold		   = 0.9f;
	};

	struct aa_data
	{
		shared_type::aa_data aa_data_gpu;

		bool enabled;
		bool opaque_rtaa_enabled;
		bool transparent_rtaa_enabled;
		bool fxaa_on_offscreen;

		uint8 opaque_ray_per_px;
		uint8 transparent_ray_per_px;

		uint8_2 _;

		float  aa_px_cap;
		float  aa_px_headroom;
		uint32 opaque_ray_budget;
		uint32 transparent_ray_budget;

		resource_handle h_ray_buffer;
		srv_desc_handle h_ray_buffer_srv_desc;
		uav_desc_handle h_ray_buffer_uav_desc;

		resource_handle		  h_indirect_arg_buffer;
		uav_desc_handle		  h_indirect_arg_buffer_uav_desc;
		clear_uav_desc_handle h_indirect_arg_buffer_clear_uav_desc;
	};

	struct debug_view_slot_desc
	{
		graphics::e::hrp_debug_view_system_kind		  system_kind;
		uint32										  system_debug_view_kind;
		uint32										  system_debug_view_overlay_flags;
		uint32										  system_debug_view_cursor_overlay_flags;
		uint32										  system_popup_view_kind;
		graphics::e::hrp_debug_view_slot_option_flags option_flags;		   // enabled/disabled, freeze, clear, enable_cursor_interact
		graphics::e::hrp_debug_view_color_map_kind	  color_map_kind;	   // enabled/disabled, freeze, clear, enable_cursor_interact
		float2										  size_uv;			   // default : 0.125, 0.125
		float2										  offset_uv;		   // default : 0, 0
		float2										  pos_uv;			   // default : -1, -1, disabled
		float3										  scalar_range_min;	   // [0, 1)
		float3										  scalar_range_max;	   // [0, 1)
		float										  alpha;
		float										  popup_zoom;
		float3										  background_color;
		uint32										  border_thickness;


		uint32_4 payload[4];
	};

	struct debug_view_desc
	{
		debug_view_slot_desc			fullscreen_slot_desc;	 // ignores size_uv
		std::span<debug_view_slot_desc> slot_descs;

		float2	popup_view_size_uv;
		uint32	popup_border_thickness;
		int32_2 cursor_px;
		float3	nan_color;
		float3	pos_inf_color;
		float3	neg_inf_color;
		float3	zero_color;
		float3	below_min_color;
		float3	above_max_color;
	};

	struct debug_view_data
	{
		shared_type::debug_view_data												gpu_data;
		age::array<shared_type::debug_view_slot_data, g::debug_view_slot_count_max> gpu_slot_data;

		bool	enabled		 = false;
		bool	need_cleanup = false;
		uint8_2 _;

		resource_handle		  h_debug_view_buffer;
		srv_desc_handle		  h_debug_view_buffer_srv_desc;
		uav_desc_handle		  h_debug_view_buffer_uav_desc;
		clear_uav_desc_handle h_debug_view_buffer_clear_uav_desc;

		resource_handle		  h_debug_view_scratch_buffer;
		srv_desc_handle		  h_debug_view_scratch_buffer_srv_desc;
		uav_desc_handle		  h_debug_view_scratch_buffer_uav_desc;
		clear_uav_desc_handle h_debug_view_scratch_buffer_clear_uav_desc;
	};
}	 // namespace age::graphics::render_pipeline
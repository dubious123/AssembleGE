#pragma once
#include "age.hpp"

namespace age::graphics::render_pipeline::forward_plus
{
	struct init_stage
	{
		graphics::pso::handle h_pso = {};
		ID3D12PipelineState*  p_pso = nullptr;

		inline void
		init(graphics::root_signature::handle h_root_sig) noexcept;

		inline void
		execute(t_cmd_list& cmd_list, uint32 tile_total_uint32_count) noexcept;

		inline void
		deinit() noexcept;
	};

	struct depth_stage
	{
		dsv_desc_handle h_depth_buffer_dsv_desc;

		graphics::pso::handle h_pso = {};
		ID3D12PipelineState*  p_pso = nullptr;

		inline void
		init(graphics::root_signature::handle h_root_sig) noexcept;

		inline void
		bind_dsv(graphics::resource_handle h_depth_buffer) noexcept;
		inline void
		execute(t_cmd_list& cmd_list, uint32 job_count) noexcept;

		inline void
		deinit() noexcept;
	};

	struct shadow_stage
	{
		graphics::pso::handle h_depth_reduce_pso;
		ID3D12PipelineState*  p_depth_reduce_pso;

		graphics::pso::handle h_fill_shadow_buffer_pso;
		ID3D12PipelineState*  p_fill_shadow_buffer_pso;

		graphics::pso::handle h_shadow_map_pso;
		ID3D12PipelineState*  p_shadow_map_pso;

		dsv_desc_handle h_shadow_atlas_dsv_desc;

		inline void
		init(graphics::root_signature::handle h_root_sig) noexcept;
		inline void
		bind_dsv(graphics::resource_handle h_shadow_atlas) noexcept;

		inline void
		execute(t_cmd_list&		cmd_list,
				uint32			width,
				uint32			height,
				uint32			shadow_light_count,
				uint32			shadow_light_header_count,
				ID3D12Resource& frame_data_rw_buffer,
				ID3D12Resource& shadow_light_rw_buffer,
				auto&			slot_shadow_light_rw_buffer_srv,
				uint32			job_count) noexcept;

		inline void
		deinit() noexcept;
	};

	struct light_culling_stage
	{
		graphics::pso::handle h_pso_cull;
		ID3D12PipelineState*  p_pso_cull;

		graphics::pso::handle h_pso_sort_histogram;
		ID3D12PipelineState*  p_pso_sort_histogram;

		graphics::pso::handle h_pso_sort_prefix;
		ID3D12PipelineState*  p_pso_sort_prefix;

		graphics::pso::handle h_pso_sort_scatter;
		ID3D12PipelineState*  p_pso_sort_scatter;

		graphics::pso::handle h_pso_zbin;
		ID3D12PipelineState*  p_pso_zbin;

		graphics::pso::handle h_pso_tile;
		ID3D12PipelineState*  p_pso_tile;

		inline void
		init(graphics::root_signature::handle h_root_sig) noexcept;

		inline void
		execute(t_cmd_list&		cmd_list,
				uint32			light_tile_count_x,
				uint32			light_tile_count_y,
				ID3D12Resource& unified_sorted_light_buffer,
				ID3D12Resource& frame_data_rw_buffer,
				auto&			slot_frame_data_rw_buffer_srv,
				ID3D12Resource& sort_buffer,
				auto&			slot_sort_buffer_srv,
				ID3D12Resource& zbin_buffer,
				auto&			slot_zbin_buffer_srv) noexcept;

		inline void
		deinit() noexcept;
	};

	struct opaque_stage
	{
		rtv_desc_handle h_main_buffer_rtv_desc;
		dsv_desc_handle h_depth_buffer_dsv_desc;

		graphics::pso::handle h_pso;
		ID3D12PipelineState*  p_pso;

		inline void
		init(graphics::root_signature::handle h_root_sig) noexcept;

		inline void
		bind_rtv_dsv(graphics::resource_handle h_main_buffer,
					 graphics::resource_handle h_depth_buffer) noexcept;

		inline void
		execute(t_cmd_list& cmd_list, uint32 job_count) noexcept;
		inline void
		deinit() noexcept;
	};

	struct transparent_stage
	{
		graphics::pso::handle h_pso_cull;
		ID3D12PipelineState*  p_pso_cull;

		graphics::pso::handle h_pso_sort_histogram;
		ID3D12PipelineState*  p_pso_sort_histogram;

		graphics::pso::handle h_pso_sort_prefix;
		ID3D12PipelineState*  p_pso_sort_prefix;

		graphics::pso::handle h_pso_sort_scatter;
		ID3D12PipelineState*  p_pso_sort_scatter;

		graphics::pso::handle h_pso_transparent_gen_indirect_arg;
		ID3D12PipelineState*  p_pso_transparent_gen_indirect_arg;

		graphics::pso::handle h_pso_draw;
		ID3D12PipelineState*  p_pso_draw;

		command_signature::handle h_draw_cmd_sig;
		ID3D12CommandSignature*	  p_draw_cmd_sig;

		rtv_desc_handle h_main_buffer_rtv_desc;
		dsv_desc_handle h_depth_buffer_dsv_desc;


		inline void
		init(graphics::root_signature::handle h_root_sig) noexcept;

		inline void
		bind_rtv_dsv(graphics::resource_handle h_main_buffer,
					 graphics::resource_handle h_depth_buffer) noexcept;

		inline void
		execute(t_cmd_list&		cmd_list,
				ID3D12Resource& sort_buffer,
				auto&			slot_sort_buffer_srv,
				ID3D12Resource& frame_data_rw_buffer,
				auto&			frame_data_rw_buffer_srv) noexcept;

		inline void
		deinit() noexcept;
	};

	struct presentation_stage
	{
		pso::handle			 h_pso = {};
		ID3D12PipelineState* p_pso = nullptr;

		inline void
		init(root_signature::handle h_root_sig) noexcept;

		inline void
		execute(t_cmd_list& cmd_list, render_surface& rs) noexcept;

		inline void
		deinit() noexcept;
	};

	struct pipeline
	{
		graphics::root_signature::handle h_root_sig;
		ID3D12RootSignature*			 p_root_sig;

		init_stage			stage_init;
		depth_stage			stage_depth;
		shadow_stage		stage_shadow;
		light_culling_stage stage_light_culling;
		opaque_stage		stage_opaque;
		transparent_stage	stage_transparent;
		presentation_stage	stage_presentation;

		resource_handle h_main_buffer;
		resource_handle h_depth_buffer;
		resource_handle h_sort_buffer;
		resource_handle h_zbin_buffer;
		resource_handle h_tile_mask_buffer;
		resource_handle h_shadow_atlas;
		resource_handle h_unified_sorted_light_buffer;
		resource_handle h_shadow_light_buffer;


		resource::mapping_handle h_mapping_frame_data;
		resource::mapping_handle h_mapping_opaque_meshlet_render_data_buffer;
		resource::mapping_handle h_mapping_object_data_buffer;
		resource::mapping_handle h_mapping_mesh_buffer;
		resource::mapping_handle h_mapping_directional_light_buffer;
		resource::mapping_handle h_mapping_unified_light_buffer;
		resource::mapping_handle h_mapping_frame_data_rw_buffer;
		resource::mapping_handle h_mapping_shadow_light_header_buffer;
		resource::mapping_handle h_mapping_transparent_object_render_data_buffer;
		resource::mapping_handle h_mapping_debug_buffer_uav;


		// global
		binding_config_t::reg_b<0, 0> frame_data_buffer;
		binding_config_t::reg_b<1, 0> root_constants;
		binding_config_t::reg_b<2, 0> indirect_arg;

		binding_config_t::reg_t<0, 0> opaque_render_data_buffer;
		binding_config_t::reg_t<1, 0> object_data_buffer;
		binding_config_t::reg_t<2, 0> mesh_data_buffer;

		binding_config_t::reg_t<3, 0> directional_light_buffer;
		binding_config_t::reg_t<4, 0> unified_light_buffer;

		binding_config_t::reg_t<5, 0> frame_data_rw_buffer_srv;
		binding_config_t::reg_u<5, 0> frame_data_rw_buffer_uav;


		// shadow
		binding_config_t::reg_t<0, 1> shadow_light_header_buffer;

		binding_config_t::reg_t<1, 1> shadow_light_buffer_srv;
		binding_config_t::reg_u<1, 1> shadow_light_buffer_uav;


		// light culling
		binding_config_t::reg_t<0, 2> sort_buffer_srv;
		binding_config_t::reg_u<0, 2> sort_buffer_uav;

		binding_config_t::reg_t<1, 2> zbin_buffer_srv;
		binding_config_t::reg_u<1, 2> zbin_buffer_uav;

		binding_config_t::reg_t<2, 2> tile_mask_buffer_srv;
		binding_config_t::reg_u<2, 2> tile_mask_buffer_uav;

		binding_config_t::reg_t<3, 2> unified_sorted_light_buffer_srv;
		binding_config_t::reg_u<3, 2> unified_sorted_light_buffer_uav;

		// transparent
		binding_config_t::reg_t<0, 3> transparent_object_render_data_buffer;


		// debug
		binding_config_t::reg_u<7, 7> debug_buffer_uav;


		// bindless texture
		srv_desc_handle h_main_buffer_srv_desc;
		srv_desc_handle h_depth_buffer_srv_desc;
		srv_desc_handle h_shadow_atlas_srv_desc;

		// details
		extent_2d<uint16> extent{ .width = 100, .height = 100 };

		uint32 light_tile_count_x = (extent.width + g::light_tile_size - 1) / g::light_tile_size;
		uint32 light_tile_count_y = (extent.height + g::light_tile_size - 1) / g::light_tile_size;


		age::sparse_vector<camera_desc> camera_desc_vec;
		age::sparse_vector<camera_data> camera_data_vec;

		age::sparse_vector<mesh_data> mesh_data_vec;
		uint32						  mesh_byte_offset = 0;

		age::stable_dense_vector<shared_type::object_data> object_data_vec;

		age::vector<shared_type::opaque_meshlet_render_data> opaque_meshlet_render_data_vec[graphics::g::frame_buffer_count][graphics::g::thread_count];

		age::vector<shared_type::transparent_object_render_data> transparent_object_render_data_vec[graphics::g::frame_buffer_count][graphics::g::thread_count];

		age::stable_dense_vector<shared_type::directional_light> directional_light_vec;

		age::stable_dense_vector<shared_type::unified_light> unified_light_vec;

		std::array<shadow_light_header, g::max_shadow_light_count> shadow_light_header_arr;

		t_shadow_light_id shadow_light_header_count		 = 0u;
		t_shadow_light_id next_shadow_light_id			 = 0u;
		t_shadow_light_id directional_shadow_light_count = 0u;

		// main
		void
		init() noexcept;

		void
		bind() noexcept;

		void
		deinit() noexcept;

		bool
		begin_render(render_surface_handle h_rs) noexcept;

		void
		render_mesh(uint8 thread_id, t_object_id object_id, t_mesh_id mesh_id) noexcept;

		void
		render_transparent_mesh(uint8 thread_id, t_object_id object_id, t_mesh_id mesh_id) noexcept;

		void
		end_render(render_surface_handle h_rs) noexcept;

		// mesh
		t_mesh_id
		upload_mesh(const asset::mesh_baked& baked) noexcept;

		void
		release_mesh(t_mesh_id id) noexcept;

		// object
		t_object_id
		add_object(const shared_type::object_data& data = {}) noexcept;

		void
		update_object(t_object_id id, const shared_type::object_data& data = {}) noexcept;

		void
		remove_object(t_object_id& id) noexcept;

		// camera
		t_camera_id
		add_camera(const camera_desc& desc) noexcept;

		camera_desc
		get_camera_desc(t_camera_id id) noexcept;
		void
		update_camera(t_camera_id id, const camera_desc& desc) noexcept;

		void
		remove_camera(t_camera_id& id) noexcept;

		// light
		void
		update_directional_light(t_directional_light_id id, const directional_light_desc& desc) noexcept;

		t_directional_light_id
		add_directional_light(const directional_light_desc& desc, bool cast_shadow = true) noexcept;

		void
		remove_directional_light(t_directional_light_id& id) noexcept;

		void
		update_point_light(t_unified_light_id id, const point_light_desc& desc) noexcept;

		t_unified_light_id
		add_point_light(const point_light_desc& desc, bool cast_shadow = false) noexcept;

		void
		update_spot_light(t_unified_light_id id, const spot_light_desc& desc) noexcept;

		t_unified_light_id
		add_spot_light(const spot_light_desc& desc, bool cast_shadow = false) noexcept;

		void
		remove_point_light(t_unified_light_id& id) noexcept;

		void
		remove_spot_light(t_unified_light_id& id) noexcept;

	  private:
		void
		create_resolution_dependent_buffers() noexcept;

		void
		resize_resolution_dependent_buffers(const age::extent_2d<uint16>& new_extent) noexcept;
	};
}	 // namespace age::graphics::render_pipeline::forward_plus
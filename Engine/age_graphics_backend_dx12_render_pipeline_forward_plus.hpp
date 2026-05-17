#pragma once
#include "age.hpp"

namespace age::graphics::render_pipeline::forward_plus
{
	struct depth_stage
	{
		graphics::pso::handle h_pso = {};
		ID3D12PipelineState*  p_pso = nullptr;

		void
		init(graphics::root_signature::handle h_root_sig) noexcept;

		inline void
		execute(dsv_desc_handle _,
				uint32			opaque_meshlet_count) noexcept;

		void
		deinit() noexcept;
	};

	struct skybox_stage
	{
		graphics::pso::handle h_pso;
		ID3D12PipelineState*  p_pso;

		void
		init(graphics::root_signature::handle h_root_sig) noexcept;

		inline void
		execute(rtv_desc_handle _,
				dsv_desc_handle h_dsv_readonly_desc,
				uint32			env_light_count) noexcept;

		void
		deinit() noexcept;
	};

	struct light_culling_stage
	{
		graphics::pso::handle h_pso_init;
		ID3D12PipelineState*  p_pso_init;

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

		void
		init(graphics::root_signature::handle h_root_sig) noexcept;

		inline void
		execute(uint32			light_tile_count_x,
				uint32			light_tile_count_y,
				resource_handle h_scratch_buffer) noexcept;

		void
		deinit() noexcept;
	};

	struct opaque_stage
	{
		graphics::pso::handle h_pso;
		ID3D12PipelineState*  p_pso;

		void
		init(graphics::root_signature::handle h_root_sig) noexcept;

		inline void
		execute(rtv_desc_handle _,
				dsv_desc_handle h_dsv_readonly_desc,
				uint32			meshlet_count) noexcept;

		void
		deinit() noexcept;
	};

	struct transparent_stage
	{
		graphics::pso::handle h_pso_rt;
		ID3D12PipelineState*  p_pso_rt;

		graphics::pso::handle h_pso_draw;
		ID3D12PipelineState*  p_pso_draw;

		void
		init(graphics::root_signature::handle h_root_sig) noexcept;

		inline void
		execute(rtv_desc_handle, resource_handle h_blend_tex, extent_2d<uint16> extent) noexcept;

		void
		deinit() noexcept;
	};

	struct raycast_stage
	{
		graphics::pso::handle h_pso;
		ID3D12PipelineState*  p_pso;

		void
		init(graphics::root_signature::handle h_root_sig) noexcept;

		inline void
		execute(uint32 raycast_count) noexcept;

		void
		deinit() noexcept;
	};

	struct post_process_stage
	{
		pso::handle			 h_pso = {};
		ID3D12PipelineState* p_pso = nullptr;

		void
		init(graphics::root_signature::handle h_root_sig) noexcept;

		inline void
		execute(rtv_desc_handle _) noexcept;

		void
		deinit() noexcept;
	};

	struct selection_outline_stage
	{
		pso::handle			 h_pso_mask;
		ID3D12PipelineState* p_pso_mask = nullptr;

		pso::handle			 h_pso_draw;
		ID3D12PipelineState* p_pso_draw = nullptr;

		void
		init(root_signature::handle h_root_sig) noexcept;

		inline void
		execute(uint32			selected_meshlet_count,
				rtv_desc_handle h_desc_mask_buffer,
				rtv_desc_handle h_desc_rtv_buffer,
				resource_handle h_outline_mask) noexcept;

		void
		deinit() noexcept;
	};

	struct ui_stage
	{
		pso::handle			 h_pso_screen = {};
		ID3D12PipelineState* p_pso_screen = nullptr;

		pso::handle			 h_pso_world = {};
		ID3D12PipelineState* p_pso_world = nullptr;

		void
		init(root_signature::handle h_root_sig) noexcept;

		inline void
		execute(binding_config_t::reg_b<1>&		constants,
				rtv_desc_handle					h_rtv_desc,
				dsv_desc_handle					h_dsv_desc,
				ui::e::space_mode_kind			space_mode,
				uint32							root_data_idx,
				uint32							root_data_count,
				const age::vector<util::range>& z_range_vec,
				const age::vector<util::range>& z_range_range_vec) noexcept;

		void
		deinit() noexcept;
	};

	struct presentation_stage
	{
		pso::handle			 h_pso = {};
		ID3D12PipelineState* p_pso = nullptr;

		void
		init(root_signature::handle h_root_sig) noexcept;

		inline void
		execute(render_surface& rs) noexcept;

		void
		deinit() noexcept;
	};

	struct pipeline
	{
		graphics::root_signature::handle h_root_sig;
		ID3D12RootSignature*			 p_root_sig;

		depth_stage				stage_depth;
		skybox_stage			stage_skybox;
		light_culling_stage		stage_light_culling;
		opaque_stage			stage_opaque;
		transparent_stage		stage_transparent;
		raycast_stage			stage_raycast;
		post_process_stage		stage_post_process;
		selection_outline_stage stage_selection_outline;
		ui_stage				stage_ui;
		presentation_stage		stage_presentation;

		resource_handle h_main_buffer;
		resource_handle h_post_buffer;
		resource_handle h_selection_outline_mask_buffer;
		resource_handle h_depth_buffer;
		resource_handle h_rt_transparent_texture_buffer;

		rtv_desc_handle h_main_buffer_rtv_desc;
		rtv_desc_handle h_post_buffer_rtv_desc;
		rtv_desc_handle h_selection_outline_mask_buffer_rtv_desc;
		dsv_desc_handle h_depth_buffer_dsv_readonly_desc;
		dsv_desc_handle h_depth_buffer_dsv_desc;

		resource_handle h_scratch_buffer;
		resource_handle h_light_cull_stage_buffer;
		resource_handle h_light_cull_stage_sorted_light_buffer;


		std::array<mapping_handle, graphics::g::frame_buffer_count> h_mapping_static_ring_buffer_arr;


		mapping_handle h_mapping_frame_data;
		mapping_handle h_mapping_mesh_buffer;
		mapping_handle h_mapping_rt_index_buffer;
		mapping_handle h_mapping_rt_vertex_scratch_buffer;
		mapping_handle h_mapping_material_buffer;

		std::array<mapping_handle, graphics::g::frame_buffer_count> h_mapping_env_light_buffer_arr;


		// rt, not for binding
		age::vector<D3D12_RAYTRACING_INSTANCE_DESC>		  rt_instance_data_vec[graphics::g::thread_count];
		age::vector<shared_type::rt_instance_render_data> rt_instance_render_data_vec[graphics::g::thread_count];


		std::array<mapping_handle, graphics::g::frame_buffer_count> h_mapping_rt_instance_buffer_arr;
		std::array<mapping_handle, graphics::g::frame_buffer_count> h_mapping_rt_instance_render_data_buffer_arr;
		std::array<mapping_handle, graphics::g::frame_buffer_count> h_mapping_rt_raycast_request_buffer_arr;
		resource_handle												h_rt_raycast_result_buffer;
		std::array<mapping_handle, graphics::g::frame_buffer_count> h_readback_rt_raycast_result_buffer_arr;	// readback

		resource_handle h_rt_tlas_buffer;
		resource_handle h_rt_tlas_scratch_buffer;

		// global
		binding_config_t::reg_b<0, 0> frame_data_buffer;
		binding_config_t::reg_b<1, 0> root_constants;

		binding_config_t::reg_t<0, 0> static_ring_buffer;
		binding_config_t::reg_t<1, 0> mesh_data_buffer;

		binding_config_t::reg_u<0, 0> scratch_buffer_uav;

		// light cull
		binding_config_t::reg_t<0, 1> light_cull_stage_buffer_srv;
		binding_config_t::reg_u<0, 1> light_cull_stage_buffer_uav;
		binding_config_t::reg_t<1, 1> light_cull_stage_sorted_light_buffer_srv;
		binding_config_t::reg_u<1, 1> light_cull_stage_sorted_light_buffer_uav;

		// material
		binding_config_t::reg_t<0, 2> material_buffer;

		// env_light
		binding_config_t::reg_t<1, 2> env_light_buffer;

		// selection outline
		binding_config_t::reg_t<0, 4>								selection_outline_meshlet_render_data_buffer;
		binding_config_t::reg_t<1, 4>								selection_outline_data_buffer;
		std::array<mapping_handle, graphics::g::frame_buffer_count> h_mapping_selection_outline_meshlet_render_data_buffer_arr;
		std::array<mapping_handle, graphics::g::frame_buffer_count> h_mapping_selection_outline_data_buffer_arr;

		// ui
		binding_config_t::reg_t<0, 5>								ui_root_data_buffer;
		binding_config_t::reg_t<1, 5>								ui_data_buffer;
		std::array<mapping_handle, graphics::g::frame_buffer_count> h_mapping_ui_root_data_buffer_arr;
		std::array<mapping_handle, graphics::g::frame_buffer_count> h_mapping_ui_data_buffer_arr;

		// ui_root_data_idx_arr : prefix of root_data_vec.size()
		// ui_root_data_idx_arr -> ui_root_data_vec_arr -> ui_render_data_z_range_of_range_vec -> ui_render_data_z_range_vec -> ui_render_data_vec
		age::vector<ui::render_data>												 ui_render_data_vec;
		age::vector<util::range>													 ui_render_data_z_range_vec;
		age::vector<util::range>													 ui_render_data_z_range_of_range_vec;
		age::array<age::vector<ui::root_graphics_data>, ui::e::space_mode_kind_size> ui_root_data_vec_arr;
		age::array<uint32, ui::e::space_mode_kind_size>								 ui_root_data_idx_arr;

		// rt
		binding_config_t::reg_t<0, 3> rt_instance_render_data_buffer_srv;
		binding_config_t::reg_t<1, 3> rt_index_buffer_srv;
		binding_config_t::reg_t<2, 3> rt_raycast_request_buffer_srv;
		binding_config_t::reg_u<3, 3> rt_raycast_result_buffer_uav;

		// bindless texture
		srv_desc_handle h_main_buffer_srv_desc;
		srv_desc_handle h_post_buffer_srv_desc;
		srv_desc_handle h_depth_buffer_srv_desc;
		srv_desc_handle h_rt_tlas_buffer_srv_desc;
		srv_desc_handle h_rt_transparent_tex_buffer_srv_desc;
		uav_desc_handle h_rt_transparent_tex_buffer_uav_desc;

		srv_desc_handle h_env_light_brdf_lut;

		srv_desc_handle h_selection_outline_mask_buffer_srv_desc;

		// details
		extent_2d<uint16> extent{ .width = 100, .height = 100 };

		// external textures
		age::unordered_map<t_texture_id, texture_data> texture_map;

		// mesh
		age::sparse_vector<mesh_data> mesh_data_vec;

		age::offset_pool<> mesh_persistant_buffer_offset_pool;
		age::offset_pool<> mesh_rt_index_buffer_offset_pool;

		age::stable_dense_vector<shared_type::object_data> object_data_vec;

		// material
		age::sparse_vector<asset::handle> material_vec;

		// camera
		age::sparse_vector<camera_desc> camera_desc_vec;
		age::sparse_vector<camera_data> camera_data_vec;
		t_camera_id						main_camera_id;

		age::stable_dense_vector<float3x4> object_transform_data_vec;

		age::vector<shared_type::opaque_meshlet_render_data> opaque_meshlet_render_data_vec[graphics::g::thread_count];

		// light
		uint32 light_tile_count_x = (extent.width + g::light_tile_size - 1) / g::light_tile_size;
		uint32 light_tile_count_y = (extent.height + g::light_tile_size - 1) / g::light_tile_size;

		age::stable_dense_vector<shared_type::directional_light> directional_light_vec;

		age::stable_dense_vector<shared_type::unified_light> unified_light_vec;

		// env_light
		age::stable_dense_vector<shared_type::env_light> env_light_gpu_data_vec;
		age::sparse_vector<env_light_data>				 env_light_cpu_data_vec;

		// raycast
		age::dynamic_array<shared_type::raycast_result> raycast_result_vec[graphics::g::frame_buffer_count];
		age::vector<shared_type::raycast_request>		raycast_request_vec[graphics::g::frame_buffer_count];

		// selection_outline
		age::vector<shared_type::selection_outline_data>				selection_outline_data_vec;
		age::vector<shared_type::selection_outline_meshlet_render_data> selection_outline_meshlet_render_data_vec;

		// debug & immediate & ui
		age::vector<t_object_id>							debug_object_id_vec;
		age::vector<shared_type::debug_meshlet_render_data> debug_meshlet_render_data_vec;

		// main
		void
		init() noexcept;

		void
		deinit() noexcept;

		void
		begin_frame() noexcept;	   // optional

		bool
		begin_render(render_surface_handle h_rs) noexcept;

		void
		render_mesh(uint8 thread_id, t_object_id object_id, asset::handle h_mesh, asset::handle h_mat) noexcept;

		void
		render_selection_outline(t_object_id, asset::handle h_mesh, const float4& rgba, float thickness, float softness) noexcept;

		// legacy
		void
		render_mesh(uint8 thread_id, t_object_id object_id, asset::handle h_mesh, t_material_id mat_id = age::get_invalid_id<uint32>()) noexcept;

		// legacy
		void
		render_mesh(uint8 thread_id, t_object_id object_id, t_mesh_id mesh_id, t_material_id mat_id = age::get_invalid_id<uint32>()) noexcept;

		// legacy
		void
		render_transparent_mesh(uint8 thread_id, t_object_id object_id, asset::handle h_mesh, t_material_id mat_id = age::get_invalid_id<uint32>()) noexcept;

		// legacy
		void
		render_transparent_mesh(uint8 thread_id, t_object_id object_id, t_mesh_id mesh_id, t_material_id mat_id = age::get_invalid_id<uint32>()) noexcept;

		void
		end_render(render_surface_handle h_rs) noexcept;

		// texture
		t_texture_id
		upload_texture(asset::handle _) noexcept;

		t_texture_id
		upload_texture(const void*, age::extent_2d<uint32>, graphics::e::texture_format) noexcept;

		void
		release_texture(t_texture_id& _) noexcept;

		// material
		t_material_id
		upload_material(asset::handle _) noexcept;

		void
		update_material(asset::handle _) noexcept;

		void
		release_material(t_material_id&) noexcept;

		// mesh
		t_mesh_id
		upload_mesh(asset::handle _) noexcept;

		void
		release_mesh(t_mesh_id&) noexcept;

		// object
		t_object_id
		add_object(const float3& pos, const float4& quat, const float3& scale) noexcept;

		void
		update_object(t_object_id id, const float3& pos, const float4& quat, const float3& scale) noexcept;

		float4x4
		get_object_transform_matrix(t_object_id id) const noexcept;

		void
		remove_object(t_object_id& id) noexcept;

		// camera
		t_camera_id
		add_camera(const camera_desc& desc) noexcept;

		void
		set_main_camera(t_camera_id id) noexcept;

		camera_desc
		get_camera_desc(t_camera_id id) noexcept;

		camera_data
		get_camera_data(t_camera_id id) noexcept;

		void
		update_camera(t_camera_id id, const camera_desc& desc) noexcept;

		void
		remove_camera(t_camera_id& id) noexcept;

		// light
		void
		update_directional_light(t_directional_light_id id, const directional_light_desc& desc) noexcept;

		t_directional_light_id
		add_directional_light(const directional_light_desc& desc) noexcept;

		void
		remove_directional_light(t_directional_light_id& id) noexcept;

		t_unified_light_id
		add_point_light(const point_light_desc& desc) noexcept;

		void
		update_point_light(t_unified_light_id id, const point_light_desc& desc) noexcept;

		t_unified_light_id
		add_spot_light(const spot_light_desc& desc) noexcept;

		void
		update_spot_light(t_unified_light_id id, const spot_light_desc& desc) noexcept;

		void
		remove_point_light(t_unified_light_id& id) noexcept;

		void
		remove_spot_light(t_unified_light_id& id) noexcept;

		// env_light
		t_env_light_id
		upload_env_light(asset::handle _) noexcept;

		void
		update_env_light_runtime(asset::handle _) noexcept;

		void
		release_env_light(t_env_light_id&) noexcept;

		// ui
		std::tuple<
			age::vector<ui::render_data>&,
			age::vector<util::range>&,
			age::vector<util::range>&,
			age::array<age::vector<ui::root_graphics_data>, ui::e::space_mode_kind_size>&>
		get_ui_sink() noexcept;

		// raycast
		t_raycast_id
		request_raycast(const float3& origin, const float3& dir, float max_t, e::rt_mask_kind mask = e::rt_mask_kind::all) noexcept;

		shared_type::raycast_result
		get_raycast_result(t_raycast_id _) noexcept;

		// debug
		void
		render_debug_mesh(const float3& pos, const float4& quat, const float3& scale, asset::handle h_mesh, const float3& color) noexcept;

		void
		render_debug_line() noexcept;

	  private:
		void
		create_resolution_dependent_buffers() noexcept;

		void
		resize_resolution_dependent_buffers(const age::extent_2d<uint16>& new_extent) noexcept;

		uint32
		upload_data() noexcept;
	};
}	 // namespace age::graphics::render_pipeline::forward_plus
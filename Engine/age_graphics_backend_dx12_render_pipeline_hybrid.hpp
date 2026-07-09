#pragma once
#include "age.hpp"

namespace age::graphics::render_pipeline
{
	struct depth_stage
	{
		graphics::pso::handle h_pso_opaque = {};
		ID3D12PipelineState*  p_pso_opaque = nullptr;

		graphics::pso::handle h_pso_transparent = {};
		ID3D12PipelineState*  p_pso_transparent = nullptr;

		void
		init(graphics::root_signature::handle h_root_sig) noexcept;

		inline void
		execute(rtv_desc_handle h_opaque_gbuffer_rtv_desc,
				rtv_desc_handle h_motion_buffer_rtv_desc,
				dsv_desc_handle h_opaque_depth_buffer_dsv_desc,
				uint32			opaque_meshlet_count,
				rtv_desc_handle h_transparent_gbuffer_rtv_desc,
				dsv_desc_handle h_transparent_depth_buffer_dsv_desc,
				uint32			transparent_meshlet_count) const noexcept;

		void
		deinit() noexcept;
	};

	struct segment_stage
	{
		graphics::pso::handle h_pso_resolve = {};
		ID3D12PipelineState*  p_pso_resolve = nullptr;

		void
		init(graphics::root_signature::handle h_root_sig) noexcept;

		inline void
		execute(const extent_2d<uint16>& extent) const noexcept;

		void
		deinit() noexcept;
	};

	struct ao_stage
	{
		graphics::pso::handle h_pso_resolve = {};
		ID3D12PipelineState*  p_pso_resolve = nullptr;

		void
		init(graphics::root_signature::handle h_root_sig) noexcept;

		inline void
		execute(const ao_data&, const extent_2d<uint16>&) const noexcept;

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
				uint32			env_light_count) const noexcept;

		void
		deinit() noexcept;
	};

	struct light_bin_stage
	{
		graphics::pso::handle h_pso_init;
		ID3D12PipelineState*  p_pso_init;

		graphics::pso::handle h_pso_light_sort_prepare;
		ID3D12PipelineState*  p_pso_light_sort_prepare;

		graphics::pso::handle h_pso_sort_histogram;
		ID3D12PipelineState*  p_pso_sort_histogram;

		graphics::pso::handle h_pso_sort_prefix;
		ID3D12PipelineState*  p_pso_sort_prefix;

		graphics::pso::handle h_pso_sort_scatter;
		ID3D12PipelineState*  p_pso_sort_scatter;

		graphics::pso::handle h_pso_zbin;
		ID3D12PipelineState*  p_pso_zbin;

		void
		init(graphics::root_signature::handle h_root_sig) noexcept;

		inline void
		execute(uint32			unified_light_count,
				resource_handle h_light_bin_stage_buffer,
				resource_handle h_scratch_buffer) const noexcept;

		void
		deinit() noexcept;
	};

	struct ddgi_stage
	{
		graphics::pso::handle h_pso_update_probe_state;
		ID3D12PipelineState*  p_pso_update_probe_state;

		graphics::pso::handle h_pso_reduce_ray_sum;
		ID3D12PipelineState*  p_pso_reduce_ray_sum;

		graphics::pso::handle h_pso_prefix_group;
		ID3D12PipelineState*  p_pso_prefix_group;

		graphics::pso::handle h_pso_prefix_group_sum;
		ID3D12PipelineState*  p_pso_prefix_group_sum;

		graphics::pso::handle h_pso_prefix_add;
		ID3D12PipelineState*  p_pso_prefix_add;

		graphics::pso::handle h_pso_probe_trace;
		ID3D12PipelineState*  p_pso_probe_trace;

		graphics::command_signature::handle h_cmd_sig_probe_trace;
		ID3D12CommandSignature*				p_cmd_sig_probe_trace;

		graphics::pso::handle h_pso_probe_blend;
		ID3D12PipelineState*  p_pso_probe_blend;

		graphics::pso::handle h_pso_copy_edge;
		ID3D12PipelineState*  p_pso_copy_edge;

		graphics::pso::handle h_pso_render_probes;
		ID3D12PipelineState*  p_pso_render_probes;

		void
		init(graphics::root_signature::handle h_root_sig) noexcept;

		inline void
		execute(const ddgi_data&			   ddgi_data_cpu,
				binding_config_t::reg_t<1, 7>& probe_buffer_srv,
				resource_handle				   h_indirect_arg_buffer) const noexcept;
		inline void
		execute_render_probes(rtv_desc_handle  h_main_buffer_rtv_desc,
							  dsv_desc_handle  h_depth_buffer_dsv_desc,
							  const ddgi_data& ddgi_data_cpu) const noexcept;

		void
		deinit() noexcept;
	};

	struct gibs_stage
	{
		graphics::pso::handle h_pso_cleanup;
		ID3D12PipelineState*  p_pso_cleanup;

		graphics::pso::handle h_pso_prepare;
		ID3D12PipelineState*  p_pso_prepare;

		graphics::pso::handle h_pso_tile_spawn_kill;
		ID3D12PipelineState*  p_pso_tile_spawn_kill;

		graphics::pso::handle h_pso_cell_spawn_kill;
		ID3D12PipelineState*  p_pso_cell_spawn_kill;

		graphics::pso::handle h_pso_update_surfel;
		ID3D12PipelineState*  p_pso_update_surfel;

		graphics::pso::handle h_pso_ray_ideal_count_sum;
		ID3D12PipelineState*  p_pso_ray_ideal_count_sum;

		graphics::pso::handle h_pso_ray_count_prefix;
		ID3D12PipelineState*  p_pso_ray_count_prefix;

		graphics::pso::handle h_pso_ray_entry;
		ID3D12PipelineState*  p_pso_ray_entry;

		graphics::pso::handle h_pso_tile_surfel_count_prefix;
		ID3D12PipelineState*  p_pso_tile_surfel_count_prefix;

		graphics::pso::handle h_pso_cell_surfel_count_prefix;
		ID3D12PipelineState*  p_pso_cell_surfel_count_prefix;

		graphics::pso::handle h_pso_tile_surfel_scatter;
		ID3D12PipelineState*  p_pso_tile_surfel_scatter;

		graphics::pso::handle h_pso_cell_surfel_scatter;
		ID3D12PipelineState*  p_pso_cell_surfel_scatter;

		graphics::pso::handle h_pso_gi_reproject;
		ID3D12PipelineState*  p_pso_gi_reproject;

		graphics::pso::handle h_pso_ray_trace;
		ID3D12PipelineState*  p_pso_ray_trace;

		graphics::pso::handle h_pso_ray_integrate;
		ID3D12PipelineState*  p_pso_ray_integrate;

		graphics::pso::handle h_pso_build_cdf;
		ID3D12PipelineState*  p_pso_build_cdf;

		graphics::pso::handle h_pso_gi_resolve;
		ID3D12PipelineState*  p_pso_gi_resolve;

		graphics::pso::handle h_pso_gi_reconstruct;
		ID3D12PipelineState*  p_pso_gi_reconstruct;

		graphics::pso::handle h_pso_debug_draw_surfels;
		ID3D12PipelineState*  p_pso_debug_draw_surfels;

		graphics::command_signature::handle h_cmd_sig;
		ID3D12CommandSignature*				p_cmd_sig;

		void
		init(graphics::root_signature::handle h_root_sig) noexcept;

		inline void
		execute(const gibs_data&  gibs_data_cpu,
				extent_2d<uint16> main_buffer_extent) const noexcept;


		inline void
		execute_render_surfels(rtv_desc_handle h_main_buffer_rtv_desc,
							   dsv_desc_handle h_depth_buffer_dsv_desc) const noexcept;
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
		execute(rtv_desc_handle h_main_buffer_rtv_desc) const noexcept;

		void
		deinit() noexcept;
	};

	struct aa_stage
	{
		graphics::pso::handle h_pso_opaque_ray_entry;
		ID3D12PipelineState*  p_pso_opaque_ray_entry;

		graphics::pso::handle h_pso_transparent_ray_entry;
		ID3D12PipelineState*  p_pso_transparent_ray_entry;

		graphics::pso::handle h_pso_indirect_arg;
		ID3D12PipelineState*  p_pso_indirect_arg;

		graphics::pso::handle h_pso_opaque_rt;
		ID3D12PipelineState*  p_pso_opaque_rt;

		graphics::pso::handle h_pso_transparent_rt;
		ID3D12PipelineState*  p_pso_transparent_rt;

		graphics::pso::handle h_pso_resolve;
		ID3D12PipelineState*  p_pso_resolve;

		graphics::command_signature::handle h_cmd_sig;
		ID3D12CommandSignature*				p_cmd_sig;

		void
		init(graphics::root_signature::handle h_root_sig) noexcept;

		inline void
		execute_opaque_aa(const aa_data&	aa_data_cpu,
						  rtv_desc_handle	h_main_buffer_rtv_desc,
						  resource_handle	h_blend_tex,
						  extent_2d<uint16> extent) const noexcept;

		inline void
		execute_transparent_aa(const aa_data&	 aa_data_cpu,
							   rtv_desc_handle	 h_main_buffer_rtv_desc,
							   resource_handle	 h_blend_tex,
							   extent_2d<uint16> extent) const noexcept;
		void
		deinit() noexcept;
	};

	struct transparent_stage
	{
		graphics::pso::handle h_pso_rt_with_aa;
		ID3D12PipelineState*  p_pso_rt_with_aa;

		graphics::pso::handle h_pso_resolve;
		ID3D12PipelineState*  p_pso_resolve;

		graphics::pso::handle h_pso_no_aa;
		ID3D12PipelineState*  p_pso_no_aa;


		void
		init(graphics::root_signature::handle h_root_sig) noexcept;

		inline void
		execute_with_aa(extent_2d<uint16> extent) const noexcept;

		inline void
		execute_without_aa(rtv_desc_handle _) const noexcept;

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
		execute(uint32 raycast_count) const noexcept;

		void
		deinit() noexcept;
	};

	struct bloom_stage
	{
		pso::handle			 h_pso_prefilter = {};
		ID3D12PipelineState* p_pso_prefilter = nullptr;

		pso::handle			 h_pso_downsample = {};
		ID3D12PipelineState* p_pso_downsample = nullptr;

		pso::handle			 h_pso_upsample = {};
		ID3D12PipelineState* p_pso_upsample = nullptr;

		void
		init(graphics::root_signature::handle h_root_sig) noexcept;

		inline void
		execute(binding_config_t::reg_b<1>& constants,
				resource_handle				h_bloom_chain,
				uint16						mip_count,
				const shared_type::bloom&	bloom_gpu) const noexcept;


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
		execute(rtv_desc_handle _) const noexcept;

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
				resource_handle h_outline_mask) const noexcept;

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
				const age::vector<util::range>& z_range_range_vec) const noexcept;

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
		execute(render_surface& rs) const noexcept;

		void
		deinit() noexcept;
	};

	struct debug_stage
	{
		pso::handle			 h_pso_mesh;
		ID3D12PipelineState* p_pso_mesh;

		pso::handle			 h_pso_mesh_always_on_top;
		ID3D12PipelineState* p_pso_mesh_always_on_top;

		void
		init(root_signature::handle h_root_sig) noexcept;

		inline void
		execute(binding_config_t::reg_b<1>& constants,
				rtv_desc_handle				h_desc_rtv,
				dsv_desc_handle				h_desc_dsv,
				bool						is_aot,
				uint32						render_data_count,
				uint32						render_data_offset) const noexcept;

		void
		deinit() noexcept;
	};

	struct hybrid_pipeline
	{
		graphics::root_signature::handle h_root_sig;
		ID3D12RootSignature*			 p_root_sig;

		depth_stage				stage_depth;
		segment_stage			stage_segment;
		ao_stage				stage_ao;
		skybox_stage			stage_skybox;
		light_bin_stage			stage_light_bin;
		ddgi_stage				stage_ddgi;
		gibs_stage				stage_gibs;
		opaque_stage			stage_opaque;
		aa_stage				stage_aa;
		transparent_stage		stage_transparent;
		raycast_stage			stage_raycast;
		bloom_stage				stage_bloom;
		post_process_stage		stage_post_process;
		selection_outline_stage stage_selection_outline;
		ui_stage				stage_ui;
		presentation_stage		stage_presentation;
		debug_stage				stage_debug;

		resource_handle h_main_buffer;
		rtv_desc_handle h_main_buffer_rtv_desc;
		srv_desc_handle h_main_buffer_srv_desc;

		resource_handle h_post_buffer;
		rtv_desc_handle h_post_buffer_rtv_desc;
		srv_desc_handle h_post_buffer_srv_desc;

		resource_handle h_selection_outline_mask_buffer;
		rtv_desc_handle h_selection_outline_mask_buffer_rtv_desc;
		srv_desc_handle h_selection_outline_mask_buffer_srv_desc;

		resource_handle h_debug_depth_buffer;
		dsv_desc_handle h_debug_depth_buffer_dsv_desc;

		srv_desc_handle h_env_light_brdf_lut;

		resource_handle h_opaque_depth_buffer;
		dsv_desc_handle h_opaque_depth_buffer_dsv_readonly_desc;
		dsv_desc_handle h_opaque_depth_buffer_dsv_desc;
		srv_desc_handle h_opaque_depth_buffer_srv_desc;

		resource_handle h_transparent_depth_buffer;
		dsv_desc_handle h_transparent_depth_buffer_dsv_readonly_desc;
		dsv_desc_handle h_transparent_depth_buffer_dsv_desc;
		srv_desc_handle h_transparent_depth_buffer_srv_desc;

		resource_handle		  h_blend_buffer;
		srv_desc_handle		  h_blend_buffer_srv_desc;
		uav_desc_handle		  h_blend_buffer_uav_desc;
		clear_uav_desc_handle h_blend_buffer_clear_uav_desc;

		resource_handle h_opaque_gbuffer;
		rtv_desc_handle h_opaque_gbuffer_rtv_desc;
		srv_desc_handle h_opaque_gbuffer_srv_desc;

		resource_handle h_transparent_gbuffer;
		rtv_desc_handle h_transparent_gbuffer_rtv_desc;
		srv_desc_handle h_transparent_gbuffer_srv_desc;

		resource_handle h_motion_buffer;
		rtv_desc_handle h_motion_buffer_rtv_desc;
		srv_desc_handle h_motion_buffer_srv_desc;

		resource_handle h_scratch_buffer;
		resource_handle h_light_bin_stage_buffer;
		resource_handle h_sorted_light_buffer;
		resource_handle h_indirect_arg_buffer;

		std::array<mapping_handle, global::frame_buffer_count> h_mapping_static_ring_buffer_arr;

		mapping_handle h_mapping_frame_data;
		mapping_handle h_mapping_mesh_buffer;
		mapping_handle h_mapping_rt_index_buffer;
		mapping_handle h_mapping_rt_vertex_scratch_buffer;
		mapping_handle h_mapping_material_buffer;

		std::array<mapping_handle, global::frame_buffer_count> h_mapping_env_light_buffer_arr;


		// rt, not for binding
		age::vector<D3D12_RAYTRACING_INSTANCE_DESC>		  rt_instance_data_vec[global::thread_count];
		age::vector<shared_type::rt_instance_render_data> rt_instance_render_data_vec[global::thread_count];


		std::array<mapping_handle, global::frame_buffer_count> h_mapping_rt_instance_buffer_arr;
		std::array<mapping_handle, global::frame_buffer_count> h_mapping_rt_instance_render_data_buffer_arr;
		std::array<mapping_handle, global::frame_buffer_count> h_mapping_rt_raycast_request_buffer_arr;
		resource_handle										   h_rt_raycast_result_buffer;
		std::array<mapping_handle, global::frame_buffer_count> h_readback_rt_raycast_result_buffer_arr;	   // readback

		resource_handle h_rt_tlas_buffer;
		srv_desc_handle h_rt_tlas_buffer_srv_desc;

		resource_handle h_rt_tlas_scratch_buffer;

		// global
		binding_config_t::reg_b<0, 0> frame_data_buffer;
		binding_config_t::reg_b<1, 0> root_constants;
		binding_config_t::reg_u<2, 0> indirect_arg_buffer_uav;

		binding_config_t::reg_t<0, 0> static_ring_buffer;
		binding_config_t::reg_t<1, 0> mesh_data_buffer;

		binding_config_t::reg_u<0, 0> scratch_buffer_uav;

		// light bin
		binding_config_t::reg_t<0, 1> light_bin_stage_buffer_srv;
		binding_config_t::reg_u<0, 1> light_bin_stage_buffer_uav;
		binding_config_t::reg_t<1, 1> sorted_light_buffer_srv;
		binding_config_t::reg_u<1, 1> sorted_light_buffer_uav;

		// material
		binding_config_t::reg_t<0, 2> material_buffer;

		// env_light
		binding_config_t::reg_t<1, 2> env_light_buffer;

		// selection outline
		binding_config_t::reg_t<0, 4>						   selection_outline_meshlet_render_data_buffer;
		binding_config_t::reg_t<1, 4>						   selection_outline_data_buffer;
		std::array<mapping_handle, global::frame_buffer_count> h_mapping_selection_outline_meshlet_render_data_buffer_arr;
		std::array<mapping_handle, global::frame_buffer_count> h_mapping_selection_outline_data_buffer_arr;

		// ui
		binding_config_t::reg_t<0, 5>						   ui_root_data_buffer;
		binding_config_t::reg_t<1, 5>						   ui_data_buffer;
		std::array<mapping_handle, global::frame_buffer_count> h_mapping_ui_root_data_buffer_arr;
		std::array<mapping_handle, global::frame_buffer_count> h_mapping_ui_data_buffer_arr;

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

		// debug
		binding_config_t::reg_t<0, 77>						   debug_meshlet_render_data_buffer;
		binding_config_t::reg_t<1, 77>						   debug_object_data_buffer;
		std::array<mapping_handle, global::frame_buffer_count> h_mapping_debug_meshlet_render_data_buffer_arr;
		std::array<mapping_handle, global::frame_buffer_count> h_mapping_debug_object_data_buffer_arr;

		// details
		extent_2d<uint16> extent{ .width = 100, .height = 100 };

		// external textures
		age::unordered_map<t_texture_id, texture_data> texture_map;

		// mesh
		age::sparse_vector<mesh_data> mesh_data_vec;

		age::offset_pool<> mesh_persistant_buffer_offset_pool;
		age::offset_pool<> mesh_rt_index_buffer_offset_pool;

		age::stable_dense_vector<shared_type::object_data> object_data_vec;
		// todo, save memory
		age::stable_dense_vector<shared_type::object_data> object_prev_data_vec;
		age::vector<BARE_OF(object_data_vec)::index_type>  object_pos_to_id_arr[global::frame_buffer_count];
		age::vector<uint8>								   object_generation_vec;

		// material
		age::sparse_vector<asset::handle> material_vec;

		// camera
		age::sparse_vector<camera_desc> camera_desc_vec;
		age::sparse_vector<camera_data> camera_data_vec;
		float4x4						main_cam_view_proj_prev;
		float4x4						main_cam_view_proj_inv_prev;
		t_camera_id						main_camera_id;

		// bloom
		t_bloom_id active_bloom_id;

		age::sparse_vector<bloom_desc> bloom_desc_vec;
		shared_type::bloom			   bloom_gpu{ .width = static_cast<uint16>(extent.width / 2u), .height = static_cast<uint16>(extent.height / 2u) };
		uint16						   bloom_mip_count = math::calc_mip_count(bloom_gpu.width, bloom_gpu.height);
		uint16						   _;
		resource_handle				   h_bloom_chain;
		srv_desc_handle				   h_bloom_srv_desc;
		uav_desc_handle				   h_bloom_mip_uav_desc_arr[g::max_bloom_mip_count];

		// ddgi
		binding_config_t::reg_t<1, 7> ddgi_probe_buffer_srv;
		binding_config_t::reg_u<1, 7> ddgi_probe_buffer_uav;
		binding_config_t::reg_u<2, 7> ddgi_scratch_buffer;
		ddgi_data					  ddgi_data_cpu;

		std::mt19937						  ddgi_rng{ std::random_device{}() };
		std::uniform_real_distribution<float> ddgi_dist{ 0.f, 1.f };

		// gibs
		gibs_data gibs_data_cpu;

		// ao
		ao_data ao_data_cpu;

		// segment
		segment_data segment_data_cpu;

		// aa
		aa_data aa_data_cpu;

		// object & render_data
		age::stable_dense_vector<float3x4> object_transform_data_vec;

		age::vector<shared_type::opaque_meshlet_render_data>	  opaque_meshlet_render_data_vec[global::thread_count];
		age::vector<shared_type::transparent_meshlet_render_data> transparent_meshlet_render_data_vec[global::thread_count];

		// light

		age::stable_dense_vector<shared_type::directional_light> directional_light_vec;

		age::stable_dense_vector<shared_type::unified_light> unified_light_vec;

		// env_light
		age::stable_dense_vector<shared_type::env_light> env_light_gpu_data_vec;
		age::sparse_vector<env_light_data>				 env_light_cpu_data_vec;

		// raycast
		age::vector<shared_type::raycast_result>  raycast_result_vec[global::frame_buffer_count];
		age::vector<shared_type::raycast_request> raycast_request_vec;

		// selection_outline
		age::vector<shared_type::selection_outline_data>				selection_outline_data_vec;
		age::vector<shared_type::selection_outline_meshlet_render_data> selection_outline_meshlet_render_data_vec;

		// debug & immediate & ui
		age::vector<shared_type::debug_object_data>			debug_object_data_vec;
		age::vector<t_object_id>							debug_object_id_vec;
		age::vector<shared_type::debug_meshlet_render_data> debug_meshlet_render_data_vec;
		age::vector<shared_type::debug_meshlet_render_data> debug_aot_meshlet_render_data_vec;

		resource_handle										   h_debug_assert_buffer;
		clear_uav_desc_handle								   h_debug_assert_buffer_clear_uav_desc;
		binding_config_t::reg_u<666, 666>					   debug_assert_buffer_uav;
		std::array<mapping_handle, global::frame_buffer_count> h_readback_debug_assert_buffer_arr;

		byte_buf shader_debug_assert_result_buf_arr[global::frame_buffer_count];

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

		shared_type::object_data
		get_object_data(t_object_id id) noexcept;

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
		request_raycast(const float3& origin, const float3& dir, float max_t, e::rt_mask_kind mask = e::rt_mask_kind::all ^ e::rt_mask_kind::always_on_top) noexcept;

		shared_type::raycast_result
		get_raycast_result(t_raycast_id _) noexcept;

		// debug
		t_object_id
		render_debug_mesh(const float3& pos, const float4& quat, const float3& scale, asset::handle h_mesh, const float3& color, bool draw = true, bool enable_raycast = true) noexcept;

		t_object_id
		render_debug_mesh_aot(const float3& pos, const float4& quat, const float3& scale, asset::handle h_mesh, const float3& color, bool draw = true, bool enable_raycast = true) noexcept;

		void
		render_debug_line() noexcept;

		// bloom
		t_bloom_id
		add_bloom(const bloom_desc& desc, bool set_active = false) noexcept;

		void
		update_bloom(t_bloom_id id, const bloom_desc& desc) noexcept;

		void
		set_bloom_active(t_bloom_id id, bool active) noexcept;

		void
		remove_bloom(t_bloom_id& id) noexcept;

		// ddgi
		void
		enable_ddgi(const ddgi_desc& _) noexcept;

		void
		disable_ddgi() noexcept;

		void
		update_ddgi(const ddgi_desc& _) noexcept;

		void
		update_ddgi_debug_flags(graphics::e::ddgi_debug_flags e) noexcept;

		bool
		ddgi_enabled() const noexcept;

		// ddgi editor
		void
		clear_ddgi() noexcept;

		float
		get_ddgi_convergence() noexcept;

		// gibs
		void
		enable_gibs(const gibs_desc& _) noexcept;

		void
		disable_gibs() noexcept;

		void
		update_gibs(const gibs_desc& _) noexcept;

		void
		update_gibs_debug_flags(graphics::e::gibs_debug_flags e) noexcept;

		bool
		gibs_enabled() const noexcept;

		uint32
		gibs_max_surfel_count() const noexcept;

		// ao
		void
		enable_ao(const ao_desc&) noexcept;

		void
		update_ao(const ao_desc&) noexcept;

		void
		disable_ao() noexcept;

		bool
		ao_enabled() const noexcept;

		// aa
		void
		enable_aa(const aa_desc&) noexcept;

		void
		disable_aa() noexcept;

		void
		update_aa(const aa_desc&) noexcept;

		bool
		aa_enabled() const noexcept;

	  private:
		void
		create_resolution_dependent_buffers() noexcept;

		void
		resize_resolution_dependent_buffers(const age::extent_2d<uint16>& new_extent) noexcept;

		std::tuple<uint32, uint32>
		upload_data() noexcept;
	};
}	 // namespace age::graphics::render_pipeline
#pragma once
#include "age.hpp"

namespace age::graphics::render_pipeline::forward_plus
{
	struct init_stage
	{
		graphics::pso::handle h_pso = {};
		ID3D12PipelineState*  p_pso = nullptr;

		inline void
		init(graphics::root_signature::handle h_root_sig) noexcept
		{
			using namespace graphics::pso;

			h_pso = graphics::pso::create(
				pss_root_signature{ .subobj = graphics::g::root_signature_ptr_vec[h_root_sig] },
				pss_cs{ .subobj = shader::get_d3d12_bytecode(shader::shader_handle{ std::to_underlying(shader::e::engine_shader_kind::forward_plus_init_cs) }) });

			p_pso = graphics::g::pso_ptr_vec[h_pso];
		}

		inline void
		execute(t_cmd_list& cmd_list, uint32 tile_total_uint32_count) noexcept
		{
			{
				cmd_list.SetPipelineState(p_pso);

				cmd_list.Dispatch((tile_total_uint32_count + 255) / 256, 1, 1);
			}
		}

		inline void
		deinit() noexcept
		{
			pso::destroy(h_pso);
		}
	};

	struct depth_stage
	{
		dsv_desc_handle h_depth_buffer_dsv_desc;

		graphics::pso::handle h_pso = {};
		ID3D12PipelineState*  p_pso = nullptr;

		inline void
		init(graphics::root_signature::handle h_root_sig) noexcept
		{
			using namespace graphics::pso;

			auto as_byte_code = shader::get_d3d12_bytecode(shader::shader_handle{ std::to_underlying(shader::e::engine_shader_kind::forward_plus_opaque_as) });
			auto ms_byte_code = shader::get_d3d12_bytecode(shader::shader_handle{ std::to_underlying(shader::e::engine_shader_kind::forward_plus_depth_ms) });

			h_pso = graphics::pso::create(
				pss_root_signature{ .subobj = graphics::g::root_signature_ptr_vec[h_root_sig] },
				pss_as{ .subobj = as_byte_code },
				pss_ms{ .subobj = ms_byte_code },
				// no PS
				pss_primitive_topology{ .subobj = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE },
				pss_render_target_formats{ .subobj = D3D12_RT_FORMAT_ARRAY{ .RTFormats{}, .NumRenderTargets = 0 } },
				pss_depth_stencil_format{ .subobj = DXGI_FORMAT_D32_FLOAT },
				pss_rasterizer{ .subobj = defaults::rasterizer_desc::backface_cull },
				pss_depth_stencil1{ .subobj = defaults::depth_stencil_desc1::depth_only_reversed },
				pss_sample_desc{ .subobj = DXGI_SAMPLE_DESC{ .Count = 1, .Quality = 0 } },
				pss_node_mask{ .subobj = 0 });

			p_pso = graphics::g::pso_ptr_vec[h_pso];

			h_depth_buffer_dsv_desc = graphics::g::dsv_desc_pool.pop();
		}

		inline void
		bind_dsv(graphics::resource_handle h_depth_buffer) noexcept
		{
			resource::create_view(h_depth_buffer,
								  h_depth_buffer_dsv_desc,
								  defaults::dsv_view_desc::d32_float_2d);
		}

		inline void
		execute(t_cmd_list& cmd_list, uint32 job_count) noexcept
		{
			c_auto render_pass_ds_desc = defaults::render_pass_ds_desc::depth_clear_preserve(h_depth_buffer_dsv_desc, 0.f);

			cmd_list.BeginRenderPass(
				0,			// no render targets
				nullptr,	// no render targets
				&render_pass_ds_desc,
				D3D12_RENDER_PASS_FLAG_NONE);

			{
				cmd_list.SetPipelineState(p_pso);

				if (job_count > 0) [[likely]]
				{
					cmd_list.DispatchMesh((job_count + 31u) / 32u, 1, 1);
				}
			}

			cmd_list.EndRenderPass();
		}

		inline void
		deinit() noexcept
		{
			graphics::g::dsv_desc_pool.push(h_depth_buffer_dsv_desc);
			pso::destroy(h_pso);
		}
	};

	struct shadow_stage
	{
		graphics::pso::handle h_pso;
		ID3D12PipelineState*  p_pso;

		dsv_desc_handle h_shadow_atlas_dsv_desc;

		inline void
		init(graphics::root_signature::handle h_root_sig) noexcept
		{
			using namespace graphics::pso;

			h_pso = graphics::pso::create(
				pss_root_signature{ .subobj = graphics::g::root_signature_ptr_vec[h_root_sig] },
				pss_as{ .subobj = shader::get_d3d12_bytecode(shader::shader_handle{ std::to_underlying(shader::e::engine_shader_kind::forward_plus_shadow_as) }) },
				pss_ms{ .subobj = shader::get_d3d12_bytecode(shader::shader_handle{ std::to_underlying(shader::e::engine_shader_kind::forward_plus_shadow_ms) }) },
				// no PS
				pss_primitive_topology{ .subobj = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE },
				pss_render_target_formats{ .subobj = D3D12_RT_FORMAT_ARRAY{ .RTFormats{}, .NumRenderTargets = 0 } },
				pss_depth_stencil_format{ .subobj = DXGI_FORMAT_D32_FLOAT },
				pss_rasterizer{ .subobj = defaults::rasterizer_desc::shadow_depth_bias(-g::shadow_depth_bias, -g::shadow_slope_bias) },
				pss_depth_stencil1{ .subobj = defaults::depth_stencil_desc1::depth_only_reversed },
				pss_sample_desc{ .subobj = DXGI_SAMPLE_DESC{ .Count = 1, .Quality = 0 } },
				pss_node_mask{ .subobj = 0 });

			p_pso = graphics::g::pso_ptr_vec[h_pso];

			h_shadow_atlas_dsv_desc = graphics::g::dsv_desc_pool.pop();
		}

		inline void
		bind_dsv(graphics::resource_handle h_shadow_atlas) noexcept
		{
			resource::create_view(h_shadow_atlas,
								  h_shadow_atlas_dsv_desc,
								  defaults::dsv_view_desc::d32_float_2d);
		}

		inline void
		execute(t_cmd_list& cmd_list, const data_structure::stable_dense_vector<shadow_light_header>& shadow_light_header_vec, uint32 job_count) noexcept
		{
			if (job_count == 0) [[unlikely]]
			{
				return;
			}


			c_auto render_pass_ds_desc = defaults::render_pass_ds_desc::depth_clear_preserve(h_shadow_atlas_dsv_desc, 0.f);

			cmd_list.BeginRenderPass(
				0,			// no render targets
				nullptr,	// no render targets
				&render_pass_ds_desc,
				D3D12_RENDER_PASS_FLAG_NONE);

			cmd_list.SetPipelineState(p_pso);

			for (auto&& [i, header] : shadow_light_header_vec | std::views::enumerate)
			{
				uint32 slice_index = header.slice_index;

				c_auto col	   = slice_index % g::shadow_atlas_seg_u;
				c_auto row	   = slice_index / g::shadow_atlas_seg_u;
				c_auto atlas_x = col * g::shadow_map_width;
				c_auto atlas_y = row * g::shadow_map_height;

				c_auto viewport = D3D12_VIEWPORT{
					(float)atlas_x, (float)atlas_y,
					(float)g::shadow_map_width, (float)g::shadow_map_height,
					0.0f, 1.0f
				};
				c_auto scissor = D3D12_RECT{
					(LONG)atlas_x,
					(LONG)atlas_y,
					(LONG)(atlas_x + g::shadow_map_width),
					(LONG)(atlas_y + g::shadow_map_height)
				};

				cmd_list.RSSetViewports(1, &viewport);
				cmd_list.RSSetScissorRects(1, &scissor);

				cmd_list.SetGraphicsRoot32BitConstants(
					binding_config_t::reg_b<1>::slot_id,
					sizeof(uint32) / 4,
					&slice_index,
					offsetof(shared_type::root_constants, shadow_light_index) / 4);

				if (job_count > 0) [[likely]]
				{
					cmd_list.DispatchMesh((job_count + 31u) / 32u, 1, 1);
				}
			}

			cmd_list.EndRenderPass();
		}

		inline void
		deinit() noexcept
		{
			pso::destroy(h_pso);

			graphics::g::dsv_desc_pool.push(h_shadow_atlas_dsv_desc);
		}
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
		init(graphics::root_signature::handle h_root_sig) noexcept
		{
			using namespace graphics::pso;

			h_pso_cull = graphics::pso::create(
				pss_root_signature{ .subobj = graphics::g::root_signature_ptr_vec[h_root_sig] },
				pss_cs{ .subobj = shader::get_d3d12_bytecode(shader::shader_handle{ std::to_underlying(shader::e::engine_shader_kind::forward_plus_light_cull_cs) }) });

			p_pso_cull = graphics::g::pso_ptr_vec[h_pso_cull];

			h_pso_sort_histogram = graphics::pso::create(
				pss_root_signature{ .subobj = graphics::g::root_signature_ptr_vec[h_root_sig] },
				pss_cs{ .subobj = shader::get_d3d12_bytecode(shader::shader_handle{ std::to_underlying(shader::e::engine_shader_kind::forward_plus_light_sort_histogram_cs) }) });

			p_pso_sort_histogram = graphics::g::pso_ptr_vec[h_pso_sort_histogram];

			h_pso_sort_prefix = graphics::pso::create(
				pss_root_signature{ .subobj = graphics::g::root_signature_ptr_vec[h_root_sig] },
				pss_cs{ .subobj = shader::get_d3d12_bytecode(shader::shader_handle{ std::to_underlying(shader::e::engine_shader_kind::forward_plus_light_sort_prefix_cs) }) });

			p_pso_sort_prefix = graphics::g::pso_ptr_vec[h_pso_sort_prefix];

			h_pso_sort_scatter = graphics::pso::create(
				pss_root_signature{ .subobj = graphics::g::root_signature_ptr_vec[h_root_sig] },
				pss_cs{ .subobj = shader::get_d3d12_bytecode(shader::shader_handle{ std::to_underlying(shader::e::engine_shader_kind::forward_plus_light_sort_scatter_cs) }) });

			p_pso_sort_scatter = graphics::g::pso_ptr_vec[h_pso_sort_scatter];

			h_pso_zbin = graphics::pso::create(
				pss_root_signature{ .subobj = graphics::g::root_signature_ptr_vec[h_root_sig] },
				pss_cs{ .subobj = shader::get_d3d12_bytecode(shader::shader_handle{ std::to_underlying(shader::e::engine_shader_kind::forward_plus_light_zbin_cs) }) });

			p_pso_zbin = graphics::g::pso_ptr_vec[h_pso_zbin];

			h_pso_tile = graphics::pso::create(
				pss_root_signature{ .subobj = graphics::g::root_signature_ptr_vec[h_root_sig] },
				pss_cs{ .subobj = shader::get_d3d12_bytecode(shader::shader_handle{ std::to_underlying(shader::e::engine_shader_kind::forward_plus_light_tile_cs) }) });

			p_pso_tile = graphics::g::pso_ptr_vec[h_pso_tile];
		}

		inline void
		execute(t_cmd_list&					cmd_list,
				graphics::resource_barrier& barrier,
				uint32						light_tile_count_x,
				uint32						light_tile_count_y,
				ID3D12Resource&				frame_data_rw_buffer,
				ID3D12Resource&				sort_buffer,
				ID3D12Resource&				zbin_buffer,
				ID3D12Resource&				tile_mask_buffer,
				ID3D12Resource&				unified_sorted_light_buffer,
				uint32						light_count) noexcept
		{
			if (light_count > 0)
			{
				cmd_list.SetPipelineState(p_pso_cull);
				cmd_list.Dispatch((g::max_light_count + g::light_cull_cs_thread_count - 1) / g::light_cull_cs_thread_count, 1, 1);
				barrier.add_uav(sort_buffer);
				barrier.apply_and_reset(cmd_list);

				for (uint32 pass : std::views::iota(0u) | std::views::take(g::light_sort_iteration_count))
				{
					cmd_list.SetComputeRoot32BitConstants(
						binding_config_t::reg_b<1>::slot_id,
						sizeof(uint32) / 4,
						&pass,
						offsetof(shared_type::root_constants, light_radix_sort_pass) / 4);

					cmd_list.SetPipelineState(p_pso_sort_histogram);
					cmd_list.Dispatch(g::light_sort_group_count, 1, 1);
					barrier.add_uav(sort_buffer);
					barrier.apply_and_reset(cmd_list);

					cmd_list.SetPipelineState(p_pso_sort_prefix);
					cmd_list.Dispatch(g::light_sort_bin_count, 1, 1);
					barrier.add_uav(sort_buffer);
					barrier.apply_and_reset(cmd_list);

					cmd_list.SetPipelineState(p_pso_sort_scatter);
					cmd_list.Dispatch(g::light_sort_group_count, 1, 1);
					barrier.add_uav(sort_buffer);
					barrier.apply_and_reset(cmd_list);
				}

				cmd_list.SetPipelineState(p_pso_zbin);
				cmd_list.Dispatch((g::max_visible_light_count + g::zbin_thread_count - 1) / g::zbin_thread_count, 1, 1);

				barrier.add_uav(zbin_buffer);
				barrier.add_uav(sort_buffer);
				barrier.add_uav(frame_data_rw_buffer);
				barrier.apply_and_reset(cmd_list);

				cmd_list.SetPipelineState(p_pso_tile);
				cmd_list.Dispatch(light_tile_count_x, light_tile_count_y, 1);
				barrier.add_uav(tile_mask_buffer);
				barrier.add_uav(unified_sorted_light_buffer);
				barrier.apply_and_reset(cmd_list);
			}
		}

		inline void
		deinit() noexcept
		{
			pso::destroy(h_pso_cull);

			pso::destroy(h_pso_sort_histogram);
			pso::destroy(h_pso_sort_prefix);
			pso::destroy(h_pso_sort_scatter);

			pso::destroy(h_pso_zbin);
			pso::destroy(h_pso_tile);
		}
	};

	struct opaque_stage
	{
		rtv_desc_handle h_main_buffer_rtv_desc;
		dsv_desc_handle h_depth_buffer_dsv_desc;

		graphics::pso::handle h_pso = {};
		ID3D12PipelineState*  p_pso = nullptr;

		inline void
		init(graphics::root_signature::handle h_root_sig) noexcept
		{
			using namespace graphics::pso;

			auto as_byte_code = shader::get_d3d12_bytecode(shader::shader_handle{ std::to_underlying(shader::e::engine_shader_kind::forward_plus_opaque_as) });
			auto ms_byte_code = shader::get_d3d12_bytecode(shader::shader_handle{ std::to_underlying(shader::e::engine_shader_kind::forward_plus_opaque_ms) });
			auto ps_byte_code = shader::get_d3d12_bytecode(shader::shader_handle{ std::to_underlying(shader::e::engine_shader_kind::forward_plus_opaque_ps) });

			h_pso = graphics::pso::create(
				pss_root_signature{ .subobj = graphics::g::root_signature_ptr_vec[h_root_sig] },
				pss_as{ .subobj = as_byte_code },
				pss_ms{ .subobj = ms_byte_code },
				pss_ps{ .subobj = ps_byte_code },
				pss_primitive_topology{ .subobj = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE },
				pss_render_target_formats{ .subobj = D3D12_RT_FORMAT_ARRAY{ .RTFormats{ DXGI_FORMAT_R16G16B16A16_FLOAT }, .NumRenderTargets = 1 } },
				pss_depth_stencil_format{ .subobj = DXGI_FORMAT_D32_FLOAT },
				pss_rasterizer{ .subobj = defaults::rasterizer_desc::backface_cull },
				pss_depth_stencil1{ .subobj = defaults::depth_stencil_desc1::depth_equal_readonly_reversed },
				pss_blend{ .subobj = defaults::blend_desc::opaque },
				pss_sample_desc{ .subobj = DXGI_SAMPLE_DESC{ .Count = 1, .Quality = 0 } },
				pss_node_mask{ .subobj = 0 });

			p_pso = graphics::g::pso_ptr_vec[h_pso];

			h_main_buffer_rtv_desc	= graphics::g::rtv_desc_pool.pop();
			h_depth_buffer_dsv_desc = graphics::g::dsv_desc_pool.pop();
		}

		inline void
		bind_rtv_dsv(graphics::resource_handle h_main_buffer,
					 graphics::resource_handle h_depth_buffer) noexcept
		{
			resource::create_view(h_main_buffer,
								  h_main_buffer_rtv_desc,
								  defaults::rtv_view_desc::hdr_rgba16_2d);

			resource::create_view(h_depth_buffer,
								  h_depth_buffer_dsv_desc,
								  defaults::dsv_view_desc::d32_float_2d_readonly);
		}

		inline void
		execute(t_cmd_list& cmd_list, uint32 job_count) noexcept
		{
			auto render_pass_rt_desc = defaults::render_pass_rtv_desc::clear_preserve(h_main_buffer_rtv_desc, nullptr);
			auto render_pass_ds_desc = defaults::render_pass_ds_desc::depth_load_preserve(h_depth_buffer_dsv_desc);

			cmd_list.BeginRenderPass(
				1,
				&render_pass_rt_desc,
				&render_pass_ds_desc,
				D3D12_RENDER_PASS_FLAG_BIND_READ_ONLY_DEPTH);

			{
				cmd_list.SetPipelineState(p_pso);

				if (job_count > 0) [[likely]]
				{
					cmd_list.DispatchMesh((job_count + 31u) / 32u, 1, 1);
				}
			}

			cmd_list.EndRenderPass();
		}

		inline void
		deinit() noexcept
		{
			graphics::g::rtv_desc_pool.push(h_main_buffer_rtv_desc);
			graphics::g::dsv_desc_pool.push(h_depth_buffer_dsv_desc);

			pso::destroy(h_pso);
		}
	};

	struct presentation_stage
	{
		pso::handle			 h_pso = {};
		ID3D12PipelineState* p_pso = nullptr;

		inline void
		init(root_signature::handle h_root_sig) noexcept
		{
			using namespace graphics::pso;

			auto ms_byte_code = shader::get_d3d12_bytecode(shader::shader_handle{ std::to_underlying(shader::e::engine_shader_kind::forward_plus_presentation_ms) });

			auto&& [ps_byte_code, back_buffer_rt_format] = [&]() {
				auto space = global::get<graphics::interface>().display_color_space();
				switch (space)
				{
				case color_space::srgb:
					return std::tuple{
						shader::get_d3d12_bytecode(shader::shader_handle{ std::to_underlying(shader::e::engine_shader_kind::forward_plus_presentation_sdr_ps) }),
						DXGI_FORMAT_R8G8B8A8_UNORM_SRGB

					};
				case color_space::hdr:
					return std::tuple{
						shader::get_d3d12_bytecode(shader::shader_handle{ std::to_underlying(shader::e::engine_shader_kind::forward_plus_presentation_hdr10_ps) }),
						DXGI_FORMAT_R10G10B10A2_UNORM
					};
				default:
					AGE_UNREACHABLE("invalid color space");
				}
			}();

			h_pso = graphics::pso::create(
				pss_root_signature{ .subobj = graphics::g::root_signature_ptr_vec[h_root_sig] },
				pss_ms{ .subobj = ms_byte_code },
				pss_ps{ .subobj = ps_byte_code },
				pss_primitive_topology{ .subobj = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE },
				pss_render_target_formats{ .subobj = D3D12_RT_FORMAT_ARRAY{ .RTFormats{ back_buffer_rt_format }, .NumRenderTargets = 1 } },
				pss_depth_stencil_format{ .subobj = DXGI_FORMAT_UNKNOWN },
				pss_rasterizer{ .subobj = defaults::rasterizer_desc::no_cull },
				pss_depth_stencil1{ .subobj = defaults::depth_stencil_desc1::disabled },
				pss_sample_desc{ .subobj = DXGI_SAMPLE_DESC{ .Count = 1, .Quality = 0 } },
				pss_node_mask{ .subobj = 0 });

			p_pso = graphics::g::pso_ptr_vec[h_pso];
		}

		inline void
		deinit() noexcept
		{
			pso::destroy(h_pso);
		}

		inline void
		execute(t_cmd_list& cmd_list, render_surface& rs) noexcept
		{
			auto render_pass_rt_desc = defaults::render_pass_rtv_desc::overwrite_preserve(rs.h_rtv_desc());

			cmd_list.BeginRenderPass(
				1,
				&render_pass_rt_desc,
				nullptr,
				D3D12_RENDER_PASS_FLAG_NONE);

			{
				cmd_list.SetPipelineState(p_pso);
				cmd_list.DispatchMesh(1, 1, 1);
			}

			cmd_list.EndRenderPass();
		}
	};

	struct pipeline
	{
		extent_2d<uint16> extent{ .width = 100, .height = 100 };

		uint32 light_tile_count_x = (extent.width + g::light_tile_size - 1) / g::light_tile_size;
		uint32 light_tile_count_y = (extent.height + g::light_tile_size - 1) / g::light_tile_size;

		resource_handle h_main_buffer;
		resource_handle h_depth_buffer;
		resource_handle h_light_sort_buffer;
		resource_handle h_zbin_buffer;
		resource_handle h_tile_mask_buffer;
		resource_handle h_shadow_atlas;
		resource_handle h_unified_sorted_light_buffer;


		ID3D12Resource* p_main_buffer;
		ID3D12Resource* p_depth_buffer;
		ID3D12Resource* p_shadow_atlas;
		ID3D12Resource* p_light_sort_buffer;
		ID3D12Resource* p_zbin_buffer;
		ID3D12Resource* p_tile_mask_buffer;
		ID3D12Resource* p_unified_sorted_light_buffer;

		graphics::root_signature::handle h_root_sig;
		ID3D12RootSignature*			 p_root_sig;

		init_stage			stage_init;
		depth_stage			stage_depth;
		shadow_stage		stage_shadow;
		light_culling_stage stage_light_culling;
		opaque_stage		stage_opaque;
		presentation_stage	stage_presentation;

		binding_config_t::reg_b<0> frame_data_buffer;
		binding_config_t::reg_b<1> root_constants;
		binding_config_t::reg_t<0> job_data_buffer;
		binding_config_t::reg_t<1> object_data_buffer;
		binding_config_t::reg_t<2> mesh_data_buffer;

		binding_config_t::reg_t<3> directional_light_buffer;

		binding_config_t::reg_t<6> unified_light_buffer;

		binding_config_t::reg_u<2> frame_data_rw_buffer;

		binding_config_t::reg_u<0, 3> light_sort_buffer_uav;
		binding_config_t::reg_t<0, 3> light_sort_buffer_srv;
		binding_config_t::reg_u<1, 3> zbin_buffer_uav;
		binding_config_t::reg_t<1, 3> zbin_buffer_srv;

		binding_config_t::reg_u<2, 3> tile_mask_buffer_uav;
		binding_config_t::reg_t<2, 3> tile_mask_buffer_srv;

		binding_config_t::reg_u<3, 3> unified_sorted_light_buffer_uav;
		binding_config_t::reg_t<3, 3> unified_sorted_light_buffer_srv;

		binding_config_t::reg_t<4, 3> shadow_light_buffer;

		binding_config_t::reg_u<7, 7> debug_buffer_uav;

		resource::mapping_handle h_mapping_frame_data;
		resource::mapping_handle h_mapping_job_data_buffer;
		resource::mapping_handle h_mapping_object_data_buffer;
		resource::mapping_handle h_mapping_mesh_buffer;
		resource::mapping_handle h_mapping_directional_light_buffer;
		resource::mapping_handle h_mapping_unified_light_buffer;
		resource::mapping_handle h_mapping_frame_data_rw_buffer;
		resource::mapping_handle h_mapping_shadow_light_buffer;
		resource::mapping_handle h_mapping_debug_buffer_uav;

		// bindless texture
		srv_desc_handle h_main_buffer_srv_desc;
		srv_desc_handle h_depth_buffer_srv_desc;

		srv_desc_handle h_shadow_atlas_srv_desc;

		resource_barrier barrier;

		data_structure::stable_dense_vector<shared_type::object_data> object_data_vec;

		// for contents updating camera
		data_structure::sparse_vector<camera_desc> camera_desc_vec;
		data_structure::sparse_vector<camera_data> camera_data_vec;

		data_structure::sparse_vector<mesh_data> mesh_data_vec;
		uint32									 mesh_byte_offset = 0;

		shared_type::job_data job_array[graphics::g::frame_buffer_count][graphics::g::thread_count][max_job_count_per_thread];
		uint32				  job_count_array[graphics::g::frame_buffer_count][graphics::g::thread_count];

		data_structure::stable_dense_vector<shared_type::directional_light> directional_light_vec;

		data_structure::stable_dense_vector<shared_type::unified_light> unified_light_vec;

		data_structure::stable_dense_vector<shadow_light_header> shadow_light_header_vec;

		std::array<shared_type::shadow_light, g::max_shadow_light_count> shadow_light_arr;

		std::array<float, g::directional_shadow_cascade_count + 1> cascade_splits;

		t_mesh_id
		upload_mesh(const asset::mesh_baked& baked) noexcept
		{
			AGE_ASSERT(baked.buffer.byte_size() < std::numeric_limits<uint32>::max() - mesh_byte_offset);
			AGE_ASSERT(baked.buffer.byte_size() % 4 == 0);

			auto id = static_cast<t_mesh_id>(mesh_data_vec.emplace_back(
				mesh_data{
					.id			   = static_cast<t_mesh_id>(mesh_data_vec.size()),
					.offset		   = mesh_byte_offset,
					.byte_size	   = baked.buffer.byte_size<uint32>(),
					.meshlet_count = baked.get_header().meshlet_count }));

			std::memcpy(h_mapping_mesh_buffer->ptr + mesh_byte_offset, baked.buffer.data(), baked.buffer.byte_size());

			mesh_byte_offset += baked.buffer.byte_size<uint32>();

			return id;
		}

		void
		release_mesh(t_mesh_id id) noexcept
		{
			mesh_data_vec.remove(id);
		}

	  private:
		template <bool reversed = false>
		static FORCE_INLINE camera_data
		calc_camera_data(const camera_desc& desc) noexcept
		{
			auto&& [xm_pos, xm_quat] = simd::load(desc.pos, desc.quaternion);
			c_auto xm_forward		 = simd::g::xm_forward_f4 | simd::rotate3(xm_quat);
			c_auto xm_up			 = simd::g::xm_up_f4 | simd::rotate3(xm_quat);

			c_auto xm_view = simd::view_look_to(xm_pos, xm_forward, xm_up);

			auto xm_proj = xm_mat{};

			if constexpr (reversed)
			{
				xm_proj = (desc.kind == e::camera_kind::perspective)
							? simd::proj_perspective_fov_reversed(desc.perspective.fov_y, desc.perspective.aspect_ratio, desc.near_z, desc.far_z)
							: simd::proj_orthographic_reversed(desc.orthographic.view_width, desc.orthographic.view_height, desc.near_z, desc.far_z);
			}
			else
			{
				xm_proj = (desc.kind == e::camera_kind::perspective)
							? simd::proj_perspective_fov(desc.perspective.fov_y, desc.perspective.aspect_ratio, desc.near_z, desc.far_z)
							: simd::proj_orthographic(desc.orthographic.view_width, desc.orthographic.view_height, desc.near_z, desc.far_z);
			}

			c_auto xm_view_proj		= xm_proj * xm_view;
			c_auto xm_view_proj_inv = xm_view_proj | simd::mat_inv();

			c_auto frustum_plane_arr = std::array{
				xm_view_proj.r[3] | simd::add(xm_view_proj.r[0]) | simd::plane_normalize() | simd::to<float4>(),	// left
				xm_view_proj.r[3] | simd::sub(xm_view_proj.r[0]) | simd::plane_normalize() | simd::to<float4>(),	// right
				xm_view_proj.r[3] | simd::sub(xm_view_proj.r[1]) | simd::plane_normalize() | simd::to<float4>(),	// top
				xm_view_proj.r[3] | simd::add(xm_view_proj.r[1]) | simd::plane_normalize() | simd::to<float4>(),	// bottom
				xm_view_proj.r[2] | simd::plane_normalize() | simd::to<float4>(),									// near
				xm_view_proj.r[3] | simd::sub(xm_view_proj.r[2]) | simd::plane_normalize() | simd::to<float4>(),	// far
			};

			if (desc.kind == e::camera_kind::perspective)
			{
				return camera_data{
					.pos			   = desc.pos,
					.forward		   = xm_forward | simd::to<float3>(),
					.right			   = simd::g::xm_right_f4 | simd::rotate3(xm_quat) | simd::to<float3>(),
					.view_proj		   = xm_view_proj | simd::to<float4x4>(),
					.view_proj_inv	   = xm_view_proj_inv | simd::to<float4x4>(),
					.frustum_plane_arr = frustum_plane_arr
				};
			}
			else
			{
				AGE_ASSERT(false, "orthographic camera is not supported yet");
				return {};
			}
		}

	  public:
		t_camera_id
		add_camera(const camera_desc& desc) noexcept
		{
			c_auto desc_id = camera_desc_vec.emplace_back(desc);
			c_auto data_id = camera_data_vec.emplace_back(calc_camera_data<true>(desc));

			AGE_ASSERT(desc_id == data_id);

			return static_cast<t_camera_id>(data_id);
		}

		camera_desc
		get_camera_desc(t_camera_id id) noexcept
		{
			return camera_desc_vec[id];
		}

		void
		update_camera(t_camera_id id, const camera_desc& desc) noexcept
		{
			camera_data_vec[id] = calc_camera_data<true>(desc);
			camera_desc_vec[id] = desc;

			for (auto&& [i, split] : cascade_splits | std::views::enumerate)
			{
				const float t		  = static_cast<float>(i) / g::directional_shadow_cascade_count;
				const float safe_near = std::max(desc.near_z, 0.1f);
				const float log_t	  = safe_near * std::pow(desc.far_z / safe_near, t);
				split				  = log_t * 0.5f + (safe_near + t * (desc.far_z - safe_near)) * 0.5f;
			}

			cascade_splits[0] = desc.near_z;
		}

		void
		remove_camera(t_camera_id& id) noexcept
		{
			camera_desc_vec.remove(id);
			camera_data_vec.remove(id);
			id = invalid_id_uint32;
		}

		void
		update_directional_light(t_directional_light_id id, const directional_light_desc& desc) noexcept
		{
			auto& light		= directional_light_vec[id];
			light.direction = desc.direction;
			light.intensity = desc.intensity;
			light.color		= desc.color;
		}

		t_directional_light_id
		add_directional_light(const directional_light_desc& desc, bool cast_shadow = true) noexcept
		{
			t_unified_light_id id = static_cast<t_directional_light_id>(directional_light_vec.emplace_back());

			if (cast_shadow)
			{
				// shadow_light_buffer
				c_auto shadow_id = static_cast<t_shadow_light_id>(shadow_light_header_vec.count());

				directional_light_vec[id].shadow_id = shadow_id;

				for (auto i : std::views::iota(shadow_id) | std::views::take(g::directional_shadow_cascade_count))
				{
					shadow_light_header_vec.emplace_back(shadow_light_header{
						.slice_index = i });
				}
			}
			else
			{
				directional_light_vec[id].shadow_id = age::get_invalid_id<t_shadow_light_id>();
			}

			update_directional_light(id, desc);

			return id;
		}

		void
		remove_directional_light(t_directional_light_id& id) noexcept
		{
			c_auto& data = directional_light_vec[id];
			if (data.shadow_id != age::get_invalid_id<t_shadow_light_id>())
			{
				for (auto i : std::views::iota(data.shadow_id) | std::views::take(g::directional_shadow_cascade_count))
				{
					shadow_light_header_vec.remove(i);
				}
			}

			directional_light_vec.remove(id);

			id = age::get_invalid_id<t_directional_light_id>();
		}

		void
		update_point_light(t_unified_light_id id, const point_light_desc& desc, bool cast_shadow = false) noexcept
		{
			unified_light_vec[id] = shared_type::unified_light{
				.position  = desc.position,
				.range	   = desc.range,
				.color	   = math::cvt_to<half3>(desc.color),
				.intensity = math::cvt_to<half>(desc.intensity),
				.direction = math::cvt_to<half3>(float3{ 1.f, 0.f, 0.f }),
				.cos_inner = math::cvt_to<half>(-1.f),
				.cos_outer = math::cvt_to<half>(-2.f),
			};
		}

		t_unified_light_id
		add_point_light(const point_light_desc& desc, bool cast_shadow = false) noexcept
		{
			t_unified_light_id id = static_cast<t_unified_light_id>(unified_light_vec.emplace_back());

			update_point_light(id, desc);

			return id;
		}

		void
		update_spot_light(t_unified_light_id id, const spot_light_desc& desc, bool cast_shadow = false) noexcept
		{
			unified_light_vec[id] = shared_type::unified_light{
				.position  = desc.position,
				.range	   = desc.range,
				.color	   = math::cvt_to<half3>(desc.color),
				.intensity = math::cvt_to<half>(desc.intensity),
				.direction = math::cvt_to<half3>(desc.direction),
				.cos_inner = math::cvt_to<half>(desc.cos_inner),
				.cos_outer = math::cvt_to<half>(desc.cos_outer),
			};
		}

		t_unified_light_id
		add_spot_light(const spot_light_desc& desc, bool cast_shadow = false) noexcept
		{
			t_unified_light_id id = static_cast<t_unified_light_id>(unified_light_vec.emplace_back());

			update_spot_light(id, desc);

			return id;
		}

		void
		remove_unified_light(t_unified_light_id& id) noexcept
		{
			unified_light_vec.remove(id);
			id = age::get_invalid_id<t_unified_light_id>();
		}

		t_object_id
		add_object(const shared_type::object_data& data = {}) noexcept
		{
			c_auto id = static_cast<t_object_id>(object_data_vec.count());

			object_data_vec.emplace_back(data);

			return id;
		}

		void
		update_object(t_object_id id, const shared_type::object_data& data = {}) noexcept
		{
			object_data_vec[id] = data;
		}

		void
		remove_object(t_object_id& id) noexcept
		{
			object_data_vec.remove(id);
			id = age::get_invalid_id<t_object_id>();
		}

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
		end_render(render_surface_handle h_rs) noexcept;

		void
		flush_jobs() noexcept
		{
			for (auto& i : job_count_array | std::views::join)
			{
				i = 0;
			}
		}


	  private:
		void
		create_resolution_dependent_buffers() noexcept;

		void
		resize_resolution_dependent_buffers(const age::extent_2d<uint16>& new_extent) noexcept;
	};
}	 // namespace age::graphics::render_pipeline::forward_plus
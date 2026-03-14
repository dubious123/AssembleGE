#include "age_pch.hpp"
#include "age.hpp"
#if defined(AGE_GRAPHICS_BACKEND_DX12)
// stage_init
namespace age::graphics::render_pipeline::forward_plus
{
	inline void
	init_stage::init(graphics::root_signature::handle h_root_sig) noexcept
	{
		using namespace graphics::pso;

		h_pso = graphics::pso::create(
			pss_root_signature{ .subobj = graphics::g::root_signature_ptr_vec[h_root_sig] },
			pss_cs{ .subobj = shader::get_d3d12_bytecode(shader::shader_handle{ std::to_underlying(shader::e::engine_shader_kind::forward_plus_init_cs) }) });

		p_pso = graphics::g::pso_ptr_vec[h_pso];
	}

	inline void
	init_stage::execute(t_cmd_list& cmd_list, uint32 tile_total_uint32_count) noexcept
	{
		cmd_list.SetPipelineState(p_pso);

		cmd_list.Dispatch((tile_total_uint32_count + 255) / 256, 1, 1);
	}

	inline void
	init_stage::deinit() noexcept
	{
		pso::destroy(h_pso);
	}
}	 // namespace age::graphics::render_pipeline::forward_plus

// stage depth
namespace age::graphics::render_pipeline::forward_plus
{
	inline void
	depth_stage::init(graphics::root_signature::handle h_root_sig) noexcept
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
	depth_stage::bind_dsv(graphics::resource_handle h_depth_buffer) noexcept
	{
		resource::create_view(h_depth_buffer,
							  h_depth_buffer_dsv_desc,
							  defaults::dsv_view_desc::d32_float_2d);
	}

	inline void
	depth_stage::execute(t_cmd_list& cmd_list, uint32 job_count) noexcept
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
	depth_stage::deinit() noexcept
	{
		graphics::g::dsv_desc_pool.push(h_depth_buffer_dsv_desc);
		pso::destroy(h_pso);
	}
}	 // namespace age::graphics::render_pipeline::forward_plus

// stage shadow
namespace age::graphics::render_pipeline::forward_plus
{
	inline void
	shadow_stage::init(graphics::root_signature::handle h_root_sig) noexcept
	{
		using namespace graphics::pso;

		h_depth_reduce_pso = graphics::pso::create(
			pss_root_signature{ .subobj = graphics::g::root_signature_ptr_vec[h_root_sig] },
			pss_cs{ .subobj = shader::get_d3d12_bytecode(shader::shader_handle{ std::to_underlying(shader::e::engine_shader_kind::forward_plus_shadow_depth_reduce_cs) }) });

		p_depth_reduce_pso = graphics::g::pso_ptr_vec[h_depth_reduce_pso];

		h_fill_shadow_buffer_pso = graphics::pso::create(
			pss_root_signature{ .subobj = graphics::g::root_signature_ptr_vec[h_root_sig] },
			pss_cs{ .subobj = shader::get_d3d12_bytecode(shader::shader_handle{ std::to_underlying(shader::e::engine_shader_kind::forward_plus_shadow_fill_shadow_buffer_cs) }) });

		p_fill_shadow_buffer_pso = graphics::g::pso_ptr_vec[h_fill_shadow_buffer_pso];

		h_shadow_map_pso = graphics::pso::create(
			pss_root_signature{ .subobj = graphics::g::root_signature_ptr_vec[h_root_sig] },
			pss_as{ .subobj = shader::get_d3d12_bytecode(shader::shader_handle{ std::to_underlying(shader::e::engine_shader_kind::forward_plus_shadow_as) }) },
			pss_ms{ .subobj = shader::get_d3d12_bytecode(shader::shader_handle{ std::to_underlying(shader::e::engine_shader_kind::forward_plus_shadow_ms) }) },
			pss_primitive_topology{ .subobj = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE },
			pss_render_target_formats{ .subobj = D3D12_RT_FORMAT_ARRAY{ .RTFormats{}, .NumRenderTargets = 0 } },
			pss_depth_stencil_format{ .subobj = DXGI_FORMAT_D32_FLOAT },
			pss_rasterizer{ .subobj = defaults::rasterizer_desc::shadow_depth_bias(-g::shadow_depth_bias, -g::shadow_slope_bias) },
			pss_depth_stencil1{ .subobj = defaults::depth_stencil_desc1::depth_only_reversed },
			pss_sample_desc{ .subobj = DXGI_SAMPLE_DESC{ .Count = 1, .Quality = 0 } },
			pss_node_mask{ .subobj = 0 });

		p_shadow_map_pso = graphics::g::pso_ptr_vec[h_shadow_map_pso];

		h_shadow_atlas_dsv_desc = graphics::g::dsv_desc_pool.pop();
	}

	inline void
	shadow_stage::bind_dsv(graphics::resource_handle h_shadow_atlas) noexcept
	{
		resource::create_view(h_shadow_atlas,
							  h_shadow_atlas_dsv_desc,
							  defaults::dsv_view_desc::d32_float_2d);
	}

	inline void
	shadow_stage::execute(t_cmd_list&	  cmd_list,
						  uint32		  width,
						  uint32		  height,
						  uint32		  shadow_light_count,
						  uint32		  shadow_light_header_count,
						  ID3D12Resource& frame_data_rw_buffer,
						  ID3D12Resource& shadow_light_rw_buffer,
						  auto&			  slot_shadow_light_rw_buffer_srv,
						  uint32		  job_count) noexcept
	{
		if (job_count == 0) [[unlikely]]
		{
			return;
		}

		cmd_list.SetPipelineState(p_depth_reduce_pso);
		cmd_list.Dispatch(
			(width + SHADOW_CS_DEPTH_REDUCE_THREAD_COUNT - 1) / SHADOW_CS_DEPTH_REDUCE_THREAD_COUNT,
			(height + SHADOW_CS_DEPTH_REDUCE_THREAD_COUNT - 1) / SHADOW_CS_DEPTH_REDUCE_THREAD_COUNT,
			1);

		apply_barriers(cmd_list, barrier::buf_uav_to_uav(&frame_data_rw_buffer));

		cmd_list.SetPipelineState(p_fill_shadow_buffer_pso);
		cmd_list.Dispatch(shadow_light_header_count, 1, 1);

		apply_barriers(cmd_list,
					   barrier::buf_uav_to_srv(&shadow_light_rw_buffer,
											   D3D12_BARRIER_SYNC_VERTEX_SHADING));
		slot_shadow_light_rw_buffer_srv.apply(cmd_list);

		c_auto render_pass_ds_desc = defaults::render_pass_ds_desc::depth_clear_preserve(h_shadow_atlas_dsv_desc, 0.f);

		cmd_list.BeginRenderPass(
			0,			// no render targets
			nullptr,	// no render targets
			&render_pass_ds_desc,
			D3D12_RENDER_PASS_FLAG_NONE);

		cmd_list.SetPipelineState(p_shadow_map_pso);

		for (auto shadow_id : std::views::iota(0) | std::views::take(shadow_light_count))
		{
			c_auto col	   = shadow_id % g::shadow_atlas_seg_u;
			c_auto row	   = shadow_id / g::shadow_atlas_seg_u;
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
				&shadow_id,
				offsetof(shared_type::root_constants, shadow_light_index) / 4);

			if (job_count > 0) [[likely]]
			{
				cmd_list.DispatchMesh((job_count + 31u) / 32u, 1, 1);
			}
		}

		cmd_list.EndRenderPass();
	}

	inline void
	shadow_stage::deinit() noexcept
	{
		pso::destroy(h_depth_reduce_pso);
		pso::destroy(h_fill_shadow_buffer_pso);
		pso::destroy(h_shadow_map_pso);

		graphics::g::dsv_desc_pool.push(h_shadow_atlas_dsv_desc);
	}
}	 // namespace age::graphics::render_pipeline::forward_plus

// stage light cull
namespace age::graphics::render_pipeline::forward_plus
{
	inline void
	light_culling_stage::init(graphics::root_signature::handle h_root_sig) noexcept
	{
		using namespace graphics::pso;

		h_pso_cull = graphics::pso::create(
			pss_root_signature{ .subobj = graphics::g::root_signature_ptr_vec[h_root_sig] },
			pss_cs{ .subobj = shader::get_d3d12_bytecode(shader::shader_handle{ std::to_underlying(shader::e::engine_shader_kind::forward_plus_light_cull_cs) }) });

		p_pso_cull = graphics::g::pso_ptr_vec[h_pso_cull];

		h_pso_sort_histogram = graphics::pso::create(
			pss_root_signature{ .subobj = graphics::g::root_signature_ptr_vec[h_root_sig] },
			pss_cs{ .subobj = shader::get_d3d12_bytecode(shader::shader_handle{ std::to_underlying(shader::e::engine_shader_kind::forward_plus_sort_histogram_cs) }) });

		p_pso_sort_histogram = graphics::g::pso_ptr_vec[h_pso_sort_histogram];

		h_pso_sort_prefix = graphics::pso::create(
			pss_root_signature{ .subobj = graphics::g::root_signature_ptr_vec[h_root_sig] },
			pss_cs{ .subobj = shader::get_d3d12_bytecode(shader::shader_handle{ std::to_underlying(shader::e::engine_shader_kind::forward_plus_sort_prefix_cs) }) });

		p_pso_sort_prefix = graphics::g::pso_ptr_vec[h_pso_sort_prefix];

		h_pso_sort_scatter = graphics::pso::create(
			pss_root_signature{ .subobj = graphics::g::root_signature_ptr_vec[h_root_sig] },
			pss_cs{ .subobj = shader::get_d3d12_bytecode(shader::shader_handle{ std::to_underlying(shader::e::engine_shader_kind::forward_plus_sort_scatter_cs) }) });

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
	light_culling_stage::execute(t_cmd_list&	 cmd_list,
								 uint32			 light_tile_count_x,
								 uint32			 light_tile_count_y,
								 ID3D12Resource& unified_sorted_light_buffer,
								 ID3D12Resource& frame_data_rw_buffer,
								 auto&			 slot_frame_data_rw_buffer_srv,
								 ID3D12Resource& sort_buffer,
								 auto&			 slot_sort_buffer_srv,
								 ID3D12Resource& zbin_buffer,
								 auto&			 slot_zbin_buffer_srv) noexcept
	{
		cmd_list.SetPipelineState(p_pso_cull);
		cmd_list.Dispatch((g::max_sort_count + g::light_cull_cs_thread_count - 1) / g::light_cull_cs_thread_count, 1, 1);

		apply_barriers(cmd_list, barrier::buf_uav_to_uav(&sort_buffer));

		for (uint32 pass : std::views::iota(0u) | std::views::take(g::sort_iteration_count))
		{
			cmd_list.SetComputeRoot32BitConstants(
				binding_config_t::reg_b<1>::slot_id,
				sizeof(uint32) / 4,
				&pass,
				offsetof(shared_type::root_constants, radix_sort_pass) / 4);

			cmd_list.SetPipelineState(p_pso_sort_histogram);
			cmd_list.Dispatch(g::sort_group_count, 1, 1);

			apply_barriers(cmd_list, barrier::buf_uav_to_uav(&sort_buffer));

			cmd_list.SetPipelineState(p_pso_sort_prefix);
			cmd_list.Dispatch(g::sort_bin_count, 1, 1);

			apply_barriers(cmd_list, barrier::buf_uav_to_uav(&sort_buffer));

			cmd_list.SetPipelineState(p_pso_sort_scatter);
			cmd_list.Dispatch(g::sort_group_count, 1, 1);

			apply_barriers(cmd_list, barrier::buf_uav_to_uav(&sort_buffer));
		}

		cmd_list.SetPipelineState(p_pso_zbin);
		cmd_list.Dispatch((g::max_visible_light_count + g::zbin_thread_count - 1) / g::zbin_thread_count, 1, 1);

		apply_barriers(
			cmd_list,
			// used by opaque_ps
			barrier::buf_uav_to_srv(&zbin_buffer, D3D12_BARRIER_SYNC_PIXEL_SHADING),
			// zbin_cs writes not_culled_light_count,
			// tile_cs reads not_culled_light_count
			// opaque_ps reads cascade_splits
			barrier::buf_uav_to_srv(&frame_data_rw_buffer,
									D3D12_BARRIER_SYNC_COMPUTE_SHADING | D3D12_BARRIER_SYNC_PIXEL_SHADING),

			barrier::buf_uav_to_srv(&sort_buffer, D3D12_BARRIER_SYNC_COMPUTE_SHADING));

		slot_zbin_buffer_srv.apply(cmd_list);

		slot_frame_data_rw_buffer_srv.apply_compute(cmd_list);
		slot_frame_data_rw_buffer_srv.apply(cmd_list);

		slot_sort_buffer_srv.apply_compute(cmd_list);

		cmd_list.SetPipelineState(p_pso_tile);
		cmd_list.Dispatch(light_tile_count_x, light_tile_count_y, 1);
	}

	inline void
	light_culling_stage::deinit() noexcept
	{
		pso::destroy(h_pso_cull);

		pso::destroy(h_pso_sort_histogram);
		pso::destroy(h_pso_sort_prefix);
		pso::destroy(h_pso_sort_scatter);

		pso::destroy(h_pso_zbin);
		pso::destroy(h_pso_tile);
	}
}	 // namespace age::graphics::render_pipeline::forward_plus

// stage opaque
namespace age::graphics::render_pipeline::forward_plus
{
	inline void
	opaque_stage::init(graphics::root_signature::handle h_root_sig) noexcept
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
	opaque_stage::bind_rtv_dsv(graphics::resource_handle h_main_buffer,
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
	opaque_stage::execute(t_cmd_list& cmd_list, uint32 job_count) noexcept
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
	opaque_stage::deinit() noexcept
	{
		graphics::g::rtv_desc_pool.push(h_main_buffer_rtv_desc);
		graphics::g::dsv_desc_pool.push(h_depth_buffer_dsv_desc);

		pso::destroy(h_pso);
	}
}	 // namespace age::graphics::render_pipeline::forward_plus

// stage transparent
namespace age::graphics::render_pipeline::forward_plus
{
	inline void
	transparent_stage::init(graphics::root_signature::handle h_root_sig) noexcept
	{
		using namespace graphics::pso;

		h_pso_cull = graphics::pso::create(
			pss_root_signature{ .subobj = graphics::g::root_signature_ptr_vec[h_root_sig] },
			pss_cs{ .subobj = shader::get_d3d12_bytecode(shader::shader_handle{ std::to_underlying(shader::e::engine_shader_kind::forward_plus_transparent_cull_cs) }) });

		p_pso_cull = graphics::g::pso_ptr_vec[h_pso_cull];

		h_pso_sort_histogram = graphics::pso::create(
			pss_root_signature{ .subobj = graphics::g::root_signature_ptr_vec[h_root_sig] },
			pss_cs{ .subobj = shader::get_d3d12_bytecode(shader::shader_handle{ std::to_underlying(shader::e::engine_shader_kind::forward_plus_sort_histogram_cs) }) });

		p_pso_sort_histogram = graphics::g::pso_ptr_vec[h_pso_sort_histogram];

		h_pso_sort_prefix = graphics::pso::create(
			pss_root_signature{ .subobj = graphics::g::root_signature_ptr_vec[h_root_sig] },
			pss_cs{ .subobj = shader::get_d3d12_bytecode(shader::shader_handle{ std::to_underlying(shader::e::engine_shader_kind::forward_plus_sort_prefix_cs) }) });

		p_pso_sort_prefix = graphics::g::pso_ptr_vec[h_pso_sort_prefix];

		h_pso_sort_scatter = graphics::pso::create(
			pss_root_signature{ .subobj = graphics::g::root_signature_ptr_vec[h_root_sig] },
			pss_cs{ .subobj = shader::get_d3d12_bytecode(shader::shader_handle{ std::to_underlying(shader::e::engine_shader_kind::forward_plus_sort_scatter_cs) }) });

		p_pso_sort_scatter = graphics::g::pso_ptr_vec[h_pso_sort_scatter];

		h_pso_transparent_gen_indirect_arg = graphics::pso::create(
			pss_root_signature{ .subobj = graphics::g::root_signature_ptr_vec[h_root_sig] },
			pss_cs{ .subobj = shader::get_d3d12_bytecode(shader::shader_handle{ std::to_underlying(shader::e::engine_shader_kind::forward_plus_transparent_gen_indirect_arg_cs) }) });

		p_pso_transparent_gen_indirect_arg = graphics::g::pso_ptr_vec[h_pso_transparent_gen_indirect_arg];

		h_pso_draw = graphics::pso::create(
			pss_root_signature{ .subobj = graphics::g::root_signature_ptr_vec[h_root_sig] },
			pss_as{ .subobj = shader::get_d3d12_bytecode(shader::shader_handle{ std::to_underlying(shader::e::engine_shader_kind::forward_plus_transparent_as) }) },
			pss_ms{ .subobj = shader::get_d3d12_bytecode(shader::shader_handle{ std::to_underlying(shader::e::engine_shader_kind::forward_plus_transparent_ms) }) },
			pss_ps{ .subobj = shader::get_d3d12_bytecode(shader::shader_handle{ std::to_underlying(shader::e::engine_shader_kind::forward_plus_transparent_ps) }) },
			pss_primitive_topology{ .subobj = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE },
			pss_render_target_formats{ .subobj = D3D12_RT_FORMAT_ARRAY{ .RTFormats{ DXGI_FORMAT_R16G16B16A16_FLOAT }, .NumRenderTargets = 1 } },
			pss_depth_stencil_format{ .subobj = DXGI_FORMAT_D32_FLOAT },
			pss_rasterizer{ .subobj = defaults::rasterizer_desc::no_cull },
			pss_depth_stencil1{ .subobj = defaults::depth_stencil_desc1::depth_readonly_reversed },
			pss_blend{ .subobj = defaults::blend_desc::alpha },
			pss_sample_desc{ .subobj = DXGI_SAMPLE_DESC{ .Count = 1, .Quality = 0 } },
			pss_node_mask{ .subobj = 0 });

		p_pso_draw = graphics::g::pso_ptr_vec[h_pso_draw];

		h_draw_cmd_sig = command_signature::create<shared_type::transparent_indirect_arg>(
			h_root_sig,
			defaults::cmd_sig::constant(binding_config_t::reg_b<2, 0>::slot_id, 2),
			defaults::cmd_sig::dispatch_mesh);

		p_draw_cmd_sig = h_draw_cmd_sig.ptr();

		h_main_buffer_rtv_desc	= graphics::g::rtv_desc_pool.pop();
		h_depth_buffer_dsv_desc = graphics::g::dsv_desc_pool.pop();
	}

	inline void
	transparent_stage::bind_rtv_dsv(graphics::resource_handle h_main_buffer,
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
	transparent_stage::execute(t_cmd_list&	   cmd_list,
							   ID3D12Resource& sort_buffer,
							   auto&		   slot_sort_buffer_srv,
							   ID3D12Resource& frame_data_rw_buffer,
							   auto&		   frame_data_rw_buffer_srv) noexcept
	{
		cmd_list.SetPipelineState(p_pso_cull);

		slot_sort_buffer_srv.apply_compute(cmd_list);
		frame_data_rw_buffer_srv.apply_compute(cmd_list);

		// we need to initialize all sort keys and values
		cmd_list.Dispatch((g::max_sort_count + g::transparent_cull_thread_count - 1) / g::transparent_cull_thread_count, 1, 1);

		apply_barriers(cmd_list, barrier::buf_uav_to_uav(&sort_buffer));

		for (uint32 pass : std::views::iota(0u) | std::views::take(g::sort_iteration_count))
		{
			cmd_list.SetComputeRoot32BitConstants(
				binding_config_t::reg_b<1>::slot_id,
				sizeof(uint32) / 4,
				&pass,
				offsetof(shared_type::root_constants, radix_sort_pass) / 4);

			cmd_list.SetPipelineState(p_pso_sort_histogram);
			cmd_list.Dispatch(g::sort_group_count, 1, 1);

			apply_barriers(cmd_list, barrier::buf_uav_to_uav(&sort_buffer));

			cmd_list.SetPipelineState(p_pso_sort_prefix);
			cmd_list.Dispatch(g::sort_bin_count, 1, 1);

			apply_barriers(cmd_list, barrier::buf_uav_to_uav(&sort_buffer));

			cmd_list.SetPipelineState(p_pso_sort_scatter);
			cmd_list.Dispatch(g::sort_group_count, 1, 1);

			apply_barriers(cmd_list, barrier::buf_uav_to_uav(&sort_buffer));
		}

		apply_barriers(cmd_list,
					   barrier::buf_srv_to_uav(&frame_data_rw_buffer, D3D12_BARRIER_SYNC_COMPUTE_SHADING));

		cmd_list.SetPipelineState(p_pso_transparent_gen_indirect_arg);
		cmd_list.Dispatch((g::max_transparent_object_count + 31) / 32, 1, 1);

		apply_barriers(cmd_list,
					   barrier::buf_uav_to_indirect(&sort_buffer),
					   barrier::buf_uav_to_indirect(&frame_data_rw_buffer));

		cmd_list.SetPipelineState(p_pso_draw);

		cmd_list.ExecuteIndirect(
			p_draw_cmd_sig,																   // command signature
			g::max_transparent_object_count,											   // max command count
			&sort_buffer,																   // argument buffer
			TRANSPARENT_INDIRECT_ARG_OFFSET * sizeof(uint32),							   // argument buffer byte offset
			&frame_data_rw_buffer,														   // count buffer
			offsetof(shared_type::frame_data_rw, not_culled_transparent_object_count));	   // count buffer bytes offset
	}

	inline void
	transparent_stage::deinit() noexcept
	{
		graphics::g::rtv_desc_pool.push(h_main_buffer_rtv_desc);
		graphics::g::dsv_desc_pool.push(h_depth_buffer_dsv_desc);

		pso::destroy(h_pso_cull);

		pso::destroy(h_pso_sort_histogram);
		pso::destroy(h_pso_sort_prefix);
		pso::destroy(h_pso_sort_scatter);

		pso::destroy(h_pso_transparent_gen_indirect_arg);

		pso::destroy(h_pso_draw);

		command_signature::destroy(h_draw_cmd_sig);
	}
}	 // namespace age::graphics::render_pipeline::forward_plus

// stage presentation
namespace age::graphics::render_pipeline::forward_plus
{
	inline void
	presentation_stage::init(root_signature::handle h_root_sig) noexcept
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
	presentation_stage::execute(t_cmd_list& cmd_list, render_surface& rs) noexcept
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

	inline void
	presentation_stage::deinit() noexcept
	{
		pso::destroy(h_pso);
	}
}	 // namespace age::graphics::render_pipeline::forward_plus
#endif
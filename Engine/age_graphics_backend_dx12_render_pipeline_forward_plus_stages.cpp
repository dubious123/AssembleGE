#include "age_pch.hpp"
#include "age.hpp"
#if defined(AGE_GRAPHICS_BACKEND_DX12)
// stage depth
namespace age::graphics::render_pipeline::forward_plus
{
	void
	depth_stage::init(graphics::root_signature::handle h_root_sig) noexcept
	{
		using namespace graphics::pso;

		h_pso = graphics::pso::create(
			pss_root_signature{ .subobj = graphics::g::root_signature_ptr_vec[h_root_sig] },
			pss_as{ .subobj = shader::get_d3d12_bytecode(e::engine_shader_kind::forward_plus_opaque_as) },
			pss_ms{ .subobj = shader::get_d3d12_bytecode(e::engine_shader_kind::forward_plus_depth_ms) },
			// no PS
			pss_primitive_topology{ .subobj = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE },
			pss_render_target_formats{ .subobj = D3D12_RT_FORMAT_ARRAY{ .RTFormats{}, .NumRenderTargets = 0 } },
			pss_depth_stencil_format{ .subobj = DXGI_FORMAT_D32_FLOAT },
			pss_rasterizer{ .subobj = defaults::rasterizer_desc::backface_cull },
			pss_depth_stencil1{ .subobj = defaults::depth_stencil_desc1::depth_only_reversed },
			pss_sample_desc{ .subobj = DXGI_SAMPLE_DESC{ .Count = 1, .Quality = 0 } },
			pss_node_mask{ .subobj = 0 });

		p_pso = graphics::g::pso_ptr_vec[h_pso];

		h_pso.set_name(L"pso_depth");
	}

	inline void
	depth_stage::execute(dsv_desc_handle h_depth_buffer_dsv_desc,
						 uint32			 opaque_meshlet_count) noexcept
	{
		if (opaque_meshlet_count == 0) [[unlikely]]
		{
			command::clear_dsv(h_depth_buffer_dsv_desc.h_cpu,
							   D3D12_CLEAR_FLAG_DEPTH,
							   0.0f,
							   0,
							   0,
							   nullptr);
			return;
		}

		c_auto render_pass_ds_desc = defaults::render_pass_ds_desc::depth_clear_preserve(h_depth_buffer_dsv_desc, 0.f);

		command::begin_render_pass(
			0,			// no render targets
			nullptr,	// no render targets
			&render_pass_ds_desc,
			D3D12_RENDER_PASS_FLAG_NONE);

		command::set_pso(p_pso);

		command::dispatch_mesh((opaque_meshlet_count + 31u) / 32u, 1, 1);

		command::end_render_pass();
	}

	void
	depth_stage::deinit() noexcept
	{
		pso::destroy(h_pso);
	}
}	 // namespace age::graphics::render_pipeline::forward_plus

// skybox stage
namespace age::graphics::render_pipeline::forward_plus
{
	void
	skybox_stage::init(graphics::root_signature::handle h_root_sig) noexcept
	{
		using namespace graphics::pso;

		h_pso = graphics::pso::create(
			pss_root_signature{ .subobj = graphics::g::root_signature_ptr_vec[h_root_sig] },
			pss_ms{ .subobj = shader::get_d3d12_bytecode(e::engine_shader_kind::forward_plus_skybox_ms) },
			pss_ps{ .subobj = shader::get_d3d12_bytecode(e::engine_shader_kind::forward_plus_skybox_ps) },
			pss_primitive_topology{ .subobj = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE },
			pss_render_target_formats{ .subobj = D3D12_RT_FORMAT_ARRAY{ .RTFormats{ DXGI_FORMAT_R16G16B16A16_FLOAT }, .NumRenderTargets = 1 } },
			pss_depth_stencil_format{ .subobj = DXGI_FORMAT_D32_FLOAT },
			pss_rasterizer{ .subobj = defaults::rasterizer_desc::no_cull },
			pss_depth_stencil1{ .subobj = defaults::depth_stencil_desc1::depth_equal_readonly_reversed },
			pss_blend{ .subobj = defaults::blend_desc::opaque },
			pss_sample_desc{ .subobj = DXGI_SAMPLE_DESC{ .Count = 1, .Quality = 0 } },
			pss_node_mask{ .subobj = 0 });

		p_pso = graphics::g::pso_ptr_vec[h_pso];
		h_pso.set_name(L"pso_skybox");
	}

	inline void
	skybox_stage::execute(rtv_desc_handle h_main_buffer_rtv_desc,
						  dsv_desc_handle h_depth_buffer_dsv_readonly_desc,
						  uint32		  env_light_count) noexcept
	{
		constexpr c_auto clear_color		 = float4{ 0, 0, 0, 1 };
		auto			 render_pass_rt_desc = defaults::render_pass_rtv_desc::clear_preserve(h_main_buffer_rtv_desc, &clear_color);
		auto			 render_pass_ds_desc = defaults::render_pass_ds_desc::depth_load_preserve(h_depth_buffer_dsv_readonly_desc);

		command::begin_render_pass(
			1,
			&render_pass_rt_desc,
			&render_pass_ds_desc,
			D3D12_RENDER_PASS_FLAG_BIND_READ_ONLY_DEPTH);

		{
			command::set_pso(p_pso);

			if (env_light_count > 0) [[likely]]
			{
				command::dispatch_mesh(1, 1, 1);
			}
		}

		command::end_render_pass();
	}

	void
	skybox_stage::deinit() noexcept
	{
		pso::destroy(h_pso);
	}
}	 // namespace age::graphics::render_pipeline::forward_plus

// stage light cull
namespace age::graphics::render_pipeline::forward_plus
{
	void
	light_culling_stage::init(graphics::root_signature::handle h_root_sig) noexcept
	{
		using namespace graphics::pso;

		h_pso_init = graphics::pso::create(
			pss_root_signature{ .subobj = graphics::g::root_signature_ptr_vec[h_root_sig] },
			pss_cs{ .subobj = shader::get_d3d12_bytecode(e::engine_shader_kind::forward_plus_light_init_cs) });

		p_pso_init = graphics::g::pso_ptr_vec[h_pso_init];
		h_pso_init.set_name(L"p_pso_init");

		h_pso_cull = graphics::pso::create(
			pss_root_signature{ .subobj = graphics::g::root_signature_ptr_vec[h_root_sig] },
			pss_cs{ .subobj = shader::get_d3d12_bytecode(e::engine_shader_kind::forward_plus_light_cull_cs) });

		p_pso_cull = graphics::g::pso_ptr_vec[h_pso_cull];
		h_pso_cull.set_name(L"p_pso_cull");

		h_pso_sort_histogram = graphics::pso::create(
			pss_root_signature{ .subobj = graphics::g::root_signature_ptr_vec[h_root_sig] },
			pss_cs{ .subobj = shader::get_d3d12_bytecode(e::engine_shader_kind::forward_plus_sort_histogram_cs) });

		p_pso_sort_histogram = graphics::g::pso_ptr_vec[h_pso_sort_histogram];
		h_pso_sort_histogram.set_name(L"p_pso_sort_histogram");

		h_pso_sort_prefix = graphics::pso::create(
			pss_root_signature{ .subobj = graphics::g::root_signature_ptr_vec[h_root_sig] },
			pss_cs{ .subobj = shader::get_d3d12_bytecode(e::engine_shader_kind::forward_plus_sort_prefix_cs) });

		p_pso_sort_prefix = graphics::g::pso_ptr_vec[h_pso_sort_prefix];
		h_pso_sort_prefix.set_name(L"p_pso_sort_prefix");

		h_pso_sort_scatter = graphics::pso::create(
			pss_root_signature{ .subobj = graphics::g::root_signature_ptr_vec[h_root_sig] },
			pss_cs{ .subobj = shader::get_d3d12_bytecode(e::engine_shader_kind::forward_plus_sort_scatter_cs) });

		p_pso_sort_scatter = graphics::g::pso_ptr_vec[h_pso_sort_scatter];
		h_pso_sort_scatter.set_name(L"p_pso_sort_scatter");

		h_pso_zbin = graphics::pso::create(
			pss_root_signature{ .subobj = graphics::g::root_signature_ptr_vec[h_root_sig] },
			pss_cs{ .subobj = shader::get_d3d12_bytecode(e::engine_shader_kind::forward_plus_light_zbin_cs) });

		p_pso_zbin = graphics::g::pso_ptr_vec[h_pso_zbin];
		h_pso_zbin.set_name(L"p_pso_zbin");

		h_pso_tile = graphics::pso::create(
			pss_root_signature{ .subobj = graphics::g::root_signature_ptr_vec[h_root_sig] },
			pss_cs{ .subobj = shader::get_d3d12_bytecode(e::engine_shader_kind::forward_plus_light_tile_cs) });

		p_pso_tile = graphics::g::pso_ptr_vec[h_pso_tile];
		h_pso_tile.set_name(L"p_pso_tile");
	}

	inline void
	light_culling_stage::execute(uint32			 light_tile_count_x,
								 uint32			 light_tile_count_y,
								 resource_handle h_scratch_buffer) noexcept
	{
		command::set_pso(p_pso_init);
		command::dispatch((light_tile_count_x * light_tile_count_y * g::light_bitmask_uint32_count + 255) / 256, 1, 1);

		command::apply_barriers(barrier::buf_uav_to_uav(h_scratch_buffer->p_resource));

		command::set_pso(p_pso_cull);
		command::dispatch((g::max_sort_count + g::light_cull_thread_count - 1) / g::light_cull_thread_count, 1, 1);

		command::apply_barriers(barrier::buf_uav_to_uav(h_scratch_buffer->p_resource));

		for (uint32 pass : std::views::iota(0u) | std::views::take(g::sort_iteration_count))
		{
			command::set_compute_root_constants(
				binding_config_t::reg_b<1>::slot_id,
				static_cast<uint32>(sizeof(uint32) / 4),
				&pass,
				static_cast<uint32>(offsetof(shared_type::root_constants, radix_sort_pass) / 4));

			command::set_pso(p_pso_sort_histogram);
			command::dispatch(g::sort_group_count, 1, 1);

			command::apply_barriers(barrier::buf_uav_to_uav(h_scratch_buffer->p_resource));

			command::set_pso(p_pso_sort_prefix);
			command::dispatch(g::sort_bin_count, 1, 1);

			command::apply_barriers(barrier::buf_uav_to_uav(h_scratch_buffer->p_resource));

			command::set_pso(p_pso_sort_scatter);
			command::dispatch(g::sort_group_count, 1, 1);

			command::apply_barriers(barrier::buf_uav_to_uav(h_scratch_buffer->p_resource));
		}

		command::set_pso(p_pso_zbin);
		command::dispatch((g::max_visible_light_count + g::zbin_thread_count - 1) / g::zbin_thread_count, 1, 1);

		command::apply_barriers(barrier::buf_uav_to_uav(h_scratch_buffer->p_resource));

		command::set_pso(p_pso_tile);
		command::dispatch(light_tile_count_x, light_tile_count_y, 1);
	}

	void
	light_culling_stage::deinit() noexcept
	{
		pso::destroy(h_pso_init);
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
	void
	opaque_stage::init(graphics::root_signature::handle h_root_sig) noexcept
	{
		using namespace graphics::pso;

		h_pso = graphics::pso::create(
			pss_root_signature{ .subobj = graphics::g::root_signature_ptr_vec[h_root_sig] },
			pss_as{ .subobj = shader::get_d3d12_bytecode(e::engine_shader_kind::forward_plus_opaque_as) },
			pss_ms{ .subobj = shader::get_d3d12_bytecode(e::engine_shader_kind::forward_plus_opaque_ms) },
			pss_ps{ .subobj = shader::get_d3d12_bytecode(e::engine_shader_kind::forward_plus_opaque_ps) },
			pss_primitive_topology{ .subobj = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE },
			pss_render_target_formats{ .subobj = D3D12_RT_FORMAT_ARRAY{ .RTFormats{ DXGI_FORMAT_R16G16B16A16_FLOAT }, .NumRenderTargets = 1 } },
			pss_depth_stencil_format{ .subobj = DXGI_FORMAT_D32_FLOAT },
			pss_rasterizer{ .subobj = defaults::rasterizer_desc::backface_cull },
			pss_depth_stencil1{ .subobj = defaults::depth_stencil_desc1::depth_equal_readonly_reversed },
			pss_blend{ .subobj = defaults::blend_desc::opaque },
			pss_sample_desc{ .subobj = DXGI_SAMPLE_DESC{ .Count = 1, .Quality = 0 } },
			pss_node_mask{ .subobj = 0 });

		p_pso = graphics::g::pso_ptr_vec[h_pso];
		h_pso.set_name(L"pso_opaque");
	}

	inline void
	opaque_stage::execute(rtv_desc_handle h_main_buffer_rtv_desc,
						  dsv_desc_handle h_depth_buffer_dsv_desc,
						  uint32		  meshlet_count) noexcept
	{
		auto render_pass_rt_desc = defaults::render_pass_rtv_desc::load_preserve(h_main_buffer_rtv_desc);
		auto render_pass_ds_desc = defaults::render_pass_ds_desc::depth_load_preserve(h_depth_buffer_dsv_desc);

		command::begin_render_pass(
			1,
			&render_pass_rt_desc,
			&render_pass_ds_desc,
			D3D12_RENDER_PASS_FLAG_BIND_READ_ONLY_DEPTH);

		{
			command::set_pso(p_pso);

			if (meshlet_count > 0) [[likely]]
			{
				command::dispatch_mesh((meshlet_count + 31u) / 32u, 1, 1);
			}
		}

		command::end_render_pass();
	}

	void
	opaque_stage::deinit() noexcept
	{
		pso::destroy(h_pso);
	}
}	 // namespace age::graphics::render_pipeline::forward_plus

// stage transparent
namespace age::graphics::render_pipeline::forward_plus
{
	void
	transparent_stage::init(graphics::root_signature::handle h_root_sig) noexcept
	{
		using namespace graphics::pso;

		h_pso_rt = graphics::pso::create(
			pss_root_signature{ .subobj = graphics::g::root_signature_ptr_vec[h_root_sig] },
			pss_cs{ .subobj = shader::get_d3d12_bytecode(e::engine_shader_kind::forward_plus_transparent_rt_cs) });

		p_pso_rt = graphics::g::pso_ptr_vec[h_pso_rt];
		h_pso_rt.set_name(L"pso_transparent_rt");

		h_pso_draw = graphics::pso::create(
			pss_root_signature{ .subobj = graphics::g::root_signature_ptr_vec[h_root_sig] },
			pss_ms{ .subobj = shader::get_d3d12_bytecode(e::engine_shader_kind::forward_plus_presentation_ms) },
			pss_ps{ .subobj = shader::get_d3d12_bytecode(e::engine_shader_kind::forward_plus_transparent_blend_ps) },
			pss_primitive_topology{ .subobj = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE },
			pss_render_target_formats{ .subobj = D3D12_RT_FORMAT_ARRAY{ .RTFormats{ DXGI_FORMAT_R16G16B16A16_FLOAT }, .NumRenderTargets = 1 } },
			pss_rasterizer{ .subobj = defaults::rasterizer_desc::no_cull },
			pss_depth_stencil1{ .subobj = defaults::depth_stencil_desc1::disabled },
			pss_blend{ .subobj = defaults::blend_desc::alpha },
			pss_sample_desc{ .subobj = DXGI_SAMPLE_DESC{ .Count = 1, .Quality = 0 } },
			pss_node_mask{ .subobj = 0 });

		p_pso_draw = graphics::g::pso_ptr_vec[h_pso_draw];
		h_pso_draw.set_name(L"pso_transparent_blend");
	}

	inline void
	transparent_stage::execute(rtv_desc_handle h_main_buffer_rtv_desc, resource_handle h_blend_tex, extent_2d<uint16> extent) noexcept
	{
		command::set_pso(p_pso_rt);

		command::dispatch((extent.width + 7) / 8, (extent.height + 7) / 8, 1);

		command::apply_barriers(barrier::tex_uav_to_srv(h_blend_tex->p_resource, D3D12_BARRIER_SYNC_COMPUTE_SHADING, D3D12_BARRIER_SYNC_PIXEL_SHADING));

		auto render_pass_rt_desc = defaults::render_pass_rtv_desc::load_preserve(h_main_buffer_rtv_desc);

		command::begin_render_pass(
			1,
			&render_pass_rt_desc,
			nullptr,
			D3D12_RENDER_PASS_FLAG_NONE);

		command::set_pso(p_pso_draw);

		command::dispatch_mesh(1, 1, 1);

		command::end_render_pass();
	}

	void
	transparent_stage::deinit() noexcept
	{
		pso::destroy(h_pso_rt);
		pso::destroy(h_pso_draw);
	}
}	 // namespace age::graphics::render_pipeline::forward_plus

namespace age::graphics::render_pipeline::forward_plus
{
	void
	raycast_stage::init(graphics::root_signature::handle h_root_sig) noexcept
	{
		using namespace graphics::pso;

		h_pso = graphics::pso::create(
			pss_root_signature{ .subobj = graphics::g::root_signature_ptr_vec[h_root_sig] },
			pss_cs{ .subobj = shader::get_d3d12_bytecode(e::engine_shader_kind::forward_plus_raycast_cs) });

		p_pso = graphics::g::pso_ptr_vec[h_pso];
		h_pso.set_name(L"pso_raycast");
	}

	inline void
	raycast_stage::execute(uint32 raycast_count) noexcept
	{
		command::set_pso(p_pso);
		command::dispatch(util::ceil(raycast_count, 32), 1, 1);
	}

	void
	raycast_stage::deinit() noexcept
	{
		pso::destroy(h_pso);
	}
}	 // namespace age::graphics::render_pipeline::forward_plus

namespace age::graphics::render_pipeline::forward_plus
{
	void
	post_process_stage::init(graphics::root_signature::handle h_root_sig) noexcept
	{
		using namespace graphics::pso;

		h_pso = graphics::pso::create(
			pss_root_signature{ .subobj = graphics::g::root_signature_ptr_vec[h_root_sig] },
			pss_ms{ .subobj = shader::get_d3d12_bytecode(e::engine_shader_kind::forward_plus_presentation_ms) },
			pss_ps{ .subobj = shader::get_d3d12_bytecode(e::engine_shader_kind::forward_plus_post_process_ps) },
			pss_primitive_topology{ .subobj = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE },
			pss_render_target_formats{ .subobj = D3D12_RT_FORMAT_ARRAY{ .RTFormats{ DXGI_FORMAT_R16G16B16A16_FLOAT }, .NumRenderTargets = 1 } },
			pss_rasterizer{ .subobj = defaults::rasterizer_desc::no_cull },
			pss_blend{ .subobj = defaults::blend_desc::opaque },
			pss_sample_desc{ .subobj = DXGI_SAMPLE_DESC{ .Count = 1, .Quality = 0 } },
			pss_node_mask{ .subobj = 0 });

		p_pso = graphics::g::pso_ptr_vec[h_pso];
		h_pso.set_name(L"pso_post_process");
	}

	inline void
	post_process_stage::execute(rtv_desc_handle h_post_buffer_rtv_desc) noexcept
	{
		auto render_pass_rt_desc = defaults::render_pass_rtv_desc::overwrite_preserve(h_post_buffer_rtv_desc);

		command::begin_render_pass(
			1,
			&render_pass_rt_desc,
			nullptr,
			D3D12_RENDER_PASS_FLAG_NONE);

		command::set_pso(p_pso);
		command::dispatch_mesh(1, 1, 1);

		command::end_render_pass();
	}

	void
	post_process_stage::deinit() noexcept
	{
		pso::destroy(h_pso);
	}
}	 // namespace age::graphics::render_pipeline::forward_plus

// stage selection outline
namespace age::graphics::render_pipeline::forward_plus
{
	void
	selection_outline_stage::init(root_signature::handle h_root_sig) noexcept
	{
		using namespace graphics::pso;

		h_pso_mask = graphics::pso::create(
			pss_root_signature{ .subobj = graphics::g::root_signature_ptr_vec[h_root_sig] },
			pss_as{ .subobj = shader::get_d3d12_bytecode(e::engine_shader_kind::forward_plus_selection_outline_mask_as) },
			pss_ms{ .subobj = shader::get_d3d12_bytecode(e::engine_shader_kind::forward_plus_selection_outline_mask_ms) },
			pss_ps{ .subobj = shader::get_d3d12_bytecode(e::engine_shader_kind::forward_plus_selection_outline_mask_ps) },
			pss_primitive_topology{ .subobj = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE },
			pss_render_target_formats{ .subobj = D3D12_RT_FORMAT_ARRAY{ .RTFormats{ DXGI_FORMAT_R8_UINT }, .NumRenderTargets = 1 } },
			pss_depth_stencil1{ .subobj = defaults::depth_stencil_desc1::disabled },
			pss_rasterizer{ .subobj = defaults::rasterizer_desc::backface_cull },
			pss_blend{ .subobj = defaults::blend_desc::opaque },
			pss_sample_desc{ .subobj = DXGI_SAMPLE_DESC{ .Count = 1, .Quality = 0 } },
			pss_node_mask{ .subobj = 0 });

		p_pso_mask = graphics::g::pso_ptr_vec[h_pso_mask];
		h_pso_mask.set_name(L"pso_selection_outline_mask");

		h_pso_draw = graphics::pso::create(
			pss_root_signature{ .subobj = graphics::g::root_signature_ptr_vec[h_root_sig] },
			pss_ms{ .subobj = shader::get_d3d12_bytecode(e::engine_shader_kind::fullscreen_ms) },
			pss_ps{ .subobj = shader::get_d3d12_bytecode(e::engine_shader_kind::forward_plus_selection_outline_draw_ps) },
			pss_primitive_topology{ .subobj = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE },
			pss_render_target_formats{ .subobj = D3D12_RT_FORMAT_ARRAY{ .RTFormats{ DXGI_FORMAT_R16G16B16A16_FLOAT }, .NumRenderTargets = 1 } },
			pss_depth_stencil1{ .subobj = defaults::depth_stencil_desc1::disabled },
			pss_rasterizer{ .subobj = defaults::rasterizer_desc::no_cull },
			pss_blend{ .subobj = defaults::blend_desc::alpha },
			pss_sample_desc{ .subobj = DXGI_SAMPLE_DESC{ .Count = 1, .Quality = 0 } },
			pss_node_mask{ .subobj = 0 });

		p_pso_draw = graphics::g::pso_ptr_vec[h_pso_draw];
		h_pso_draw.set_name(L"pso_selection_outline_draw");
	}

	inline void
	selection_outline_stage::execute(uint32			 selected_meshlet_count,
									 rtv_desc_handle h_desc_mask_buffer,
									 rtv_desc_handle h_desc_rtv_buffer,
									 resource_handle h_outline_mask) noexcept
	{
		if (selected_meshlet_count == 0)
		{
			command::apply_barriers(barrier::rtv_to_srv(h_outline_mask->p_resource, D3D12_BARRIER_SYNC_PIXEL_SHADING));
			return;
		}

		constexpr float clear_color[]			 = { 255.f, 255.f, 255.f, 255.f };
		c_auto			render_pass_rt_desc_mask = defaults::render_pass_rtv_desc::clear_preserve(h_desc_mask_buffer, clear_color);

		command::begin_render_pass(
			1,
			&render_pass_rt_desc_mask,
			nullptr,
			D3D12_RENDER_PASS_FLAG_NONE);

		{
			command::set_pso(p_pso_mask);

			command::dispatch_mesh(util::ceil(selected_meshlet_count, 32u), 1, 1);
		}

		command::end_render_pass();

		command::apply_barriers(barrier::rtv_to_srv(h_outline_mask->p_resource, D3D12_BARRIER_SYNC_PIXEL_SHADING));

		c_auto render_pass_rt_desc_draw = defaults::render_pass_rtv_desc::load_preserve(h_desc_rtv_buffer);

		command::begin_render_pass(
			1,
			&render_pass_rt_desc_draw,
			nullptr,
			D3D12_RENDER_PASS_FLAG_NONE);

		{
			command::set_pso(p_pso_draw);

			command::dispatch_mesh(1, 1, 1);
		}

		command::end_render_pass();
	}

	void
	selection_outline_stage::deinit() noexcept
	{
		pso::destroy(h_pso_mask);
		pso::destroy(h_pso_draw);
	}
}	 // namespace age::graphics::render_pipeline::forward_plus

// stage ui
namespace age::graphics::render_pipeline::forward_plus
{
	void
	ui_stage::init(graphics::root_signature::handle h_root_sig) noexcept
	{
		using namespace graphics::pso;

		h_pso_screen = graphics::pso::create(
			pss_root_signature{ .subobj = graphics::g::root_signature_ptr_vec[h_root_sig] },
			pss_ms{ .subobj = shader::get_d3d12_bytecode(e::engine_shader_kind::forward_plus_ui_ms) },
			pss_ps{ .subobj = shader::get_d3d12_bytecode(e::engine_shader_kind::forward_plus_ui_ps) },
			pss_primitive_topology{ .subobj = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE },
			pss_render_target_formats{ .subobj = D3D12_RT_FORMAT_ARRAY{ .RTFormats{ DXGI_FORMAT_R16G16B16A16_FLOAT }, .NumRenderTargets = 1 } },
			pss_rasterizer{ .subobj = defaults::rasterizer_desc::no_cull },
			pss_blend{ .subobj = defaults::blend_desc::alpha },
			pss_sample_desc{ .subobj = DXGI_SAMPLE_DESC{ .Count = 1, .Quality = 0 } },
			pss_node_mask{ .subobj = 0 });

		p_pso_screen = graphics::g::pso_ptr_vec[h_pso_screen];
		h_pso_screen.set_name(L"pso_ui_screen");

		h_pso_world = graphics::pso::create(
			pss_root_signature{ .subobj = graphics::g::root_signature_ptr_vec[h_root_sig] },
			pss_ms{ .subobj = shader::get_d3d12_bytecode(e::engine_shader_kind::forward_plus_ui_ms) },
			pss_ps{ .subobj = shader::get_d3d12_bytecode(e::engine_shader_kind::forward_plus_ui_ps) },
			pss_primitive_topology{ .subobj = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE },
			pss_render_target_formats{ .subobj = D3D12_RT_FORMAT_ARRAY{ .RTFormats{ DXGI_FORMAT_R16G16B16A16_FLOAT }, .NumRenderTargets = 1 } },
			pss_rasterizer{ .subobj = defaults::rasterizer_desc::no_cull },
			pss_depth_stencil_format{ .subobj = DXGI_FORMAT_D32_FLOAT },
			pss_depth_stencil1{ .subobj = defaults::depth_stencil_desc1::depth_readonly_reversed },
			pss_blend{ .subobj = defaults::blend_desc::alpha },
			pss_sample_desc{ .subobj = DXGI_SAMPLE_DESC{ .Count = 1, .Quality = 0 } },
			pss_node_mask{ .subobj = 0 });

		p_pso_world = graphics::g::pso_ptr_vec[h_pso_world];
		h_pso_world.set_name(L"pso_ui_world");
	}

	inline void
	ui_stage::execute(binding_config_t::reg_b<1>&	  constants,
					  rtv_desc_handle				  h_rtv_desc,
					  dsv_desc_handle				  h_dsv_desc,
					  ui::e::space_mode_kind		  space_mode,
					  uint32						  root_data_idx,
					  uint32						  root_data_count,
					  const age::vector<util::range>& z_range_vec,
					  const age::vector<util::range>& z_range_range_vec) noexcept
	{
		auto render_pass_rt_desc = defaults::render_pass_rtv_desc::load_preserve(h_rtv_desc);
		if (space_mode == ui::e::space_mode_kind::screen
			or space_mode == ui::e::space_mode_kind::world_always_on_top)
		{
			command::begin_render_pass(
				1,
				&render_pass_rt_desc,
				nullptr,
				D3D12_RENDER_PASS_FLAG_NONE);

			command::set_pso(p_pso_screen);
		}
		else
		{
			auto render_pass_dsv_desc = defaults::render_pass_ds_desc::depth_load_preserve(h_dsv_desc);
			command::begin_render_pass(
				1,
				&render_pass_rt_desc,
				&render_pass_dsv_desc,
				D3D12_RENDER_PASS_FLAG_BIND_READ_ONLY_DEPTH);

			command::set_pso(p_pso_world);
		}


		for (auto i : views::loop(root_data_count))
		{
			c_auto root_idx = root_data_idx + i;

			c_auto& z_range_range = z_range_range_vec[root_idx];

			for (auto j : views::loop(z_range_range.count))
			{
				c_auto& z_range = z_range_vec[z_range_range.offset + j];

				struct
				{
					uint32 ui_space_mode_and_extra;
					uint32 ui_root_data_idx;
					uint32 ui_data_id_offset;
					uint32 ui_data_id_count;
				} payload{ to_idx(space_mode), root_idx, z_range.offset, z_range.count };

				constants.apply_graphics_member<&shared_type::root_constants::ui_space_mode_and_extra, sizeof(payload)>(payload);
				command::dispatch_mesh((z_range.count + 31u) / 32u, 1, 1);
			}
		}

		command::end_render_pass();
	}

	void
	ui_stage::deinit() noexcept
	{
		pso::destroy(h_pso_screen);
		pso::destroy(h_pso_world);
	}
}	 // namespace age::graphics::render_pipeline::forward_plus

// stage presentation
namespace age::graphics::render_pipeline::forward_plus
{
	void
	presentation_stage::init(root_signature::handle h_root_sig) noexcept
	{
		using namespace graphics::pso;

		auto ms_byte_code = shader::get_d3d12_bytecode(e::engine_shader_kind::forward_plus_presentation_ms);

		auto&& [ps_byte_code, back_buffer_rt_format] = [&]() {
			switch (graphics::i_color.get_display_color_space())
			{
			case color_space::srgb:
				return std::tuple{
					shader::get_d3d12_bytecode(e::engine_shader_kind::forward_plus_presentation_sdr_ps),
					DXGI_FORMAT_R8G8B8A8_UNORM_SRGB

				};
			case color_space::hdr:
				return std::tuple{
					shader::get_d3d12_bytecode(e::engine_shader_kind::forward_plus_presentation_hdr10_ps),
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
		h_pso.set_name(L"pso_presentation");
	}

	inline void
	presentation_stage::execute(render_surface& rs) noexcept
	{
		auto render_pass_rt_desc = defaults::render_pass_rtv_desc::overwrite_preserve(rs.h_rtv_desc());

		command::begin_render_pass(
			1,
			&render_pass_rt_desc,
			nullptr,
			D3D12_RENDER_PASS_FLAG_NONE);

		command::set_pso(p_pso);
		command::dispatch_mesh(1, 1, 1);

		command::end_render_pass();
	}

	void
	presentation_stage::deinit() noexcept
	{
		pso::destroy(h_pso);
	}
}	 // namespace age::graphics::render_pipeline::forward_plus
#endif
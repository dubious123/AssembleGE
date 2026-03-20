#include "age_pch.hpp"
#include "age.hpp"
#if defined(AGE_GRAPHICS_BACKEND_DX12)
// stage depth
namespace age::graphics::render_pipeline::forward_plus
{
	inline void
	depth_stage::init(graphics::root_signature::handle h_root_sig) noexcept
	{
		using namespace graphics::pso;

		auto as_byte_code = shader::get_d3d12_bytecode(shader::shader_handle{ std::to_underlying(e::engine_shader_kind::forward_plus_opaque_as) });
		auto ms_byte_code = shader::get_d3d12_bytecode(shader::shader_handle{ std::to_underlying(e::engine_shader_kind::forward_plus_depth_ms) });

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
	depth_stage::execute(uint32 opaque_meshlet_count) noexcept
	{
		c_auto render_pass_ds_desc = defaults::render_pass_ds_desc::depth_clear_preserve(h_depth_buffer_dsv_desc, 0.f);

		command::begin_render_pass(
			0,			// no render targets
			nullptr,	// no render targets
			&render_pass_ds_desc,
			D3D12_RENDER_PASS_FLAG_NONE);

		{
			command::set_pso(p_pso);

			if (opaque_meshlet_count > 0) [[likely]]
			{
				command::dispatch_mesh((opaque_meshlet_count + 31u) / 32u, 1, 1);
			}
		}

		command::end_render_pass();
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

		h_init_pso = graphics::pso::create(
			pss_root_signature{ .subobj = graphics::g::root_signature_ptr_vec[h_root_sig] },
			pss_cs{ .subobj = shader::get_d3d12_bytecode(shader::shader_handle{ std::to_underlying(e::engine_shader_kind::forward_plus_shadow_init_cs) }) });

		p_init_pso = graphics::g::pso_ptr_vec[h_init_pso];

		h_depth_reduce_pso = graphics::pso::create(
			pss_root_signature{ .subobj = graphics::g::root_signature_ptr_vec[h_root_sig] },
			pss_cs{ .subobj = shader::get_d3d12_bytecode(shader::shader_handle{ std::to_underlying(e::engine_shader_kind::forward_plus_shadow_depth_reduce_cs) }) });

		p_depth_reduce_pso = graphics::g::pso_ptr_vec[h_depth_reduce_pso];

		h_fill_shadow_buffer_pso = graphics::pso::create(
			pss_root_signature{ .subobj = graphics::g::root_signature_ptr_vec[h_root_sig] },
			pss_cs{ .subobj = shader::get_d3d12_bytecode(shader::shader_handle{ std::to_underlying(e::engine_shader_kind::forward_plus_shadow_fill_shadow_buffer_cs) }) });

		p_fill_shadow_buffer_pso = graphics::g::pso_ptr_vec[h_fill_shadow_buffer_pso];

		h_shadow_map_pso = graphics::pso::create(
			pss_root_signature{ .subobj = graphics::g::root_signature_ptr_vec[h_root_sig] },
			pss_as{ .subobj = shader::get_d3d12_bytecode(shader::shader_handle{ std::to_underlying(e::engine_shader_kind::forward_plus_shadow_as) }) },
			pss_ms{ .subobj = shader::get_d3d12_bytecode(shader::shader_handle{ std::to_underlying(e::engine_shader_kind::forward_plus_shadow_ms) }) },
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
	shadow_stage::execute(uint32		  width,
						  uint32		  height,
						  uint32		  shadow_light_count,
						  uint32		  shadow_light_header_count,
						  uint32		  opaque_meshlet_count,
						  resource_handle h_shadow_stage_buffer,
						  auto&			  shadow_stage_buffer_srv,
						  resource_handle h_shadow_stage_shadow_light_buffer,
						  auto&			  shadow_stage_shadow_light_buffer_srv) noexcept
	{
		if (opaque_meshlet_count == 0) [[unlikely]]
		{
			return;
		}


		command::set_pso(p_init_pso);
		command::dispatch(1, 1, 1);

		command::apply_barriers(barrier::buf_uav_to_uav(h_shadow_stage_buffer->p_resource));

		command::set_pso(p_depth_reduce_pso);
		command::dispatch(
			(width + SHADOW_CS_DEPTH_REDUCE_THREAD_COUNT - 1) / SHADOW_CS_DEPTH_REDUCE_THREAD_COUNT,
			(height + SHADOW_CS_DEPTH_REDUCE_THREAD_COUNT - 1) / SHADOW_CS_DEPTH_REDUCE_THREAD_COUNT,
			1);

		command::apply_barriers(barrier::buf_uav_to_uav(h_shadow_stage_buffer->p_resource));

		command::set_pso(p_fill_shadow_buffer_pso);
		command::dispatch(shadow_light_header_count, 1, 1);

		command::apply_barriers(barrier::buf_uav_to_srv(h_shadow_stage_buffer->p_resource, D3D12_BARRIER_SYNC_VERTEX_SHADING),
								barrier::buf_uav_to_srv(h_shadow_stage_shadow_light_buffer->p_resource, D3D12_BARRIER_SYNC_VERTEX_SHADING | D3D12_BARRIER_SYNC_PIXEL_SHADING));

		shadow_stage_buffer_srv.apply();
		shadow_stage_shadow_light_buffer_srv.apply();

		c_auto render_pass_ds_desc = defaults::render_pass_ds_desc::depth_clear_preserve(h_shadow_atlas_dsv_desc, 0.f);

		command::begin_render_pass(
			0,			// no render targets
			nullptr,	// no render targets
			&render_pass_ds_desc,
			D3D12_RENDER_PASS_FLAG_NONE);

		command::set_pso(p_shadow_map_pso);

		for (auto shadow_id : std::views::iota(0u) | std::views::take(shadow_light_count))
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

			command::set_view_ports(1, &viewport);
			command::set_scissor_rects(1, &scissor);

			command::set_graphics_root_constants(
				binding_config_t::reg_b<1>::slot_id,
				static_cast<uint32>(sizeof(uint32) / 4),
				&shadow_id,
				static_cast<uint32>(offsetof(shared_type::root_constants, shadow_light_index) / 4));

			command::dispatch_mesh((opaque_meshlet_count + 31u) / 32u, 1, 1);
		}

		command::end_render_pass();
	}

	inline void
	shadow_stage::deinit() noexcept
	{
		pso::destroy(h_init_pso);
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

		h_pso_init = graphics::pso::create(
			pss_root_signature{ .subobj = graphics::g::root_signature_ptr_vec[h_root_sig] },
			pss_cs{ .subobj = shader::get_d3d12_bytecode(shader::shader_handle{ std::to_underlying(e::engine_shader_kind::forward_plus_light_init_cs) }) });

		p_pso_init = graphics::g::pso_ptr_vec[h_pso_init];


		h_pso_cull = graphics::pso::create(
			pss_root_signature{ .subobj = graphics::g::root_signature_ptr_vec[h_root_sig] },
			pss_cs{ .subobj = shader::get_d3d12_bytecode(shader::shader_handle{ std::to_underlying(e::engine_shader_kind::forward_plus_light_cull_cs) }) });

		p_pso_cull = graphics::g::pso_ptr_vec[h_pso_cull];

		h_pso_sort_histogram = graphics::pso::create(
			pss_root_signature{ .subobj = graphics::g::root_signature_ptr_vec[h_root_sig] },
			pss_cs{ .subobj = shader::get_d3d12_bytecode(shader::shader_handle{ std::to_underlying(e::engine_shader_kind::forward_plus_sort_histogram_cs) }) });

		p_pso_sort_histogram = graphics::g::pso_ptr_vec[h_pso_sort_histogram];

		h_pso_sort_prefix = graphics::pso::create(
			pss_root_signature{ .subobj = graphics::g::root_signature_ptr_vec[h_root_sig] },
			pss_cs{ .subobj = shader::get_d3d12_bytecode(shader::shader_handle{ std::to_underlying(e::engine_shader_kind::forward_plus_sort_prefix_cs) }) });

		p_pso_sort_prefix = graphics::g::pso_ptr_vec[h_pso_sort_prefix];

		h_pso_sort_scatter = graphics::pso::create(
			pss_root_signature{ .subobj = graphics::g::root_signature_ptr_vec[h_root_sig] },
			pss_cs{ .subobj = shader::get_d3d12_bytecode(shader::shader_handle{ std::to_underlying(e::engine_shader_kind::forward_plus_sort_scatter_cs) }) });

		p_pso_sort_scatter = graphics::g::pso_ptr_vec[h_pso_sort_scatter];

		h_pso_zbin = graphics::pso::create(
			pss_root_signature{ .subobj = graphics::g::root_signature_ptr_vec[h_root_sig] },
			pss_cs{ .subobj = shader::get_d3d12_bytecode(shader::shader_handle{ std::to_underlying(e::engine_shader_kind::forward_plus_light_zbin_cs) }) });

		p_pso_zbin = graphics::g::pso_ptr_vec[h_pso_zbin];

		h_pso_tile = graphics::pso::create(
			pss_root_signature{ .subobj = graphics::g::root_signature_ptr_vec[h_root_sig] },
			pss_cs{ .subobj = shader::get_d3d12_bytecode(shader::shader_handle{ std::to_underlying(e::engine_shader_kind::forward_plus_light_tile_cs) }) });

		p_pso_tile = graphics::g::pso_ptr_vec[h_pso_tile];
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
		command::dispatch((g::max_sort_count + g::light_cull_cs_thread_count - 1) / g::light_cull_cs_thread_count, 1, 1);

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

	inline void
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
	inline void
	opaque_stage::init(graphics::root_signature::handle h_root_sig) noexcept
	{
		using namespace graphics::pso;

		auto as_byte_code = shader::get_d3d12_bytecode(shader::shader_handle{ std::to_underlying(e::engine_shader_kind::forward_plus_opaque_as) });
		auto ms_byte_code = shader::get_d3d12_bytecode(shader::shader_handle{ std::to_underlying(e::engine_shader_kind::forward_plus_opaque_ms) });
		auto ps_byte_code = shader::get_d3d12_bytecode(shader::shader_handle{ std::to_underlying(e::engine_shader_kind::forward_plus_opaque_ps) });

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
	opaque_stage::execute(uint32 meshlet_count) noexcept
	{
		auto render_pass_rt_desc = defaults::render_pass_rtv_desc::clear_preserve(h_main_buffer_rtv_desc, nullptr);
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
			pss_cs{ .subobj = shader::get_d3d12_bytecode(shader::shader_handle{ std::to_underlying(e::engine_shader_kind::forward_plus_transparent_cull_cs) }) });

		p_pso_cull = graphics::g::pso_ptr_vec[h_pso_cull];

		h_pso_sort_histogram = graphics::pso::create(
			pss_root_signature{ .subobj = graphics::g::root_signature_ptr_vec[h_root_sig] },
			pss_cs{ .subobj = shader::get_d3d12_bytecode(shader::shader_handle{ std::to_underlying(e::engine_shader_kind::forward_plus_sort_histogram_cs) }) });

		p_pso_sort_histogram = graphics::g::pso_ptr_vec[h_pso_sort_histogram];

		h_pso_sort_prefix = graphics::pso::create(
			pss_root_signature{ .subobj = graphics::g::root_signature_ptr_vec[h_root_sig] },
			pss_cs{ .subobj = shader::get_d3d12_bytecode(shader::shader_handle{ std::to_underlying(e::engine_shader_kind::forward_plus_sort_prefix_cs) }) });

		p_pso_sort_prefix = graphics::g::pso_ptr_vec[h_pso_sort_prefix];

		h_pso_sort_scatter = graphics::pso::create(
			pss_root_signature{ .subobj = graphics::g::root_signature_ptr_vec[h_root_sig] },
			pss_cs{ .subobj = shader::get_d3d12_bytecode(shader::shader_handle{ std::to_underlying(e::engine_shader_kind::forward_plus_sort_scatter_cs) }) });

		p_pso_sort_scatter = graphics::g::pso_ptr_vec[h_pso_sort_scatter];

		h_pso_transparent_gen_indirect_arg = graphics::pso::create(
			pss_root_signature{ .subobj = graphics::g::root_signature_ptr_vec[h_root_sig] },
			pss_cs{ .subobj = shader::get_d3d12_bytecode(shader::shader_handle{ std::to_underlying(e::engine_shader_kind::forward_plus_transparent_gen_indirect_arg_cs) }) });

		p_pso_transparent_gen_indirect_arg = graphics::g::pso_ptr_vec[h_pso_transparent_gen_indirect_arg];

		h_pso_draw = graphics::pso::create(
			pss_root_signature{ .subobj = graphics::g::root_signature_ptr_vec[h_root_sig] },
			pss_as{ .subobj = shader::get_d3d12_bytecode(shader::shader_handle{ std::to_underlying(e::engine_shader_kind::forward_plus_transparent_as) }) },
			pss_ms{ .subobj = shader::get_d3d12_bytecode(shader::shader_handle{ std::to_underlying(e::engine_shader_kind::forward_plus_transparent_ms) }) },
			pss_ps{ .subobj = shader::get_d3d12_bytecode(shader::shader_handle{ std::to_underlying(e::engine_shader_kind::forward_plus_transparent_ps) }) },
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
	transparent_stage::execute(resource_handle h_scratch_buffer,
							   resource_handle transparent_stage_buffer) noexcept
	{
		command::set_pso(p_pso_cull);
		command::dispatch((g::max_sort_count + g::transparent_cull_thread_count - 1) / g::transparent_cull_thread_count, 1, 1);

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

		command::set_pso(p_pso_transparent_gen_indirect_arg);
		command::dispatch((g::max_transparent_object_count + 31) / 32, 1, 1);

		command::apply_barriers(barrier::buf_uav_to_indirect(transparent_stage_buffer->p_resource));

		auto render_pass_rt_desc = defaults::render_pass_rtv_desc::load_preserve(h_main_buffer_rtv_desc);
		auto render_pass_ds_desc = defaults::render_pass_ds_desc::depth_load_preserve(h_depth_buffer_dsv_desc);

		command::begin_render_pass(
			1,
			&render_pass_rt_desc,
			&render_pass_ds_desc,
			D3D12_RENDER_PASS_FLAG_BIND_READ_ONLY_DEPTH);

		command::set_pso(p_pso_draw);

		command::execute_indirect(
			p_draw_cmd_sig,								 // command signature
			g::max_transparent_object_count,			 // max command count
			transparent_stage_buffer->p_resource,		 // argument buffer
			TRANSPARENT_EXECUTE_INDIRECT_ARG_OFFSET,	 // argument buffer byte offset
			transparent_stage_buffer->p_resource,		 // count buffer
			TRANSPARENT_VISIBLE_OBJECT_COUNT_OFFSET);	 // count buffer bytes offset

		command::end_render_pass();
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

		auto ms_byte_code = shader::get_d3d12_bytecode(shader::shader_handle{ std::to_underlying(e::engine_shader_kind::forward_plus_presentation_ms) });

		auto&& [ps_byte_code, back_buffer_rt_format] = [&]() {
			switch (graphics::i_color.get_display_color_space())
			{
			case color_space::srgb:
				return std::tuple{
					shader::get_d3d12_bytecode(shader::shader_handle{ std::to_underlying(e::engine_shader_kind::forward_plus_presentation_sdr_ps) }),
					DXGI_FORMAT_R8G8B8A8_UNORM_SRGB

				};
			case color_space::hdr:
				return std::tuple{
					shader::get_d3d12_bytecode(shader::shader_handle{ std::to_underlying(e::engine_shader_kind::forward_plus_presentation_hdr10_ps) }),
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
	presentation_stage::execute(render_surface& rs) noexcept
	{
		auto render_pass_rt_desc = defaults::render_pass_rtv_desc::overwrite_preserve(rs.h_rtv_desc());

		command::begin_render_pass(
			1,
			&render_pass_rt_desc,
			nullptr,
			D3D12_RENDER_PASS_FLAG_NONE);

		{
			command::set_pso(p_pso);
			command::dispatch_mesh(1, 1, 1);
		}

		command::end_render_pass();
	}

	inline void
	presentation_stage::deinit() noexcept
	{
		pso::destroy(h_pso);
	}
}	 // namespace age::graphics::render_pipeline::forward_plus
#endif
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
	light_bin_stage::init(graphics::root_signature::handle h_root_sig) noexcept
	{
		using namespace graphics::pso;

		h_pso_init = graphics::pso::create(
			pss_root_signature{ .subobj = graphics::g::root_signature_ptr_vec[h_root_sig] },
			pss_cs{ .subobj = shader::get_d3d12_bytecode(e::engine_shader_kind::forward_plus_light_init_cs) });

		p_pso_init = graphics::g::pso_ptr_vec[h_pso_init];
		h_pso_init.set_name(L"p_pso_light_init");

		h_pso_light_sort_prepare = graphics::pso::create(
			pss_root_signature{ .subobj = graphics::g::root_signature_ptr_vec[h_root_sig] },
			pss_cs{ .subobj = shader::get_d3d12_bytecode(e::engine_shader_kind::forward_plus_light_sort_prepare_cs) });

		p_pso_light_sort_prepare = graphics::g::pso_ptr_vec[h_pso_light_sort_prepare];
		h_pso_light_sort_prepare.set_name(L"pso_light_sort_prepare");

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
		h_pso_zbin.set_name(L"p_pso_light_zbin");
	}

	inline void
	light_bin_stage::execute(uint32			 unified_light_count,
							 resource_handle h_light_bin_stage_buffer,
							 resource_handle h_scratch_buffer) const noexcept
	{
		command::set_pso(p_pso_init);
		command::dispatch(util::ceil(g::light_axis_slice_sum + g::light_axis_slice_sum * g::light_bitmask_uint32_count, g::light_bin_init_thread_count), 1, 1);

		command::set_pso(p_pso_light_sort_prepare);
		command::dispatch(util::ceil(g::max_sort_count, g::light_cull_thread_count), 1, 1);

		command::apply_barriers(
			barrier::buf_uav_to_uav(h_light_bin_stage_buffer->p_resource),
			barrier::buf_uav_to_uav(h_scratch_buffer->p_resource));

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
		command::dispatch(util::ceil(unified_light_count, g::zbin_thread_count), 1, 1);

		// command::apply_barriers(barrier::buf_uav_to_uav(h_scratch_buffer->p_resource));

		// command::set_pso(p_pso_tile);
		// command::dispatch(light_tile_count_x, light_tile_count_y, 1);
	}

	void
	light_bin_stage::deinit() noexcept
	{
		pso::destroy(h_pso_init);
		pso::destroy(h_pso_light_sort_prepare);

		pso::destroy(h_pso_sort_histogram);
		pso::destroy(h_pso_sort_prefix);
		pso::destroy(h_pso_sort_scatter);

		pso::destroy(h_pso_zbin);
	}
}	 // namespace age::graphics::render_pipeline::forward_plus

// stage ddgi
namespace age::graphics::render_pipeline::forward_plus
{
	void
	ddgi_stage::init(graphics::root_signature::handle h_root_sig) noexcept
	{
		using namespace graphics::pso;

		h_pso_update_probe_state = graphics::pso::create(
			pss_root_signature{ .subobj = graphics::g::root_signature_ptr_vec[h_root_sig] },
			pss_cs{ .subobj = shader::get_d3d12_bytecode(e::engine_shader_kind::forward_plus_ddgi_update_probe_state_cs) });

		p_pso_update_probe_state = graphics::g::pso_ptr_vec[h_pso_update_probe_state];
		h_pso_update_probe_state.set_name(L"pso_ddgi_update_probe_state");

		h_pso_reduce_ray_sum = graphics::pso::create(
			pss_root_signature{ .subobj = graphics::g::root_signature_ptr_vec[h_root_sig] },
			pss_cs{ .subobj = shader::get_d3d12_bytecode(e::engine_shader_kind::forward_plus_ddgi_reduce_ray_sum_cs) });

		p_pso_reduce_ray_sum = graphics::g::pso_ptr_vec[h_pso_reduce_ray_sum];
		h_pso_reduce_ray_sum.set_name(L"pso_ddgi_reduce_ray_sum");


		h_pso_prefix_group = graphics::pso::create(
			pss_root_signature{ .subobj = graphics::g::root_signature_ptr_vec[h_root_sig] },
			pss_cs{ .subobj = shader::get_d3d12_bytecode(e::engine_shader_kind::forward_plus_ddgi_prefix_group_cs) });

		p_pso_prefix_group = graphics::g::pso_ptr_vec[h_pso_prefix_group];
		h_pso_prefix_group.set_name(L"pso_ddgi_prefix_group");


		h_pso_prefix_group_sum = graphics::pso::create(
			pss_root_signature{ .subobj = graphics::g::root_signature_ptr_vec[h_root_sig] },
			pss_cs{ .subobj = shader::get_d3d12_bytecode(e::engine_shader_kind::forward_plus_ddgi_prefix_group_sum_cs) });

		p_pso_prefix_group_sum = graphics::g::pso_ptr_vec[h_pso_prefix_group_sum];
		h_pso_prefix_group_sum.set_name(L"pso_ddgi_prefix_group_sum");


		h_pso_prefix_add = graphics::pso::create(
			pss_root_signature{ .subobj = graphics::g::root_signature_ptr_vec[h_root_sig] },
			pss_cs{ .subobj = shader::get_d3d12_bytecode(e::engine_shader_kind::forward_plus_ddgi_prefix_add_cs) });


		p_pso_prefix_add = graphics::g::pso_ptr_vec[h_pso_prefix_add];
		h_pso_prefix_add.set_name(L"pso_ddgi_prefix_add");

		h_pso_probe_trace = graphics::pso::create(
			pss_root_signature{ .subobj = graphics::g::root_signature_ptr_vec[h_root_sig] },
			pss_cs{ .subobj = shader::get_d3d12_bytecode(e::engine_shader_kind::forward_plus_ddgi_probe_trace_cs) });

		p_pso_probe_trace = graphics::g::pso_ptr_vec[h_pso_probe_trace];
		h_pso_probe_trace.set_name(L"pso_ddgi_probe_trace");

		h_cmd_sig_probe_trace = graphics::command_signature::create<uint32_3>(graphics::defaults::cmd_sig::dispatch_compute);
		p_cmd_sig_probe_trace = h_cmd_sig_probe_trace.ptr();

		h_pso_probe_blend = graphics::pso::create(
			pss_root_signature{ .subobj = graphics::g::root_signature_ptr_vec[h_root_sig] },
			pss_cs{ .subobj = shader::get_d3d12_bytecode(e::engine_shader_kind::forward_plus_ddgi_probe_blend_cs) });

		p_pso_probe_blend = graphics::g::pso_ptr_vec[h_pso_probe_blend];
		h_pso_probe_blend.set_name(L"pso_ddgi_probe_blend");

		h_pso_copy_edge = graphics::pso::create(
			pss_root_signature{ .subobj = graphics::g::root_signature_ptr_vec[h_root_sig] },
			pss_cs{ .subobj = shader::get_d3d12_bytecode(e::engine_shader_kind::forward_plus_ddgi_copy_edge_cs) });

		p_pso_copy_edge = graphics::g::pso_ptr_vec[h_pso_copy_edge];
		h_pso_copy_edge.set_name(L"pso_ddgi_copy_edge");


		h_pso_render_probes = graphics::pso::create(
			pss_root_signature{ .subobj = graphics::g::root_signature_ptr_vec[h_root_sig] },
			pss_as{ .subobj = shader::get_d3d12_bytecode(e::engine_shader_kind::forward_plus_ddgi_render_probes_as) },
			pss_ms{ .subobj = shader::get_d3d12_bytecode(e::engine_shader_kind::forward_plus_ddgi_render_probes_ms) },
			pss_ps{ .subobj = shader::get_d3d12_bytecode(e::engine_shader_kind::forward_plus_ddgi_render_probes_ps) },
			pss_primitive_topology{ .subobj = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE },
			pss_render_target_formats{ .subobj = D3D12_RT_FORMAT_ARRAY{ .RTFormats{ DXGI_FORMAT_R16G16B16A16_FLOAT }, .NumRenderTargets = 1 } },
			pss_depth_stencil_format{ .subobj = DXGI_FORMAT_D16_UNORM },
			// pss_rasterizer{ .subobj = defaults::rasterizer_desc::backface_cull },
			pss_rasterizer{ .subobj = defaults::rasterizer_desc::backface_cull },
			pss_depth_stencil1{ .subobj = defaults::depth_stencil_desc1::depth_only_reversed },
			pss_blend{ .subobj = defaults::blend_desc::opaque },
			pss_sample_desc{ .subobj = DXGI_SAMPLE_DESC{ .Count = 1, .Quality = 0 } },
			pss_node_mask{ .subobj = 0 });

		p_pso_render_probes = graphics::g::pso_ptr_vec[h_pso_render_probes];
		h_pso_render_probes.set_name(L"pso_ddgi_render_probes");
	}

	inline void
	ddgi_stage::execute(const ddgi_data&			   ddgi_data_cpu,
						binding_config_t::reg_t<1, 7>& probe_buffer_srv,
						resource_handle				   h_indirect_arg_buffer) const noexcept
	{
		c_auto& ddgi_data_gpu = ddgi_data_cpu.ddgi_data_gpu;

		c_auto ppl		   = 1u << ((ddgi_data_gpu.ppl_log_2_and_ppl_bitwidth >> 24u) & 0xff);
		c_auto level_count = ddgi_data_gpu.level_count__tile_count_w_log2 & 0xff;
		c_auto probe_count = ppl * level_count;

		command::set_pso(p_pso_update_probe_state);

		command::dispatch(util::ceil(ppl, g::ddgi_update_probe_state_probe_per_group), 1, level_count);

		command::apply_barriers(barrier::buf_uav_to_uav(ddgi_data_cpu.h_ddgi_scratch_buffer->p_resource),
								barrier::buf_uav_to_srv(ddgi_data_cpu.h_probe_buffer->p_resource, D3D12_BARRIER_SYNC_COMPUTE_SHADING));

		command::set_pso(p_pso_reduce_ray_sum);

		// todo: config wave_count
		c_auto group_count_x   = util::ceil(ppl, g::ddgi_update_probe_state_probe_per_group);
		c_auto group_sum_count = group_count_x * level_count;
		c_auto sum_count	   = util::ceil(group_sum_count, 32);
		command::dispatch(sum_count, 1, 1);
		command::apply_barriers(barrier::buf_uav_to_uav(ddgi_data_cpu.h_ddgi_scratch_buffer->p_resource));

		command::set_pso(p_pso_prefix_group);
		c_auto group_count = util::ceil(probe_count, g::ddgi_prefix_element_per_group);
		AGE_ASSERT(group_count <= g::ddgi_prefix_element_per_group);

		command::dispatch(group_count, 1, 1);
		command::apply_barriers(barrier::buf_uav_to_uav(ddgi_data_cpu.h_ddgi_scratch_buffer->p_resource));

		command::set_pso(p_pso_prefix_group_sum);
		command::dispatch(1, 1, 1);
		command::apply_barriers(barrier::buf_uav_to_uav(ddgi_data_cpu.h_ddgi_scratch_buffer->p_resource));

		command::set_pso(p_pso_prefix_add);
		command::dispatch(group_count, 1, 1);
		command::apply_barriers(barrier::buf_uav_to_uav(ddgi_data_cpu.h_ddgi_scratch_buffer->p_resource),
								barrier::buf_uav_to_indirect(h_indirect_arg_buffer->p_resource));

		probe_buffer_srv.apply_compute();
		command::set_pso(p_pso_probe_trace);
		command::execute_indirect(p_cmd_sig_probe_trace, 1u, h_indirect_arg_buffer->p_resource, 0ull, nullptr, 0ull);

		command::apply_barriers(
			barrier::tex_srv_to_uav(ddgi_data_cpu.h_irradiance_atlas->p_resource, D3D12_BARRIER_SYNC_COMPUTE_SHADING | D3D12_BARRIER_SYNC_PIXEL_SHADING),
			barrier::tex_srv_to_uav(ddgi_data_cpu.h_visibility_atlas->p_resource, D3D12_BARRIER_SYNC_COMPUTE_SHADING | D3D12_BARRIER_SYNC_PIXEL_SHADING),
			barrier::buf_srv_to_uav(ddgi_data_cpu.h_probe_buffer->p_resource, D3D12_BARRIER_SYNC_COMPUTE_SHADING),
			barrier::buf_uav_to_uav(ddgi_data_cpu.h_ddgi_scratch_buffer->p_resource));

		command::set_pso(p_pso_probe_blend);

		{
			probe_buffer_srv.apply_compute();
			c_auto axis_x = 1u << ((ddgi_data_gpu.ppl_log_2_and_ppl_bitwidth >> 0u) & 0xff);
			c_auto axis_y = 1u << ((ddgi_data_gpu.ppl_log_2_and_ppl_bitwidth >> 8u) & 0xff);
			c_auto axis_z = 1u << ((ddgi_data_gpu.ppl_log_2_and_ppl_bitwidth >> 16u) & 0xff);

			AGE_ASSERT(axis_z * level_count <= 0xffff);
			command::dispatch(axis_x, axis_y, axis_z * level_count);
		}

		command::apply_barriers(barrier::tex_uav_to_uav(ddgi_data_cpu.h_irradiance_atlas->p_resource),
								barrier::tex_uav_to_uav(ddgi_data_cpu.h_visibility_atlas->p_resource));


		command::set_pso(p_pso_copy_edge);
		{
			const uint32 dispatch_x		   = ddgi_data_cpu.irradiance_atlas_extent.width / g::ddgi_irradiance_tile_size;
			const uint32 dispatch_y		   = ddgi_data_cpu.irradiance_atlas_extent.height / g::ddgi_irradiance_tile_size;
			const uint32 thread_group_x	   = g::ddgi_irradiance_resolution;
			const uint32 thread_group_y	   = 4;
			const uint32 irradiance_edge_z = thread_group_y * (g::ddgi_irradiance_resolution / thread_group_x);
			const uint32 visibility_edge_z = thread_group_y * (g::ddgi_visibility_resolution / thread_group_x);
			const uint32 dispatch_z		   = 1 + irradiance_edge_z + visibility_edge_z;

			AGE_ASSERT(is_even(dispatch_x) and is_even(dispatch_y));

			command::dispatch(dispatch_x, dispatch_y, dispatch_z);
		}
	}

	void
	ddgi_stage::execute_render_probes(rtv_desc_handle  h_main_buffer_rtv_desc,
									  dsv_desc_handle  h_debug_depth_buffer_dsv_desc,
									  const ddgi_data& ddgi_data_cpu) const noexcept
	{
		c_auto& ddgi_data_gpu = ddgi_data_cpu.ddgi_data_gpu;

		c_auto ppl		   = 1u << ((ddgi_data_gpu.ppl_log_2_and_ppl_bitwidth >> 24u) & 0xff);
		c_auto level_count = ddgi_data_gpu.level_count__tile_count_w_log2 & 0xff;

		if (ddgi_data_cpu.render_probes)
		{
			auto render_pass_rt_desc = defaults::render_pass_rtv_desc::load_preserve(h_main_buffer_rtv_desc);
			auto render_pass_ds_desc = defaults::render_pass_ds_desc::depth_clear_discard(h_debug_depth_buffer_dsv_desc, 0.f, DXGI_FORMAT_D16_UNORM);

			command::begin_render_pass(
				1,
				&render_pass_rt_desc,
				&render_pass_ds_desc,
				D3D12_RENDER_PASS_FLAG_NONE);

			{
				command::set_pso(p_pso_render_probes);
				AGE_ASSERT(ppl % 32 == 0);
				command::dispatch_mesh(util::ceil(ppl, 32), 1, level_count);
			}

			command::end_render_pass();
		}
	}

	void
	ddgi_stage::deinit() noexcept
	{
		pso::destroy(h_pso_update_probe_state);
		pso::destroy(h_pso_reduce_ray_sum);
		pso::destroy(h_pso_prefix_group);
		pso::destroy(h_pso_prefix_group_sum);
		pso::destroy(h_pso_prefix_add);
		pso::destroy(h_pso_probe_trace);
		pso::destroy(h_pso_probe_blend);
		pso::destroy(h_pso_copy_edge);
		pso::destroy(h_pso_render_probes);

		command_signature::destroy(h_cmd_sig_probe_trace);
	}
}	 // namespace age::graphics::render_pipeline::forward_plus

// stage gibs
namespace age::graphics::render_pipeline::forward_plus
{
	void
	gibs_stage::init(graphics::root_signature::handle h_root_sig) noexcept
	{
		using namespace graphics::pso;

		h_pso_depth_prepass = graphics::pso::create(
			pss_root_signature{ .subobj = graphics::g::root_signature_ptr_vec[h_root_sig] },
			pss_as{ .subobj = shader::get_d3d12_bytecode(e::engine_shader_kind::forward_plus_opaque_as) },
			pss_ms{ .subobj = shader::get_d3d12_bytecode(e::engine_shader_kind::forward_plus_gibs_depth_prepass_ms) },
			pss_ps{ .subobj = shader::get_d3d12_bytecode(e::engine_shader_kind::forward_plus_gibs_depth_prepass_ps) },
			pss_primitive_topology{ .subobj = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE },
			pss_render_target_formats{ .subobj = D3D12_RT_FORMAT_ARRAY{ .RTFormats{ DXGI_FORMAT_R32G32_UINT }, .NumRenderTargets = 1 } },
			pss_depth_stencil_format{ .subobj = DXGI_FORMAT_D32_FLOAT },
			pss_rasterizer{ .subobj = defaults::rasterizer_desc::backface_cull },
			pss_depth_stencil1{ .subobj = defaults::depth_stencil_desc1::depth_only_reversed },
			pss_sample_desc{ .subobj = DXGI_SAMPLE_DESC{ .Count = 1, .Quality = 0 } },
			pss_node_mask{ .subobj = 0 });

		p_pso_depth_prepass = graphics::g::pso_ptr_vec[h_pso_depth_prepass];

		h_pso_depth_prepass.set_name(L"pso_gibs_depth_prepass");
	}

	inline void
	gibs_stage::execute_depth_prepass(const gibs_data& gibs_data_cpu,
									  dsv_desc_handle  h_depth_buffer_dsv_desc,
									  uint32		   opaque_meshlet_count) const noexcept
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

		c_auto render_pass_rt_desc = defaults::render_pass_rtv_desc::overwrite_preserve(gibs_data_cpu.h_gbuffer_rtv_desc);
		c_auto render_pass_ds_desc = defaults::render_pass_ds_desc::depth_clear_preserve(h_depth_buffer_dsv_desc, 0.f);

		command::begin_render_pass(
			1,
			&render_pass_rt_desc,
			&render_pass_ds_desc,
			D3D12_RENDER_PASS_FLAG_NONE);

		command::set_pso(p_pso_depth_prepass);

		command::dispatch_mesh(util::ceil(opaque_meshlet_count, 32u), 1, 1);

		command::end_render_pass();
	}

	inline void
	gibs_stage::execute(const gibs_data& gibs_data_cpu,
						resource_handle	 h_indirect_arg_buffer) const noexcept
	{
	}

	inline void
	gibs_stage::execute_render_surfels(rtv_desc_handle	h_main_buffer_rtv_desc,
									   dsv_desc_handle	h_depth_buffer_dsv_desc,
									   const gibs_data& gibs_data_cpu) const noexcept
	{
	}

	void
	gibs_stage::deinit() noexcept
	{
		pso::destroy(h_pso_depth_prepass);
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
	bloom_stage::init(graphics::root_signature::handle h_root_sig) noexcept
	{
		using namespace graphics::pso;

		h_pso_prefilter = graphics::pso::create(
			pss_root_signature{ .subobj = graphics::g::root_signature_ptr_vec[h_root_sig] },
			pss_cs{ .subobj = shader::get_d3d12_bytecode(e::engine_shader_kind::forward_plus_bloom_prefilter_cs) });

		p_pso_prefilter = graphics::g::pso_ptr_vec[h_pso_prefilter];
		h_pso_prefilter.set_name(L"pso_bloom_prefilter");

		h_pso_downsample = graphics::pso::create(
			pss_root_signature{ .subobj = graphics::g::root_signature_ptr_vec[h_root_sig] },
			pss_cs{ .subobj = shader::get_d3d12_bytecode(e::engine_shader_kind::forward_plus_bloom_downsample_cs) });

		p_pso_downsample = graphics::g::pso_ptr_vec[h_pso_downsample];
		h_pso_downsample.set_name(L"pso_bloom_down_sample");

		h_pso_upsample = graphics::pso::create(
			pss_root_signature{ .subobj = graphics::g::root_signature_ptr_vec[h_root_sig] },
			pss_cs{ .subobj = shader::get_d3d12_bytecode(e::engine_shader_kind::forward_plus_bloom_upsample_cs) });

		p_pso_upsample = graphics::g::pso_ptr_vec[h_pso_upsample];
		h_pso_upsample.set_name(L"pso_bloom_upsample");
	}

	inline void
	bloom_stage::execute(binding_config_t::reg_b<1>& constants, resource_handle h_bloom_chain, uint16 mip_count, const shared_type::bloom& bloom_gpu) noexcept
	{
		AGE_ASSERT(mip_count > 0);

		command::set_pso(p_pso_prefilter);
		command::apply_barriers(barrier::tex_srv_to_uav(h_bloom_chain->p_resource, D3D12_BARRIER_SYNC_PIXEL_SHADING, D3D12_BARRIER_SYNC_COMPUTE_SHADING, {}, barrier::tex2d_mip(0)),
								barrier::tex_srv_to_uav(h_bloom_chain->p_resource, D3D12_BARRIER_SYNC_COMPUTE_SHADING, D3D12_BARRIER_SYNC_COMPUTE_SHADING, {}, barrier::tex2d_mip_range(1, mip_count - 1)));
		command::dispatch(util::ceil(bloom_gpu.width, 8), util::ceil(bloom_gpu.height, 8), 1);

		command::set_pso(p_pso_downsample);
		for (auto mip_dst : views::loop(mip_count) | std::views::drop(1))
		{
			// 1, ..., mip_count - 1
			auto width_dst	= bloom_gpu.width >> mip_dst;
			auto height_dst = bloom_gpu.height >> mip_dst;

			c_auto mip_src = static_cast<uint16>(mip_dst - 1);
			constants.apply_compute_member<&shared_type::root_constants::bloom_mip_level_and_extra>(mip_src);

			command::apply_barriers(barrier::tex_uav_to_srv(h_bloom_chain->p_resource, D3D12_BARRIER_SYNC_COMPUTE_SHADING, D3D12_BARRIER_SYNC_COMPUTE_SHADING, {}, barrier::tex2d_mip(mip_src)));

			command::dispatch(util::ceil(width_dst, 8), util::ceil(height_dst, 8), 1);
		}


		command::set_pso(p_pso_upsample);
		for (auto mip_dst : views::loop(mip_count - 1) | std::views::reverse)
		{
			// mip_count - 2, mip_count - 3, ... , 0;
			auto width_dst	= bloom_gpu.width >> mip_dst;
			auto height_dst = bloom_gpu.height >> mip_dst;

			c_auto mip_src = static_cast<uint16>(mip_dst + 1);
			constants.apply_compute_member<&shared_type::root_constants::bloom_mip_level_and_extra>(mip_src);

			command::apply_barriers(barrier::tex_uav_to_srv(h_bloom_chain->p_resource, D3D12_BARRIER_SYNC_COMPUTE_SHADING, D3D12_BARRIER_SYNC_COMPUTE_SHADING, {}, barrier::tex2d_mip(mip_src)),
									barrier::tex_srv_to_uav(h_bloom_chain->p_resource, D3D12_BARRIER_SYNC_COMPUTE_SHADING, D3D12_BARRIER_SYNC_COMPUTE_SHADING, {}, barrier::tex2d_mip(mip_dst)));

			command::dispatch(util::ceil(width_dst, 8), util::ceil(height_dst, 8), 1);
		}

		command::apply_barriers(barrier::tex_uav_to_srv(h_bloom_chain->p_resource, D3D12_BARRIER_SYNC_COMPUTE_SHADING, D3D12_BARRIER_SYNC_PIXEL_SHADING, {}, barrier::tex2d_mip(0)));
	}

	void
	bloom_stage::deinit() noexcept
	{
		pso::destroy(h_pso_prefilter);
		pso::destroy(h_pso_downsample);
		pso::destroy(h_pso_upsample);
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

namespace age::graphics::render_pipeline::forward_plus
{
	void
	debug_stage::init(root_signature::handle h_root_sig) noexcept
	{
		using namespace graphics::pso;

		h_pso_mesh = graphics::pso::create(
			pss_root_signature{ .subobj = graphics::g::root_signature_ptr_vec[h_root_sig] },
			pss_as{ .subobj = shader::get_d3d12_bytecode(e::engine_shader_kind::forward_plus_debug_mesh_as) },
			pss_ms{ .subobj = shader::get_d3d12_bytecode(e::engine_shader_kind::forward_plus_debug_mesh_ms) },
			pss_ps{ .subobj = shader::get_d3d12_bytecode(e::engine_shader_kind::forward_plus_debug_mesh_ps) },
			pss_primitive_topology{ .subobj = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE },
			pss_render_target_formats{ .subobj = D3D12_RT_FORMAT_ARRAY{ .RTFormats{ DXGI_FORMAT_R16G16B16A16_FLOAT }, .NumRenderTargets = 1 } },
			pss_rasterizer{ .subobj = defaults::rasterizer_desc::backface_cull },
			pss_depth_stencil_format{ .subobj = DXGI_FORMAT_D16_UNORM },
			pss_depth_stencil1{ .subobj = defaults::depth_stencil_desc1::depth_only_reversed },
			pss_blend{ .subobj = defaults::blend_desc::alpha },
			pss_sample_desc{ .subobj = DXGI_SAMPLE_DESC{ .Count = 1, .Quality = 0 } },
			pss_node_mask{ .subobj = 0 });


		p_pso_mesh = graphics::g::pso_ptr_vec[h_pso_mesh];
		h_pso_mesh.set_name(L"pso_debug_mesh");

		h_pso_mesh_always_on_top = graphics::pso::create(
			pss_root_signature{ .subobj = graphics::g::root_signature_ptr_vec[h_root_sig] },
			pss_as{ .subobj = shader::get_d3d12_bytecode(e::engine_shader_kind::forward_plus_debug_mesh_as) },
			pss_ms{ .subobj = shader::get_d3d12_bytecode(e::engine_shader_kind::forward_plus_debug_mesh_ms) },
			pss_ps{ .subobj = shader::get_d3d12_bytecode(e::engine_shader_kind::forward_plus_debug_mesh_aot_ps) },
			pss_primitive_topology{ .subobj = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE },
			pss_render_target_formats{ .subobj = D3D12_RT_FORMAT_ARRAY{ .RTFormats{ DXGI_FORMAT_R16G16B16A16_FLOAT }, .NumRenderTargets = 1 } },
			pss_rasterizer{ .subobj = defaults::rasterizer_desc::backface_cull },
			pss_depth_stencil_format{ .subobj = DXGI_FORMAT_D16_UNORM },
			pss_depth_stencil1{ .subobj = defaults::depth_stencil_desc1::depth_only_reversed },
			pss_blend{ .subobj = defaults::blend_desc::alpha },
			pss_sample_desc{ .subobj = DXGI_SAMPLE_DESC{ .Count = 1, .Quality = 0 } },
			pss_node_mask{ .subobj = 0 });

		p_pso_mesh_always_on_top = graphics::g::pso_ptr_vec[h_pso_mesh_always_on_top];
		h_pso_mesh_always_on_top.set_name(L"pso_mesh_always_on_top");
	}

	inline void
	debug_stage::execute(binding_config_t::reg_b<1>& constants,
						 rtv_desc_handle			 h_desc_rtv,
						 dsv_desc_handle			 h_desc_dsv,
						 bool						 is_aot,
						 uint32						 render_data_count,
						 uint32						 render_data_offset) noexcept
	{
		auto render_pass_rt_desc = defaults::render_pass_rtv_desc::load_preserve(h_desc_rtv);
		auto render_pass_ds_desc = defaults::render_pass_ds_desc::depth_clear_discard(h_desc_dsv, 0.f, DXGI_FORMAT_D16_UNORM);

		command::begin_render_pass(
			1,
			&render_pass_rt_desc,
			&render_pass_ds_desc,
			D3D12_RENDER_PASS_FLAG_NONE);

		{
			struct
			{
				uint32 offset;
				uint32 count;
			} payload{ render_data_offset, render_data_count };

			constants.apply_graphics_member<&shared_type::root_constants::debug_meshlet_render_data_offset, sizeof(payload)>(payload);
			command::set_pso(is_aot ? p_pso_mesh_always_on_top : p_pso_mesh);

			if (render_data_count > 0)
			{
				command::dispatch_mesh((render_data_count + 31u) / 32u, 1, 1);
			}
		}

		command::end_render_pass();
	}

	void
	debug_stage::deinit() noexcept
	{
		pso::destroy(h_pso_mesh);
		pso::destroy(h_pso_mesh_always_on_top);
	}
}	 // namespace age::graphics::render_pipeline::forward_plus
#endif
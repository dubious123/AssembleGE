#include "age_pch.hpp"
#include "age.hpp"

namespace age::graphics::bake
{
	void
	init() noexcept
	{
		using namespace graphics::pso;

		g::bake_pipeline.h_root_sig = binding_config_t::create_root_signature(
			D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED
			| D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS
			| D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS
			| D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS
			| D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS);

		g::bake_pipeline.h_pso_env_light_radiance = graphics::pso::create(
			pss_root_signature{ .subobj = g::bake_pipeline.h_root_sig.ptr() },
			pss_cs{ .subobj = shader::get_d3d12_bytecode(shader::shader_handle{ to_idx(e::engine_shader_kind::bake_env_light_radiance_cs) }) });

		g::bake_pipeline.h_pso_env_light_irradiance = graphics::pso::create(
			pss_root_signature{ .subobj = g::bake_pipeline.h_root_sig.ptr() },
			pss_cs{ .subobj = shader::get_d3d12_bytecode(shader::shader_handle{ to_idx(e::engine_shader_kind::bake_env_light_irradiance_cs) }) });

		g::bake_pipeline.h_pso_env_light_prefilter = graphics::pso::create(
			pss_root_signature{ .subobj = g::bake_pipeline.h_root_sig.ptr() },
			pss_cs{ .subobj = shader::get_d3d12_bytecode(shader::shader_handle{ to_idx(e::engine_shader_kind::bake_env_light_prefilter_cs) }) });

		g::bake_pipeline.h_pso_env_light_build_marginal_cdf = graphics::pso::create(
			pss_root_signature{ .subobj = g::bake_pipeline.h_root_sig.ptr() },
			pss_cs{ .subobj = shader::get_d3d12_bytecode(shader::shader_handle{ to_idx(e::engine_shader_kind::bake_env_light_build_marginal_cdf_cs) }) });

		g::bake_pipeline.h_pso_env_light_build_conditional_cdf = graphics::pso::create(
			pss_root_signature{ .subobj = g::bake_pipeline.h_root_sig.ptr() },
			pss_cs{ .subobj = shader::get_d3d12_bytecode(shader::shader_handle{ to_idx(e::engine_shader_kind::bake_env_light_build_conditional_cdf_cs) }) });

		g::bake_pipeline.h_pso_down_sample_cube = graphics::pso::create(
			pss_root_signature{ .subobj = g::bake_pipeline.h_root_sig.ptr() },
			pss_cs{ .subobj = shader::get_d3d12_bytecode(shader::shader_handle{ to_idx(e::engine_shader_kind::bake_down_sample_cube_cs) }) });

		g::bake_pipeline.h_pso_env_light_radiance.set_name(L"pso_env_light_radiance");
		g::bake_pipeline.h_pso_env_light_irradiance.set_name(L"pso_env_light_irradiance");
		g::bake_pipeline.h_pso_env_light_prefilter.set_name(L"pso_env_light_prefilter");
		g::bake_pipeline.h_pso_env_light_build_marginal_cdf.set_name(L"pso_env_light_build_marginal_cdf");
		g::bake_pipeline.h_pso_env_light_build_conditional_cdf.set_name(L"pso_env_light_build_conditional_cdf");
		g::bake_pipeline.h_pso_down_sample_cube.set_name(L"pso_down_sample_cube");

		pop_descriptor(g::bake_pipeline.h_env_light_input_srv_desc);
		pop_descriptor(g::bake_pipeline.h_env_light_radiance_srv_desc);
		pop_descriptor(g::bake_pipeline.h_env_light_irradiance_uav_desc);

		g::bake_pipeline.h_env_light_input_tex = resource::create_committed(
			{ .d3d12_resource_desc = defaults::resource_desc::texture_2d(
				  128, 128,
				  DXGI_FORMAT_R16G16B16A16_FLOAT),
			  .initial_layout	= D3D12_BARRIER_LAYOUT_DIRECT_QUEUE_COPY_DEST,
			  .heap_memory_kind = e::memory_kind::gpu_only });

		resource::create_view(g::bake_pipeline.h_env_light_input_tex,
							  g::bake_pipeline.h_env_light_input_srv_desc,
							  defaults::srv_view_desc::tex2d(DXGI_FORMAT_R16G16B16A16_FLOAT));

		g::bake_pipeline.h_env_light_radiance_luminance_buffer = resource::create_committed(
			{ .d3d12_resource_desc = defaults::resource_desc::buffer_uav(1024),
			  .initial_layout	   = D3D12_BARRIER_LAYOUT_UNDEFINED,
			  .heap_memory_kind	   = e::memory_kind::gpu_only,
			  .has_clear_value	   = false });
		g::bake_pipeline.h_env_light_marginal_cdf_buffer = resource::create_committed(
			{ .d3d12_resource_desc = defaults::resource_desc::buffer_uav(1024),
			  .initial_layout	   = D3D12_BARRIER_LAYOUT_UNDEFINED,
			  .heap_memory_kind	   = e::memory_kind::gpu_only,
			  .has_clear_value	   = false });
		g::bake_pipeline.h_env_light_conditional_cdf_buffer = resource::create_committed(
			{ .d3d12_resource_desc = defaults::resource_desc::buffer_uav(1024),
			  .initial_layout	   = D3D12_BARRIER_LAYOUT_UNDEFINED,
			  .heap_memory_kind	   = e::memory_kind::gpu_only,
			  .has_clear_value	   = false });

		g::bake_pipeline.h_env_light_radiance_luminance_buffer->set_name(L"h_env_light_radiance_luminance_buffer");
		g::bake_pipeline.h_env_light_marginal_cdf_buffer->set_name(L"h_env_light_margianl_cdf_buffer");
		g::bake_pipeline.h_env_light_conditional_cdf_buffer->set_name(L"h_env_light_conditional_cdf_buffer");

		g::bake_pipeline.env_light_radiance_luminance_srv.bind(g::bake_pipeline.h_env_light_radiance_luminance_buffer);
		g::bake_pipeline.env_light_radiance_luminance_uav.bind(g::bake_pipeline.h_env_light_radiance_luminance_buffer);
		g::bake_pipeline.env_light_marginal_cdf_srv.bind(g::bake_pipeline.h_env_light_marginal_cdf_buffer);
		g::bake_pipeline.env_light_margianl_cdf_uav.bind(g::bake_pipeline.h_env_light_marginal_cdf_buffer);
		g::bake_pipeline.env_light_conditional_cdf_srv.bind(g::bake_pipeline.h_env_light_conditional_cdf_buffer);
		g::bake_pipeline.env_light_conditional_cdf_uav.bind(g::bake_pipeline.h_env_light_conditional_cdf_buffer);
	}

	void
	deinit() noexcept
	{
		resource::release(g::bake_pipeline.h_env_light_input_tex);
		resource::release(g::bake_pipeline.h_env_light_radiance_luminance_buffer);
		resource::release(g::bake_pipeline.h_env_light_marginal_cdf_buffer);
		resource::release(g::bake_pipeline.h_env_light_conditional_cdf_buffer);

		pso::destroy(g::bake_pipeline.h_pso_env_light_radiance);
		pso::destroy(g::bake_pipeline.h_pso_env_light_irradiance);
		pso::destroy(g::bake_pipeline.h_pso_env_light_prefilter);
		pso::destroy(g::bake_pipeline.h_pso_env_light_build_marginal_cdf);
		pso::destroy(g::bake_pipeline.h_pso_env_light_build_conditional_cdf);
		pso::destroy(g::bake_pipeline.h_pso_down_sample_cube);

		root_signature::destroy(g::bake_pipeline.h_root_sig);

		push_descriptor(g::bake_pipeline.h_env_light_input_srv_desc);
		push_descriptor(g::bake_pipeline.h_env_light_radiance_srv_desc);
		push_descriptor(g::bake_pipeline.h_env_light_irradiance_uav_desc);

		for (auto h : g::bake_pipeline.h_env_light_radiance_uav_desc_vec)
		{
			push_descriptor(h);
		}

		for (auto h : g::bake_pipeline.h_env_light_prefilter_uav_desc_vec)
		{
			push_descriptor(h);
		}

		g::bake_pipeline.h_env_light_radiance_uav_desc_vec.clear();
		g::bake_pipeline.h_env_light_prefilter_uav_desc_vec.clear();
	}
}	 // namespace age::graphics::bake

namespace age::graphics::bake::detail
{
	void
	begin() noexcept
	{
		command::begin();
		command::set_descriptor_heaps(1, &graphics::g::cbv_srv_uav_desc_heap.p_heap);
		command::set_graphics_root_sig(g::bake_pipeline.h_root_sig.ptr());
		command::set_compute_root_sig(g::bake_pipeline.h_root_sig.ptr());

		g::bake_pipeline.env_light_radiance_luminance_srv.apply_compute();
		g::bake_pipeline.env_light_radiance_luminance_uav.apply_compute();
		g::bake_pipeline.env_light_marginal_cdf_srv.apply_compute();
		g::bake_pipeline.env_light_margianl_cdf_uav.apply_compute();
		g::bake_pipeline.env_light_conditional_cdf_srv.apply_compute();
		g::bake_pipeline.env_light_conditional_cdf_uav.apply_compute();
	}
}	 // namespace age::graphics::bake::detail

namespace age::graphics::bake
{
	env_light_result
	env_light(asset::handle h_tex, const asset::env_light_desc& desc) noexcept
	{
		c_auto& entry			   = h_tex.get_entry<asset::e::kind::texture>();
		c_auto& header			   = entry.get_header();
		c_auto	dx12_format		   = graphics::dx12_format(header.format);
		c_auto	radiance_mip_count = std::bit_width(desc.cubemap_size);

		AGE_ASSERT(header.tex_depth_or_array_size == 1);
		AGE_ASSERT(header.mip_count == 1);
		AGE_ASSERT(desc.prefilter_size >= (1u << (desc.prefilter_mip_count - 1)));
		AGE_ASSERT(header.extent.width < 32 * 1024);
		AGE_ASSERT(header.extent.height < 32 * 1024);

		for (auto _ : views::loop(std::max<uint64>(g::bake_pipeline.h_env_light_radiance_uav_desc_vec.size(), radiance_mip_count) - g::bake_pipeline.h_env_light_radiance_uav_desc_vec.size()))
		{
			pop_descriptor(g::bake_pipeline.h_env_light_radiance_uav_desc_vec.emplace_back());
		}

		for (auto _ : views::loop(std::max<uint64>(g::bake_pipeline.h_env_light_prefilter_uav_desc_vec.size(), desc.prefilter_mip_count) - g::bake_pipeline.h_env_light_prefilter_uav_desc_vec.size()))
		{
			pop_descriptor(g::bake_pipeline.h_env_light_prefilter_uav_desc_vec.emplace_back());
		}

		if (resource::resize_buffer(g::bake_pipeline.h_env_light_radiance_luminance_buffer, header.extent.width * header.extent.height * sizeof(float)))
		{
			g::bake_pipeline.env_light_radiance_luminance_srv.bind(g::bake_pipeline.h_env_light_radiance_luminance_buffer);
			g::bake_pipeline.env_light_radiance_luminance_uav.bind(g::bake_pipeline.h_env_light_radiance_luminance_buffer);
		}
		if (resource::resize_buffer(g::bake_pipeline.h_env_light_marginal_cdf_buffer, header.extent.height * sizeof(float)))
		{
			g::bake_pipeline.env_light_marginal_cdf_srv.bind(g::bake_pipeline.h_env_light_marginal_cdf_buffer);
			g::bake_pipeline.env_light_margianl_cdf_uav.bind(g::bake_pipeline.h_env_light_marginal_cdf_buffer);
		}
		if (resource::resize_buffer(g::bake_pipeline.h_env_light_conditional_cdf_buffer, (header.extent.width + 1) * header.extent.height * sizeof(float)))
		{
			g::bake_pipeline.env_light_conditional_cdf_srv.bind(g::bake_pipeline.h_env_light_conditional_cdf_buffer);
			g::bake_pipeline.env_light_conditional_cdf_uav.bind(g::bake_pipeline.h_env_light_conditional_cdf_buffer);
		}

		if (resource::resize_texture_2d(g::bake_pipeline.h_env_light_input_tex, header.extent, dx12_format))
		{
			resource::create_view(g::bake_pipeline.h_env_light_input_tex,
								  g::bake_pipeline.h_env_light_input_srv_desc,
								  defaults::srv_view_desc::tex2d(dx12_format));
		}

		c_auto h_radiance = resource::create_committed(
			{ .d3d12_resource_desc = defaults::resource_desc::texture_2d_array(
				  desc.cubemap_size, desc.cubemap_size,
				  DXGI_FORMAT_R16G16B16A16_FLOAT,
				  6,
				  D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
				  radiance_mip_count),
			  .initial_layout	= D3D12_BARRIER_LAYOUT_DIRECT_QUEUE_UNORDERED_ACCESS,
			  .heap_memory_kind = e::memory_kind::gpu_only });

		c_auto h_irradiance = resource::create_committed(
			{ .d3d12_resource_desc = defaults::resource_desc::texture_2d_array(
				  desc.irradiance_size, desc.irradiance_size,
				  DXGI_FORMAT_R16G16B16A16_FLOAT,
				  6,
				  D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS),
			  .initial_layout	= D3D12_BARRIER_LAYOUT_DIRECT_QUEUE_UNORDERED_ACCESS,
			  .heap_memory_kind = e::memory_kind::gpu_only });

		c_auto h_prefilter = resource::create_committed(
			{ .d3d12_resource_desc = defaults::resource_desc::texture_2d_array(
				  desc.prefilter_size, desc.prefilter_size,
				  DXGI_FORMAT_R16G16B16A16_FLOAT,
				  6,
				  D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
				  desc.prefilter_mip_count),
			  .initial_layout	= D3D12_BARRIER_LAYOUT_DIRECT_QUEUE_UNORDERED_ACCESS,
			  .heap_memory_kind = e::memory_kind::gpu_only });


		resource::create_view(h_radiance,
							  g::bake_pipeline.h_env_light_radiance_srv_desc,
							  defaults::srv_view_desc::tex_cube(DXGI_FORMAT_R16G16B16A16_FLOAT));


		for (auto mip : views::loop(radiance_mip_count))
		{
			resource::create_view(h_radiance,
								  g::bake_pipeline.h_env_light_radiance_uav_desc_vec[mip],
								  defaults::uav_view_desc::tex_cube(DXGI_FORMAT_R16G16B16A16_FLOAT, mip));
		}

		resource::create_view(h_irradiance,
							  g::bake_pipeline.h_env_light_irradiance_uav_desc,
							  defaults::uav_view_desc::tex_cube(DXGI_FORMAT_R16G16B16A16_FLOAT));


		for (auto mip : views::loop(desc.prefilter_mip_count))
		{
			resource::create_view(h_prefilter,
								  g::bake_pipeline.h_env_light_prefilter_uav_desc_vec[mip],
								  defaults::uav_view_desc::tex_cube(DXGI_FORMAT_R16G16B16A16_FLOAT, mip));
		}

		detail::begin();

		resource::upload_texture(g::bake_pipeline.h_env_light_input_tex, entry.get_texture_buffer());

		g::bake_pipeline.root_constants.bind(
			{
				.env_light_input_texture_width		 = header.extent.width,
				.env_light_input_texture_height		 = header.extent.height,
				.env_light_input_texture_srv_id		 = calc_desc_idx(g::bake_pipeline.h_env_light_input_srv_desc),
				.env_light_radiance_texture_srv_id	 = calc_desc_idx(g::bake_pipeline.h_env_light_radiance_srv_desc),
				.env_light_radiance_texture_uav_id	 = calc_desc_idx(g::bake_pipeline.h_env_light_radiance_uav_desc_vec[0]),
				.env_light_irradiance_texture_uav_id = calc_desc_idx(g::bake_pipeline.h_env_light_irradiance_uav_desc),
				.env_light_prefilter_mip_max_count	 = desc.prefilter_mip_count,
				.env_light_radiance_size			 = desc.cubemap_size,
				.env_light_irradiance_size			 = desc.irradiance_size,
				.env_light_prefilter_size			 = desc.prefilter_size,
				.down_sample_input_texture_srv_id	 = calc_desc_idx(g::bake_pipeline.h_env_light_radiance_srv_desc),
			});
		g::bake_pipeline.root_constants.apply_compute();

		command::apply_barriers(barrier::tex_copy_dest_to_srv(g::bake_pipeline.h_env_light_input_tex, D3D12_BARRIER_SYNC_COMPUTE_SHADING));

		command::set_pso(g::bake_pipeline.h_pso_env_light_build_conditional_cdf.ptr());
		command::dispatch(1, header.extent.height, 1);

		command::apply_barriers(barrier::buf_uav_to_srv(g::bake_pipeline.h_env_light_radiance_luminance_buffer, D3D12_BARRIER_SYNC_COMPUTE_SHADING),
								barrier::buf_uav_to_srv(g::bake_pipeline.h_env_light_conditional_cdf_buffer, D3D12_BARRIER_SYNC_COMPUTE_SHADING));
		g::bake_pipeline.env_light_radiance_luminance_srv.apply_compute();
		g::bake_pipeline.env_light_conditional_cdf_srv.apply_compute();

		command::set_pso(g::bake_pipeline.h_pso_env_light_build_marginal_cdf.ptr());
		command::dispatch(1, 1, 1);

		command::apply_barriers(barrier::buf_uav_to_srv(g::bake_pipeline.h_env_light_marginal_cdf_buffer, D3D12_BARRIER_SYNC_COMPUTE_SHADING));
		g::bake_pipeline.env_light_marginal_cdf_srv.apply_compute();

		command::set_pso(g::bake_pipeline.h_pso_env_light_radiance.ptr());
		command::dispatch(util::ceil(desc.cubemap_size, 8), util::ceil(desc.cubemap_size, 8), 6);

		command::set_pso(g::bake_pipeline.h_pso_down_sample_cube.ptr());
		for (auto mip : views::loop(radiance_mip_count) | std::views::drop(1))
		{
			auto mip_size = desc.cubemap_size >> mip;

			struct
			{
				uint32 tex_uav;
				uint32 src_mip;
				uint32 output_size;
			} cube_down_sample_root_constant{ calc_desc_idx(g::bake_pipeline.h_env_light_radiance_uav_desc_vec[mip]), static_cast<uint32>(mip - 1), mip_size };

			g::bake_pipeline.root_constants.apply_compute_member<&shared_type::root_constants::down_sample_output_texture_uav_id, 12u>(cube_down_sample_root_constant);

			command::apply_barriers(barrier::tex_uav_to_srv(h_radiance, D3D12_BARRIER_SYNC_COMPUTE_SHADING, D3D12_BARRIER_SYNC_COMPUTE_SHADING, {}, barrier::cube_mip(mip - 1)));

			command::dispatch(util::ceil(mip_size, 8), util::ceil(mip_size, 8), 6);
		}
		command::apply_barriers(barrier::tex_uav_to_srv(h_radiance, D3D12_BARRIER_SYNC_COMPUTE_SHADING, D3D12_BARRIER_SYNC_COMPUTE_SHADING, {}, barrier::cube_mip(radiance_mip_count - 1)));

		command::set_pso(g::bake_pipeline.h_pso_env_light_irradiance.ptr());
		command::dispatch(util::ceil(desc.irradiance_size, 8), util::ceil(desc.irradiance_size, 8), 6);

		command::set_pso(g::bake_pipeline.h_pso_env_light_prefilter.ptr());

		for (auto mip : views::loop(desc.prefilter_mip_count))
		{
			struct
			{
				uint32 tex_idx;
				uint32 mip;
			} tex_idx_and_mip_count{ calc_desc_idx(g::bake_pipeline.h_env_light_prefilter_uav_desc_vec[mip]), static_cast<uint32>(mip) };

			g::bake_pipeline.root_constants.apply_compute_member<&shared_type::root_constants::env_light_prefilter_texture_uav_id, 8u>(tex_idx_and_mip_count);
			command::dispatch(util::ceil(desc.prefilter_size >> mip, 8), util::ceil(desc.prefilter_size >> mip, 8), 6);
		}

		command::apply_barriers(
			barrier::tex_srv_to_copy_dst(g::bake_pipeline.h_env_light_input_tex, D3D12_BARRIER_SYNC_COMPUTE_SHADING),
			barrier::tex_srv_to_copy_src(h_radiance, D3D12_BARRIER_SYNC_COMPUTE_SHADING),
			barrier::tex_uav_to_copy_src(h_irradiance, D3D12_BARRIER_SYNC_COMPUTE_SHADING),
			barrier::tex_uav_to_copy_src(h_prefilter, D3D12_BARRIER_SYNC_COMPUTE_SHADING));
		command::execute_and_wait();

		return env_light_result{
			.h_radiance	  = h_radiance,
			.h_irradiance = h_irradiance,
			.h_prefilter  = h_prefilter,
		};
	}

	resource_handle
	bake_brdf_lut(extent_2d<uint32> extent) noexcept
	{
		using namespace graphics::pso;

		AGE_ASSERT(util::popcount(extent.width) == 1 and util::popcount(extent.height) == 1);
		AGE_ASSERT(extent.width > 8 and extent.height > 8);

		auto h_pso = graphics::pso::create(
			pss_root_signature{ .subobj = g::bake_pipeline.h_root_sig.ptr() },
			pss_cs{ .subobj = shader::get_d3d12_bytecode(shader::shader_handle{ to_idx(e::engine_shader_kind::bake_brdf_lut_cs) }) });

		auto h_res = resource::create_committed(
			{ .d3d12_resource_desc = defaults::resource_desc::texture_2d(
				  extent.width, extent.height,
				  DXGI_FORMAT_R16G16_FLOAT),
			  .initial_layout	= D3D12_BARRIER_LAYOUT_DIRECT_QUEUE_COPY_DEST,
			  .heap_memory_kind = e::memory_kind::gpu_only });


		auto h_scratch = resource::create_committed(
			{ .d3d12_resource_desc = defaults::resource_desc::texture_2d(
				  extent.width, extent.height,
				  DXGI_FORMAT_R16G16_FLOAT,
				  D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS),
			  .initial_layout	= D3D12_BARRIER_LAYOUT_DIRECT_QUEUE_UNORDERED_ACCESS,
			  .heap_memory_kind = e::memory_kind::gpu_only });

		auto h_scratch_uav_desc = pop_descriptor<uav_desc_handle>();

		resource::create_view(h_scratch,
							  h_scratch_uav_desc,
							  defaults::uav_view_desc::tex2d(DXGI_FORMAT_R16G16_FLOAT));

		detail::begin();

		g::bake_pipeline.root_constants.bind({
			.brdf_lut_width	 = extent.width,
			.brdf_lut_height = extent.height,
			.brdf_lut_uav_id = calc_desc_idx(h_scratch_uav_desc),
		});

		g::bake_pipeline.root_constants.apply_compute();

		command::set_pso(h_pso.ptr());
		command::dispatch(util::ceil(extent.width, 8), util::ceil(extent.height, 8), 1);

		command::apply_barriers(barrier::tex_uav_to_copy_src(h_scratch, D3D12_BARRIER_SYNC_COMPUTE_SHADING));

		c_auto src = defaults::copy_location::src_subresource(h_scratch);

		c_auto dst = defaults::copy_location::dst_subresource(h_res);

		command::copy_texture(&dst, 0, 0, 0, &src, nullptr);

		command::apply_barriers(barrier::tex_copy_dest_to_srv(h_res, D3D12_BARRIER_SYNC_PIXEL_SHADING));

		command::execute_and_wait();

		push_descriptor(h_scratch_uav_desc);

		resource::release(h_scratch);

		pso::destroy(h_pso);

		return h_res;
	}
}	 // namespace age::graphics::bake
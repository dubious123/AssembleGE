#include "age_pch.hpp"
#include "age.hpp"

#if defined(AGE_GRAPHICS_BACKEND_DX12)

namespace age::graphics::stage
{
	void
	init() noexcept
	{
		// geometry_stage default root sig
		{
			using namespace root_signature;

			g::h_geometry_stage_default_root_sig = root_signature::create(
				constants{
					.reg		= b{ .idx = 1, .space = 0 },
					.num_32bit	= 3,
					.visibility = D3D12_SHADER_VISIBILITY_PIXEL });

			g::root_signature_ptr_vec[g::h_geometry_stage_default_root_sig.id]->SetName(L"GPass Default Root Sig");
		}

		// geometry_stage default pso
		{
			auto vs_byte_code = shader::get_d3d12_bytecode(shader::shader_handle{ std::to_underlying(shader::engine_shader::fullscreen_triangle_vs) });
			auto ps_byte_code = shader::get_d3d12_bytecode(shader::shader_handle{ std::to_underlying(shader::engine_shader::fill_color_ps) });

			auto stream = pss_stream{
				pss_root_signature{ .subobj = g::root_signature_ptr_vec[g::h_geometry_stage_default_root_sig.id] },
				pss_vs{ .subobj = vs_byte_code },
				pss_ps{ .subobj = ps_byte_code },
				pss_primitive_topology{ .subobj = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE },
				pss_render_target_formats{ .subobj = D3D12_RT_FORMAT_ARRAY{ .RTFormats{ DXGI_FORMAT_R16G16B16A16_FLOAT }, .NumRenderTargets = 1 } },
				pss_depth_stencil_format{ .subobj = DXGI_FORMAT_D32_FLOAT },
				pss_rasterizer{ .subobj = g::rasterizer_desc.no_cull },
				pss_depth_stencil1{ .subobj = g::depth_stencil_desc1.disabled },
			};

			g::h_geometry_stage_default_pso = create_pso(stream.storage, stream.size_in_bytes);
			g::pso_ptr_vec[g::h_geometry_stage_default_pso.id]->SetName(L"GPass Default PSO");
		}

		// fx_present_pass default root sig
		{
			using namespace root_signature;

			g::h_fx_present_stage_default_root_sig = root_signature::create(
				descriptor_table{
					D3D12_SHADER_VISIBILITY_PIXEL,
					descriptor_range{ .reg = t{ .idx = 0, .space = 0 }, .count = 1 } });

			g::root_signature_ptr_vec[g::h_fx_present_stage_default_root_sig.id]->SetName(L"FX Present Pass Default Root Sig");
		}

		// fx_present_pass default pso (srgb)
		{
			auto vs_byte_code = shader::get_d3d12_bytecode(shader::shader_handle{ std::to_underlying(shader::engine_shader::fullscreen_triangle_vs) });
			auto ps_byte_code = shader::get_d3d12_bytecode(shader::shader_handle{ std::to_underlying(shader::engine_shader::fx_present_ps) });

			auto stream = pss_stream{
				pss_root_signature{ .subobj = g::root_signature_ptr_vec[g::h_fx_present_stage_default_root_sig.id] },
				pss_vs{ .subobj = vs_byte_code },
				pss_ps{ .subobj = ps_byte_code },
				pss_primitive_topology{ .subobj = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE },
				pss_render_target_formats{ .subobj = D3D12_RT_FORMAT_ARRAY{ .RTFormats{ DXGI_FORMAT_R8G8B8A8_UNORM_SRGB }, .NumRenderTargets = 1 } },
				pss_rasterizer{ .subobj = g::rasterizer_desc.no_cull },
			};

			g::h_fx_present_stage_default_pso = create_pso(stream.storage, stream.size_in_bytes);
			g::pso_ptr_vec[g::h_fx_present_stage_default_pso.id]->SetName(L"FX Present Pass Default PSO");
		}

		// meshlet

		{
			using namespace root_signature;

			// auto h_meshlet_stage_root_sig = root_signature::create(
			//	descriptor_table{
			//		D3D12_SHADER_VISIBILITY_AMPLIFICATION,
			//	});
		}
	}

	void
	deinit() noexcept
	{
		destroy_pso(g::h_geometry_stage_default_pso);
		root_signature::destroy(g::h_geometry_stage_default_root_sig);

		destroy_pso(g::h_fx_present_stage_default_pso);
		root_signature::destroy(g::h_fx_present_stage_default_root_sig);
	}
}	 // namespace age::graphics::stage
#endif

namespace age::graphics::stage
{
	void
	my_stage::init() noexcept
	{
		main_buffer_view.init();
		depth_buffer_view.init();
	}

	void
	my_stage::bind(graphics::root_signature::handle h_root_sig,
				   graphics::pso_handle				h_pso,
				   graphics::resource_handle		h_main_buffer,
				   graphics::resource_handle		h_depth_buffer) noexcept
	{
		p_root_sig = g::root_signature_ptr_vec[h_root_sig];
		p_pso	   = g::pso_ptr_vec[h_pso];

		auto& main_buffer  = *g::resource_vec[h_main_buffer].p_resource;
		auto& depth_buffer = *g::resource_vec[h_depth_buffer].p_resource;

		main_buffer_view.bind(main_buffer);
		depth_buffer_view.bind(depth_buffer);
	}

	void
	my_stage::execute(t_igraphics_cmd_list& cmd_list, render_surface& rs) noexcept
	{
		static auto frame_count = 0ul;

		struct
		{
			float  width;
			float  height;
			uint32 frame;
		} constants{ .width = rs.default_viewport.Width, .height = rs.default_viewport.Height, .frame = frame_count++ };

		auto barrier = resource_barrier{};

		barrier.add_transition(rs.get_back_buffer(),
							   D3D12_RESOURCE_STATE_PRESENT,
							   D3D12_RESOURCE_STATE_RENDER_TARGET,
							   D3D12_RESOURCE_BARRIER_FLAG_BEGIN_ONLY);
		barrier.apply_and_reset(cmd_list);

		{
			auto _ = phase_geo(cmd_list);

			cmd_list.SetGraphicsRootSignature(p_root_sig);
			cmd_list.SetPipelineState(p_pso);

			cmd_list.SetGraphicsRoot32BitConstants(0, 3, &constants, 0);

			cmd_list.IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			cmd_list.DrawInstanced(3, 1, 0, 0);
		}

		barrier.add_transition(rs.get_back_buffer(),
							   D3D12_RESOURCE_STATE_PRESENT,
							   D3D12_RESOURCE_STATE_RENDER_TARGET,
							   D3D12_RESOURCE_BARRIER_FLAG_END_ONLY);
		barrier.apply_and_reset(cmd_list);

		{
			phase_fx(cmd_list);
			cmd_list.SetGraphicsRootSignature(g::root_signature_ptr_vec[g::h_fx_present_stage_default_root_sig]);
			cmd_list.SetPipelineState(g::pso_ptr_vec[g::h_fx_present_stage_default_pso]);

			cmd_list.SetGraphicsRootDescriptorTable(0, main_buffer_view.h_srv_desc.h_gpu);

			cmd_list.IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			auto d = rs.get_rtv();
			cmd_list.OMSetRenderTargets(1, &d, 1, nullptr);
			cmd_list.DrawInstanced(3, 1, 0, 0);
		}

		barrier.add_transition(
			rs.get_back_buffer(),
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_PRESENT);
		barrier.apply_and_reset(cmd_list);
	}

	void
	my_stage::deinit() noexcept
	{
		main_buffer_view.deinit();
		depth_buffer_view.deinit();
	}
}	 // namespace age::graphics::stage

namespace age::graphics::stage
{
	void
	my_pipeline::execute(t_igraphics_cmd_list& cmd_list, render_surface& rs) noexcept
	{
		auto new_extent = age::extent_2d<uint16>{
			.width	= std::max(extent.width, static_cast<uint16>(age::platform::get_client_width(rs.h_window))),
			.height = std::max(extent.height, static_cast<uint16>(age::platform::get_client_height(rs.h_window)))
		};

		if (extent != new_extent) [[unlikely]]
		{
			this->resize(new_extent);

			stage.bind(
				g::h_geometry_stage_default_root_sig,
				g::h_geometry_stage_default_pso,
				h_main_buffer,
				h_depth_buffer);
		}

		cmd_list.RSSetViewports(1, &rs.default_viewport);
		cmd_list.RSSetScissorRects(1, &rs.default_scissor_rect);

		stage.execute(cmd_list, rs);
	}
}	 // namespace age::graphics::stage

namespace age::graphics::stage
{
	void
	my_mesh_shader_stage::init() noexcept
	{
		main_buffer_view.init();
		depth_buffer_view.init();
	}

	void
	my_mesh_shader_stage::bind(graphics::root_signature::handle h_root_sig,
							   graphics::pso_handle				h_pso,
							   graphics::resource_handle		h_main_buffer,
							   graphics::resource_handle		h_depth_buffer) noexcept
	{
		p_root_sig = g::root_signature_ptr_vec[h_root_sig];
		p_pso	   = g::pso_ptr_vec[h_pso];

		auto& main_buffer  = *g::resource_vec[h_main_buffer].p_resource;
		auto& depth_buffer = *g::resource_vec[h_depth_buffer].p_resource;

		main_buffer_view.bind(main_buffer);
		depth_buffer_view.bind(depth_buffer);
	}

	void
	my_mesh_shader_stage::execute(t_igraphics_cmd_list& cmd_list, render_surface& rs) noexcept
	{
		static auto frame_count = 0ul;

		struct
		{
			float  width;
			float  height;
			uint32 frame;
		} constants{ .width = rs.default_viewport.Width, .height = rs.default_viewport.Height, .frame = frame_count++ };

		auto barrier = resource_barrier{};

		barrier.add_transition(rs.get_back_buffer(),
							   D3D12_RESOURCE_STATE_PRESENT,
							   D3D12_RESOURCE_STATE_RENDER_TARGET,
							   D3D12_RESOURCE_BARRIER_FLAG_BEGIN_ONLY);
		barrier.apply_and_reset(cmd_list);

		{
			auto _ = phase_geo(cmd_list);

			cmd_list.SetGraphicsRootSignature(p_root_sig);
			cmd_list.SetPipelineState(p_pso);

			cmd_list.SetGraphicsRoot32BitConstants(0, 3, &constants, 0);

			cmd_list.IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			cmd_list.DrawInstanced(3, 1, 0, 0);
		}

		barrier.add_transition(rs.get_back_buffer(),
							   D3D12_RESOURCE_STATE_PRESENT,
							   D3D12_RESOURCE_STATE_RENDER_TARGET,
							   D3D12_RESOURCE_BARRIER_FLAG_END_ONLY);
		barrier.apply_and_reset(cmd_list);

		{
			phase_fx(cmd_list);
			cmd_list.SetGraphicsRootSignature(g::root_signature_ptr_vec[g::h_fx_present_stage_default_root_sig]);
			cmd_list.SetPipelineState(g::pso_ptr_vec[g::h_fx_present_stage_default_pso]);

			cmd_list.SetGraphicsRootDescriptorTable(0, main_buffer_view.h_srv_desc.h_gpu);

			cmd_list.IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			auto d = rs.get_rtv();
			cmd_list.OMSetRenderTargets(1, &d, 1, nullptr);
			cmd_list.DrawInstanced(3, 1, 0, 0);
		}

		barrier.add_transition(
			rs.get_back_buffer(),
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_PRESENT);
		barrier.apply_and_reset(cmd_list);
	}

	void
	my_mesh_shader_stage::deinit() noexcept
	{
		main_buffer_view.deinit();
		depth_buffer_view.deinit();
	}
}	 // namespace age::graphics::stage

namespace age::graphics::stage
{
	void
	my_mesh_shader_pipeline::execute(t_igraphics_cmd_list& cmd_list, render_surface& rs) noexcept
	{
		auto new_extent = age::extent_2d<uint16>{
			.width	= std::max(extent.width, static_cast<uint16>(age::platform::get_client_width(rs.h_window))),
			.height = std::max(extent.height, static_cast<uint16>(age::platform::get_client_height(rs.h_window)))
		};

		if (extent != new_extent) [[unlikely]]
		{
			this->resize(new_extent);

			stage.bind(
				g::h_geometry_stage_default_root_sig,
				g::h_geometry_stage_default_pso,
				h_main_buffer,
				h_depth_buffer);
		}

		cmd_list.RSSetViewports(1, &rs.default_viewport);
		cmd_list.RSSetScissorRects(1, &rs.default_scissor_rect);

		stage.execute(cmd_list, rs);
	}
}	 // namespace age::graphics::stage
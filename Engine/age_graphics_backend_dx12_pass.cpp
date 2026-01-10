#include "age_pch.hpp"
#include "age.hpp"

#if defined(AGE_GRAPHICS_BACKEND_DX12)

namespace age::graphics::pass
{
	void
	init() noexcept
	{
		// gpass default root sig
		{
			using namespace root_signature;

			g::h_gpass_default_root_sig = root_signature::create(
				constants{
					.reg		= b{ .idx = 1, .space = 0 },
					.num_32bit	= 3,
					.visibility = D3D12_SHADER_VISIBILITY_PIXEL });

			g::root_signature_ptr_vec[g::h_gpass_default_root_sig.id]->SetName(L"GPass Default Root Sig");
		}

		// gpass default pso
		{
			auto vs_byte_code = shader::get_d3d12_bytecode(shader::shader_handle{ std::to_underlying(shader::engine_shader::fullscreen_triangle_vs) });
			auto ps_byte_code = shader::get_d3d12_bytecode(shader::shader_handle{ std::to_underlying(shader::engine_shader::fill_color_ps) });

			auto stream = pss_stream{
				pss_root_signature{ .subobj = g::root_signature_ptr_vec[g::h_gpass_default_root_sig.id] },
				pss_vs{ .subobj = vs_byte_code },
				pss_ps{ .subobj = ps_byte_code },
				pss_primitive_topology{ .subobj = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE },
				pss_render_target_formats{ .subobj = D3D12_RT_FORMAT_ARRAY{ .RTFormats{ gpass_context::rtv_format }, .NumRenderTargets = 1 } },
				pss_depth_stencil_format{ .subobj = gpass_context::dsv_format },
				pss_rasterizer{ .subobj = g::rasterizer_desc.no_cull },
				pss_depth_stencil1{ .subobj = g::depth_stencil_desc1.disabled },
			};

			g::h_gpass_default_pso = create_pso(stream.storage, stream.size_in_bytes);
			g::pso_ptr_vec[g::h_gpass_default_pso.id]->SetName(L"GPass Default PSO");
		}

		// fx_present_pass default root sig
		{
			using namespace root_signature;

			g::h_fx_present_pass_default_root_sig = root_signature::create(
				descriptor_table{
					D3D12_SHADER_VISIBILITY_PIXEL,
					descriptor_range{ .reg = t{ .idx = 0, .space = 0 }, .count = 1 } });

			g::root_signature_ptr_vec[g::h_fx_present_pass_default_root_sig.id]->SetName(L"FX Present Pass Default Root Sig");
		}

		// fx_present_pass default pso (srgb)
		{
			auto vs_byte_code = shader::get_d3d12_bytecode(shader::shader_handle{ std::to_underlying(shader::engine_shader::fullscreen_triangle_vs) });
			auto ps_byte_code = shader::get_d3d12_bytecode(shader::shader_handle{ std::to_underlying(shader::engine_shader::fx_present_ps) });

			auto stream = pss_stream{
				pss_root_signature{ .subobj = g::root_signature_ptr_vec[g::h_fx_present_pass_default_root_sig.id] },
				pss_vs{ .subobj = vs_byte_code },
				pss_ps{ .subobj = ps_byte_code },
				pss_primitive_topology{ .subobj = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE },
				pss_render_target_formats{ .subobj = D3D12_RT_FORMAT_ARRAY{ .RTFormats{ DXGI_FORMAT_R8G8B8A8_UNORM_SRGB }, .NumRenderTargets = 1 } },
				pss_rasterizer{ .subobj = g::rasterizer_desc.no_cull },
			};

			g::h_fx_present_pass_default_pso = create_pso(stream.storage, stream.size_in_bytes);
			g::pso_ptr_vec[g::h_fx_present_pass_default_pso.id]->SetName(L"FX Present Pass Default PSO");
		}
	}

	void
	deinit() noexcept
	{
		destroy_pso(g::h_gpass_default_pso);
		root_signature::destroy(g::h_gpass_default_root_sig);

		destroy_pso(g::h_fx_present_pass_default_pso);
		root_signature::destroy(g::h_fx_present_pass_default_root_sig);
	}
}	 // namespace age::graphics::pass

#endif

// gpass_ctx
namespace age::graphics::pass
{
	void
	gpass_context::init(uint32 width, uint32 height, root_signature::handle h_root_sig, pso_handle h_pso) noexcept
	{
		p_root_sig = g::root_signature_ptr_vec[h_root_sig.id];
		p_pso	   = g::pso_ptr_vec[h_pso.id];

		h_rtv_desc = g::rtv_desc_pool.pop();
		h_dsv_desc = g::dsv_desc_pool.pop();
		h_srv_desc = g::cbv_srv_uav_desc_pool.pop();

		this->create_buffers(width, height);

		AGE_ASSERT(p_root_sig is_not_nullptr and p_pso is_not_nullptr);
	}

	void
	gpass_context::create_buffers(uint32 width, uint32 height) noexcept
	{
		h_main_buffer = resource::create_resource(
			{ .d3d12_desc{
				  .Dimension		= D3D12_RESOURCE_DIMENSION_TEXTURE2D,
				  .Alignment		= 0,
				  .Width			= width,
				  .Height			= height,
				  .DepthOrArraySize = 1,
				  .MipLevels		= 1,
				  .Format			= rtv_format,
				  .SampleDesc		= { 1, 0 },
				  .Layout			= D3D12_TEXTURE_LAYOUT_UNKNOWN,
				  .Flags			= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET

			  },
			  .clear_value{
				  .Format = rtv_format,
				  .Color  = { clear_color[0], clear_color[1], clear_color[2], clear_color[3] } },
			  .initial_state	= D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			  .heap_memory_kind = resource::memory_kind::gpu_only,
			  .has_clear_value	= true });

		h_depth_buffer = resource::create_resource(
			{ .d3d12_desc{
				  .Dimension		= D3D12_RESOURCE_DIMENSION_TEXTURE2D,
				  .Alignment		= 0,
				  .Width			= width,
				  .Height			= height,
				  .DepthOrArraySize = 1,
				  .MipLevels		= 1,
				  .Format			= dsv_format,
				  .SampleDesc		= { 1, 0 },
				  .Layout			= D3D12_TEXTURE_LAYOUT_UNKNOWN,
				  .Flags			= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL

			  },
			  .clear_value{
				  .Format		= dsv_format,
				  .DepthStencil = { .Depth = clear_depth, .Stencil = 0 } },
			  .initial_state	= D3D12_RESOURCE_STATE_DEPTH_READ,
			  .heap_memory_kind = resource::memory_kind::gpu_only,
			  .has_clear_value	= true });

		main_buffer	 = g::resource_vec[h_main_buffer.id];
		depth_buffer = g::resource_vec[h_depth_buffer.id];

		auto dsv_desc = D3D12_DEPTH_STENCIL_VIEW_DESC{
			.Format		   = dsv_format,
			.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D,
			.Flags		   = D3D12_DSV_FLAG_NONE,
			.Texture2D	   = { .MipSlice = 0 }
		};

		g::p_main_device->CreateDepthStencilView(
			depth_buffer.p_resource,
			&dsv_desc,
			h_dsv_desc.h_cpu);

		auto rtv_desc = D3D12_RENDER_TARGET_VIEW_DESC{
			.Format		   = rtv_format,
			.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D,

			.Texture2D = { .MipSlice = 0, .PlaneSlice = 0 }
		};

		g::p_main_device->CreateRenderTargetView(
			main_buffer.p_resource,
			&rtv_desc,
			h_rtv_desc.h_cpu);

		auto srv_desc = D3D12_SHADER_RESOURCE_VIEW_DESC{
			.Format					 = DXGI_FORMAT_R16G16B16A16_FLOAT,
			.ViewDimension			 = D3D12_SRV_DIMENSION_TEXTURE2D,
			.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
			.Texture2D				 = { .MostDetailedMip	  = 0,
										 .MipLevels			  = 1,
										 .PlaneSlice		  = 0,
										 .ResourceMinLODClamp = 0.f }
		};

		g::p_main_device->CreateShaderResourceView(main_buffer.p_resource, &srv_desc, h_srv_desc.h_cpu);
	}

	void
	gpass_context::resize(uint32 width, uint32 height) noexcept
	{
		resource::release_resource(h_main_buffer);
		resource::release_resource(h_depth_buffer);

		this->create_buffers(width, height);
	}

	void
	gpass_context::deinit() noexcept
	{
		p_root_sig = nullptr;
		p_pso	   = nullptr;

		resource::release_resource(h_main_buffer);
		resource::release_resource(h_depth_buffer);

		main_buffer	 = {};
		depth_buffer = {};

		g::rtv_desc_pool.push(h_rtv_desc);
		g::dsv_desc_pool.push(h_dsv_desc);
	}

	void
	gpass_context::begin(ID3D12GraphicsCommandList9& cmd_list, resource_barrier& barrier, render_surface& rs) noexcept
	{
		barrier.add_transition(*main_buffer.p_resource,
							   D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
							   D3D12_RESOURCE_STATE_RENDER_TARGET);
		barrier.add_transition(*depth_buffer.p_resource,
							   D3D12_RESOURCE_STATE_DEPTH_READ,
							   D3D12_RESOURCE_STATE_DEPTH_WRITE);

		barrier.apply_and_reset(cmd_list);

		cmd_list.SetGraphicsRootSignature(p_root_sig);
		cmd_list.SetPipelineState(p_pso);

		cmd_list.RSSetViewports(1, &rs.default_viewport);
		cmd_list.RSSetScissorRects(1, &rs.default_scissor_rect);

		cmd_list.OMSetRenderTargets(1, &h_rtv_desc.h_cpu, BOOL{ FALSE }, &h_dsv_desc.h_cpu);
		cmd_list.ClearRenderTargetView(h_rtv_desc.h_cpu, clear_color, 0, nullptr);
		cmd_list.ClearDepthStencilView(h_dsv_desc.h_cpu, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, clear_depth, 0, 0, nullptr);
	}

	void
	gpass_context::execute(ID3D12GraphicsCommandList9& cmd_list, render_surface& rs) noexcept
	{
		static auto frame_count = 0ul;

		struct
		{
			float  width;
			float  height;
			uint32 frame;
		} constants{ .width = rs.default_viewport.Width, .height = rs.default_viewport.Height, .frame = frame_count++ };

		cmd_list.SetGraphicsRoot32BitConstants(0, 3, &constants, 0);
		cmd_list.IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		cmd_list.DrawInstanced(3, 1, 0, 0);
	}

	void
	gpass_context::end(resource_barrier& barrier) noexcept
	{
		barrier.add_transition(*main_buffer.p_resource,
							   D3D12_RESOURCE_STATE_RENDER_TARGET,
							   D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		barrier.add_transition(*depth_buffer.p_resource,
							   D3D12_RESOURCE_STATE_DEPTH_WRITE,
							   D3D12_RESOURCE_STATE_DEPTH_READ);
	}
}	 // namespace age::graphics::pass

// fx_present_pass_ctx
namespace age::graphics::pass
{
	void
	fx_present_pass_context::init(root_signature::handle h_root_sig, pso_handle h_pso) noexcept
	{
		p_root_sig = g::root_signature_ptr_vec[h_root_sig.id];
		p_pso	   = g::pso_ptr_vec[h_pso.id];
	}

	void
	fx_present_pass_context::execute(
		ID3D12GraphicsCommandList9&								  cmd_list,
		descriptor_handle<D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV> srv_desc,
		descriptor_handle<D3D12_DESCRIPTOR_HEAP_TYPE_RTV>		  rtv_desc) noexcept
	{
		cmd_list.SetGraphicsRootSignature(p_root_sig);
		cmd_list.SetPipelineState(p_pso);

		cmd_list.SetGraphicsRootDescriptorTable(0, srv_desc.h_gpu);

		cmd_list.IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		cmd_list.OMSetRenderTargets(1, &rtv_desc.h_cpu, 1, nullptr);
		cmd_list.DrawInstanced(3, 1, 0, 0);
	}
}	 // namespace age::graphics::pass
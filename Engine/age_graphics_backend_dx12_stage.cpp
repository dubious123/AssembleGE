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
				D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT,
				constants{
					.reg		= b{ .idx = 1, .space = 0 },
					.num_32bit	= 3,
					.visibility = D3D12_SHADER_VISIBILITY_PIXEL });

			g::root_signature_ptr_vec[g::h_geometry_stage_default_root_sig.id]->SetName(L"GPass Default Root Sig");
		}

		// geometry_stage default pso
		{
			using namespace pso;
			auto vs_byte_code = shader::get_d3d12_bytecode(shader::shader_handle{ std::to_underlying(shader::e::engine_shader_kind::fullscreen_triangle_vs) });
			auto ps_byte_code = shader::get_d3d12_bytecode(shader::shader_handle{ std::to_underlying(shader::e::engine_shader_kind::fill_color_ps) });

			g::h_geometry_stage_default_pso = pso::create(
				pss_root_signature{ .subobj = g::root_signature_ptr_vec[g::h_geometry_stage_default_root_sig] },
				pss_vs{ .subobj = vs_byte_code },
				pss_ps{ .subobj = ps_byte_code },
				pss_primitive_topology{ .subobj = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE },
				pss_render_target_formats{ .subobj = D3D12_RT_FORMAT_ARRAY{ .RTFormats{ DXGI_FORMAT_R16G16B16A16_FLOAT }, .NumRenderTargets = 1 } },
				pss_depth_stencil_format{ .subobj = DXGI_FORMAT_D32_FLOAT },
				pss_rasterizer{ .subobj = defaults::rasterizer_desc::no_cull },
				pss_depth_stencil1{ .subobj = defaults::depth_stencil_desc1::disabled });

			g::pso_ptr_vec[g::h_geometry_stage_default_pso.id]->SetName(L"GPass Default PSO");
		}

		// fx_present_pass default root sig
		{
			using namespace root_signature;

			g::h_fx_present_stage_default_root_sig = root_signature::create(
				D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT,
				descriptor_table{
					D3D12_SHADER_VISIBILITY_PIXEL,
					descriptor_range{ .reg = t{ .idx = 0, .space = 0 }, .count = 1 } });

			g::root_signature_ptr_vec[g::h_fx_present_stage_default_root_sig.id]->SetName(L"FX Present Pass Default Root Sig");
		}

		// fx_present_pass default pso (srgb)
		{
			using namespace pso;
			auto vs_byte_code = shader::get_d3d12_bytecode(shader::shader_handle{ std::to_underlying(shader::e::engine_shader_kind::fullscreen_triangle_vs) });
			auto ps_byte_code = shader::get_d3d12_bytecode(shader::shader_handle{ std::to_underlying(shader::e::engine_shader_kind::fx_present_ps) });

			g::h_fx_present_stage_default_pso = pso::create(
				pss_root_signature{ .subobj = g::root_signature_ptr_vec[g::h_fx_present_stage_default_root_sig.id] },
				pss_vs{ .subobj = vs_byte_code },
				pss_ps{ .subobj = ps_byte_code },
				pss_primitive_topology{ .subobj = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE },
				pss_render_target_formats{ .subobj = D3D12_RT_FORMAT_ARRAY{ .RTFormats{ DXGI_FORMAT_R10G10B10A2_UNORM }, .NumRenderTargets = 1 } },
				pss_rasterizer{ .subobj = defaults::rasterizer_desc::no_cull });

			g::pso_ptr_vec[g::h_fx_present_stage_default_pso]->SetName(L"FX Present Pass Default PSO");
		}
	}

	void
	deinit() noexcept
	{
		pso::destroy(g::h_geometry_stage_default_pso);
		root_signature::destroy(g::h_geometry_stage_default_root_sig);

		pso::destroy(g::h_fx_present_stage_default_pso);
		root_signature::destroy(g::h_fx_present_stage_default_root_sig);
	}
}	 // namespace age::graphics::stage
#endif
#include "age_pch.hpp"
#include "age.hpp"
#if defined(AGE_GRAPHICS_BACKEND_DX12)
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
				   graphics::pso::handle			h_pso,
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
	my_stage::execute(t_cmd_list& cmd_list, render_surface& rs) noexcept
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
			auto d = rs.get_h_cpu_desc();
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
	my_pipeline::execute(t_cmd_list& cmd_list, render_surface& rs) noexcept
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
#endif
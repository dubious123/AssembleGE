#include "age_pch.hpp"
#include "age.hpp"

#if defined(AGE_GRAPHICS_BACKEND_DX12)
// main
namespace age::graphics
{
	void
	init() noexcept
	{
		if constexpr (age::config::debug_mode)
		{
			auto* p_debug = (ID3D12Debug3*)nullptr;
			AGE_HR_CHECK(::D3D12GetDebugInterface(IID_PPV_ARGS(&p_debug)));

			p_debug->EnableDebugLayer();

			// #if 1
			//		p_debug->SetEnableGPUBasedValidation(true);
			// #endif

			p_debug->Release();
		}

		AGE_HR_CHECK(::CreateDXGIFactory2(g::dxgi_factory_flag, IID_PPV_ARGS(&g::p_dxgi_factory)));

		for (uint32 adapter_idx : std::views::iota(0))
		{
			auto* p_adapter = (IDXGIAdapter4*)nullptr;
			auto* p_device	= (ID3D12Device11*)nullptr;
			if (g::p_dxgi_factory->EnumAdapterByGpuPreference(adapter_idx, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&p_adapter)) == DXGI_ERROR_NOT_FOUND)
			{
				assert(false);
				break;
			}

			assert(p_adapter is_not_nullptr);
			if (FAILED(::D3D12CreateDevice(p_adapter, g::minimum_feature_level, IID_PPV_ARGS(&p_device))))
			{
				p_adapter->Release();
			}
			else
			{
				g::p_main_adapter = p_adapter;
				g::p_main_device  = p_device;
				break;
			}
		}

		AGE_ASSERT(g::p_main_adapter is_not_nullptr);
		AGE_ASSERT(g::p_main_device is_not_nullptr);


		if constexpr (age::config::debug_mode)
		{
			g::p_main_device->SetName(L"age graphics main device");

			auto* p_info_queue = (ID3D12InfoQueue*)nullptr;
			AGE_HR_CHECK(g::p_main_device->QueryInterface(IID_PPV_ARGS(&p_info_queue)));

			p_info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
			p_info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
			p_info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);

			p_info_queue->Release();
		}

		{
			g::cmd_system_direct.init();
			g::cmd_system_compute.init();
			g::cmd_system_copy.init();
		}

		age::graphics::resource::init();

		{
			g::rtv_desc_pool.init();
			g::dsv_desc_pool.init();
			g::cbv_srv_uav_desc_pool.init();
			g::sampler_desc_pool.init();
		}

		age::graphics::root_signature::init();

		age::graphics::shader::init();

		age::graphics::stage::init();

		g::test_pipeline.init();
	}

	void
	deinit() noexcept
	{
		if constexpr (age::config::debug_mode)
		{
			auto* p_info_queue = (ID3D12InfoQueue*)nullptr;
			AGE_HR_CHECK(g::p_main_device->QueryInterface(IID_PPV_ARGS(&p_info_queue)));

			p_info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, false);
			p_info_queue->Release();
		}

		{
			g::cmd_system_compute.flush();
			g::cmd_system_copy.flush();
			g::cmd_system_direct.flush();
		}

		{
			for (auto& rs : g::render_surface_vec)
			{
				rs.deinit();
			}

			if constexpr (age::config::debug_mode)
			{
				g::render_surface_vec.debug_validate();
			}

			// test
			g::test_pipeline.deinit();
		}

		age::graphics::stage::deinit();

		age::graphics::shader::deinit();

		age::graphics::root_signature::deinit();

		{
			for (auto* p_pso : g::pso_ptr_vec)
			{
				p_pso->Release();
			}

			if constexpr (age::config::debug_mode)
			{
				g::pso_ptr_vec.debug_validate();
			}

			g::pso_ptr_vec.clear();
		}

		{
			g::sampler_desc_pool.deinit();
			g::cbv_srv_uav_desc_pool.deinit();
			g::dsv_desc_pool.deinit();
			g::rtv_desc_pool.deinit();
		}

		age::graphics::resource::deinit();

		{
			if constexpr (age::config::debug_mode)
			{
				g::current_fence_value = std::numeric_limits<decltype(g::current_fence_value)>::max();
			}

			g::cmd_system_copy.deinit();
			g::cmd_system_compute.deinit();
			g::cmd_system_direct.deinit();
		}

		if constexpr (age::config::debug_mode)
		{
			// requires "graphics tools" optional feature
			// settings -> system -> optional features -> add on aptional feature -> select and install graphics tools
			auto* p_debug_device = (ID3D12DebugDevice2*)nullptr;
			AGE_HR_CHECK(g::p_main_device->QueryInterface(IID_PPV_ARGS(&p_debug_device)));

			AGE_HR_CHECK(p_debug_device->ReportLiveDeviceObjects(
				D3D12_RLDO_SUMMARY | D3D12_RLDO_DETAIL | D3D12_RLDO_IGNORE_INTERNAL));

			p_debug_device->Release();
		}

		g::p_main_device->Release();
		g::p_main_adapter->Release();
		g::p_dxgi_factory->Release();

		if constexpr (age::config::debug_mode)
		{
			auto* p_dxgi_debug = (IDXGIDebug1*)nullptr;
			AGE_HR_CHECK(::DXGIGetDebugInterface1(0, IID_PPV_ARGS(&p_dxgi_debug)));
			p_dxgi_debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
			p_dxgi_debug->Release();
		}
	}

	void
	begin_frame() noexcept
	{
		for (auto& req : request::for_each<subsystem::type::graphics>())
		{
			switch (req.type)
			{
			case request::type::window_closed:
			{
				AGE_ASSERT(req.phase == 0, "[{}] : invalid phase : {}", to_string(req.type), req.phase);

				auto  h_window = req.req_param.as<platform::window_handle>();
				auto  h_rs	   = graphics::find_render_surface(h_window);
				auto& rs	   = g::render_surface_vec[h_rs];

				rs.should_render = false;

				{
					AGE_HR_CHECK(rs.p_swap_chain->SetFullscreenState(false, nullptr));
				}

				auto wait_event = ::WaitForSingleObject(rs.present_waitable_obj, 0);
				AGE_ASSERT((wait_event == WAIT_TIMEOUT) or (wait_event == WAIT_OBJECT_0));

				auto is_pending =
					wait_event == WAIT_TIMEOUT
					or g::cmd_system_direct.p_fence->GetCompletedValue() <= rs.last_used_cmd_fence_value;

				if (is_pending is_false)
				{
					rs.deinit();
					g::render_surface_vec.remove(h_rs);

					request::set_done<
						subsystem::type::graphics,
						request::type::window_closed, 0>(req);
				}
				break;
			}
			case request::type::window_resized:
			{
				AGE_ASSERT(req.phase == 0, "[{}] : invalid phase : {}", to_string(req.type), req.phase);

				auto  h_window = req.req_param.as<platform::window_handle>();
				auto  h_rs	   = graphics::find_render_surface(h_window);
				auto& rs	   = g::render_surface_vec[h_rs];

				rs.should_render = false;

				auto wait_event = ::WaitForSingleObject(rs.present_waitable_obj, 0);
				AGE_ASSERT((wait_event == WAIT_TIMEOUT) or (wait_event == WAIT_OBJECT_0));

				auto is_pending =
					wait_event == WAIT_TIMEOUT
					or g::cmd_system_direct.p_fence->GetCompletedValue() <= rs.last_used_cmd_fence_value;

				if (is_pending is_false)
				{
					if (platform::get_window_state(h_window) != platform::window_state::maximized
						and is_tearing_allowed())
					{
						rs.present_flags |= DXGI_PRESENT_ALLOW_TEARING;
					}

					rs.resize();
					rs.should_render = true;

					request::set_done<
						subsystem::type::graphics,
						request::type::window_resized, 0>(req);
				}

				break;
			}
			case request::type::window_maximized:
			{
				AGE_ASSERT(req.phase == 0, "[{}] : invalid phase : {}", to_string(req.type), req.phase);

				auto  h_window = req.req_param.as<platform::window_handle>();
				auto  h_rs	   = graphics::find_render_surface(h_window);
				auto& rs	   = g::render_surface_vec[h_rs];

				rs.present_flags &= ~DXGI_PRESENT_ALLOW_TEARING;

				{
					auto p_target = (IDXGIOutput*)nullptr;
					AGE_HR_CHECK(rs.p_swap_chain->GetContainingOutput(&p_target));
					rs.p_swap_chain->SetFullscreenState(true, p_target);
					p_target->Release();
				}

				rs.should_render = false;
				request::set_done<
					subsystem::type::graphics,
					request::type::window_maximized, 0>(req);

				request::create<request::type::window_resized>(h_window);
				break;
			}
			case request::type::window_minimized:
			{
				AGE_ASSERT(req.phase == 0, "[{}] : invalid phase : {}", to_string(req.type), req.phase);

				auto  h_window = req.req_param.as<platform::window_handle>();
				auto  h_rs	   = graphics::find_render_surface(h_window);
				auto& rs	   = g::render_surface_vec[h_rs];

				rs.should_render = false;

				request::set_done<
					subsystem::type::graphics,
					request::type::window_minimized, 0>(req);
				break;
			}
			default:
			{
				AGE_UNREACHABLE("invalid request type : {}", to_string(req.type));
			}
			}
		}

		g::cmd_system_direct.wait();
		g::cmd_system_direct.begin_frame();
		g::cmd_system_compute.wait();
		g::cmd_system_compute.begin_frame();
		g::cmd_system_copy.wait();
		g::cmd_system_copy.begin_frame();
	}

	void
	render() noexcept
	{
		constexpr auto thread_idx = 4;
		auto&		   cmd_list	  = *g::cmd_system_direct.cmd_list_pool[g::frame_buffer_idx][thread_idx];

		cmd_list.SetDescriptorHeaps(1, &g::cbv_srv_uav_desc_pool.p_descriptor_heap);

		for (auto& rs : g::render_surface_vec)
		{
			if (rs.should_render is_false) [[unlikely]]
			{
				continue;
			}

			g::test_pipeline.execute(cmd_list, rs);
		}
	}

	void
	end_frame() noexcept
	{
		g::cmd_system_direct.end_frame();
		g::cmd_system_compute.end_frame();
		g::cmd_system_copy.end_frame();


		for (auto n : std::views::iota(0) | std::views::take(g::render_surface_vec.count()) | std::views::reverse)
		{
			auto& rs = g::render_surface_vec.nth_data(n);

			if (rs.should_render is_false) [[unlikely]]
			{
				continue;
			}

			rs.present();
		}


		g::frame_buffer_idx = (g::frame_buffer_idx + 1) % g::frame_buffer_count;

		AGE_DEBUG_LOG("fence_value : {}", g::current_fence_value);

		++g::current_fence_value;
	}
}	 // namespace age::graphics
#endif
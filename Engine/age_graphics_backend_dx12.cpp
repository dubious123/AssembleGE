#include "age_pch.hpp"
#include "age.hpp"

#if defined AGE_GRAPHICS_BACKEND_DX12

// cmd_system member func
namespace age::graphics
{
	template <auto cmd_list_type, auto cmd_list_count>
	FORCE_INLINE void
	cmd_system<cmd_list_type, cmd_list_count>::init() noexcept
	{
		constexpr auto wstr_type = [] constexpr {
			switch (cmd_list_type)
			{
			case D3D12_COMMAND_LIST_TYPE_DIRECT:
				return L"Direct";
			case D3D12_COMMAND_LIST_TYPE_COMPUTE:
				return L"Compute";
			case D3D12_COMMAND_LIST_TYPE_COPY:
				return L"Copy";
			default:
				return L"Unknown";
			}
		}();

		D3D12_COMMAND_QUEUE_DESC desc{
			.Type	  = cmd_list_type,
			.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL,
			.Flags	  = D3D12_COMMAND_QUEUE_FLAG_NONE,
			.NodeMask = 0
		};


		{
			AGE_HR_CHECK(g::p_main_device->CreateCommandQueue(&desc, IID_PPV_ARGS(&p_cmd_queue)));
			auto queue_name = std::format(L"[{}] cmd queue", wstr_type);
			p_cmd_queue->SetName(queue_name.c_str());
		}

		{
			AGE_HR_CHECK(g::p_main_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&p_fence)));
			auto fence_name = std::format(L"[{}] fence", wstr_type);
			p_fence->SetName(fence_name.c_str());
		}

		{
			fence_event = ::CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS);
			assert(fence_event);
		}

		for (auto i : std::views::iota(0, g::frame_buffer_count))
		{
			for (auto j : std::views::iota(0, cmd_list_count))
			{
				AGE_HR_CHECK(g::p_main_device->CreateCommandAllocator(cmd_list_type, IID_PPV_ARGS(&cmd_allocator_pool[i][j])));

				AGE_HR_CHECK(g::p_main_device->CreateCommandList(
					0,
					cmd_list_type,
					cmd_allocator_pool[i][j],
					nullptr,
					IID_PPV_ARGS(&cmd_list_pool[i][j])));

				AGE_HR_CHECK(cmd_list_pool[i][j]->Close());

				auto list_name	= std::format(L"[{}] cmd list[{}][{}]", wstr_type, i, j);
				auto queue_name = std::format(L"[{}] cmd list allocator[{}][{}]", wstr_type, i, j);

				cmd_list_pool[i][j]->SetName(list_name.c_str());
				cmd_allocator_pool[i][j]->SetName(queue_name.c_str());
			}
		}
	}

	template <auto cmd_list_type, auto cmd_list_count>
	FORCE_INLINE void
	cmd_system<cmd_list_type, cmd_list_count>::deinit() noexcept
	{
		{
			AGE_HR_CHECK(p_cmd_queue->Signal(p_fence, g::current_fence_value));
			AGE_HR_CHECK(p_fence->SetEventOnCompletion(g::current_fence_value, fence_event));
			::WaitForSingleObject(fence_event, INFINITE);
		}

		p_cmd_queue->Release();
		p_fence->Release();
		::CloseHandle(fence_event);

		for (auto* p_cmd_list : std::views::join(cmd_list_pool))
		{
			p_cmd_list->Release();
		}

		for (auto* p_cmd_allocator : std::views::join(cmd_allocator_pool))
		{
			p_cmd_allocator->Release();
		}
	}

	template <auto cmd_list_type, auto cmd_list_count>
	FORCE_INLINE void
	cmd_system<cmd_list_type, cmd_list_count>::wait() noexcept
	{
		auto completed_value = p_fence->GetCompletedValue();
		auto need_to_wait	 = completed_value + g::frame_buffer_count < g::current_fence_value;

		std::println("[{}], fence_value = {}, completed_value= {}, need_to_wait= {},", (int)cmd_list_type, g::current_fence_value, completed_value, need_to_wait);

		if (need_to_wait)
		{
			AGE_HR_CHECK(p_fence->SetEventOnCompletion(completed_value + 1, fence_event));

			::WaitForSingleObject(fence_event, INFINITE);
		}
	}

	template <auto cmd_list_type, auto cmd_list_count>
	FORCE_INLINE void
	cmd_system<cmd_list_type, cmd_list_count>::begin_frame() noexcept
	{
		for (auto i : std::views::iota(0, cmd_list_count))
		{
			AGE_HR_CHECK(cmd_allocator_pool[g::frame_buffer_idx][i]->Reset());
			AGE_HR_CHECK(cmd_list_pool[g::frame_buffer_idx][i]->Reset(cmd_allocator_pool[g::frame_buffer_idx][i], nullptr));
		}
	}

	template <auto cmd_list_type, auto cmd_list_count>
	FORCE_INLINE void
	cmd_system<cmd_list_type, cmd_list_count>::end_frame() noexcept
	{
		// todo hmm...
		auto cmd_lists = std::array<ID3D12CommandList*, cmd_list_count>{};
		for (auto i : std::views::iota(0, cmd_list_count))
		{
			AGE_HR_CHECK(cmd_list_pool[g::frame_buffer_idx][i]->Close());

			cmd_lists[i] = cmd_list_pool[g::frame_buffer_idx][i];
		}

		p_cmd_queue->ExecuteCommandLists(cmd_list_count, cmd_lists.data());

		std::println("[{}], signaling fence, from {} to {}", (int)cmd_list_type, p_fence->GetCompletedValue(), g::current_fence_value);
		AGE_HR_CHECK(p_cmd_queue->Signal(p_fence, g::current_fence_value));
	}

	template <auto cmd_list_type, auto cmd_list_count>
	FORCE_INLINE void
	cmd_system<cmd_list_type, cmd_list_count>::flush() noexcept
	{
		AGE_HR_CHECK(p_fence->SetEventOnCompletion(g::current_fence_value - 1, fence_event));

		::WaitForSingleObject(fence_event, INFINITE);
	}
}	 // namespace age::graphics

// swap_chain member func
namespace age::graphics
{
	void
	render_surface::init(age::platform::window_handle w_handle) noexcept
	{
		this->w_handle = w_handle;

		{
			auto allow_tearing = BOOL{ FALSE };
			{
				AGE_HR_CHECK(g::p_dxgi_factory->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allow_tearing, sizeof(allow_tearing)));
				present_flags = allow_tearing ? DXGI_PRESENT_ALLOW_TEARING : UINT{ 0 };
			}

			DXGI_FORMAT dxgi_format{};
			{
				switch (global::get<interface>().display_color_space())
				{
				case color_space::srgb:
					dxgi_format = DXGI_FORMAT_R8G8B8A8_UNORM;
					rtv_format	= DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
					break;
				case color_space::hdr:
					dxgi_format = DXGI_FORMAT_R10G10B10A2_UNORM;
					rtv_format	= DXGI_FORMAT_R10G10B10A2_UNORM;
					break;
				default:
					std::unreachable();
				}
			}

			auto swap_chain_desc = DXGI_SWAP_CHAIN_DESC1{
				/*UINT					*/ .Width		= platform::get_client_width(w_handle),
				/*UINT					*/ .Height		= platform::get_client_height(w_handle),
				/*DXGI_FORMAT			*/ .Format		= dxgi_format,
				/*BOOL					*/ .Stereo		= false,
				/*DXGI_SAMPLE_DESC		*/ .SampleDesc	= { .Count = 1, .Quality = 0 },
				/*DXGI_USAGE			*/ .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
				/*UINT					*/ .BufferCount = g::frame_buffer_count,
				/*DXGI_SCALING			*/ .Scaling		= DXGI_SCALING_STRETCH,
				/*DXGI_SWAP_EFFECT		*/ .SwapEffect	= DXGI_SWAP_EFFECT_FLIP_DISCARD,
				/*DXGI_ALPHA_MODE		*/ .AlphaMode	= DXGI_ALPHA_MODE_UNSPECIFIED,
				/*UINT					*/ .Flags =
					DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT
					| (allow_tearing ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : UINT{ 0 }),
			};

			auto full_screen_desc = DXGI_SWAP_CHAIN_FULLSCREEN_DESC{
				/*DXGI_RATIONAL				*/ .RefreshRate		 = DXGI_RATIONAL{ 0, 1 },
				/*DXGI_MODE_SCANLINE_ORDER	*/ .ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED,
				/*DXGI_MODE_SCALING			*/ .Scaling			 = DXGI_MODE_SCALING_UNSPECIFIED,
				/*BOOL						*/ .Windowed		 = BOOL{ true },
			};

			auto* p_swap_chain_1 = (IDXGISwapChain1*)nullptr;
			{
				AGE_HR_CHECK(g::p_dxgi_factory->CreateSwapChainForHwnd(
					g::cmd_system_direct.p_cmd_queue,
					platform::get_hwnd(w_handle),
					&swap_chain_desc,
					&full_screen_desc,
					nullptr,
					&p_swap_chain_1));
			}

			AGE_HR_CHECK(g::p_dxgi_factory->MakeWindowAssociation(platform::get_hwnd(w_handle), DXGI_MWA_NO_ALT_ENTER));

			AGE_HR_CHECK(p_swap_chain_1->QueryInterface(IID_PPV_ARGS(&p_swap_chain)));
			p_swap_chain_1->Release();
		}

		AGE_HR_CHECK(p_swap_chain->SetMaximumFrameLatency(3));

		present_waitable_obj = p_swap_chain->GetFrameLatencyWaitableObject();

		AGE_ASSERT(present_waitable_obj != NULL);

		back_buffer_idx = p_swap_chain->GetCurrentBackBufferIndex();

		for (uint32 idx : std::views::iota(0) | std::views::take(g::frame_buffer_count))
		{
			rtv_desc_handle_arr[idx] = g::rtv_desc_pool.pop();
		}

		rebuild_from_swapchain();
	}

	void
	render_surface::resize() noexcept
	{
		for (auto* p_resource : back_buffer_ptr_arr)
		{
			p_resource->Release();
		}

		auto allow_tearing = BOOL{ FALSE };
		{
			AGE_HR_CHECK(g::p_dxgi_factory->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allow_tearing, sizeof(allow_tearing)));
			present_flags = allow_tearing ? DXGI_PRESENT_ALLOW_TEARING : UINT{ 0 };
		}

		p_swap_chain->ResizeBuffers(
			g::frame_buffer_count,
			0,
			0,
			DXGI_FORMAT_UNKNOWN,
			DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT
				| (allow_tearing ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : UINT{ 0 }));


		rebuild_from_swapchain();
	}

	void
	render_surface::present() noexcept
	{
		AGE_HR_CHECK(p_swap_chain->Present(0, present_flags));
		back_buffer_idx			  = p_swap_chain->GetCurrentBackBufferIndex();
		last_used_cmd_fence_value = g::current_fence_value;

		std::println("present, fence_value :  {}", g::current_fence_value);
	}

	void
	render_surface::deinit() noexcept
	{
		for (auto idx : std::views::iota(0) | std::views::take(g::frame_buffer_count))
		{
			g::rtv_desc_pool.push(rtv_desc_handle_arr[idx]);
			back_buffer_ptr_arr[idx]->Release();
		}

		p_swap_chain->Release();

		::CloseHandle(present_waitable_obj);
	}

	void
	render_surface::rebuild_from_swapchain() noexcept
	{
		for (uint32 idx : std::views::iota(0) | std::views::take(g::frame_buffer_count))
		{
			AGE_HR_CHECK(p_swap_chain->GetBuffer(idx, IID_PPV_ARGS(&back_buffer_ptr_arr[idx])));

			auto rtv_desc = D3D12_RENDER_TARGET_VIEW_DESC{
				/*DXGI_FORMAT			*/ .Format		  = rtv_format,
				/*D3D12_RTV_DIMENSION	*/ .ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D,
				/*D3D12_TEX2D_RTV		*/ .Texture2D	  = D3D12_TEX2D_RTV{ .MipSlice = 0, .PlaneSlice = 0 }
			};

			g::p_main_device->CreateRenderTargetView(back_buffer_ptr_arr[idx], &rtv_desc, rtv_desc_handle_arr[idx].h_cpu);
		}

		auto desc = DXGI_SWAP_CHAIN_DESC{};

		AGE_HR_CHECK(p_swap_chain->GetDesc(&desc));
		AGE_ASSERT(desc.BufferDesc.Width == platform::get_client_width(w_handle));
		AGE_ASSERT(desc.BufferDesc.Height == platform::get_client_height(w_handle));

		default_viewport = D3D12_VIEWPORT{
			.TopLeftX = 0.f,
			.TopLeftY = 0.f,
			.Width	  = static_cast<float>(desc.BufferDesc.Width),
			.Height	  = static_cast<float>(desc.BufferDesc.Height),
			.MinDepth = 0.f,
			.MaxDepth = 1.f,
		};

		default_scissor_rect = D3D12_RECT{
			.left	= 0l,
			.top	= 0l,
			.right	= static_cast<int32>(desc.BufferDesc.Width),
			.bottom = static_cast<int32>(desc.BufferDesc.Height),
		};
	}
}	 // namespace age::graphics

namespace age::graphics
{
	pso_handle
	create_pso(void* pss_stream, uint32 size_in_bytes) noexcept
	{
		AGE_ASSERT(pss_stream is_not_nullptr);
		AGE_ASSERT(size_in_bytes > 0);
		auto* p_pso = (ID3D12PipelineState*)nullptr;
		auto  desc	= D3D12_PIPELINE_STATE_STREAM_DESC{
			  .SizeInBytes					 = size_in_bytes,
			  .pPipelineStateSubobjectStream = pss_stream,
		};

		AGE_HR_CHECK(g::p_main_device->CreatePipelineState(&desc, IID_PPV_ARGS(&p_pso)));

		return pso_handle{ .id = g::pso_ptr_vec.emplace_back(p_pso) };
	}

	void
	destroy_pso(pso_handle h_pso) noexcept
	{
		g::pso_ptr_vec[h_pso.id]->Release();
		g::pso_ptr_vec.remove(h_pso.id);
	}
}	 // namespace age::graphics

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

		age::graphics::pass::init();

		{
			using namespace root_signature;
			auto rs_handle = create(
				constants{
					.reg	   = b{ .idx = 0, .space = 0 },
					.num_32bit = 16 },
				descriptor{ .reg = b{ .idx = 1, .space = 0 } });

			auto input_layout = std::array{ D3D12_INPUT_ELEMENT_DESC{
				/*LPCSTR					   */ .SemanticName		= "POSITION",
				/*UINT					   */ .SemanticIndex		= 0,
				/*DXGI_FORMAT				   */ .Format			= DXGI_FORMAT_R32G32B32A32_FLOAT,
				/*UINT					   */ .InputSlot			= 0,
				/*UINT					   */ .AlignedByteOffset	= 0,
				/*D3D12_INPUT_CLASSIFICATION */ .InputSlotClass		= D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
				/*UINT					   */ .InstanceDataStepRate = 0,
			} };


			auto vs_byte_code = shader::get_d3d12_bytecode(shader::shader_handle{ std::to_underlying(shader::engine_shader::test_vs) });
			auto ps_byte_code = shader::get_d3d12_bytecode(shader::shader_handle{ std::to_underlying(shader::engine_shader::test_ps) });


			auto stream = pss_stream{
				pss_root_signature{ .subobj = g::root_signature_ptr_vec[rs_handle.id] },
				pss_input_layout{ .subobj = D3D12_INPUT_LAYOUT_DESC{
									  .pInputElementDescs = input_layout.data(), .NumElements = 1 } },
				pss_vs{ .subobj = vs_byte_code },
				pss_ps{ .subobj = ps_byte_code },
				pss_primitive_topology{ .subobj = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE },
				pss_render_target_formats{ .subobj = D3D12_RT_FORMAT_ARRAY{ .RTFormats{ DXGI_FORMAT_R8G8B8A8_UNORM }, .NumRenderTargets = 1 } },
				pss_sample_desc{ .subobj = DXGI_SAMPLE_DESC{ .Count = 1, .Quality = 0 } },
			};

			create_pso(stream.storage, stream.size_in_bytes);
		}
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
			for (auto& gpass_ctx : g::gpass_ctx_vec)
			{
				gpass_ctx.deinit();
			}

			if constexpr (age::config::debug_mode)
			{
				g::gpass_ctx_vec.debug_validate();
			}

			for (auto& fx_ctx : g::fx_present_pass_ctx_vec)
			{
				//			fx_ctx.deinit();
			}

			if constexpr (age::config::debug_mode)
			{
				g::fx_present_pass_ctx_vec.debug_validate();
			}
		}

		age::graphics::pass::deinit();

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
	}

	render_surface_handle
	create_render_surface(platform::window_handle w_handle) noexcept
	{
		auto id = g::render_surface_vec.emplace_back();
		g::render_surface_vec[id].init(w_handle);

		auto& rs = g::render_surface_vec[id];

		// test code
		auto gpass_ctx = age::graphics::pass::gpass_context{};
		gpass_ctx.init(static_cast<uint32>(rs.default_viewport.Width), static_cast<uint32>(rs.default_viewport.Height), g::h_gpass_default_root_sig, g::h_gpass_default_pso);
		g::gpass_ctx_vec.emplace_back(gpass_ctx);

		auto fx_ctx = age::graphics::pass::fx_present_pass_context{};
		fx_ctx.init(g::h_fx_present_pass_default_root_sig, g::h_fx_present_pass_default_pso);
		g::fx_present_pass_ctx_vec.emplace_back(fx_ctx);

		return render_surface_handle{ .id = id };
	}

	void
	begin_frame() noexcept
	{
		// for( auto _ : std::views::iota(0, age::request::count<age::module::graphics>()) )
		// auto& request = age::request::pop<age::module::graphics>();
		//	switch(request.type)
		//	  case age::request::type::window_close :
		//		 auto h_window = request.param.as<age::platform::window_handle>();
		//		 auto h_render_surface = ???
		//		 auto& rs = g::render_surface_vec[h_render_surface.id];
		//		 rs.rendering_enabled = false;
		//       if( rs.is_somebody_using )
		//			age::request::push_pending(request)
		//       else
		//			rs.deinit();
		//          g::render_surface_vec.remove(h_render_surface.id);
		//		    age::request::push_done(request)
		//
		//


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
		auto  thread_num = 4;
		auto  barrier	 = resource_barrier{};
		auto& cmd_list	 = *g::cmd_system_direct.cmd_list_pool[g::frame_buffer_idx][thread_num];
		for (auto idx : std::views::iota(0) | std::views::take(g::render_surface_vec.count()))
		{
			auto& rs		   = g::render_surface_vec[idx];
			auto& gpass		   = g::gpass_ctx_vec[idx];
			auto& present_pass = g::fx_present_pass_ctx_vec[idx];

			if (platform::is_closing(rs.w_handle)) [[unlikely]]
			{
				continue;
			}


			cmd_list.SetDescriptorHeaps(1, &g::cbv_srv_uav_desc_pool.p_descriptor_heap);
			barrier.add_transition(rs.get_back_buffer(),
								   D3D12_RESOURCE_STATE_PRESENT,
								   D3D12_RESOURCE_STATE_RENDER_TARGET,
								   D3D12_RESOURCE_BARRIER_FLAG_BEGIN_ONLY);

			gpass.begin(cmd_list, barrier, rs);

			gpass.execute(cmd_list, rs);

			gpass.end(barrier);

			// post process
			barrier.add_transition(rs.get_back_buffer(),
								   D3D12_RESOURCE_STATE_PRESENT,
								   D3D12_RESOURCE_STATE_RENDER_TARGET,
								   D3D12_RESOURCE_BARRIER_FLAG_END_ONLY);

			barrier.apply_and_reset(cmd_list);

			present_pass.execute(cmd_list, gpass.h_srv_desc, rs.h_rtv_desc());

			barrier.add_transition(
				rs.get_back_buffer(),
				D3D12_RESOURCE_STATE_RENDER_TARGET,
				D3D12_RESOURCE_STATE_PRESENT);

			barrier.apply_and_reset(cmd_list);

			// rs.present();
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
			auto  id = g::render_surface_vec.nth_id(n);

			if (platform::is_closing(rs.w_handle)) [[unlikely]]
			{
				bool is_pending = false;

				auto res = ::WaitForSingleObject(rs.present_waitable_obj, 0);
				AGE_ASSERT((res == WAIT_TIMEOUT) or (res == WAIT_OBJECT_0));

				is_pending |= (res == WAIT_TIMEOUT);
				is_pending |= g::cmd_system_direct.p_fence->GetCompletedValue() < rs.last_used_cmd_fence_value;
				std::println("[{}] is_pending =  {} < {}", id, g::cmd_system_direct.p_fence->GetCompletedValue(), rs.last_used_cmd_fence_value);

				platform::set_graphics_cleanup_pending(rs.w_handle, is_pending);	// this should not happen

				if (is_pending is_false)
				{
					rs.deinit();
					g::render_surface_vec.remove(id);
				}
			}
			else
			{
				rs.present();
			}
		}

		g::frame_buffer_idx = (g::frame_buffer_idx + 1) % g::frame_buffer_count;

		std::println("fence_value : {}", g::current_fence_value);

		++g::current_fence_value;
	}
}	 // namespace age::graphics
#endif
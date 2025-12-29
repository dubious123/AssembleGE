#include "age_pch.hpp"
#include "age.hpp"

#if defined AGE_GRAPHICS_BACKEND_DX12

// internal globals
namespace age::graphics::g
{
	auto frame_buffer_idx = uint8{ 0 };
	auto next_fence_value = uint64{ frame_buffer_count };

	auto* p_dxgi_factory = (IDXGIFactory7*)nullptr;
	auto* p_main_adapter = (IDXGIAdapter4*)nullptr;
	auto* p_main_device	 = (ID3D12Device11*)nullptr;

	auto cmd_system_direct	= cmd_system<D3D12_COMMAND_LIST_TYPE_DIRECT, 8>{};
	auto cmd_system_compute = cmd_system<D3D12_COMMAND_LIST_TYPE_COMPUTE, 2>{};
	auto cmd_system_copy	= cmd_system<D3D12_COMMAND_LIST_TYPE_COPY, 2>{};

	// todo config capacity
	auto rtv_desc_pool		   = descriptor_pool<D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 2048>{};
	auto dsv_desc_pool		   = descriptor_pool<D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 512>{};
	auto cbv_srv_uav_desc_pool = descriptor_pool<D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 512 * 1024>{};
	auto sampler_desc_pool	   = descriptor_pool<D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, 512>{};

	auto render_surfaces	= data_structure::sparse_vector<render_surface>{};
	auto render_surface_ids = data_structure::vector<std::size_t>{};
}	 // namespace age::graphics::g

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
			AGE_HR_CHECK(p_cmd_queue->Signal(p_fence, g::next_fence_value));
			AGE_HR_CHECK(p_fence->SetEventOnCompletion(g::next_fence_value, fence_event));
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
		auto expected_value	 = g::next_fence_value - g::frame_buffer_count;
		auto need_to_wait	 = completed_value < expected_value;

		// std::println("[{}], fence_value = {}, completed_value= {}, need_to_wait= {},", (int)cmd_list_type, fence_value, completed_value, need_to_wait);

		if (need_to_wait)
		{
			AGE_HR_CHECK(p_fence->SetEventOnCompletion(expected_value, fence_event));

			::WaitForSingleObject(fence_event, INFINITE);
		}
	}

	template <auto cmd_list_type, auto cmd_list_count>
	FORCE_INLINE void
	cmd_system<cmd_list_type, cmd_list_count>::begin_frame() noexcept
	{
		wait();
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

		// std::println("[{}], signaling fence, from {} to {}", (int)cmd_list_type, p_fence->GetCompletedValue(), fence_value);
		AGE_HR_CHECK(p_cmd_queue->Signal(p_fence, g::next_fence_value));
	}
}	 // namespace age::graphics

// descriptor_pool member func
namespace age::graphics
{
	template <D3D12_DESCRIPTOR_HEAP_TYPE heap_type, std::size_t capacity>
	FORCE_INLINE void
	descriptor_pool<heap_type, capacity>::init() noexcept
	{
		static_assert((capacity > 0) and (capacity <= D3D12_MAX_SHADER_VISIBLE_DESCRIPTOR_HEAP_SIZE_TIER_2));
		static_assert(not((heap_type == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER) and (capacity > D3D12_MAX_SHADER_VISIBLE_SAMPLER_HEAP_SIZE)));

		{
			D3D12_DESCRIPTOR_HEAP_DESC desc{
				.Type			= heap_type,
				.NumDescriptors = capacity,
				.Flags			= is_shader_visible() ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
				.NodeMask		= 0
			};

			constexpr auto wstr_type = [] constexpr {
				switch (heap_type)
				{
				case D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV:
					return L"[CBV_SRV_UAV] Descriptor Heap";
				case D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER:
					return L"[SAMPLER] Descriptor Heap";
				case D3D12_DESCRIPTOR_HEAP_TYPE_RTV:
					return L"[RTV] Descriptor Heap";
				case D3D12_DESCRIPTOR_HEAP_TYPE_DSV:
					return L"[DSV] Descriptor Heap";
				default:
					return L"Unknown";
				}
			}();

			AGE_HR_CHECK(g::p_main_device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&p_descriptor_heap)));
			p_descriptor_heap->SetName(wstr_type);
		}

		descriptor_size = g::p_main_device->GetDescriptorHandleIncrementSize(heap_type);

		if constexpr (is_shader_visible())
		{
			start_handle.h_cpu = p_descriptor_heap->GetCPUDescriptorHandleForHeapStart();
			start_handle.h_gpu = p_descriptor_heap->GetGPUDescriptorHandleForHeapStart();
		}
		else
		{
			start_handle.h_cpu = p_descriptor_heap->GetCPUDescriptorHandleForHeapStart();
		}
	}

	template <D3D12_DESCRIPTOR_HEAP_TYPE heap_type, std::size_t capacity>
	FORCE_INLINE void
	descriptor_pool<heap_type, capacity>::deinit() noexcept
	{
		p_descriptor_heap->Release();

		if constexpr (age::config::debug_mode)
		{
			desc_idx_pool.debug_validate();
		}
	}

	template <D3D12_DESCRIPTOR_HEAP_TYPE heap_type, std::size_t capacity>
	descriptor_pool<heap_type, capacity>::t_descriptor_handle
	descriptor_pool<heap_type, capacity>::pop() noexcept
	{
		auto idx = desc_idx_pool.pop();

		if constexpr (is_shader_visible())
		{
			return t_descriptor_handle{
				.h_cpu = D3D12_CPU_DESCRIPTOR_HANDLE{ .ptr = start_handle.h_cpu.ptr + descriptor_size * idx },
				.h_gpu = D3D12_GPU_DESCRIPTOR_HANDLE{ .ptr = start_handle.h_gpu.ptr + descriptor_size * idx },
				AGE_DEBUG_OP(.idx = idx)
			};
		}
		else
		{
			return t_descriptor_handle{
				.h_cpu = D3D12_CPU_DESCRIPTOR_HANDLE{ .ptr = start_handle.h_cpu.ptr + descriptor_size * idx },
				AGE_DEBUG_OP(.idx = idx)
			};
		}
	}

	template <D3D12_DESCRIPTOR_HEAP_TYPE heap_type, std::size_t capacity>
	descriptor_pool<heap_type, capacity>::t_descriptor_handle
	descriptor_pool<heap_type, capacity>::get(uint32 idx) noexcept
	{
		desc_idx_pool.get(idx);

		if constexpr (is_shader_visible())
		{
			return t_descriptor_handle{
				.h_cpu = D3D12_CPU_DESCRIPTOR_HANDLE{ .ptr = start_handle.h_cpu.ptr + descriptor_size * idx },
				.h_gpu = D3D12_GPU_DESCRIPTOR_HANDLE{ .ptr = start_handle.h_gpu.ptr + descriptor_size * idx },
				AGE_DEBUG_OP(.idx = idx)
			};
		}
		else
		{
			return t_descriptor_handle{
				.h_cpu = D3D12_CPU_DESCRIPTOR_HANDLE{ .ptr = start_handle.h_cpu.ptr + descriptor_size * idx },
				AGE_DEBUG_OP(.idx = idx)
			};
		}
	}

	template <D3D12_DESCRIPTOR_HEAP_TYPE heap_type, std::size_t capacity>
	void
	descriptor_pool<heap_type, capacity>::push(t_descriptor_handle h) noexcept
	{
		auto diff = h.h_cpu.ptr - start_handle.h_cpu.ptr;
		auto idx  = static_cast<uint32>(diff / descriptor_size);

		AGE_ASSERT((diff % descriptor_size) == 0);
		AGE_ASSERT(idx == h.idx);

		desc_idx_pool.push(idx);
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

			auto dxgi_format = DXGI_FORMAT{};
			{
				switch (global::get<interface>().display_color_space())
				{
				case color_space::srgb:
					dxgi_format = DXGI_FORMAT_R8G8B8A8_UNORM;
					break;
				case color_space::hdr:
					dxgi_format = DXGI_FORMAT_R10G10B10A2_UNORM;
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
				/*UINT					*/ .Flags		= allow_tearing ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : UINT{ 0 },

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

		back_buffer_idx = p_swap_chain->GetCurrentBackBufferIndex();

		for (uint32 idx : std::views::iota(0) | std::views::take(g::frame_buffer_count))
		{
			rtv_desc_handle_arr[idx] = g::rtv_desc_pool.pop();
		}
	}

	void
	render_surface::resize() noexcept
	{
		// todo

		this->rebuild_from_swapchain();
	}

	void
	render_surface::present() noexcept
	{
		AGE_HR_CHECK(p_swap_chain->Present(0, present_flags));
		back_buffer_idx = p_swap_chain->GetCurrentBackBufferIndex();
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
	}

	void
	render_surface::rebuild_from_swapchain() noexcept
	{
		for (uint32 idx : std::views::iota(0) | std::views::take(g::frame_buffer_count))
		{
			AGE_HR_CHECK(p_swap_chain->GetBuffer(idx, IID_PPV_ARGS(&back_buffer_ptr_arr[idx])));

			auto rtv_format = DXGI_FORMAT{};
			{
				switch (global::get<interface>().display_color_space())
				{
				case color_space::srgb:
					rtv_format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
					break;
				case color_space::hdr:
					rtv_format = DXGI_FORMAT_R10G10B10A2_UNORM;
					break;
				default:
					std::unreachable();
				}
			}

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

		{
			g::rtv_desc_pool.init();
			g::dsv_desc_pool.init();
			g::cbv_srv_uav_desc_pool.init();
			g::sampler_desc_pool.init();
		}

		{
			// init swap_chain
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
			// p_main_swap_chain->Release();
		}

		{
			g::sampler_desc_pool.deinit();
			g::cbv_srv_uav_desc_pool.deinit();
			g::dsv_desc_pool.deinit();
			g::rtv_desc_pool.deinit();
		}


		{
			if constexpr (age::config::debug_mode)
			{
				g::next_fence_value = std::numeric_limits<decltype(g::next_fence_value)>::max();
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
			g::render_surfaces.debug_validate();
		}
	}

	void
	create_render_surface(platform::window_handle w_handle) noexcept
	{
		g::render_surface_ids.emplace_back(
			g::render_surfaces.emplace_back(w_handle));
	}

	void
	begin_frame() noexcept
	{
		g::cmd_system_direct.begin_frame();
		g::cmd_system_compute.begin_frame();
		g::cmd_system_copy.begin_frame();
	}

	void
	end_frame() noexcept
	{
		++g::next_fence_value;

		g::cmd_system_direct.end_frame();
		g::cmd_system_compute.end_frame();
		g::cmd_system_copy.end_frame();

		g::frame_buffer_idx = (g::frame_buffer_idx + 1) % g::frame_buffer_count;
	}

	void
	render() noexcept
	{
	}
}	 // namespace age::graphics

#endif
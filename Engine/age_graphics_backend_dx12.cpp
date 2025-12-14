#include "age.hpp"

namespace age::graphics
{
#if defined AGE_DEBUG
	constexpr auto dxgi_factory_flag = DXGI_CREATE_FACTORY_DEBUG;
#else
	constexpr auto dxgi_factory_flag = UINT{ 0 };
#endif
	constexpr auto minimum_feature_level = D3D_FEATURE_LEVEL_12_1;
	constexpr auto frame_buffer_count	 = 3;

	auto frame_buffer_idx = uint8{ 0 };

	auto next_fence_value = uint64{ frame_buffer_count };
}	 // namespace age::graphics

namespace age::graphics
{
	template <auto cmd_list_type, auto cmd_list_num>
	struct cmd_system
	{
		ID3D12CommandQueue*			p_cmd_queue = nullptr;
		ID3D12GraphicsCommandList9* cmd_list_pool[frame_buffer_count][cmd_list_num]{ nullptr };
		ID3D12CommandAllocator*		cmd_allocator_pool[frame_buffer_count][cmd_list_num]{ nullptr };
		ID3D12Fence1*				p_fence		= nullptr;
		HANDLE						fence_event = nullptr;


		constexpr cmd_system() = default;

		AGE_DISABLE_COPY_MOVE(cmd_system)

		FORCE_INLINE void
		init(ID3D12Device11* p_main_device) noexcept
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
				AGE_HR_CHECK(p_main_device->CreateCommandQueue(&desc, IID_PPV_ARGS(&p_cmd_queue)));
				auto queue_name = std::format(L"[{}] cmd queue", wstr_type);
				p_cmd_queue->SetName(queue_name.c_str());
			}

			{
				AGE_HR_CHECK(p_main_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&p_fence)));
				auto fence_name = std::format(L"[{}] fence", wstr_type);
				p_fence->SetName(fence_name.c_str());
			}

			{
				fence_event = ::CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS);
				assert(fence_event);
			}

			for (auto i : std::views::iota(0, frame_buffer_count))
			{
				for (auto j : std::views::iota(0, cmd_list_num))
				{
					AGE_HR_CHECK(p_main_device->CreateCommandAllocator(cmd_list_type, IID_PPV_ARGS(&cmd_allocator_pool[i][j])));

					AGE_HR_CHECK(p_main_device->CreateCommandList(
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

		FORCE_INLINE void
		deinit() noexcept
		{
			{
				AGE_HR_CHECK(p_cmd_queue->Signal(p_fence, next_fence_value));
				AGE_HR_CHECK(p_fence->SetEventOnCompletion(next_fence_value, fence_event));
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

		FORCE_INLINE void
		wait() noexcept
		{
			auto completed_value = p_fence->GetCompletedValue();
			auto expected_value	 = next_fence_value - frame_buffer_count;
			auto need_to_wait	 = completed_value < expected_value;

			// std::println("[{}], fence_value = {}, completed_value= {}, need_to_wait= {},", (int)cmd_list_type, fence_value, completed_value, need_to_wait);

			if (need_to_wait)
			{
				AGE_HR_CHECK(p_fence->SetEventOnCompletion(expected_value, fence_event));

				::WaitForSingleObject(fence_event, INFINITE);
			}
		}

		FORCE_INLINE void
		begin_frame() noexcept
		{
			wait();
			for (auto i : std::views::iota(0, cmd_list_num))
			{
				AGE_HR_CHECK(cmd_allocator_pool[frame_buffer_idx][i]->Reset());
				AGE_HR_CHECK(cmd_list_pool[frame_buffer_idx][i]->Reset(cmd_allocator_pool[frame_buffer_idx][i], nullptr));
			}
		}

		FORCE_INLINE void
		end_frame() noexcept
		{
			// todo hmm...
			auto cmd_lists = std::array<ID3D12CommandList*, cmd_list_num>{};
			for (auto i : std::views::iota(0, cmd_list_num))
			{
				AGE_HR_CHECK(cmd_list_pool[frame_buffer_idx][i]->Close());

				cmd_lists[i] = cmd_list_pool[frame_buffer_idx][i];
			}

			p_cmd_queue->ExecuteCommandLists(cmd_list_num, cmd_lists.data());

			// std::println("[{}], signaling fence, from {} to {}", (int)cmd_list_type, p_fence->GetCompletedValue(), fence_value);
			AGE_HR_CHECK(p_cmd_queue->Signal(p_fence, next_fence_value));
		}
	};
}	 // namespace age::graphics

namespace age::graphics
{
	template <D3D12_DESCRIPTOR_HEAP_TYPE heap_type>
	constexpr inline bool is_shader_visible_v =
		heap_type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV || heap_type == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;

	template <D3D12_DESCRIPTOR_HEAP_TYPE heap_type>
	struct descriptor_handle;

	template <D3D12_DESCRIPTOR_HEAP_TYPE heap_type>
	requires(is_shader_visible_v<heap_type>)
	struct descriptor_handle<heap_type>
	{
		D3D12_CPU_DESCRIPTOR_HANDLE h_cpu = {};
		D3D12_GPU_DESCRIPTOR_HANDLE h_gpu = {};

#ifdef AGE_DEBUG
		uint32 idx = 0;
#endif
	};

	template <D3D12_DESCRIPTOR_HEAP_TYPE heap_type>
	requires(is_shader_visible_v<heap_type> is_false)
	struct descriptor_handle<heap_type>
	{
		D3D12_CPU_DESCRIPTOR_HANDLE h_cpu = {};

#ifdef AGE_DEBUG
		uint32 idx = 0;
#endif
	};

	template <D3D12_DESCRIPTOR_HEAP_TYPE heap_type, std::size_t capacity>
	struct descriptor_pool
	{
		using t_descriptor_handle = descriptor_handle<heap_type>;

		ID3D12DescriptorHeap*				  p_descriptor_heap = nullptr;
		uint32								  descriptor_size	= 0;
		age::util::idx_pool<uint32, capacity> desc_idx_pool{};

		t_descriptor_handle start_handle = {};

		constexpr descriptor_pool() noexcept = default;

		AGE_DISABLE_COPY_MOVE(descriptor_pool);

		static consteval bool
		is_shader_visible()
		{
			return is_shader_visible_v<heap_type>;
		}

		FORCE_INLINE void
		init(ID3D12Device11* p_main_device) noexcept
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

				AGE_HR_CHECK(p_main_device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&p_descriptor_heap)));
				p_descriptor_heap->SetName(wstr_type);
			}

			descriptor_size = p_main_device->GetDescriptorHandleIncrementSize(heap_type);

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

		FORCE_INLINE void
		deinit() noexcept
		{
			p_descriptor_heap->Release();

			if constexpr (age::config::debug_mode)
			{
				desc_idx_pool.debug_validate();
			}
		}

		t_descriptor_handle
		pop() noexcept
		{
			auto idx = desc_idx_pool.pop();

			if constexpr (is_shader_visible())
			{
				return t_descriptor_handle{
					.h_cpu.ptr = start_handle.h_cpu.ptr + descriptor_size * idx,
					.h_gpu.ptr = start_handle.h_gpu.ptr + descriptor_size * idx,
					AGE_DEBUG_OP(.idx = idx)
				};
			}
			else
			{
				return t_descriptor_handle{
					.h_cpu.ptr = start_handle.h_cpu.ptr + descriptor_size * idx,
					AGE_DEBUG_OP(.idx = idx)
				};
			}
		}

		t_descriptor_handle
		get(uint32 idx) noexcept
		{
			desc_idx_pool.get(idx);

			if constexpr (is_shader_visible())
			{
				return t_descriptor_handle{
					.h_cpu.ptr = start_handle.h_cpu.ptr + descriptor_size * idx,
					.h_gpu.ptr = start_handle.h_gpu.ptr + descriptor_size * idx,
					AGE_DEBUG_OP(.idx = idx)
				};
			}
			else
			{
				return t_descriptor_handle{
					.h_cpu.ptr = start_handle.h_cpu.ptr + descriptor_size * idx,
					AGE_DEBUG_OP(.idx = idx)
				};
			}
		}

		void
		push(t_descriptor_handle h) noexcept
		{
			auto diff = h.h_cpu.ptr - start_handle.h_cpu.ptr;
			auto idx  = diff / descriptor_size;

			AGE_ASSERT((diff % descriptor_size) == 0);
			AGE_ASSERT(idx == h.idx);

			desc_idx_pool.push(idx);
		}
	};
}	 // namespace age::graphics

namespace age::graphics

{
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
}	 // namespace age::graphics

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

		AGE_HR_CHECK(::CreateDXGIFactory2(dxgi_factory_flag, IID_PPV_ARGS(&p_dxgi_factory)));

		for (uint32 adapter_idx : std::views::iota(0))
		{
			auto* p_adapter = (IDXGIAdapter4*)nullptr;
			auto* p_device	= (ID3D12Device11*)nullptr;
			if (p_dxgi_factory->EnumAdapterByGpuPreference(adapter_idx, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&p_adapter)) == DXGI_ERROR_NOT_FOUND)
			{
				assert(false);
				break;
			}

			assert(p_adapter is_not_nullptr);
			if (FAILED(::D3D12CreateDevice(p_adapter, minimum_feature_level, IID_PPV_ARGS(&p_device))))
			{
				p_adapter->Release();
			}
			else
			{
				p_main_adapter = p_adapter;
				p_main_device  = p_device;
				break;
			}
		}


		if constexpr (age::config::debug_mode)
		{
			p_main_device->SetName(L"age graphics main device");

			auto* p_info_queue = (ID3D12InfoQueue*)nullptr;
			AGE_HR_CHECK(p_main_device->QueryInterface(IID_PPV_ARGS(&p_info_queue)));

			p_info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
			p_info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
			p_info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);

			p_info_queue->Release();
		}

		{
			cmd_system_direct.init(p_main_device);
			cmd_system_compute.init(p_main_device);
			cmd_system_copy.init(p_main_device);
		}

		{

			rtv_desc_pool.init(p_main_device);
			dsv_desc_pool.init(p_main_device);
			cbv_srv_uav_desc_pool.init(p_main_device);
			sampler_desc_pool.init(p_main_device);
		}
	}

	void
	deinit() noexcept
	{
		if constexpr (age::config::debug_mode)
		{
			auto* p_info_queue = (ID3D12InfoQueue*)nullptr;
			AGE_HR_CHECK(p_main_device->QueryInterface(IID_PPV_ARGS(&p_info_queue)));

			p_info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, false);
			p_info_queue->Release();
		}

		{
			sampler_desc_pool.deinit();
			cbv_srv_uav_desc_pool.deinit();
			dsv_desc_pool.deinit();
			rtv_desc_pool.deinit();
		}

		{
			if constexpr (age::config::debug_mode)
			{
				next_fence_value = std::numeric_limits<decltype(next_fence_value)>::max();
			}

			cmd_system_copy.deinit();
			cmd_system_compute.deinit();
			cmd_system_direct.deinit();
		}


		if constexpr (age::config::debug_mode)
		{
			// requires "graphics tools" optional feature
			// settings -> system -> optional features -> add on aptional feature -> select and install graphics tools
			auto* p_debug_device = (ID3D12DebugDevice2*)nullptr;
			AGE_HR_CHECK(p_main_device->QueryInterface(IID_PPV_ARGS(&p_debug_device)));


			AGE_HR_CHECK(p_debug_device->ReportLiveDeviceObjects(
				D3D12_RLDO_SUMMARY | D3D12_RLDO_DETAIL | D3D12_RLDO_IGNORE_INTERNAL));

			p_debug_device->Release();
		}

		p_main_device->Release();
		p_main_adapter->Release();
		p_dxgi_factory->Release();
	}

	void
	begin_frame() noexcept
	{
		cmd_system_direct.begin_frame();
		cmd_system_compute.begin_frame();
		cmd_system_copy.begin_frame();
	}

	void
	end_frame() noexcept
	{
		++next_fence_value;

		cmd_system_direct.end_frame();
		cmd_system_compute.end_frame();
		cmd_system_copy.end_frame();

		frame_buffer_idx = (frame_buffer_idx + 1) % frame_buffer_count;
	}

	void
	render() noexcept
	{
	}
}	 // namespace age::graphics
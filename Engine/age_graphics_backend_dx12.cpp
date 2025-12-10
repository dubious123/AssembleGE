#include "age.hpp"

namespace age::graphics
{
	namespace
	{
#if defined AGE_DEBUG
		constexpr UINT dxgi_factory_flag = DXGI_CREATE_FACTORY_DEBUG;
#else
		constexpr UINT dxgi_factory_flag = 0;
#endif

		constexpr D3D_FEATURE_LEVEL minimum_feature_level = D3D_FEATURE_LEVEL_12_1;

		// 1 cpu thread -> 1 cmd list
		// 1 cmd list -> 1 cmd allocator
		// n cmd allocator -> 1 frame
		// 1 cmd queue -> n cmd list, n cmd_allocator
		// cmd list [ # of frame to handle, or frame buffer count ]   [ # of thread ( # of async access from cpu ) ]
		// cmd allocator

		constexpr inline uint32 frame_buffer_count = 3;
	}	 // namespace

	namespace
	{
		IDXGIFactory7*	p_dxgi_factory = nullptr;
		IDXGIAdapter4*	p_main_adapter = nullptr;
		ID3D12Device11* p_main_device  = nullptr;

		void
		failed_init() noexcept
		{
			assert(false);
		}

		template <typename t>
		constexpr void
		release(t*& p_resource)
		{
			if (p_resource is_not_nullptr)
			{
				p_resource->Release();
				p_resource = nullptr;
			}
			else
			{
				p_resource = nullptr;
			}
		}
	}	 // namespace
}	 // namespace age::graphics

namespace age::graphics
{
	void
	init() noexcept
	{
		if constexpr (age::config::debug_mode)
		{
			ID3D12Debug3* p_debug = nullptr;
			if (FAILED(::D3D12GetDebugInterface(IID_PPV_ARGS(&p_debug))))
			{
				assert(false);
			}

			p_debug->EnableDebugLayer();
			p_debug->Release();
		}

		if (FAILED(::CreateDXGIFactory2(dxgi_factory_flag, IID_PPV_ARGS(&p_dxgi_factory))))
		{
			assert(false);
		}

		for (uint32 adapter_idx : std::views::iota(0))
		{
			IDXGIAdapter4*	p_adapter = nullptr;
			ID3D12Device11* p_device  = nullptr;
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

			ID3D12InfoQueue* p_info_queue = nullptr;
			if (FAILED(p_main_device->QueryInterface(IID_PPV_ARGS(&p_info_queue))))
			{
				assert(false);
			}

			p_info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
			p_info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
			p_info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);

			p_info_queue->Release();
		}
	}

	void
	deinit() noexcept
	{
		if constexpr (age::config::debug_mode)
		{
			ID3D12InfoQueue* p_info_queue = nullptr;
			if (FAILED(p_main_device->QueryInterface(IID_PPV_ARGS(&p_info_queue))))
			{
				assert(false);
			}

			p_info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, false);
			p_info_queue->Release();
		}
		if constexpr (age::config::debug_mode)
		{
			// requires "graphics tools" optional feature
			// settings -> system -> optional features -> add on aptional feature -> select and install graphics tools
			ID3D12DebugDevice2* p_debug_device = nullptr;
			if (FAILED(p_main_device->QueryInterface(IID_PPV_ARGS(&p_debug_device))))
			{
				assert(false);
			}


			if (FAILED(p_debug_device->ReportLiveDeviceObjects(
					D3D12_RLDO_SUMMARY | D3D12_RLDO_DETAIL | D3D12_RLDO_IGNORE_INTERNAL)))
			{
				assert(false);
			}

			p_debug_device->Release();
		}

		p_main_device->Release();
		p_main_adapter->Release();
		p_dxgi_factory->Release();
	}

	namespace command_system
	{
		constexpr auto queue_type_arr	 = std::array{ D3D12_COMMAND_LIST_TYPE_DIRECT, D3D12_COMMAND_LIST_TYPE_COMPUTE, D3D12_COMMAND_LIST_TYPE_COPY };
		constexpr auto command_queue_arr = std::array<ID3D12CommandQueue*, queue_type_arr.size()>{ nullptr };


	}	 // namespace command_system

	void
	render() noexcept
	{
		// #of thread => # of command list
		// #of command allocator => # of threads * #of frame buffer

		// for (auto queue_type : queue_type_arr)
		//{
		// }

		// struct d3d12_commnad
		//{
		//	ID3D12CommandQueue*			p_cmd_queue = nullptr;
		//	ID3D12GraphicsCommandList9* p_cmd_list	= nullptr;


		//	std::array<ID3D12CommandAllocator*, frame_buffer_count> cmd_frame;

		//	struct commnad_frame
		//	{
		//		ID3D12CommandAllocator* p_cmd_allocator = nullptr;
		//	};
		//};

		// D3D12_COMMAND_QUEUE_DESC desc{
		//	.Type	  = D3D12_COMMAND_LIST_TYPE_DIRECT,
		//	.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL,
		//	.Flags	  = D3D12_COMMAND_QUEUE_FLAG_NONE,
		//	.NodeMask = 0
		// };

		// ID3D12CommandQueue* p_cmd_queue = nullptr;
		// if (FAILED(p_main_device->CreateCommandQueue(&desc, IID_PPV_ARGS(&p_cmd_queue))))
		//{
		//	assert(false);
		// }

		// p_cmd_queue->SetName(L"command queue");


		// ID3D12CommandList* p_cmd_list = nullptr;

		// if (FAILED(p_main_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, nullptr, nullptr, IID_PPV_ARGS(&p_cmd_list))))
		//{
		//	assert(false);
		// }
	}
}	 // namespace age::graphics
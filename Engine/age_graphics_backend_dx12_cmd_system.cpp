#include "age_pch.hpp"
#include "age.hpp"
#if defined(AGE_GRAPHICS_BACKEND_DX12)
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

#endif
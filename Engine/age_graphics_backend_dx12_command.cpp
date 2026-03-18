#include "age_pch.hpp"
#include "age.hpp"

#if defined(AGE_GRAPHICS_BACKEND_DX12)
namespace age::graphics::command
{
	FORCE_INLINE void
	cpu_wait(e::queue_kind kind) noexcept
	{
		auto& ctx = g::queue_ctx[std::to_underlying(kind)];

		if (ctx.p_fence->GetCompletedValue() < ctx.fence_value)
		{
			AGE_HR_CHECK(ctx.p_fence->SetEventOnCompletion(ctx.fence_value, ctx.h_fence_event));

			::WaitForSingleObject(ctx.h_fence_event, INFINITE);
		}
	}

	FORCE_INLINE void
	gpu_wait(e::queue_kind who, e::queue_kind what) noexcept
	{
		auto& ctx_what = g::queue_ctx[std::to_underlying(what)];
		auto& ctx_who  = g::queue_ctx[std::to_underlying(who)];

		AGE_HR_CHECK(ctx_who.p_queue->Wait(ctx_what.p_fence, ctx_what.fence_value));
	}
}	 // namespace age::graphics::command

namespace age::graphics::command
{
	namespace detail
	{
		FORCE_INLINE void
		begin(e::queue_kind kind, uint8 frame_idx, uint8 thread_idx) noexcept
		{
			auto& ctx = g::queue_ctx[std::to_underlying(kind)];
			AGE_HR_CHECK(ctx.p_allocator[frame_idx][thread_idx]->Reset());
			AGE_HR_CHECK(ctx.p_cmd_list[thread_idx]->Reset(ctx.p_allocator[frame_idx][thread_idx], nullptr));
		}
	}	 // namespace detail

	FORCE_INLINE void
	begin(e::queue_kind kind, auto... thread_idx) noexcept
	{
		(detail::begin(kind, g::frame_buffer_idx, thread_idx), ...);
	}

	FORCE_INLINE void
	begin(e::queue_kind kind) noexcept
	{
		begin(kind, 0);
	}

	FORCE_INLINE void
	begin_frame(e::queue_kind kind, auto... thread_idx) noexcept
	{
		auto& ctx = g::queue_ctx[std::to_underlying(kind)];

		if (ctx.p_fence->GetCompletedValue() < ctx.frame_fence_value[g::frame_buffer_idx])
		{
			AGE_HR_CHECK(ctx.p_fence->SetEventOnCompletion(
				ctx.frame_fence_value[g::frame_buffer_idx], ctx.h_fence_event));

			::WaitForSingleObject(ctx.h_fence_event, INFINITE);
		}


		(detail::begin(kind, g::frame_buffer_idx, thread_idx), ...);
	}

	FORCE_INLINE void
	begin_frame(e::queue_kind kind) noexcept
	{
		begin_frame(kind, 0);
	}

	FORCE_INLINE uint64
	signal(e::queue_kind kind) noexcept
	{
		auto& ctx = g::queue_ctx[std::to_underlying(kind)];
		++ctx.fence_value;
		AGE_HR_CHECK(ctx.p_queue->Signal(ctx.p_fence, ctx.fence_value));
		return ctx.fence_value;
	}

	FORCE_INLINE void
	execute(e::queue_kind kind, auto... thread_idx) noexcept
	{
		auto& ctx = g::queue_ctx[std::to_underlying(kind)];

		((ctx.p_cmd_list[thread_idx]->Close()), ...);

		ID3D12CommandList* cmd_lists[] = { (ctx.p_cmd_list[thread_idx])... };

		ctx.p_queue->ExecuteCommandLists(sizeof...(thread_idx), cmd_lists);

		++ctx.fence_value;
		AGE_HR_CHECK(ctx.p_queue->Signal(ctx.p_fence, ctx.fence_value));
	}

	FORCE_INLINE void
	execute(e::queue_kind kind) noexcept
	{
		execute(kind, 0);
	}

	FORCE_INLINE void
	execute_and_wait(e::queue_kind kind, auto... thread_idx) noexcept
	{
		execute(kind, thread_idx...);

		cpu_wait(kind);
	}

	FORCE_INLINE void
	execute_and_wait(e::queue_kind kind) noexcept
	{
		execute_and_wait(kind, 0);
	}

	FORCE_INLINE void
	end_frame(e::queue_kind kind, auto... thread_idx) noexcept
	{
		auto& ctx = g::queue_ctx[std::to_underlying(kind)];

		execute(kind, thread_idx...);

		ctx.frame_fence_value[g::frame_buffer_idx] = ctx.fence_value;
	}

	FORCE_INLINE void
	end_frame(e::queue_kind kind) noexcept
	{
		end_frame(kind, 0);
	}
}	 // namespace age::graphics::command

namespace age::graphics::command
{
	uint64
	current_fence_value(e::queue_kind kind) noexcept
	{
		return g::queue_ctx[std::to_underlying(kind)].fence_value;
	}

	bool
	is_complete(e::queue_kind kind, uint64 fence_value) noexcept
	{
		return g::queue_ctx[std::to_underlying(kind)].p_fence->GetCompletedValue() >= fence_value;
	}

	bool
	is_idle(e::queue_kind kind) noexcept
	{
		auto& ctx = g::queue_ctx[std::to_underlying(kind)];

		// >= instead of == to handle UINT64_MAX on device lost
		return ctx.p_fence->GetCompletedValue() >= ctx.fence_value;
	}
}	 // namespace age::graphics::command

namespace age::graphics::command
{
	void
	init() noexcept
	{
		constexpr auto d3d12_type = std::array{
			D3D12_COMMAND_LIST_TYPE_DIRECT,
			D3D12_COMMAND_LIST_TYPE_COMPUTE,
			D3D12_COMMAND_LIST_TYPE_COPY
		};

		for (auto kind : {
				 e::queue_kind::direct,
				 e::queue_kind::compute,
				 e::queue_kind::copy })
		{
			auto& ctx  = g::queue_ctx[std::to_underlying(kind)];
			auto  desc = D3D12_COMMAND_QUEUE_DESC{
				.Type	  = d3d12_type[std::to_underlying(kind)],
				.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL,
				.Flags	  = D3D12_COMMAND_QUEUE_FLAG_NONE,
				.NodeMask = 0
			};

			{
				AGE_HR_CHECK(g::p_main_device->CreateCommandQueue(&desc, IID_PPV_ARGS(&ctx.p_queue)));
				c_auto queue_name = std::format(L"[{}] cmd queue", e::to_wstring(kind));
				ctx.p_queue->SetName(queue_name.c_str());
			}

			{
				AGE_HR_CHECK(g::p_main_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&ctx.p_fence)));
				c_auto fence_name = std::format(L"[{}] fence", e::to_wstring(kind));
				ctx.p_fence->SetName(fence_name.c_str());
			}

			{
				ctx.h_fence_event = ::CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS);
				AGE_ASSERT(ctx.h_fence_event is_not_nullptr);
			}

			for (auto&& [i, j] : std::views::cartesian_product(
					 std::views::iota(0u, g::frame_buffer_count),
					 std::views::iota(0u, g::thread_count)))
			{
				AGE_HR_CHECK(g::p_main_device->CreateCommandAllocator(
					desc.Type, IID_PPV_ARGS(&ctx.p_allocator[i][j])));

				c_auto cmd_allocator_name = std::format(L"[{}] cmd list allocator[{}][{}]", e::to_wstring(kind), i, j);

				ctx.p_allocator[i][j]->SetName(cmd_allocator_name.c_str());
			}


			for (auto j : std::views::iota(0u, g::thread_count))
			{
				AGE_HR_CHECK(g::p_main_device->CreateCommandList(
					0,
					desc.Type,
					ctx.p_allocator[0][j],
					nullptr,
					IID_PPV_ARGS(&ctx.p_cmd_list[j])));

				AGE_HR_CHECK(ctx.p_cmd_list[j]->Close());

				c_auto cmd_list_name = std::format(L"[{}] cmd list[{}]", e::to_wstring(kind), j);
				ctx.p_cmd_list[j]->SetName(cmd_list_name.c_str());
			}

			ctx.fence_value = 0;

			for (auto& f_value : ctx.frame_fence_value)
			{
				f_value = 0;
			}
		}
	}

	void
	deinit() noexcept
	{
		for (auto kind : {
				 e::queue_kind::direct,
				 e::queue_kind::compute,
				 e::queue_kind::copy })
		{
			cpu_wait(kind);
		}

		for (auto& ctx : g::queue_ctx)
		{
			ctx.p_queue->Release();
			ctx.p_fence->Release();
			::CloseHandle(ctx.h_fence_event);

			for (auto* p_cmd_list : ctx.p_cmd_list)
			{
				p_cmd_list->Release();
			}

			for (auto* p_allocator : std::views::join(ctx.p_allocator))
			{
				p_allocator->Release();
			}
		}
	}
}	 // namespace age::graphics::command

#endif
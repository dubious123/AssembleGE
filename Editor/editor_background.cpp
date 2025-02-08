#include "pch.h"
#include "editor_common.h"
#include "editor_background.h"
#include "concurrent_queue.h"

namespace
{
	auto _job_queue_arr	 = std::array<Concurrency::concurrent_queue<std::tuple<std::function<void()>, std::function<void()>>>, Background_Count>();
	auto _thread_arr	 = std::array<std::thread, Background_Count>();
	auto _terminate_flag = std::array<bool, Background_Count>();
}	 // namespace

namespace
{
	void _thread_loop(int32 idx)
	{
		auto& job_queue = _job_queue_arr[idx];

		while (_terminate_flag[idx] is_false)
		{
			while (job_queue.empty() is_false)
			{
				std::tuple<std::function<void()>, std::function<void()>> tpl;
				if (job_queue.try_pop(tpl))
				{
					std::get<0>(tpl)();
					auto callback = std::get<1>(tpl);
					if (callback)
					{
						callback();
					}
				}
			}

			Sleep(500);
		}
	}
}	 // namespace

void editor::background::init()
{
	std::ranges::fill(_terminate_flag, false);

	std::ranges::for_each(std::views::iota(0, Background_Count), [](auto idx) {
		_thread_arr[idx] = std::thread(_thread_loop, idx);
	});
}

void editor::background::add(editor_background type, std::function<void()>&& task, std::function<void()>&& callback)
{
	_job_queue_arr[type].push({ std::move(task), std::move(callback) });
}

void editor::background::deinit()
{
	std::ranges::fill(_terminate_flag, true);	 // todo do we need atomic??

	std::ranges::for_each(_job_queue_arr, [](auto& qeueue) { qeueue.clear(); });
	std::ranges::for_each(_thread_arr, [](auto& th) { th.join(); });
}
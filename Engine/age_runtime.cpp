#include "age_pch.hpp"
#include "age.hpp"

namespace age::runtime
{
	void
	init() noexcept
	{
		i_init.set_now = std::chrono::steady_clock::now();
	}

	void
	update() noexcept
	{
		constexpr auto max_delta = std::chrono::nanoseconds{ 1'000'000'000 / age::config::min_fps };

		auto time_now = std::chrono::steady_clock::now();

		auto raw_delta	 = time_now - i_update.get_now();
		i_update.set_now = time_now;

		i_update.set_delta_time_ns = std::min(raw_delta, max_delta);
		// i_runtime.delta_time_ns() = raw_delta;
	}

	void
	deinit() noexcept
	{
		i_update.set_now = std::chrono::steady_clock::now();
	}
}	 // namespace age::runtime
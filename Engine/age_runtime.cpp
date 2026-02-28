#include "age_pch.hpp"
#include "age.hpp"

namespace age::runtime
{
	void
	init() noexcept
	{
		auto i_runtime	= age::global::get<runtime::interface>();
		i_runtime.now() = std::chrono::steady_clock::now();
	}

	void
	update() noexcept
	{
		constexpr auto max_delta = std::chrono::nanoseconds{ 1'000'000'000 / age::config::min_fps };

		auto i_runtime = age::global::get<runtime::interface>();
		auto time_now  = std::chrono::steady_clock::now();

		auto raw_delta	= time_now - i_runtime.now();
		i_runtime.now() = time_now;

		i_runtime.delta_time_ns() = std::min(raw_delta, max_delta);
	}
}	 // namespace age::runtime
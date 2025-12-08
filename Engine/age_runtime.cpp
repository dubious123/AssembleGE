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
		auto i_runtime = age::global::get<runtime::interface>();
		auto time_now  = std::chrono::steady_clock::now();

		i_runtime.delta_time_ns() = time_now - i_runtime.now();
		i_runtime.now()			  = time_now;
	}
}	 // namespace age::runtime
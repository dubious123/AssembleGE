#pragma once

namespace age::global
{
	struct state
	{
		std::string name = "age_engine";

		std::chrono::steady_clock::time_point now;

		std::chrono::nanoseconds delta_time_ns{ 0 };

		bool running = true;

		// graphics
		graphics::color_space display_color_space = graphics::color_space::hdr;
	};

	inline state g_state;

	template <template <typename> typename t_interface>
	FORCE_INLINE decltype(auto)
	get() noexcept
	{
		return t_interface{ g_state };
	}
}	 // namespace age::global
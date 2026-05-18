#pragma once
#include "age.hpp"

namespace age::global
{
	constexpr c_auto thread_count		= uint8{ 8 };
	constexpr c_auto frame_buffer_count = uint8{ 3 };

	struct state
	{
		std::string name = "age_engine";

		std::chrono::steady_clock::time_point now;

		std::chrono::nanoseconds delta_time_ns{ 0 };

		uint32 frame_count = 0;

		bool running = true;

		// graphics
		graphics::color_space display_color_space = graphics::color_space::hdr;
	};

	namespace detail
	{
		inline state ctx;
	}
}	 // namespace age::global
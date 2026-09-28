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

		uint8 frame_buffer_idx = uint8{ 0 };
	};

	namespace detail
	{
		inline state ctx;
	}

	struct
	{
		AGE_GETSET(frame_buffer_idx, frame_buffer_idx);
	} i_graphics;
}	 // namespace age::global
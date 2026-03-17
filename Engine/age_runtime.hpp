#pragma once
#include "age.hpp"

namespace age::runtime
{
	struct
	{
		AGE_GET(running, running)
	} i;

	struct
	{
		AGE_GET(now, now)
		AGE_GET(delta_time_ns, delta_time_ns)
		AGE_GET(frame_count, frame_count)

		FORCE_INLINE constexpr float
		get_delta_time_s() noexcept
		{
			return std::chrono::duration<float>(get_delta_time_ns()).count();
		}
	} i_time;

	struct
	{
		AGE_GETSET(now, now)
	} i_init;

	struct
	{
		AGE_GETSET(now, now)
		AGE_SET(delta_time_ns, delta_time_ns)
	} i_update;

	struct
	{
		AGE_GETSET(now, now)
	} i_deinit;

	FORCE_INLINE bool
	is_handle_invalid(auto&& any_handle) noexcept
	{
		if constexpr (std::is_same_v<uint32, BARE_OF(any_handle.id)>)
		{
			return any_handle.id == invalid_id_uint32;
		}
		else
		{
			static_assert(false);
		}
	}

	FORCE_INLINE bool
	is_handle_valid(auto&& any_handle) noexcept
	{
		if constexpr (std::is_same_v<uint32, BARE_OF(any_handle.id)>)
		{
			return any_handle.id != invalid_id_uint32;
		}
		else
		{
			static_assert(false);
		}
	}
}	 // namespace age::runtime

namespace age::runtime
{
	FORCE_INLINE decltype(auto)
	assign_to(auto&... dst) noexcept
		requires(sizeof...(dst) > 1)
	{
		return [&] INLINE_LAMBDA_FRONT(auto&&... src) noexcept INLINE_LAMBDA_BACK -> decltype(auto) {
			((dst = FWD(src)), ...);
			return std::forward_as_tuple(dst...);
		};
	}

	FORCE_INLINE decltype(auto)
	assign_to(auto& dst) noexcept
	{
		return [&] INLINE_LAMBDA_FRONT(auto&& src) noexcept INLINE_LAMBDA_BACK -> decltype(auto) {
			if constexpr (std::is_assignable_v<decltype(src), decltype(dst)>)
			{
				dst = FWD(src);
				return dst;
			}
			else if constexpr (meta::is_tuple_like_v<BARE_OF(src)>)
			{
				if constexpr (std::is_assignable_v<decltype(std::get<0>(src)), decltype(dst)>)
				{
					dst = std::get<0>(FWD(src));
					return dst;
				}
				else
				{
					static_assert(false);
				}
			}
			else
			{
				static_assert(false);
			}
		};
	}
}	 // namespace age::runtime

namespace age::runtime
{
	FORCE_INLINE decltype(auto)
	when_window_alive(age::platform::window_handle& h_window) noexcept
	{
		return age::ecs::system::when{
			[&h_window] INLINE_LAMBDA_FRONT() mutable noexcept INLINE_LAMBDA_BACK -> bool {
				if (age::runtime::is_handle_invalid(h_window))
				{
					return false;
				}
				else if (age::platform::window_close_requested(h_window))
				{
					if (age::platform::is_window_closed(h_window))
					{
						age::platform::remove_window(h_window);
						return false;
					}

					age::platform::close_window(h_window);
					return false;
				}
				else
				{
					return true;
				}
			}
		};
	}
}	 // namespace age::runtime

namespace age::runtime
{
	void
	init() noexcept;

	void
	update() noexcept;
}	 // namespace age::runtime
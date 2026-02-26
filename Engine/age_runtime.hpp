#pragma once
#include "age.hpp"

namespace age::runtime
{
	template <typename t>
	struct interface
	{
	  private:
		no_unique_addr t data;

	  public:
		constexpr interface(auto&& arg) noexcept : data(FWD(arg)) { }

		AGE_PROP(now)

		AGE_PROP(delta_time_ns)

		AGE_PROP(running)

		// FORCE_INLINE constexpr decltype((data.now))
		// now()
		//{
		//	return data.now;
		// }

		// FORCE_INLINE constexpr decltype((data.delta_time_ns))
		// delta_time_ns()
		//{
		//	return data.delta_time_ns;
		// }

		// FORCE_INLINE constexpr bool&
		// running()
		//{
		//	return data.running;
		// }
	};

	template <typename t>
	interface(t&&) -> interface<t>;

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
}	 // namespace age::runtime

namespace age::runtime
{
	FORCE_INLINE decltype(auto)
	assign_to(auto&... dst) noexcept
	{
		return [&] INLINE_LAMBDA_FRONT(auto&&... src) noexcept INLINE_LAMBDA_BACK -> decltype(auto) {
			((dst = FWD(src)), ...);
			return std::forward_as_tuple(dst...);
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
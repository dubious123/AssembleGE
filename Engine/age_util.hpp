#pragma once

namespace age::util
{
	template <typename t>
	FORCE_INLINE constexpr auto
	popcount(t x)
	{
#ifdef _MSC_VER
		if constexpr (sizeof(t) == 16)
		{
			return __popcnt16(x);
		}
		else if constexpr (sizeof(t) == 32)
		{
			return __popcnt(x);
		}
		else
		{
			return __popcnt64(x);
		}
#elif defined(__GNUC__) || defined(__clang__)
		return __builtin_popcount(x);
#else
		return std::popcount(x);
#endif
	}

	template <typename... t>
	FORCE_INLINE consteval std::size_t
	max_alignof()
	{
		return std::ranges::max({ alignof(t)... });
	}

	// assume align is power of 2
	FORCE_INLINE constexpr std::size_t
	align_up(std::size_t offset, std::size_t align)
	{
		// (offset + align - 1) / align * align
		return (offset + align - 1) & ~(align - 1);
	}
}	 // namespace age::util

namespace age::util
{
	template <typename t_func>
	struct scope_guard
	{
		static_assert(std::is_nothrow_destructible_v<t_func> and noexcept(std::declval<t_func&>()()),
					  "scope_guard: fn() should be noexcept and you must ensure no-throw in dtor.");

		no_unique_addr t_func func;

		explicit constexpr scope_guard(t_func f) noexcept(std::is_nothrow_move_constructible_v<t_func>)
			: func(FWD(f)) { }

		scope_guard(const scope_guard&) = delete;
		scope_guard&
		operator=(const scope_guard&) = delete;

		constexpr ~scope_guard() noexcept
		{
			func();
		}
	};
}	 // namespace age::util
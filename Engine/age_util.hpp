#pragma once

namespace age::util
{
	template <std::unsigned_integral t>
	FORCE_INLINE constexpr auto
	popcount(t x)
	{
		AGE_WARNING_PUSH
		AGE_WARNING_DISABLE_MSVC(5063)
		if constexpr (std::is_constant_evaluated())
		{
			return std::popcount(x);
		}
		AGE_WARNING_POP
		else
		{
#ifdef AGE_COMPILER_MSVC
			if constexpr (sizeof(t) == 1)
			{
				return __popcnt16(x);
			}
			else if constexpr (sizeof(t) == 4)
			{
				return __popcnt(x);
			}
			else
			{
				return __popcnt64(x);
			}

#elif AGE_COMPILER_GCC || AGE_COMPILER_CLANG
			return __builtin_popcount(x);
#else
			return std::popcount(x);
#endif
		}
	}

	template <typename... t>
	FORCE_INLINE consteval std::size_t
	max_alignof()
	{
		return std::ranges::max({ alignof(t)... });
	}

	// assume align is power of 2
	template <std::integral t_ret = std::size_t>
	FORCE_INLINE constexpr t_ret
	align_up(std::size_t offset, std::size_t align)
	{
		// (offset + align - 1) / align * align
		return static_cast<t_ret>((offset + align - 1) & ~(align - 1));
	}
}	 // namespace age::util

namespace age::util
{
	constexpr std::size_t
	mdspan_idx(auto&& md_span, auto&&... idx) noexcept
	{
		static_assert(sizeof...(idx) == BARE_OF(md_span)::rank());
		return []<std::size_t... n> INLINE_LAMBDA_FRONT(std::index_sequence<n...>, auto&& m, auto&&... i) noexcept INLINE_LAMBDA_BACK {
			return ((m.mapping().stride(n) * i) + ...);
		}(std::make_index_sequence<sizeof...(idx)>{}, FWD(md_span), FWD(idx)...);
	}

	FORCE_INLINE constexpr void
	assign_n(auto&& indexable, auto&&... arg) noexcept
	{
		[]<std::size_t... i> INLINE_LAMBDA_FRONT(std::index_sequence<i...>, auto&& v, auto&&... o) noexcept INLINE_LAMBDA_BACK {
			((v[i] = { FWD(o) }), ...);
		}(std::make_index_sequence<sizeof...(arg)>{}, FWD(indexable), FWD(arg)...);
	}
}	 // namespace age::util

namespace age::util
{
	inline void
	ensure_dir_exists(const std::filesystem::path& dir_path) noexcept
	{
		auto ec = std::error_code{};
		std::filesystem::create_directories(dir_path, ec);
		AGE_ASSERT(not ec, "Error : {} \nValue : {} \nCategory:", ec.message(), ec.value(), ec.category().name());
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

namespace age::util
{
	template <auto n>
	struct alignas(n) empty final
	{
		constexpr empty() noexcept = default;
	};
}	 // namespace age::util

namespace age::util
{
	template <meta::cx_implicit_lifetime t>
	FORCE_INLINE constexpr t*
	start_lifetime_as(void* ptr) noexcept
	{
		return std::launder(static_cast<t*>(std::memmove(ptr, ptr, sizeof(t))));
	}
}	 // namespace age::util

namespace age::util
{
	template <std::integral t_idx, std::size_t size>
	struct idx_pool
	{
		// if free => free_dense[pos[idx]] == idx;
		// if allocated => pos[idx] == inv;
		// if free : f_0, f_1, ... => free_dense[0], free_dense[1], ... free_dense[free_count - 1] = { f_0, f_1, ... }
		// for k < free_count: pos[free_dense[k]] == k
		std::array<t_idx, size> free_dense;
		std::array<t_idx, size> pos;
		t_idx					free_count;

		static constexpr t_idx inv = -1;

		constexpr idx_pool() noexcept : free_count{ size }
		{
			std::iota(free_dense.begin(), free_dense.end(), t_idx{ 0 });
			std::iota(pos.begin(), pos.end(), t_idx{ 0 });
		}

		constexpr bool
		is_free(t_idx idx)
		{
			AGE_ASSERT((idx >= 0) and (idx < size));
			return pos[idx] != inv;
		}

		constexpr t_idx
		pop() noexcept
		{
			AGE_ASSERT(free_count > 0);
			AGE_ASSERT(free_dense[free_count - 1] < size);
			AGE_ASSERT(pos[free_dense[free_count - 1]] != inv);

			--free_count;

			auto back_idx = free_dense[free_count];

			pos[back_idx] = inv;

			return back_idx;
		}

		constexpr void
		get(t_idx idx) noexcept
		{
			AGE_ASSERT(free_count > 0);
			AGE_ASSERT(idx < size);
			AGE_ASSERT(pos[idx] != inv);
			AGE_ASSERT(pos[free_dense[free_count - 1]] != inv);

			--free_count;

			auto back_idx = free_dense[free_count];

			free_dense[pos[idx]] = back_idx;
			pos[back_idx]		 = pos[idx];
			pos[idx]			 = inv;
		}

		constexpr void
		push(t_idx idx) noexcept
		{
			AGE_ASSERT(free_count < size);
			AGE_ASSERT(idx < size);
			AGE_ASSERT(pos[idx] == inv);

			pos[idx]			 = free_count;
			free_dense[pos[idx]] = idx;

			++free_count;
		}

		constexpr void
		cleanup() noexcept
		{
			// O(free_count log( free_count ) )

			std::sort(free_dense.begin(), free_dense.begin() + free_count);

			for (t_idx k : std::views::iota(0) | std::views::take(free_count))
			{
				pos[free_dense[k]] = k;
			}
		}

		void
		debug_validate()
		{
			AGE_ASSERT(free_count <= size);

			// for k < free_count: pos[free_dense[k]] == k
			for (t_idx idx : std::views::iota(t_idx{ 0 }) | std::views::take(free_count))
			{
				AGE_ASSERT((free_dense[idx] >= 0) and (free_dense[idx] < size));
				AGE_ASSERT(pos[free_dense[idx]] == idx);
			}

			// if free => free_dense[pos[idx]] == idx;
			for (t_idx idx : std::views::iota(t_idx{ 0 }) | std::views::take(size) | std::views::filter([this](auto idx) { return pos[idx] != inv; }))
			{
				AGE_ASSERT((pos[idx] >= 0) and (pos[idx] < free_count));
				AGE_ASSERT(free_dense[pos[idx]] == idx);
			}
		}
	};
}	 // namespace age::util
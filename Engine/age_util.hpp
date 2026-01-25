#pragma once

namespace age::util
{

	template <std::unsigned_integral t>
	FORCE_INLINE constexpr auto
	popcount(t x)
	{
		AGE_MSVC_WARNING_PUSH
		AGE_MSVC_WARNING_DISABLE(5063)
		if constexpr (std::is_constant_evaluated())
		{
			return std::popcount(x);
		}
		AGE_MSVC_WARNING_POP
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

namespace age::util
{
	struct empty final
	{
		constexpr empty() noexcept = default;
	};
}	 // namespace age::util

namespace age::util
{
	template <typename t>
	concept cx_implicit_lifetime =
		std::is_trivially_copyable_v<std::remove_cvref_t<t>>
		and std::is_trivially_destructible_v<std::remove_cvref_t<t>>;

	template <cx_implicit_lifetime t>
	FORCE_INLINE constexpr t*
	start_lifetime_as(void* ptr) noexcept
	{
		return std::launder(static_cast<t*>(std::memmove(ptr, ptr, sizeof(t))));
	}

	template <typename t>
	FORCE_INLINE void
	write_bytes(t* dst, const t& v) noexcept
		requires std::is_trivially_copyable_v<t>

	{
		std::memcpy(dst, &v, sizeof(v));
	}

	template <typename t>
	FORCE_INLINE void
	write_bytes(void* dst, const t& v) noexcept
		requires std::is_trivially_copyable_v<t>

	{
		std::memcpy(dst, &v, sizeof(v));
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

namespace age::util
{
	namespace detail
	{
		template <typename t>
		concept cx_is_unsigned_enum =
			std::is_enum_v<std::remove_cvref_t<t>>
			and std::unsigned_integral<std::underlying_type_t<std::remove_cvref_t<t>>>;


		template <typename t>
		concept cx_is_bits_or_flag =
			std::unsigned_integral<std::remove_cvref_t<t>>
			or cx_is_unsigned_enum<std::remove_cvref_t<t>>;

		template <typename t>
		struct unsigned_bits_impl
		{
			using type = std::remove_cvref_t<t>;
		};

		template <cx_is_unsigned_enum t>
		struct unsigned_bits_impl<t>
		{
			using type = std::underlying_type_t<std::remove_cvref_t<t>>;
		};

		template <typename t>
		using t_unsigned_bits = typename unsigned_bits_impl<t>::type;

		template <typename t_arg>
		using t_fn_set_bit_it_deref = decltype([](auto bits) { return static_cast<t_arg>(bits & (~bits + 1)); });

		template <typename t_arg>
		using t_fn_set_bit_idx_it_deref = decltype([](auto bits) { return static_cast<t_arg>(std::countr_zero(bits)); });

		template <typename t_arg>
		using t_fn_set_bit_pair_it_deref = decltype([](auto bits) { 
			auto lsb_one = bits & (~bits + 1); 
			return std::pair{ static_cast<t_arg>(std::countr_zero(bits)), static_cast<t_arg>(lsb_one) }; });

		template <typename t_arg, typename t_it_deref_fn>
		struct set_bit_iterator_base
		{
			using t_bits = t_unsigned_bits<t_arg>;

			using iterator_concept = std::forward_iterator_tag;
			using difference_type  = std::ptrdiff_t;
			using value_type	   = decltype(t_it_deref_fn{}(std::declval<t_bits>()));

			t_bits bits = 0;

			[[nodiscard]]
			constexpr value_type
			operator*() const noexcept
			{
				return t_it_deref_fn{}(bits);
			}

			constexpr set_bit_iterator_base&
			operator++() noexcept
			{
				bits &= (bits - 1);
				return *this;
			}

			constexpr set_bit_iterator_base
			operator++(int) noexcept
			{
				auto tmp = *this;
				++(*this);
				return tmp;
			}

			[[nodiscard]]
			friend constexpr bool
			operator==(const set_bit_iterator_base& a, const set_bit_iterator_base& b) noexcept
			{
				return a.bits == b.bits;
			}
		};

		template <typename t_arg>
		using set_bit_iterator = set_bit_iterator_base<t_arg, t_fn_set_bit_it_deref<t_arg>>;

		template <typename t_arg>
		using set_bit_idx_iterator = set_bit_iterator_base<t_arg, t_fn_set_bit_idx_it_deref<t_arg>>;

		template <typename t_arg>
		using set_bit_pair_iterator = set_bit_iterator_base<t_arg, t_fn_set_bit_pair_it_deref<t_arg>>;

		template <typename t_iterator>
		struct set_bit_range : public std::ranges::view_interface<set_bit_range<t_iterator>>
		{
			using t_bits = t_unsigned_bits<typename t_iterator::t_bits>;

			t_bits bits = 0;

			constexpr set_bit_range(auto&& arg) noexcept
				: bits{ static_cast<t_bits>(FWD(arg)) }
			{
			}

			[[nodiscard]]
			constexpr t_iterator
			begin() const noexcept
			{
				return t_iterator{ bits };
			}

			[[nodiscard]]
			constexpr t_iterator
			end() const noexcept
			{
				return t_iterator{ t_bits{ 0 } };
			}
		};
	}	 // namespace detail

	FORCE_INLINE [[nodiscard]]
	constexpr decltype(auto)
	each_set_bit(detail::cx_is_bits_or_flag auto&& arg) noexcept
	{
		return detail::set_bit_range<detail::set_bit_iterator<std::remove_cvref_t<decltype(FWD(arg))>>>{ FWD(arg) };
	}

	FORCE_INLINE [[nodiscard]]
	constexpr decltype(auto)
	each_set_bit_idx(detail::cx_is_bits_or_flag auto&& arg) noexcept
	{
		return detail::set_bit_range<detail::set_bit_idx_iterator<std::remove_cvref_t<decltype(FWD(arg))>>>{ FWD(arg) };
	}

	FORCE_INLINE [[nodiscard]]
	constexpr decltype(auto)
	each_set_bit_pair(detail::cx_is_bits_or_flag auto&& arg) noexcept
	{
		// returns std::pair{ bit_idx, bit }
		return detail::set_bit_range<detail::set_bit_pair_iterator<std::remove_cvref_t<decltype(FWD(arg))>>>{ FWD(arg) };
	}
}	 // namespace age::util
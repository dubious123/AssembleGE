#pragma once

// each_set_bit
namespace age::views
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
}	 // namespace age::views

// circular adjacent
namespace age::views
{
	namespace detail
	{
		template <std::size_t n, typename t_v>
		class circular_adjacent_view : public std::ranges::view_interface<circular_adjacent_view<n, t_v>>
		{
			static_assert(n > 0);

		  private:
			t_v base_{};

			using t_base	   = t_v;
			using t_base_ref   = std::ranges::range_reference_t<t_base>;
			using t_base_value = std::ranges::range_value_t<t_base>;
			using t_size	   = std::ranges::range_size_t<t_base>;
			using t_base_iter  = std::ranges::iterator_t<t_base>;

			static constexpr bool base_ok =
				std::ranges::random_access_range<t_base> && std::ranges::sized_range<t_base>;

			static_assert(base_ok, "circular_adjacent<n> requires random_access_range + sized_range");

			[[nodiscard]] constexpr t_size
			size_() const noexcept
			{
				return std::ranges::size(base_);
			}

			[[nodiscard]] constexpr t_base_value
			at_(t_size i) const noexcept
			{
				return static_cast<t_base_value>(*(std::ranges::begin(base_) + static_cast<std::ptrdiff_t>(i)));
			}

			template <std::size_t... t_i>
			[[nodiscard]] constexpr std::array<t_base_value, n>
			window_(t_size i, std::index_sequence<t_i...>) const noexcept
			{
				const t_size sz = size_();
				return { at_((i + static_cast<t_size>(t_i)) % sz)... };
			}

		  public:
			constexpr circular_adjacent_view() noexcept = default;

			constexpr explicit circular_adjacent_view(t_v base) noexcept
				: base_(std::move(base))
			{
			}

			class iterator
			{
			  private:
				const circular_adjacent_view* parent{};
				t_size						  i{};

			  public:
				using iterator_category = std::input_iterator_tag;
				using difference_type	= std::ptrdiff_t;
				using value_type		= std::array<t_base_value, n>;

				constexpr iterator() noexcept = default;

				constexpr iterator(const circular_adjacent_view& p, t_size idx) noexcept
					: parent(&p), i(idx)
				{
				}

				[[nodiscard]] constexpr value_type
				operator*() const noexcept
				{
					return parent->window_(i, std::make_index_sequence<n>{});
				}

				constexpr iterator&
				operator++() noexcept
				{
					++i;
					return *this;
				}

				constexpr void
				operator++(int) noexcept
				{
					++(*this);
				}

				friend constexpr bool
				operator==(const iterator& a, const iterator& b) noexcept
				{
					return a.i == b.i;
				}
			};

			[[nodiscard]] constexpr iterator
			begin() const noexcept
			{
				const t_size sz = size_();
				if (sz < static_cast<t_size>(n))
				{
					return iterator{ *this, t_size{ 0 } };
				}
				else
				{
					return iterator{ *this, t_size{ 0 } };
				}
			}

			[[nodiscard]] constexpr iterator
			end() const noexcept
			{
				const t_size sz = size_();
				if (sz < static_cast<t_size>(n))
				{
					return iterator{ *this, t_size{ 0 } };
				}
				else
				{
					return iterator{ *this, sz };
				}
			}
		};

		template <std::size_t n>
		struct circular_adjacent_adaptor
		{
			static_assert(n > 0);

			template <std::ranges::viewable_range t_r>
			[[nodiscard]] constexpr auto
			operator()(t_r&& r) const noexcept
			{
				auto v = std::views::all(std::forward<t_r>(r));
				return circular_adjacent_view<n, decltype(v)>{ std::move(v) };
			}

			template <std::ranges::viewable_range t_r>
			friend constexpr auto
			operator|(t_r&& r, const circular_adjacent_adaptor& self) noexcept
			{
				return self(std::forward<t_r>(r));
			}
		};
	}	 // namespace detail

	template <std::size_t n>
	inline constexpr detail::circular_adjacent_adaptor<n> circular_adjacent{};
}	 // namespace age::views

// transform each

namespace age::views
{
	namespace detail
	{
		template <typename t_arr, typename t_fn, std::size_t... t_i>
		constexpr auto
		map_each_impl(t_arr&& arr, t_fn&& fn, std::index_sequence<t_i...>) noexcept(noexcept(std::array{ fn(arr[t_i])... }))
		{
			return std::array{ fn(arr[t_i])... };
		}

		template <typename t_fn>
		struct transform_each_auto_closure
		{
			no_unique_addr t_fn fn;

			template <std::ranges::viewable_range t_r>
			constexpr auto
			operator()(t_r&& r) const
			{
				using t_view = std::views::all_t<t_r>;
				using t_val	 = std::ranges::range_value_t<t_view>;

				static_assert(
					requires { std::tuple_size<t_val>::value; },
					"transform_each requires tuple-like value_type (std::array / tuple)");

				constexpr std::size_t n = std::tuple_size_v<t_val>;

				return std::views::all(FWD(r))
					 | std::views::transform([*this](auto&& arr) {
						   return age::views::detail::map_each_impl(
							   arr,
							   fn,
							   std::make_index_sequence<n>{});
					   });
			}

			template <std::ranges::viewable_range t_r>
			friend constexpr auto
			operator|(t_r&& r, const transform_each_auto_closure& self)
			{
				return self(std::forward<t_r>(r));
			}
		};
	}	 // namespace detail

	template <typename t_fn>
	constexpr auto
	transform_each(t_fn fn)
	{
		return detail::transform_each_auto_closure<t_fn>{ std::move(fn) };
	}
}	 // namespace age::views
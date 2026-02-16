#pragma once

namespace age::meta::inline traits
{
	template <typename t, typename t_container, typename t_allocator, typename... t_args>
	concept emplace_constructible =
		std::is_same_v<t*, typename t_container::pointer>
		and std::is_same_v<typename t_container::allocator_type, typename std::allocator_traits<t_allocator>::template rebind_alloc<t>>
		and requires {
				std::allocator_traits<t_allocator>::construct(
					std::declval<t_allocator&>(),
					std::declval<t*>(),
					std::declval<t_args>()...);
			};

	template <typename t, typename t_container, typename t_allocator, typename... t_args>
	concept move_insertable =
		std::is_same_v<typename t_container::value_type, t>
		and std::is_same_v<typename t_container::allocator_type, typename std::allocator_traits<t_allocator>::template rebind_alloc<t>>
		and requires {
				std::allocator_traits<t_allocator>::construct(
					std::declval<t_allocator&>(),
					std::declval<t*>(),
					std::declval<t_args>()...);
			};

	template <typename t>
	concept cx_allocator =
		requires {
			typename std::allocator_traits<t>::value_type;
			typename std::allocator_traits<t>::pointer;
		}
		and requires(t a, std::size_t n) {
				{ std::allocator_traits<t>::allocate(a, n) } -> std::same_as<typename std::allocator_traits<t>::pointer>;
				{ std::allocator_traits<t>::deallocate(a, std::allocator_traits<t>::allocate(a, n), n) };
			};

	template <typename t>
	concept is_zero_initializable =
		std::is_trivially_default_constructible_v<t>
		and std::is_trivially_copyable_v<t>;

	template <typename t>
	concept cx_byte_writable_contiguous_range =
		std::ranges::contiguous_range<t>
		and std::ranges::sized_range<t>
		and std::is_trivially_copyable_v<std::ranges::range_value_t<t>>;

	template <typename t>
	concept cx_implicit_lifetime =
		std::is_trivially_copyable_v<std::remove_cvref_t<t>>
		and std::is_trivially_destructible_v<std::remove_cvref_t<t>>;
}	 // namespace age::meta::inline traits
#pragma once
#include "age_pch.hpp"

namespace age::inline data_structure
{
	inline constexpr std::size_t dynamic_extent = std::numeric_limits<std::size_t>::max();

	template <typename t, typename t_allocator = std::allocator<t>>
	struct dynamic_array;

	template <typename t, std::size_t size = dynamic_extent, typename t_allocator = std::allocator<t>>
	using array = std::conditional_t<size == dynamic_extent, dynamic_array<t>, std::array<t, size>>;
}	 // namespace age::inline data_structure

namespace age::inline data_structure
{
	template <typename t, typename t_allocator>
	struct dynamic_array
	{
		using value_type	 = t;
		using allocator_type = typename std::allocator_traits<t_allocator>::template rebind_alloc<t>;

		using size_type		  = std::size_t;
		using difference_type = std::ptrdiff_t;
		using reference		  = value_type&;
		using const_reference = const value_type&;
		using pointer		  = typename std::allocator_traits<allocator_type>::pointer;
		using const_pointer	  = typename std::allocator_traits<allocator_type>::const_pointer;

		using iterator				 = t*;
		using const_iterator		 = const t*;
		using reverse_iterator		 = std::reverse_iterator<iterator>;
		using const_reverse_iterator = std::reverse_iterator<const_iterator>;

		static_assert(std::is_same_v<typename std::allocator_traits<t_allocator>::value_type, t>);
		static_assert(std::is_empty_v<t_allocator>);
		static_assert(std::is_same_v<pointer, t*>, "fancy pointer not supported yet");
		static_assert(std::contiguous_iterator<iterator>);
		static_assert(std::contiguous_iterator<const_iterator>);

	  private:
		size_type count	 = {};
		t*		  p_data = nullptr;

		no_unique_addr
		allocator_type alloc;
	};

	// template <typename t, typename... u>
	// using some_type_t = std::conditional_t<meta::cx_allocator<meta::variadic_back_t<u...>>,
	//									   meta::variadic_back_t<u...>,
	//									   std::allocator<t>>;
	// static_assert(std::is_same_v<some_type_t<int, float, double>, std::allocator<int>>);

	// template <typename t, typename... u>
	// dynamic_array(t, u...) -> dynamic_array<t, std::conditional_t<meta::cx_allocator<meta::variadic_back_t<u...>>,
	//															  meta::variadic_back_t<u...>,
	//															  std::allocator<t>>>;
}	 // namespace age::inline data_structure

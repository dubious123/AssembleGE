#pragma once

namespace age::inline data_structure
{
	template <typename t, typename t_allocator = std::allocator<t>>
	struct dynamic_array;

	template <typename t, std::size_t size>
	using array = std::array<t, size>;

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

		AGE_DISABLE_COPY(dynamic_array)

	  private:
		size_type count = {};

		t* p_data = nullptr;

		no_unique_addr
		allocator_type alloc;

	  public:
		constexpr dynamic_array(dynamic_array&& other) noexcept
			: count{ std::exchange(other.count, 0) },
			  p_data{ std::exchange(other.p_data, nullptr) },
			  alloc{ other.get_allocator() }
		{
		}

		FORCE_INLINE constexpr dynamic_array(auto&&... args) noexcept
			requires(std::is_constructible_v<t, decltype(args)> and ...)
			: count{ sizeof...(args) },
			  p_data{ _alloc(allocator_type{}, sizeof...(args)) },
			  alloc(allocator_type{})
		{
			auto i = 0;
			(_construct(alloc, p_data + i++, FWD(args)), ...);
		}

		template <std::ranges::input_range r>
		constexpr dynamic_array(std::from_range_t, r&& rg) noexcept
			requires(std::ranges::sized_range<r> or std::ranges::forward_range<r>)
			: count{ static_cast<size_type>(std::ranges::size(rg)) },
			  alloc(allocator_type{})
		{
			static_assert(meta::emplace_constructible<t, dynamic_array<t, t_allocator>, allocator_type, decltype(*std::ranges::begin(rg))>);

			p_data = _alloc(alloc, count);

			if consteval
			{
				for (auto i = 0; auto&& elem : rg)
				{
					_construct(alloc, p_data + i++, FWD(elem));
				}
			}
			else
			{
				if constexpr (std::contiguous_iterator<std::ranges::iterator_t<r>> and std::is_trivially_copyable_v<t>)
				{
					std::memcpy(p_data, std::to_address(std::ranges::begin(rg)), sizeof(t) * count);
				}
				else if constexpr (std::is_nothrow_constructible_v<t, decltype(*std::ranges::begin(rg))>)
				{
					for (size_type i = 0; auto&& elem : rg)
					{
						_construct(alloc, p_data + i++, FWD(elem));
					}
				}
				else
				{
					static_assert(false, "throwable element is not supported yet");
				}
			}
		}

		template <typename t_input_it>
		FORCE_INLINE constexpr dynamic_array(t_input_it first, t_input_it last) noexcept
			: alloc{ allocator_type{} }
		{
			if constexpr (std::random_access_iterator<t_input_it>)
			{
				count  = static_cast<size_type>(last - first);
				p_data = _alloc(alloc, count);
			}
			else if constexpr (std::forward_iterator<t_input_it>)
			{
				count  = static_cast<size_type>(std::distance(first, last));
				p_data = _alloc(alloc, count);
			}
			else
			{
				static_assert(false, "not supported yet");
			}

			if consteval
			{
				for (auto i = 0; i < count; ++i)
				{
					_construct(alloc, p_data + i, *first++);
				}
			}
			else
			{
				if constexpr (std::contiguous_iterator<t_input_it> and std::is_trivially_copyable_v<t>)
				{
					std::memcpy(p_data, std::to_address(first), sizeof(t) * count);
				}
				else if constexpr (std::is_nothrow_constructible_v<t, decltype(*first)>)
				{
					for (auto i = 0; i < count; ++i)
					{
						_construct(alloc, p_data + i, *first++);
					}
				}
				else
				{
					static_assert(false, "throwable element is not supported yet");
				}
			}
		}

		FORCE_INLINE constexpr ~dynamic_array() noexcept
		{
			if consteval
			{
				for (auto i = 0; i < count; ++i)
				{
					std::allocator_traits<allocator_type>::destroy(alloc, p_data + i);
				}
			}
			else
			{
				if constexpr (std::is_trivially_destructible_v<t>)
				{
				}
				else if constexpr (std::is_nothrow_destructible_v<t>)
				{
					for (auto i = 0; i < count; ++i)
					{
						std::allocator_traits<allocator_type>::destroy(alloc, p_data + i);
					}
				}
				else
				{
					static_assert(false, "not supported yet");
				}
			}
			_dealloc(alloc, p_data, count);
		}

	  private:
		FORCE_INLINE constexpr dynamic_array(t* ptr, size_type size) noexcept
			: p_data{ ptr }, count{ size }, alloc{ allocator_type{} }
		{
			if constexpr (meta::is_zero_initializable<t>)
			{
				std::uninitialized_default_construct_n(p_data, count);
			}
			else if constexpr (std::is_nothrow_constructible_v<t>)
			{
				for (auto i = 0; i < count; ++i)
				{
					_construct(alloc, p_data + i);
				}
			}
			else
			{
				static_assert(false, "throwable element not supported yet");
			}
		}

		FORCE_INLINE constexpr dynamic_array(size_type size, void* p_hint = nullptr) noexcept
			: count{ size },
			  p_data{ _alloc(allocator_type{}, size, p_hint) }
		{
			if constexpr (meta::is_zero_initializable<t>)
			{
				std::uninitialized_default_construct_n(p_data, count);
			}
			else if constexpr (std::is_nothrow_constructible_v<t>)
			{
				for (auto i = 0; i < count; ++i)
				{
					_construct(alloc, p_data + i);
				}
			}
			else
			{
				static_assert(false, "throwable element not supported yet");
			}
		}

	  public:
		FORCE_INLINE static constexpr dynamic_array
		gen_sized_default(size_type n, void* p_hint = nullptr) noexcept
		{
			return dynamic_array{ n, p_hint };
		}

		FORCE_INLINE static constexpr dynamic_array
		gen_sized_copy(size_type n, const auto& elem, void* p_hint = nullptr) noexcept
		{
			auto alloc = allocator_type{};

			auto* p_data = _alloc(alloc, n, p_hint);

			for (auto i = 0; i < n; ++i)
			{
				_construct(alloc, p_data + i, elem);
			}

			return dynamic_array{ p_data, n };
		}

		FORCE_INLINE constexpr t&
		at(size_type i) noexcept
		{
			AGE_ASSERT(i < count);
			return p_data[i];
		}

		FORCE_INLINE constexpr const t&
		at(size_type i) const noexcept
		{
			AGE_ASSERT(i < count);
			return p_data[i];
		}

		FORCE_INLINE constexpr t&
		front() noexcept
		{
			AGE_ASSERT(not empty());
			return p_data[0];
		}

		FORCE_INLINE constexpr const t&
		front() const noexcept
		{
			AGE_ASSERT(not empty());
			return p_data[0];
		}

		FORCE_INLINE constexpr t&
		back() noexcept
		{
			AGE_ASSERT(not empty());
			return p_data[count - 1];
		}

		FORCE_INLINE constexpr const t&
		back() const noexcept
		{
			AGE_ASSERT(not empty());
			return p_data[count - 1];
		}

		FORCE_INLINE constexpr reference
		operator[](size_type i) noexcept
		{
			AGE_ASSERT(i < count);
			return p_data[i];
		}

		FORCE_INLINE constexpr reference
		operator[](size_type i) const noexcept
		{
			AGE_ASSERT(i < count);
			return p_data[i];
		}

		FORCE_INLINE constexpr const t*
		data() const noexcept
		{
			return p_data;
		}

		FORCE_INLINE constexpr t*
		data() noexcept
		{
			return p_data;
		}

		FORCE_INLINE constexpr size_type
		size() const noexcept
		{
			return count;
		}

		template <typename t_ret = std::size_t>
		FORCE_INLINE constexpr t_ret
		byte_size() const noexcept
		{
			static_assert(std::unsigned_integral<t_ret>, "invalid return type");
			AGE_ASSERT(count * sizeof(t) <= std::numeric_limits<t_ret>::max());
			return static_cast<t_ret>(count * sizeof(t));
		}

		FORCE_INLINE constexpr bool
		empty() const noexcept
		{
			return count == 0;
		}

		FORCE_INLINE constexpr iterator
		begin() noexcept
		{
			return p_data;
		}

		FORCE_INLINE constexpr const_iterator
		begin() const noexcept
		{
			return p_data;
		}

		FORCE_INLINE constexpr const_iterator
		cbegin() const noexcept
		{
			return p_data;
		}

		FORCE_INLINE constexpr iterator
		end() noexcept
		{
			return p_data + count;
		}

		FORCE_INLINE constexpr const_iterator
		end() const noexcept
		{
			return p_data + count;
		}

		FORCE_INLINE constexpr const_iterator
		cend() const noexcept
		{
			return p_data + count;
		}

		FORCE_INLINE constexpr reverse_iterator
		rbegin() noexcept
		{
			return reverse_iterator(end());
		}

		FORCE_INLINE constexpr const_reverse_iterator
		rbegin() const noexcept
		{
			return const_reverse_iterator(end());
		}

		FORCE_INLINE constexpr const_reverse_iterator
		crbegin() const noexcept
		{
			return const_reverse_iterator(cend());
		}

		FORCE_INLINE constexpr reverse_iterator
		rend() noexcept
		{
			return reverse_iterator(begin());
		}

		FORCE_INLINE constexpr const_reverse_iterator
		rend() const noexcept
		{
			return const_reverse_iterator(begin());
		}

		FORCE_INLINE constexpr const_reverse_iterator
		crend() const noexcept
		{
			return const_reverse_iterator(cbegin());
		}

		FORCE_INLINE constexpr allocator_type
		get_allocator() const
		{
			return alloc;
		}

	  private:
		FORCE_INLINE static constexpr void
		_dealloc(allocator_type alloc, t* p_data, const size_type n) noexcept
		{
			std::allocator_traits<allocator_type>::deallocate(alloc, p_data, n);
		}

		FORCE_INLINE static constexpr t*
		_alloc(allocator_type alloc, const size_type n) noexcept
		{
			return std::allocator_traits<allocator_type>::allocate(alloc, n);
		}

		FORCE_INLINE static constexpr t*
		_alloc(allocator_type alloc, const size_type n, void* p_hint) noexcept
		{
			return std::allocator_traits<allocator_type>::allocate(alloc, n, p_hint);
		}

		FORCE_INLINE static constexpr void
		_construct(allocator_type alloc, t* const p_data, auto&&... arg) noexcept
		{
			std::allocator_traits<allocator_type>::construct(alloc, p_data, FWD(arg)...);
		}
	};
}	 // namespace age::inline data_structure

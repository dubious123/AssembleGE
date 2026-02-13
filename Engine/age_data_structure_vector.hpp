#pragma once
#include "age_pch.hpp"

namespace age::inline data_structure
{
	template <typename t, typename t_allocator = std::allocator<t>>
	struct vector
	{
		using value_type	 = t;
		using allocator_type = typename std::allocator_traits<t_allocator>::template rebind_alloc<t>;

		using size_type		  = std::size_t;
		using difference_type = std::ptrdiff_t;
		using reference		  = value_type&;
		using pointer		  = typename std::allocator_traits<allocator_type>::pointer;
		using const_pointer	  = typename std::allocator_traits<allocator_type>::const_pointer;

		using iterator				 = t*;
		using const_iterator		 = const t*;
		using reverse_iterator		 = std::reverse_iterator<iterator>;
		using const_reverse_iterator = std::reverse_iterator<const_iterator>;

		static_assert(std::is_same_v<typename std::allocator_traits<t_allocator>::value_type, t>);
		static_assert(std::is_empty_v<t_allocator>);
		static_assert(std::is_same_v<pointer, t*>, "fancy pointer not supported yet");
		static_assert(meta::cx_allocator<allocator_type>);

	  private:
		size_type count	 = {};
		size_type cap	 = {};
		t*		  p_data = nullptr;

		no_unique_addr
		allocator_type alloc;

	  public:
		constexpr vector() noexcept
			: alloc{ allocator_type{} } { };

		constexpr vector(const vector& other) noexcept
			: count{ other.count },
			  cap{ other.count },
			  p_data{ _alloc(other.get_allocator(), other.count) },
			  alloc{ other.get_allocator() }

		{
			_copy_construct_n(alloc, p_data, other.p_data, count);
		}

		constexpr vector(vector&& other) noexcept
			: count{ std::exchange(other.count, 0) },
			  cap{ std::exchange(other.cap, 0) },
			  p_data{ std::exchange(other.p_data, nullptr) },
			  alloc{ other.get_allocator() }
		{
		}

		template <typename t_input_it>
		FORCE_INLINE constexpr vector(t_input_it first, t_input_it last) noexcept
			: alloc{ allocator_type{} }
		{
			if constexpr (std::random_access_iterator<t_input_it>)
			{
				count  = static_cast<size_type>(last - first);
				cap	   = count;
				p_data = _alloc(alloc, cap);
			}
			else if constexpr (std::forward_iterator<t_input_it>)
			{
				count  = static_cast<size_type>(std::distance(first, last));
				cap	   = count;
				p_data = _alloc(alloc, cap);
			}
			else
			{
				static_assert(false, "not supported yet");
			}

			if consteval
			{
				for (auto i = 0; i < count; ++i)
				{
					std::allocator_traits<allocator_type>::construct(alloc, p_data + i, *first++);
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
						std::allocator_traits<allocator_type>::construct(alloc, p_data + i, *first++);
					}
				}
				else
				{
					static_assert(false, "throwable element is not supported yet");
				}
			}
		}

		FORCE_INLINE constexpr vector(std::initializer_list<t> init) noexcept
			: vector(init.begin(), init.end())
		{
			static_assert(std::contiguous_iterator<decltype(init.begin())> and std::is_trivially_copyable_v<t>);
		}

		FORCE_INLINE constexpr vector(auto&&... args) noexcept
			requires(std::is_constructible_v<t, decltype(args)> and ...)
			: count{ sizeof...(args) },
			  cap{ sizeof...(args) },
			  p_data{ _alloc(allocator_type{}, sizeof...(args)) },
			  alloc(allocator_type{})
		{
			auto i = 0;
			(std::allocator_traits<allocator_type>::construct(alloc, p_data + i++, FWD(args)), ...);
		}

		template <std::ranges::input_range r>
		constexpr vector(std::from_range_t, r&& rg) noexcept
		{
			static_assert(meta::emplace_constructible<t, vector<t, t_allocator>, allocator_type, decltype(*std::ranges::begin(rg))>);
			static_assert(
				std::ranges::sized_range<r>
				or std::ranges::forward_range<r>
				or meta::move_insertable<t, vector<t, t_allocator>, allocator_type, decltype(*std::ranges::begin(rg))>);

			append_range(FWD(rg));
		}

		constexpr vector&
		operator=(const vector& other) noexcept
		{
			if (this == &other) { return *this; }

			_destroy_n(alloc, p_data, count);

			if (cap < other.count)
			{
				_dealloc(alloc, p_data, cap);
				p_data = _alloc(alloc, other.count);
			}

			_copy_construct_n(alloc, p_data, other.p_data, other.count);

			count = other.count;

			return *this;
		}

		constexpr vector&
		operator=(vector&& other) noexcept
		{
			if (this == &other) return *this;

			clear();

			_destroy_n(alloc, p_data, count);
			_dealloc(alloc, p_data, cap);

			p_data = other.p_data;
			count  = other.count;
			cap	   = other.cap;
			alloc  = other.get_allocator();

			other.p_data = nullptr;
			other.count	 = 0;
			other.cap	 = 0;

			return *this;
		}

		constexpr ~vector() noexcept
		{
			_destroy_n(alloc, p_data, count);
			_dealloc(alloc, p_data, cap);
		}

		FORCE_INLINE static constexpr vector
		gen_reserved(size_type n) noexcept
		{
			auto vec   = vector{};
			vec.count  = n;
			vec.cap	   = n;
			vec.p_data = _alloc(vec.alloc, n);
			return vec;
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

		FORCE_INLINE constexpr decltype(count)
		size() const noexcept
		{
			return count;
		}

		FORCE_INLINE constexpr decltype(cap)
		capacity() const noexcept
		{
			return cap;
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

		FORCE_INLINE constexpr void
		reserve(size_type new_cap) noexcept
		{
			if (cap >= new_cap)
			{
				return;
			}

			auto* p_new_data = _alloc(alloc, new_cap, p_data);

			if (p_data is_not_nullptr)
			{
				_move_construct_n(alloc, p_new_data, p_data, count);
				_dealloc(get_allocator(), p_data, cap);
			}

			p_data = p_new_data;
			cap	   = new_cap;
		}

		FORCE_INLINE constexpr void
		clear() noexcept
		{
			_destroy_n(alloc, p_data, count);
			count = 0;
		}

		FORCE_INLINE constexpr reference
		emplace_back(auto&&... arg) noexcept
		{
			static_assert(std::is_nothrow_constructible_v<t, decltype(arg)...>, "not supported");

			if (_is_full())
			{
				reserve(cap * 2 + 1);
			}

			std::allocator_traits<allocator_type>::construct(alloc, p_data + count++, FWD(arg)...);
			return back();
		}

		template <typename r>
		constexpr void
		append_range(r&& rg) noexcept
		{
			static_assert(std::assignable_from<t&, std::ranges::range_reference_t<r>>);
			static_assert(std::is_same_v<decltype(*std::ranges::begin(rg)), std::ranges::range_reference_t<r>>);
			static_assert(meta::emplace_constructible<t, vector<t, t_allocator>, allocator_type, std::ranges::range_reference_t<r>>);

			if constexpr (std::ranges::sized_range<t> or std::ranges::forward_range<r>)
			{
				if constexpr (std::ranges::sized_range<t>)
				{
					reserve(count + static_cast<size_type>(std::ranges::size(rg)));
				}
				else
				{
					reserve(count + static_cast<size_type>(std::ranges::distance(rg)));
				}

				if consteval
				{
					for (auto i = 0; auto&& elem : rg)
					{
						std::allocator_traits<allocator_type>::construct(alloc, p_data + i++, FWD(elem));
					}
				}
				else
				{
					if constexpr (std::contiguous_iterator<std::ranges::iterator_t<r>> and std::is_trivially_copyable_v<t>)
					{
						std::memcpy(p_data, std::to_address(std::ranges::begin(rg)), sizeof(t) * count);
					}
					else if constexpr (std::is_nothrow_copy_constructible_v<t>)
					{
						auto it = std::ranges::begin(rg);
						for (size_type i = 0; i < count; ++i)
						{
							std::allocator_traits<allocator_type>::construct(alloc, p_data + i, *it++);
						}
					}
					else
					{
						static_assert(false, "throwable element is not supported yet");
					}
				}
			}
			else
			{
				static_assert(false, "not efficient");
				for (auto&& item : rg)
				{
					emplace_back(FWD(item));
				}
			}
		}

		constexpr void
		pop_back() noexcept
		{
			AGE_ASSERT(empty() is_false);
			--count;
			if constexpr (std::is_trivially_destructible_v<t>)
			{
			}
			else if constexpr (std::is_nothrow_destructible_v<t>)
			{
				std::allocator_traits<allocator_type>::destroy(p_data + count);
			}
			else
			{
				static_assert(false, "not supported");
			}
		}

		constexpr void
		swap(vector& other) noexcept
		{
			std::swap(count, other.count);
			std::swap(cap, other.cap);
			std::swap(p_data, other.p_data);
		}

		constexpr void
		resize(size_type new_size) noexcept
		{
			if (new_size < count)
			{
				_destroy_n(alloc, p_data + new_size, count - new_size);
			}
			else if (new_size > count)
			{
				reserve(new_size);

				for (auto i = count; i < new_size; ++i)
				{
					std::allocator_traits<allocator_type>::construct(alloc, p_data + count);
				}
			}

			count = new_size;
		}

		constexpr void
		resize(size_type new_size, const value_type& value) noexcept
		{
			if (new_size < count)
			{
				_destroy_n(alloc, p_data + new_size, count - new_size);
			}
			else if (new_size > count)
			{
				reserve(new_size);

				for (auto i = 0; i < count - new_size; ++i)
				{
					std::allocator_traits<allocator_type>::construct(alloc, p_data + i, value);
				}
			}

			count = new_size;
		}

	  private:
		FORCE_INLINE static constexpr void
		_dealloc(allocator_type alloc, t* p_data, const size_type cap) noexcept
		{
			std::allocator_traits<allocator_type>::deallocate(alloc, p_data, cap);
		}

		FORCE_INLINE static constexpr t*
		_alloc(allocator_type alloc, const size_type cap) noexcept
		{
			return std::allocator_traits<allocator_type>::allocate(alloc, cap);
		}

		FORCE_INLINE static constexpr t*
		_alloc(allocator_type alloc, const size_type cap, t* p_hint) noexcept
		{
			return std::allocator_traits<allocator_type>::allocate(alloc, cap, p_hint);
		}

		FORCE_INLINE static constexpr void
		_copy_construct_n(allocator_type alloc, t* p_dst, t* p_src, size_type n) noexcept
		{
			if consteval
			{
				for (auto i = 0; i < n; ++i)
				{
					std::allocator_traits<allocator_type>::construct(alloc, p_dst + i, p_src[i]);
				}
			}
			else
			{
				if constexpr (std::is_trivially_copyable_v<t>)
				{
					std::memcpy(p_dst, p_src, sizeof(t) * n);
				}
				else if constexpr (std::is_nothrow_copy_constructible_v<t>)
				{
					for (auto i = 0; i < n; ++i)
					{
						std::allocator_traits<allocator_type>::construct(alloc, p_dst + i, p_src[i]);
					}
				}
				else
				{
					static_assert(false, "throwable element not supported yet");
				}
			}
		}

		FORCE_INLINE static constexpr void
		_move_construct_n(allocator_type alloc, t* p_dst, t* p_src, size_type n) noexcept
		{
			if consteval
			{
				for (auto i = 0; i < n; ++i)
				{
					std::allocator_traits<allocator_type>::construct(alloc, p_dst + i, std::move(p_src[i]));
				}
			}
			else
			{
				if constexpr (std::is_trivially_move_constructible_v<t>)
				{
					std::memcpy(p_dst, p_src, sizeof(t) * n);
				}
				else if constexpr (std::is_nothrow_move_constructible_v<t>)
				{
					for (auto i = 0; i < n; ++i)
					{
						std::allocator_traits<allocator_type>::construct(alloc, p_dst + i, std::move(p_src[i]));
					}
				}
				else
				{
					static_assert(false, "throwable element not supported yet");
				}
			}
		}

		FORCE_INLINE static constexpr void
		_destroy_n(allocator_type alloc, t* p_data, size_type n) noexcept
		{
			if consteval
			{
				for (auto i = 0; i < n; ++i)
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
					for (auto i = 0; i < n; ++i)
					{
						std::allocator_traits<allocator_type>::destroy(alloc, p_data + i);
					}
				}
				else
				{
					static_assert(false, "not supported yet");
				}
			}
		}

		FORCE_INLINE constexpr bool
		_is_full() const noexcept
		{
			return count == cap;
		}
	};

	template <typename t, typename t_alloc>
	FORCE_INLINE constexpr bool
	operator==(const vector<t, t_alloc>& lhs,
			   const vector<t, t_alloc>& rhs) noexcept
	{
		if (lhs.size() != rhs.size()) return false;

		return std::equal(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
	}

	template <typename t, typename t_alloc>
	FORCE_INLINE constexpr auto
	operator<=>(const vector<t, t_alloc>& lhs,
				const vector<t, t_alloc>& rhs) noexcept
	{
		return std::lexicographical_compare_three_way(
			lhs.begin(), lhs.end(),
			rhs.begin(), rhs.end());
	}
}	 // namespace age::inline data_structure
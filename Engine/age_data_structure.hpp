#pragma once
#include "age.hpp"
#ifdef USE_STL_VECTOR
namespace age::data_structure
{
	template <typename T>
	using vector = std::vector<T>;
}
#else
namespace age::data_structure
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

		FORCE_INLINE constexpr vector(std::initializer_list<t> init) noexcept
			: count{ init.size() },
			  cap{ init.size() },
			  p_data{ _alloc(init.size()) },
			  alloc{ allocator_type{} }
		{
			_move_construct_n(alloc, p_data, init.begin(), count);
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
				else if constexpr (std::is_nothrow_copy_constructible_v<t>)
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

		template <std::ranges::input_range r>
		constexpr vector(std::from_range_t, r&& rg) noexcept
		{
			static_assert(emplace_constructible<t, vector<t, t_allocator>, allocator_type, decltype(*std::ranges::begin(rg))>);
			static_assert(
				std::ranges::sized_range<r>
				or std::ranges::forward_range<r>
				or move_insertable<t, vector<t, t_allocator>, allocator_type, decltype(*std::ranges::begin(rg))>);

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
				cap = _alloc(alloc, other.count);
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
			static_assert(emplace_constructible<t, vector<t, t_allocator>, allocator_type, std::ranges::range_reference_t<r>>);

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
				_destroy_n(p_data + new_size, count - new_size);
			}
			else if (new_size > count)
			{
				reserve(new_size);

				for (auto i = 0; i < count - new_size; ++i)
				{
					std::allocator_traits<allocator_type>::construct(get_allocator(), p_data + i, value);
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
}	 // namespace age::data_structure
#endif

#ifdef USE_STL_SET
namespace age::data_structure
{
	template <typename T>
	using set = std::set<T>;
}	 // namespace age::data_structure
#endif

#ifdef USE_STL_MAP
namespace age::data_structure
{
	template <typename T, typename K>
	using map = std::map<T, K>;
}	 // namespace age::data_structure
#endif


#ifdef USE_STL_LIST
namespace age::data_structure
{
	template <typename T>
	using list = std::list<T>;
}
#else
namespace age::data_structure
{
	template <typename T>
	class list {
		struct node
		{
			T	  value;
			node* next = nullptr;
			node* prev = nullptr;

			// node(T&& t)
			//{
			//	value =
			// }
		};

	  private:
		node*  _head;
		node*  _tail;
		size_t _size;

	  public:
		constexpr list()
		{
			_size = 0;
			_head = nullptr;
			_tail = nullptr;
		}

		constexpr ~list()
		{
			auto p_now = _head;
			for (auto i = 0; i < _size; ++i)
			{
				auto p_next = p_now->next;
				if constexpr (std::is_trivially_destructible_v<T> == false)
				{
					(p_now->value).~T();
				}

				free(p_now);
				p_now = p_next;
			}
		}

		constexpr bool
		empty()
		{
			return _size == 0;
		}

		constexpr node*
		front()
		{
			// assert(empty() == false);
			return _head;
		}

		constexpr node*
		back()
		{
			// assert(empty() == false);
			return _tail;
		}

		constexpr size_t
		size()
		{
			return _size;
		}

		template <typename... V>
		T&
		emplace_back(V&&... args)
		{
			if (empty()) [[unlikely]]
			{
				_head = _tail = (node*)malloc(sizeof(node));
				_head->next	  = nullptr;
				_head->prev	  = nullptr;
				new (&_head->value) T{ std::forward<V>(args)... };
			}
			else
			{
				_tail->next		  = (node*)malloc(sizeof(node));
				_tail->next->next = nullptr;
				new (&_tail->next->value) T{ std::forward<V>(args)... };
				_tail->next->prev = _tail;
				_tail			  = _tail->next;
			}

			++_size;
			return _tail->value;
		}

		template <typename... V>
		T&
		emplace_front(V&&... args)
		{
			if (empty()) [[unlikely]]
			{
				_head = _tail = (node*)malloc(sizeof(node));
				_head->next	  = nullptr;
				_head->prev	  = nullptr;
				new (&_head->value) T{ std::forward<V>(args)... };
			}
			else
			{
				_head->prev		  = (node*)malloc(sizeof(node));
				_head->prev->prev = nullptr;
				new (&_head->prev->value) T{ std::forward<V>(args)... };
				_head->prev->next = _head;
				_head			  = _head->prev;
			}

			++_size;
			return _head->value;
		}

		void
		pop_front()
		{
			assert(empty() == false);
			auto* temp = _head->next;

			if constexpr (std::is_trivially_destructible_v<T> == false)
			{
				(_head->value).~T();
			}

			free(_head);
			_head = temp;
			--_size;
			//_head->prev = nullptr;
		}

		void
		pop_back()
		{
			assert(empty() == false);
			auto* temp = _tail->prev;
			if constexpr (std::is_trivially_destructible_v<T> == false)
			{
				(_tail->value).~T();
			}
			free(_tail);
			_tail = temp;
			--_size;
			//_tail->next = nullptr;
		}

		template <typename... V>
		node*
		insert(node* at, V&&... args)
		{
			auto* prev = at->prev;
			at->prev   = (node*)malloc(sizeof(node));
			new (&at->prev->value) T{ std::forward<V>(args)... };


			(at->prev)->prev = prev;
			(at->prev)->next = prev->next;

			prev->next = at->prev;
			++_size;

			return at->prev;
		}

		void
		erase(node* target)
		{
			assert(empty() == false and target != nullptr);

			if (target == _tail) [[unlikely]]
			{
				_tail = target->prev;
			}
			if (target == _head) [[unlikely]]
			{
				_head = target->next;
			}

			auto* prev = target->prev;
			auto* next = target->next;
			prev->next = next;
			next->prev = prev;

			if constexpr (std::is_trivially_destructible_v<T> == false)
			{
				(target->value).~T();
			}

			free(target);
			--_size;
		}
	};
}	 // namespace age::data_structure
#endif

namespace age::data_structure
{
	template <typename t_data, std::size_t static_count = std::dynamic_extent>
	struct sparse_vector
	{
		struct bucket
		{
			using storage_t = std::conditional_t<(sizeof(t_data) >= sizeof(std::size_t)), t_data, std::size_t>;

			alignas(alignof(storage_t))
				std::byte storage[sizeof(storage_t)];

			FORCE_INLINE
			bucket() noexcept = default;

			template <typename... t>
			FORCE_INLINE
			bucket(t&&... arg) noexcept
			{
				std::construct_at(reinterpret_cast<t_data*>(storage), std::forward<t>(arg)...);
			}

			FORCE_INLINE
			bucket(const t_data& data) noexcept
			{
				std::construct_at(reinterpret_cast<t_data*>(storage), data);
			}

			FORCE_INLINE t_data&
			data() noexcept
			{
				return *std::launder(reinterpret_cast<t_data*>(storage));
			}

			FORCE_INLINE const t_data&
			data() const noexcept
			{
				return *std::launder(reinterpret_cast<const t_data*>(storage));
			}

			FORCE_INLINE std::size_t&
			next_hole_idx() noexcept
			{
				return *std::launder(reinterpret_cast<std::size_t*>(storage));
			}
		};

		static constexpr bool is_static = static_count != std::dynamic_extent;
		using t_bucket_container		= std::conditional_t<is_static, std::array<bucket, static_count>, data_structure::vector<bucket>>;

		std::size_t hole_idx = 0;

		std::size_t hole_count = 0;

		t_bucket_container container = {};

		constexpr sparse_vector() noexcept requires(is_static)
		{
			hole_count = static_count;

			for (auto&& [idx, elem] : container | std::views::enumerate)
			{
				elem.next_hole_idx() = idx + 1;
			}
		}

		constexpr sparse_vector() noexcept requires(is_static is_false)
		= default;

		FORCE_INLINE constexpr std::size_t
		capacity() const noexcept
		{
			return container.size();
		}

		FORCE_INLINE
		std::size_t
		size() const noexcept
		{
			return container.size() - hole_count;
		}

		template <typename... t>
		std::size_t
		emplace_back(t&&... arg)
		{
			auto res = 0uz;

			if constexpr (is_static)
			{
				AGE_ASSERT(hole_count > 0);

				res				   = hole_idx;
				auto hole_idx_temp = container[hole_idx].next_hole_idx();
				std::construct_at(reinterpret_cast<t_data*>(&container[hole_idx].storage), std::forward<t>(arg)...);
				hole_idx = hole_idx_temp;
				--hole_count;

				return res;
			}
			else
			{
				if (hole_count == 0)
				{
					res = container.size();
					container.emplace_back(std::forward<t>(arg)...);
				}
				else
				{
					res				   = hole_idx;
					auto hole_idx_temp = container[hole_idx].next_hole_idx();
					std::construct_at(reinterpret_cast<t_data*>(&container[hole_idx].storage), std::forward<t>(arg)...);
					hole_idx = hole_idx_temp;
					--hole_count;
				}

				return res;
			}
		}

		void
		remove(std::size_t idx)
		{
			if constexpr (not std::is_trivially_destructible_v<t_data>)
			{
				std::destroy_at(&container[idx].data());
			}

			container[idx].next_hole_idx() = hole_idx;
			hole_idx					   = idx;
			++hole_count;
		}

		FORCE_INLINE t_data&
		operator[](std::size_t idx) noexcept
		{
			return container[idx].data();
		}

		FORCE_INLINE const t_data&
		operator[](std::size_t idx) const noexcept
		{
			return container[idx].data();
		}

		void
		debug_validate()
		{
			if constexpr (age::config::debug_mode)
			{
				AGE_ASSERT(hole_count <= container.size());
				AGE_ASSERT(hole_count + size() == container.size());

				{
					auto visited = std::vector<uint8>(container.size(), 0);

					for (auto curr = hole_idx;
						 auto _ : std::views::iota(0ul, hole_count))
					{
						AGE_ASSERT(curr < container.size());

						AGE_ASSERT(visited[curr] == 0);

						visited[curr] = 1;

						curr = container[curr].next_hole_idx();
					}
				}
			}
		}
	};
}	 // namespace age::data_structure

namespace age::data_structure
{
	template <typename t_data>
	struct stable_dense_vector final
	{
		static_assert(std::popcount(alignof(t_data)) == 1, "alignment of t_data must be power of 2");
		// todo, not supported yet
		// static_assert(std::is_implicit_lifetime_v<t_data>)
		static_assert(std::is_trivially_copyable_v<t_data>);
		static_assert(std::is_trivially_destructible_v<t_data>);

		AGE_DISABLE_COPY_MOVE(stable_dense_vector);

	  private:
		using t_idx = uint32;

		static constexpr std::size_t alignment = age::util::max_alignof<t_idx, t_data>();

		t_idx capacity;
		t_idx size;
		t_idx free_idx_count;

		std::byte* p_storage;


	  public:
		explicit stable_dense_vector(t_idx cap = 2)
			: capacity{ cap },
			  size{ 0 },
			  free_idx_count{ 0 }
		{
			AGE_ASSERT(capacity % 2 == 0);
			AGE_ASSERT(cap > 0);
			const auto bytes = (sizeof(t_idx) * 2 + sizeof(t_data)) * capacity;

			p_storage = static_cast<std::byte*>(::operator new(bytes, std::align_val_t{ alignment }));

			for (t_idx idx : std::views::iota(0) | std::views::take(cap))
			{
				this->idx_to_pos_arr()[idx] = idx;
				this->pos_to_idx_arr()[idx] = idx;
			}
		}

		~stable_dense_vector() noexcept
		{
			::operator delete(p_storage, std::align_val_t{ alignment });
		}

		FORCE_INLINE decltype(auto)
		is_empty() const noexcept
		{
			return size == 0;
		}

		FORCE_INLINE decltype(auto)
		count() const noexcept
		{
			return size;
		}

		t_idx
		emplace_back(auto&&... arg) noexcept
		{
			if (size == capacity) [[unlikely]]
			{
				resize(capacity << 1);
			}

			const auto pos = size;
			auto	   idx = size;

			if (free_idx_count > 0)
			{
				idx = this->pos_to_idx_arr()[pos];
				--free_idx_count;
			}

			{
				::new (static_cast<void*>(this->data_ptr(pos))) t_data(FWD(arg)...);
				++size;
			}

			return idx;
		}

		void
		remove(t_idx remove_idx) noexcept
		{
			AGE_ASSERT(size > 0);
			AGE_ASSERT(remove_idx < capacity);
			AGE_ASSERT(this->idx_to_pos_arr()[remove_idx] < size);

			const auto back_pos	  = size - 1;
			const auto back_idx	  = this->pos_to_idx_arr()[back_pos];
			const auto remove_pos = this->idx_to_pos_arr()[remove_idx];

			if (size - 1 != remove_pos) [[likely]]
			{
				std::memcpy(this->data_ptr(remove_pos), this->data_ptr(back_pos), sizeof(t_data));

				std::swap(this->pos_to_idx_arr()[back_pos], this->pos_to_idx_arr()[remove_pos]);
				std::swap(this->idx_to_pos_arr()[back_idx], this->idx_to_pos_arr()[remove_idx]);
			}

			if constexpr (age::config::debug_mode)
			{
				auto ptr = this->data_ptr(back_pos);
				std::memset(this->data_ptr(back_pos), static_cast<uint8>(0xcc), sizeof(t_data));
			}

			--size;
			++free_idx_count;
		}

		template <typename t_handle>
		requires requires(t_handle t) { t.id; }
		void
		remove(t_handle handle) noexcept
		{
			return remove(handle.id);
		}

		void
		remove(const t_data* p_data) noexcept
		{
			auto idx = p_data - data_ptr(0);

			AGE_ASSERT(p_data != nullptr);
			AGE_ASSERT(idx >= 0);
			AGE_ASSERT(idx < size);

			return remove(static_cast<t_idx>(idx));
		}

		FORCE_INLINE decltype(auto)
		begin(this auto& self) noexcept
		{
			return self.data_ptr(0);
		}

		FORCE_INLINE decltype(auto)
		end(this auto& self) noexcept
		{
			return self.data_ptr(self.size);
		}

		FORCE_INLINE const t_data*
		cbegin(this const auto& self) noexcept
		{
			return self.data_ptr(0);
		}

		FORCE_INLINE const t_data*
		cend(this const auto& self) noexcept
		{
			return self.data_ptr(self.size);
		}

		FORCE_INLINE decltype(auto)
		rbegin(this auto& self) noexcept
		{
			return std::reverse_iterator{ self.end() };
		}

		FORCE_INLINE decltype(auto)
		rend(this auto& self) noexcept
		{
			return std::reverse_iterator{ self.begin() };
		}

		FORCE_INLINE decltype(auto)
		crbegin(this const auto& self) noexcept
		{
			return std::reverse_iterator{ self.cend() };
		}

		FORCE_INLINE decltype(auto)
		crend(this const auto& self) noexcept
		{
			return std::reverse_iterator{ self.cbegin() };
		}

		FORCE_INLINE decltype(auto)
		operator[](this auto& self, t_idx idx) noexcept
		{
			AGE_ASSERT(idx < self.capacity);
			AGE_ASSERT(self.idx_to_pos_arr()[idx] < self.size);

			return self.data_arr()[self.idx_to_pos_arr()[idx]];
		}

		template <typename t_handle>
		requires requires(t_handle t) { t.id; }
		FORCE_INLINE decltype(auto)
		operator[](this auto& self, t_handle handle) noexcept
		{
			return self[handle.id];
		}

		FORCE_INLINE decltype(auto)
		nth_data(this auto& self, t_idx pos) noexcept
		{
			AGE_ASSERT(pos < self.size);

			return self.data_arr()[pos];
		}

		FORCE_INLINE decltype(auto)
		nth_id(this auto& self, t_idx pos) noexcept
		{
			AGE_ASSERT(pos < self.size);

			return self.pos_to_idx_arr()[pos];
		}

		void
		clear()
		{
			this->size			 = 0;
			this->free_idx_count = 0;
		}

		void
		debug_validate()
		{
			AGE_ASSERT(size + free_idx_count <= capacity);

			for (t_idx pos : std::views::iota(0) | std::views::take(size + free_idx_count))
			{
				AGE_ASSERT(this->pos_to_idx_arr()[pos] < capacity);
				AGE_ASSERT(this->idx_to_pos_arr()[this->pos_to_idx_arr()[pos]] == pos);
			}

			if (free_idx_count == 0)
			{
				return;
			}

			for (uint8* p_c : std::views::iota(reinterpret_cast<uint8*>(this->data_ptr(size)))
								  | std::views::take(free_idx_count * sizeof(t_data)))
			{
				AGE_ASSERT(*p_c == static_cast<uint8>(0xcc));
			}
		}

	  private:
		void
		resize(t_idx new_cap) noexcept
		{
			AGE_ASSERT(free_idx_count == 0);
			AGE_ASSERT(capacity < new_cap);

			const auto old_bytes = (sizeof(t_idx) * 2 + sizeof(t_data)) * capacity;
			const auto new_bytes = (sizeof(t_idx) * 2 + sizeof(t_data)) * new_cap;

			auto* const p_new_storage = static_cast<std::byte*>(::operator new(new_bytes, std::align_val_t{ alignment }));

			if constexpr (alignof(t_idx) > alignof(t_data))
			{
				// idx_to_pos_arr
				std::memcpy(
					p_new_storage,
					p_storage,
					sizeof(t_idx) * (size + free_idx_count));
				// pos_to_idx_arr
				std::memcpy(
					p_new_storage + sizeof(t_idx) * new_cap,
					p_storage + sizeof(t_idx) * capacity,
					sizeof(t_idx) * (size + free_idx_count));
				// data_arr
				std::memcpy(
					p_new_storage + sizeof(t_idx) * new_cap * 2,
					p_storage + sizeof(t_idx) * capacity * 2,
					sizeof(t_data) * (size + free_idx_count));
			}
			else
			{
				// data_arr
				std::memcpy(
					p_new_storage,
					p_storage,
					sizeof(t_data) * (size + free_idx_count));
				// idx_to_pos_arr
				std::memcpy(
					p_new_storage + sizeof(t_data) * new_cap,
					p_storage + sizeof(t_data) * capacity,
					sizeof(t_idx) * (size + free_idx_count));
				// pos_to_idx_arr
				std::memcpy(
					p_new_storage + (sizeof(t_data) + sizeof(t_idx)) * new_cap,
					p_storage + (sizeof(t_data) + sizeof(t_idx)) * capacity,
					sizeof(t_idx) * (size + free_idx_count));
			}


			::operator delete(p_storage, std::align_val_t{ alignment });

			p_storage = p_new_storage;
			capacity  = new_cap;

			for (t_idx idx : std::views::iota(size, new_cap))
			{
				this->idx_to_pos_arr()[idx] = idx;
				this->pos_to_idx_arr()[idx] = idx;
			}
		}

		FORCE_INLINE decltype(auto)
		idx_to_pos_arr(this auto& self) noexcept
		{
			using t_self	= decltype(self);
			using t_idx_ptr = std::conditional_t<
				std::is_const_v<std::remove_reference_t<t_self>>,
				const t_idx*,
				t_idx*>;

			// idx first
			if constexpr (alignof(t_idx) > alignof(t_data))
			{
				if constexpr (age::config::debug_mode)
				{
					return std::span{ reinterpret_cast<t_idx_ptr>(self.p_storage), self.capacity };
				}
				else
				{
					return reinterpret_cast<t_idx_ptr>(self.p_storage);
				}
			}
			else
			{
				if constexpr (age::config::debug_mode)
				{
					return std::span{ reinterpret_cast<t_idx_ptr>(self.p_storage + sizeof(t_data) * self.capacity), self.capacity };
				}
				else
				{
					return reinterpret_cast<t_idx_ptr>(self.p_storage + sizeof(t_data) * self.capacity);
				}
			}
		}

		FORCE_INLINE decltype(auto)
		pos_to_idx_arr(this auto& self) noexcept
		{
			using t_self	= decltype(self);
			using t_idx_ptr = std::conditional_t<
				std::is_const_v<std::remove_reference_t<t_self>>,
				const t_idx*,
				t_idx*>;

			// idx first
			if constexpr (alignof(t_idx) > alignof(t_data))
			{
				if constexpr (age::config::debug_mode)
				{
					return std::span{ reinterpret_cast<t_idx_ptr>(self.p_storage) + self.capacity, self.capacity };
				}
				else
				{
					return reinterpret_cast<t_idx_ptr>(self.p_storage) + self.capacity;
				}
			}
			else
			{
				if constexpr (age::config::debug_mode)
				{
					return std::span{ reinterpret_cast<t_idx_ptr>(self.p_storage + sizeof(t_data) * self.capacity) + self.capacity, self.capacity };
				}
				else
				{
					return reinterpret_cast<t_idx_ptr>(self.p_storage + sizeof(t_data) * self.capacity) + self.capacity;
				}
			}
		}

		FORCE_INLINE decltype(auto)
		data_arr(this auto& self) noexcept
		{
			using t_self	 = decltype(self);
			using t_data_ptr = std::conditional_t<
				std::is_const_v<std::remove_reference_t<t_self>>,
				const t_data*,
				t_data*>;

			// idx first
			if constexpr (alignof(t_idx) > alignof(t_data))
			{
				if constexpr (age::config::debug_mode)
				{
					return std::span{ reinterpret_cast<t_data_ptr>(self.p_storage + 2 * self.capacity * sizeof(t_idx)), self.capacity };
				}
				else
				{
					return reinterpret_cast<t_data_ptr>(self.p_storage + 2 * self.capacity * sizeof(t_idx));
				}
			}
			else
			{
				if constexpr (age::config::debug_mode)
				{
					return std::span{ reinterpret_cast<t_data_ptr>(self.p_storage), self.capacity };
				}
				else
				{
					return reinterpret_cast<t_data_ptr>(self.p_storage);
				}
			}
		}

		FORCE_INLINE decltype(auto)
		idx_to_pos_ptr(this auto& self, t_idx idx) noexcept
		{
			if constexpr (age::config::debug_mode)
			{
				return self.idx_to_pos_arr().data() + idx;
			}
			else
			{
				return self.idx_to_pos_arr() + idx;
			}
		}

		FORCE_INLINE decltype(auto)
		pos_to_idx_ptr(this auto& self, t_idx idx) noexcept
		{
			if constexpr (age::config::debug_mode)
			{
				return self.pos_to_idx_arr().data() + idx;
			}
			else
			{
				return self.pos_to_idx_arr() + idx;
			}
		}

		FORCE_INLINE decltype(auto)
		data_ptr(this auto& self, t_idx idx) noexcept
		{
			if constexpr (age::config::debug_mode)
			{
				return self.data_arr().data() + idx;
			}
			else
			{
				return self.data_arr() + idx;
			}
		}
	};
}	 // namespace age::data_structure
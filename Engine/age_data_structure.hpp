#pragma once
#ifdef USE_STL_VECTOR
namespace age::data_structure
{
	template <typename T>
	using vector = std::vector<T>;
}
#else
namespace age::data_structure
{
	template <typename T>
	class vector
	{
	  private:
		T*	   _head;
		size_t _size;
		size_t _capacity;

	  private:
		void
		_resize()
		{
			if (_capacity == 0)
			{
				_capacity = 3;
				_head	  = (T*)malloc(sizeof(T) * 3);
				assert(_head != nullptr);
			}
			else
			{
				_capacity *= 2;
				_head	   = (T*)realloc(_head, sizeof(T) * _capacity);
				assert(_head != nullptr);
			}
		};


	  public:
		constexpr vector()
		{
			_head	  = nullptr;
			_size	  = 0;
			_capacity = 0;
		};

		~vector()
		{
			if constexpr (std::is_trivially_destructible_v<T> == false)
			{
				for (size_t i = 0; i < _size; ++i)
				{
					(_head + i)->~T();
				}

				free(_head);
			}
			else
			{
				free(_head);
			}
		}

		constexpr void
		push_back(T&& item)
		{
			if (_size == _capacity)
			{
				_resize();
			}

			_head[_size++] = item;
		};

		constexpr void
		push_back(T item)
		{
			if (_size == _capacity)
			{
				_resize();
			}

			_head[_size++] = std::move(item);
		};

		template <typename... V>
		T&
		emplace_back(V&&... args)
		{
			if (_size == _capacity)
			{
				_resize();
			}

			return *(new (_head + _size++) T{ std::forward<V>(args)... });
		}

		constexpr T&
		back() const
		{
			assert(empty() == false);
			return _head[_size - 1];
		};

		constexpr void
		resize(size_t new_capacity)
		{
			_capacity = new_capacity;
			_head	  = (T*)realloc(_head, sizeof(T) * _capacity);
			_size	  = min(_size, new_capacity);
		}

		constexpr bool
		empty() const
		{
			return _size == 0;
		};

		constexpr size_t
		size() const
		{
			return _size;
		};

		constexpr size_t
		capacity() const
		{
			return _capacity;
		};

		constexpr T&
		operator[](size_t index) const
		{
			assert(index < _size);
			return _head[index];
		};

		// vector<T>& operator=(const vector<T>& other)
		//{
		//	free(_head);
		//	_head	  = other._head;
		//	_capacity = other.capacity();
		//	_size	  = other.size();

		//	return *this;
		//}

		// constexpr void operator=(const vector<T>&& other) noexcept
		//{
		//	free(_head);
		//	_head	  = other._head;
		//	_capacity = other.capacity();
		//	_size	  = other.size();
		// }
	};
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
	class list
	{
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

			bucket() noexcept = default;

			template <typename... t>
			bucket(t&&... arg)
			{
				std::construct_at(reinterpret_cast<t_data*>(storage), std::forward<t>(arg)...);
			}

			bucket(const t_data& data)
			{
				std::construct_at(reinterpret_cast<t_data*>(storage), data);
			}

			t_data&
			data()
			{
				return *std::launder(reinterpret_cast<t_data*>(storage));
			}

			const t_data&
			data() const
			{
				return *std::launder(reinterpret_cast<const t_data*>(storage));
			}

			std::size_t&
			next_hole_idx()
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

		FORCE_INLINE
		t_data&
		operator[](std::size_t idx) noexcept
		{
			return container[idx].data();
		}

		FORCE_INLINE
		const t_data&
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
		explicit stable_dense_vector(t_idx cap)
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
				std::memset(this->data_ptr(back_pos), static_cast<uint8>(0xcc), sizeof(t_data));
			}

			--size;
			++free_idx_count;
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
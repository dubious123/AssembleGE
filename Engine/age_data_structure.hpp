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
#pragma once

namespace age::inline data_structure
{
	template <typename t, typename t_allocator = std::allocator<t>>
	struct stable_dense_vector final
	{
		using value_type = t;

		using allocator_type = typename std::allocator_traits<t_allocator>::template rebind_alloc<t>;

		using size_type		  = std::size_t;
		using difference_type = std::ptrdiff_t;
		using reference		  = value_type&;
		using pointer		  = typename std::allocator_traits<allocator_type>::pointer;
		using const_pointer	  = typename std::allocator_traits<allocator_type>::const_pointer;

		using iterator				 = value_type*;
		using const_iterator		 = const value_type*;
		using reverse_iterator		 = std::reverse_iterator<iterator>;
		using const_reverse_iterator = std::reverse_iterator<const_iterator>;

		static_assert(std::popcount(alignof(value_type)) == 1, "alignment of value_type must be power of 2");

		static_assert(std::is_same_v<typename std::allocator_traits<t_allocator>::value_type, t>);
		static_assert(std::is_empty_v<t_allocator>);
		static_assert(std::is_same_v<pointer, t*>, "fancy pointer not supported yet");
		static_assert(meta::cx_allocator<allocator_type>);

	  private:
		using t_idx = uint32;

		static constexpr std::size_t alignment = age::util::max_alignof<t_idx, value_type>();

		t_idx	   cap			  = {};
		t_idx	   count		  = {};
		t_idx	   free_idx_count = {};
		std::byte* p_storage	  = nullptr;

		no_unique_addr
		allocator_type alloc;

	  public:
		stable_dense_vector&
		operator=(const stable_dense_vector&) = delete;

		constexpr stable_dense_vector() noexcept
			: alloc{ allocator_type{} } { };

		stable_dense_vector(const stable_dense_vector& other)
			: cap{ other.cap },
			  count{ other.count },
			  free_idx_count{ other.free_idx_count },
			  p_storage{ _alloc_storage(other.cap) },
			  alloc{ other.alloc }
		{
			if constexpr (age::config::debug_mode)
			{
				std::memcpy(idx_to_pos_arr().data(), other.idx_to_pos_arr().data(), sizeof(t_idx) * cap);
				std::memcpy(pos_to_idx_arr().data(), other.pos_to_idx_arr().data(), sizeof(t_idx) * cap);
			}
			else
			{
				std::memcpy(idx_to_pos_arr(), other.idx_to_pos_arr(), sizeof(t_idx) * cap);
				std::memcpy(pos_to_idx_arr(), other.pos_to_idx_arr(), sizeof(t_idx) * cap);
			}


			_copy_construct_n(alloc, data_ptr(0), other.data_ptr(0), count);

			if constexpr (age::config::debug_mode)
			{
				std::memset(data_ptr(count), static_cast<uint8>(0xcc), sizeof(value_type) * free_idx_count);
			}
		}

		constexpr stable_dense_vector(stable_dense_vector&& other) noexcept
			: cap{ std::exchange(other.cap, 0) },
			  count{ std::exchange(other.count, 0) },
			  free_idx_count{ std::exchange(other.free_idx_count, 0) },
			  p_storage{ std::exchange(other.p_storage, nullptr) },
			  alloc{ other.alloc }
		{
		}

		constexpr stable_dense_vector&
		operator=(stable_dense_vector&& other) noexcept
		{
			if (this == &other) return *this;

			_destroy_all();
			_dealloc_storage(p_storage);

			cap			   = std::exchange(other.cap, 0);
			count		   = std::exchange(other.count, 0);
			free_idx_count = std::exchange(other.free_idx_count, 0);
			p_storage	   = std::exchange(other.p_storage, nullptr);

			return *this;
		}

		template <typename t_input_it>
		FORCE_INLINE
		stable_dense_vector(t_input_it first, t_input_it last) noexcept
			: alloc{ allocator_type{} }
		{
			if constexpr (std::random_access_iterator<t_input_it>)
			{
				count = static_cast<t_idx>(last - first);
			}
			else if constexpr (std::forward_iterator<t_input_it>)
			{
				count = static_cast<t_idx>(std::distance(first, last));
			}
			else
			{
				static_assert(false, "not supported yet");
			}

			cap			   = count;
			free_idx_count = 0;
			p_storage	   = _alloc_storage(cap);

			for (t_idx idx : std::views::iota(t_idx{ 0 }) | std::views::take(cap))
			{
				idx_to_pos_arr()[idx] = idx;
				pos_to_idx_arr()[idx] = idx;
			}

			if constexpr (std::contiguous_iterator<t_input_it> and std::is_trivially_copyable_v<value_type>)
			{
				std::memcpy(data_ptr(0), std::to_address(first), sizeof(value_type) * count);
			}
			else if constexpr (std::is_nothrow_constructible_v<value_type, decltype(*first)>)
			{
				for (t_idx i = 0; i < count; ++i)
				{
					std::allocator_traits<allocator_type>::construct(alloc, data_ptr(i), *first++);
				}
			}
			else
			{
				static_assert(false, "throwable element is not supported yet");
			}
		}

		stable_dense_vector(std::initializer_list<value_type> init) noexcept
			: stable_dense_vector(init.begin(), init.end())
		{
		}

		FORCE_INLINE constexpr ~stable_dense_vector() noexcept
		{
			_destroy_all();
			_dealloc_storage(p_storage);
		}

		FORCE_INLINE static stable_dense_vector
		gen_reserved(t_idx cap) noexcept
		{
			AGE_ASSERT(cap > 0);

			auto vec	  = stable_dense_vector{};
			vec.count	  = 0;
			vec.cap		  = cap;
			vec.p_storage = _alloc_storage(cap);

			for (t_idx idx : std::views::iota(t_idx{ 0 }) | std::views::take(cap))
			{
				vec.idx_to_pos_arr()[idx] = idx;
				vec.pos_to_idx_arr()[idx] = idx;
			}

			return vec;
		}

		FORCE_INLINE decltype(auto)
		is_empty() const noexcept
		{
			return count == 0;
		}

		FORCE_INLINE decltype(auto)
		size() const noexcept
		{
			return count;
		}

		t_idx
		emplace_back(auto&&... arg) noexcept
		{
			if (count == cap) [[unlikely]]
			{
				_grow((cap << 1) + 1);
			}

			const auto pos = count;
			auto	   idx = count;

			if (free_idx_count > 0)
			{
				idx = this->pos_to_idx_arr()[pos];
				--free_idx_count;
			}

			{
				std::allocator_traits<allocator_type>::construct(alloc, data_ptr(pos), FWD(arg)...);
				++count;
			}

			return idx;
		}

		void
		remove(t_idx remove_idx) noexcept
		{
			AGE_ASSERT(count > 0);
			AGE_ASSERT(remove_idx < cap);
			AGE_ASSERT(idx_to_pos_arr()[remove_idx] < count);

			const auto back_pos	  = count - 1;
			const auto back_idx	  = pos_to_idx_arr()[back_pos];
			const auto remove_pos = idx_to_pos_arr()[remove_idx];

			_destroy_at(data_ptr(remove_pos));

			if (back_pos != remove_pos) [[likely]]
			{
				if constexpr (std::is_trivially_copyable_v<value_type>)
				{
					std::memcpy(data_ptr(remove_pos), data_ptr(back_pos), sizeof(value_type));
				}
				else
				{
					std::allocator_traits<allocator_type>::construct(
						alloc, data_ptr(remove_pos), std::move(*data_ptr(back_pos)));

					_destroy_at(data_ptr(back_pos));
				}

				std::swap(pos_to_idx_arr()[back_pos], pos_to_idx_arr()[remove_pos]);
				std::swap(idx_to_pos_arr()[back_idx], idx_to_pos_arr()[remove_idx]);
			}

			if constexpr (age::config::debug_mode)
			{
				auto ptr = data_ptr(back_pos);
				std::memset(data_ptr(back_pos), static_cast<uint8>(0xcc), sizeof(value_type));
			}

			--count;
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
		remove(const value_type* p_data) noexcept
		{
			c_auto pos = p_data - data_ptr(0);

			AGE_ASSERT(p_data != nullptr);
			AGE_ASSERT(pos >= 0);
			AGE_ASSERT(pos < count);

			c_auto idx = pos_to_idx_arr()[pos];
			return remove(idx);
		}

		FORCE_INLINE decltype(auto)
		begin(this auto& self) noexcept
		{
			return self.data_ptr(0);
		}

		FORCE_INLINE decltype(auto)
		end(this auto& self) noexcept
		{
			return self.data_ptr(self.count);
		}

		FORCE_INLINE const value_type*
		cbegin(this const auto& self) noexcept
		{
			return self.data_ptr(0);
		}

		FORCE_INLINE const value_type*
		cend(this const auto& self) noexcept
		{
			return self.data_ptr(self.count);
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
			AGE_ASSERT(idx < self.cap);
			AGE_ASSERT(self.idx_to_pos_arr()[idx] < self.count);

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
			AGE_ASSERT(pos < self.count);

			return self.data_arr()[pos];
		}

		FORCE_INLINE decltype(auto)
		nth_id(this auto& self, t_idx pos) noexcept
		{
			AGE_ASSERT(pos < self.count);

			return self.pos_to_idx_arr()[pos];
		}

		FORCE_INLINE decltype(auto)
		get_pos(this auto& self, t_idx idx) noexcept
		{
			return self.idx_to_pos_arr()[idx];
		}

		void
		clear()
		{
			_destroy_all();

			this->count			 = 0;
			this->free_idx_count = 0;

			for (t_idx idx : std::views::iota(0) | std::views::take(cap))
			{
				this->idx_to_pos_arr()[idx] = idx;
				this->pos_to_idx_arr()[idx] = idx;
			}
		}

		FORCE_INLINE decltype(auto)
		data() noexcept
		{
			if constexpr (age::config::debug_mode)
			{
				return data_arr().data();
			}
			else
			{
				return data_arr();
			}
		}

		void
		debug_validate()
		{
			AGE_ASSERT(count + free_idx_count <= cap);

			for (t_idx pos : std::views::iota(0) | std::views::take(count + free_idx_count))
			{
				AGE_ASSERT(this->pos_to_idx_arr()[pos] < cap);
				AGE_ASSERT(this->idx_to_pos_arr()[this->pos_to_idx_arr()[pos]] == pos);
			}

			if (free_idx_count == 0)
			{
				return;
			}

			for (uint8* p_c : std::views::iota(reinterpret_cast<uint8*>(this->data_ptr(count)))
								  | std::views::take(free_idx_count * sizeof(value_type)))
			{
				AGE_ASSERT(*p_c == static_cast<uint8>(0xcc));
			}
		}

	  private:
		FORCE_INLINE decltype(auto)
		idx_to_pos_arr(this auto& self) noexcept
		{
			using t_self	= decltype(self);
			using t_idx_ptr = std::conditional_t<
				std::is_const_v<std::remove_reference_t<t_self>>,
				const t_idx*,
				t_idx*>;

			// idx first
			if constexpr (alignof(t_idx) > alignof(value_type))
			{
				if constexpr (age::config::debug_mode)
				{
					return std::span{ reinterpret_cast<t_idx_ptr>(self.p_storage), self.cap };
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
					return std::span{ reinterpret_cast<t_idx_ptr>(self.p_storage + sizeof(value_type) * self.cap), self.cap };
				}
				else
				{
					return reinterpret_cast<t_idx_ptr>(self.p_storage + sizeof(value_type) * self.cap);
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
			if constexpr (alignof(t_idx) > alignof(value_type))
			{
				if constexpr (age::config::debug_mode)
				{
					return std::span{ reinterpret_cast<t_idx_ptr>(self.p_storage) + self.cap, self.cap };
				}
				else
				{
					return reinterpret_cast<t_idx_ptr>(self.p_storage) + self.cap;
				}
			}
			else
			{
				if constexpr (age::config::debug_mode)
				{
					return std::span{ reinterpret_cast<t_idx_ptr>(self.p_storage + sizeof(value_type) * self.cap) + self.cap, self.cap };
				}
				else
				{
					return reinterpret_cast<t_idx_ptr>(self.p_storage + sizeof(value_type) * self.cap) + self.cap;
				}
			}
		}

		FORCE_INLINE decltype(auto)
		data_arr(this auto& self) noexcept
		{
			using t_self		 = decltype(self);
			using value_type_ptr = std::conditional_t<
				std::is_const_v<std::remove_reference_t<t_self>>,
				const value_type*,
				value_type*>;

			// idx first
			if constexpr (alignof(t_idx) > alignof(value_type))
			{
				if constexpr (age::config::debug_mode)
				{
					return std::span{ reinterpret_cast<value_type_ptr>(self.p_storage + 2 * self.cap * sizeof(t_idx)), self.cap };
				}
				else
				{
					return reinterpret_cast<value_type_ptr>(self.p_storage + 2 * self.cap * sizeof(t_idx));
				}
			}
			else
			{
				if constexpr (age::config::debug_mode)
				{
					return std::span{ reinterpret_cast<value_type_ptr>(self.p_storage), self.cap };
				}
				else
				{
					return reinterpret_cast<value_type_ptr>(self.p_storage);
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

		FORCE_INLINE static std::byte*
		_alloc_storage(t_idx cap) noexcept
		{
			const auto bytes = (sizeof(t_idx) * 2 + sizeof(value_type)) * cap;
			return static_cast<std::byte*>(::operator new(bytes, std::align_val_t{ alignment }));
		}

		FORCE_INLINE static void
		_dealloc_storage(std::byte* p) noexcept
		{
			::operator delete(p, std::align_val_t{ alignment });
		}

		FORCE_INLINE static void
		_copy_construct_n(allocator_type alloc, value_type* p_dst, const value_type* p_src, t_idx n) noexcept
		{
			if constexpr (std::is_trivially_copyable_v<value_type>)
			{
				std::memcpy(p_dst, p_src, sizeof(value_type) * n);
			}
			else if constexpr (std::is_nothrow_copy_constructible_v<value_type>)
			{
				for (t_idx i = 0; i < n; ++i)
				{
					std::allocator_traits<allocator_type>::construct(alloc, p_dst + i, p_src[i]);
				}
			}
			else
			{
				static_assert(false, "throwable copy not supported");
			}
		}

		FORCE_INLINE void
		_destroy_at(value_type* p) noexcept
		{
			if constexpr (std::is_trivially_destructible_v<value_type>)
			{
			}
			else if constexpr (std::is_nothrow_destructible_v<value_type>)
			{
				std::allocator_traits<allocator_type>::destroy(alloc, p);
			}
			else
			{
				static_assert(false, "throwable destructor not supported");
			}
		}

		void
		_destroy_all() noexcept
		{
			if constexpr (std::is_trivially_destructible_v<value_type>)
			{
			}
			else if constexpr (std::is_nothrow_destructible_v<value_type>)
			{
				for (t_idx i = 0; i < count; ++i)
				{
					std::allocator_traits<allocator_type>::destroy(alloc, data_ptr(i));
				}
			}
			else
			{
				static_assert(false, "throwable destructor not supported");
			}
		}

		static void
		_relocate_n(allocator_type& alloc, value_type* p_dst, value_type* p_src, t_idx n) noexcept
		{
			if constexpr (std::is_trivially_copyable_v<value_type>)
			{
				std::memcpy(p_dst, p_src, sizeof(value_type) * n);
			}
			else if constexpr (std::is_nothrow_move_constructible_v<value_type>)
			{
				for (t_idx i = 0; i < n; ++i)
				{
					std::allocator_traits<allocator_type>::construct(alloc, p_dst + i, std::move(p_src[i]));
					std::allocator_traits<allocator_type>::destroy(alloc, p_src + i);
				}
			}
			else
			{
				static_assert(false, "not supported");
			}
		}

		void
		_grow(t_idx new_cap) noexcept
		{
			AGE_ASSERT(free_idx_count == 0);
			AGE_ASSERT(cap < new_cap);

			auto* const p_new_storage = _alloc_storage(new_cap);

			if constexpr (alignof(t_idx) > alignof(value_type))
			{
				// layout: [idx_to_pos | pos_to_idx | data]
				std::memcpy(
					p_new_storage,
					p_storage,
					sizeof(t_idx) * count);

				std::memcpy(
					p_new_storage + sizeof(t_idx) * new_cap,
					p_storage + sizeof(t_idx) * cap,
					sizeof(t_idx) * count);

				auto* p_old_data = reinterpret_cast<value_type*>(p_storage + sizeof(t_idx) * cap * 2);
				auto* p_new_data = reinterpret_cast<value_type*>(p_new_storage + sizeof(t_idx) * new_cap * 2);
				_relocate_n(alloc, p_new_data, p_old_data, count);
			}
			else
			{
				// layout: [data | idx_to_pos | pos_to_idx]
				auto* p_old_data = reinterpret_cast<value_type*>(p_storage);
				auto* p_new_data = reinterpret_cast<value_type*>(p_new_storage);
				_relocate_n(alloc, p_new_data, p_old_data, count);

				std::memcpy(
					p_new_storage + sizeof(value_type) * new_cap,
					p_storage + sizeof(value_type) * cap,
					sizeof(t_idx) * count);

				std::memcpy(
					p_new_storage + (sizeof(value_type) + sizeof(t_idx)) * new_cap,
					p_storage + (sizeof(value_type) + sizeof(t_idx)) * cap,
					sizeof(t_idx) * count);
			}

			_dealloc_storage(p_storage);

			p_storage = p_new_storage;
			cap		  = new_cap;

			for (t_idx idx : std::views::iota(count, new_cap))
			{
				this->idx_to_pos_arr()[idx] = idx;
				this->pos_to_idx_arr()[idx] = idx;
			}
		}
	};
}	 // namespace age::inline data_structure
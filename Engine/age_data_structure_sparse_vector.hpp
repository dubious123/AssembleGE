#pragma once

namespace age::inline data_structure
{
	template <typename t, typename t_allocator = std::allocator<t>>
	struct sparse_vector
	{
		using value_type	 = t;
		using allocator_type = typename std::allocator_traits<t_allocator>::template rebind_alloc<t>;

		using size_type		  = std::size_t;
		using difference_type = std::ptrdiff_t;
		using reference		  = value_type&;
		using const_reference = const value_type&;
		using pointer		  = typename std::allocator_traits<allocator_type>::pointer;
		using const_pointer	  = typename std::allocator_traits<allocator_type>::const_pointer;

		struct bucket
		{
			using storage_t = std::conditional_t<(sizeof(value_type) >= sizeof(size_type)), value_type, size_type>;

			alignas(util::max_alignof<value_type, size_type>())
				std::byte storage[sizeof(storage_t)];

			FORCE_INLINE
			bucket() noexcept = default;

			FORCE_INLINE reference
			data() noexcept
			{
				return *std::launder(reinterpret_cast<value_type*>(storage));
			}

			FORCE_INLINE const_reference
			data() const noexcept
			{
				return *std::launder(reinterpret_cast<const value_type*>(storage));
			}

			FORCE_INLINE size_type&
			next_hole_idx() noexcept
			{
				return *std::launder(reinterpret_cast<size_type*>(storage));
			}

			FORCE_INLINE const size_type&
			next_hole_idx() const noexcept
			{
				return *std::launder(reinterpret_cast<const size_type*>(storage));
			}
		};

		template <typename t_self>
		struct iterator_t
		{
			template <typename>
			friend struct iterator_t;

		  public:
			using iterator_category = std::forward_iterator_tag;
			using value_type		= typename sparse_vector::value_type;
			using difference_type	= typename sparse_vector::difference_type;
			using reference			= std::conditional_t<std::is_const_v<t_self>,
														 typename sparse_vector::const_reference,
														 typename sparse_vector::reference>;
			using pointer			= std::conditional_t<std::is_const_v<t_self>,
														 typename sparse_vector::const_pointer,
														 typename sparse_vector::pointer>;

		  private:
			t_self*	  p_container = nullptr;
			size_type current_idx = 0;

		  public:
			constexpr iterator_t() noexcept = default;

			FORCE_INLINE constexpr iterator_t(t_self* c, size_type idx) noexcept
				: p_container{ c }, current_idx{ idx } { }

			template <typename t_other>
			requires(std::is_const_v<t_self> and std::is_same_v<std::remove_const_t<t_self>, t_other>)
			FORCE_INLINE constexpr iterator_t(const iterator_t<t_other>& other) noexcept
				: p_container{ other.p_container }, current_idx{ other.current_idx }
			{
			}

			FORCE_INLINE constexpr reference
			operator*() const noexcept
			{
				return p_container->p_buckets[current_idx].data();
			}

			FORCE_INLINE constexpr pointer
			operator->() const noexcept
			{
				return &p_container->p_buckets[current_idx].data();
			}

			template <typename t_ret = size_type>
			FORCE_INLINE constexpr t_ret
			idx() const noexcept
			{ return static_cast<t_ret>(current_idx); }

			FORCE_INLINE constexpr iterator_t&
			operator++() noexcept
			{
				current_idx = p_container->next_live_idx_from(current_idx + 1);
				return *this;
			}

			FORCE_INLINE constexpr iterator_t
			operator++(int32) noexcept
			{
				auto tmp = *this;
				++(*this);
				return tmp;
			}

			friend constexpr bool
			operator==(const iterator_t& a, const iterator_t& b) noexcept
			{
				return a.current_idx == b.current_idx;
			}
		};

		template <typename t_self>
		struct reverse_iterator_t
		{
			template <typename>
			friend struct reverse_iterator_t;

		  public:
			using iterator_category = std::forward_iterator_tag;
			using value_type		= typename sparse_vector::value_type;
			using difference_type	= typename sparse_vector::difference_type;
			using reference			= std::conditional_t<std::is_const_v<t_self>,
														 typename sparse_vector::const_reference,
														 typename sparse_vector::reference>;
			using pointer			= std::conditional_t<std::is_const_v<t_self>,
														 typename sparse_vector::const_pointer,
														 typename sparse_vector::pointer>;

		  private:
			t_self*	  p_container = nullptr;
			size_type current_idx = 0;

		  public:
			constexpr reverse_iterator_t() noexcept = default;

			FORCE_INLINE constexpr reverse_iterator_t(t_self* c, size_type idx) noexcept
				: p_container{ c }, current_idx{ idx } { }

			template <typename t_other>
			requires(std::is_const_v<t_self> and std::is_same_v<std::remove_const_t<t_self>, t_other>)
			FORCE_INLINE constexpr reverse_iterator_t(const reverse_iterator_t<t_other>& other) noexcept
				: p_container{ other.p_container }, current_idx{ other.current_idx }
			{
			}

			FORCE_INLINE constexpr reference
			operator*() const noexcept
			{
				return p_container->p_buckets[current_idx].data();
			}

			FORCE_INLINE constexpr pointer
			operator->() const noexcept
			{
				return &p_container->p_buckets[current_idx].data();
			}

			template <typename t_ret = size_type>
			FORCE_INLINE constexpr t_ret
			idx() const noexcept
			{ return static_cast<t_ret>(current_idx); }

			FORCE_INLINE constexpr reverse_iterator_t&
			operator++() noexcept
			{
				current_idx = p_container->prev_live_idx_from(current_idx);
				return *this;
			}

			FORCE_INLINE constexpr reverse_iterator_t
			operator++(int) noexcept
			{
				auto tmp = *this;
				++(*this);
				return tmp;
			}

			friend constexpr bool
			operator==(const reverse_iterator_t& a, const reverse_iterator_t& b) noexcept
			{
				return a.current_idx == b.current_idx;
			}
		};

		using iterator				 = iterator_t<sparse_vector>;
		using const_iterator		 = iterator_t<const sparse_vector>;
		using reverse_iterator		 = reverse_iterator_t<sparse_vector>;
		using const_reverse_iterator = reverse_iterator_t<const sparse_vector>;

		using bucket_allocator_type = typename std::allocator_traits<t_allocator>::template rebind_alloc<bucket>;
		using bitmap_allocator_type = typename std::allocator_traits<t_allocator>::template rebind_alloc<uint64>;

		static_assert(std::is_implicit_lifetime_v<bucket>);
		static_assert(std::is_trivially_destructible_v<bucket>);
		static_assert(std::is_trivially_default_constructible_v<bucket>);

		static_assert(std::is_same_v<typename std::allocator_traits<t_allocator>::value_type, t>);
		static_assert(std::is_empty_v<t_allocator>);
		static_assert(std::is_nothrow_destructible_v<value_type>);
		static_assert(std::is_trivially_copyable_v<value_type>
					  or std::is_nothrow_move_constructible_v<value_type>);
		static_assert(std::is_same_v<pointer, t*>, "fancy pointer not supported yet");
		static_assert(meta::cx_allocator<allocator_type>);

	  private:
		size_type count = {};
		size_type cap	= {};
		// size_type hole_count = {}; hole_count == cap - count;
		size_type hole_idx = {};


		bucket* p_buckets = nullptr;
		uint64* p_bitmap  = nullptr;

		no_unique_addr
		allocator_type alloc;

	  public:
		constexpr sparse_vector() noexcept = default;

		constexpr sparse_vector(const sparse_vector& other) noexcept
			requires(std::is_trivially_copyable_v<value_type>
					 or std::is_nothrow_copy_constructible_v<value_type>)
			: count{ other.count },
			  cap{ other.cap },
			  hole_idx{ other.hole_idx },
			  alloc{ std::allocator_traits<allocator_type>::select_on_container_copy_construction(other.alloc) }
		{
			if (other.cap == 0)
			{
				return;
			}

			p_buckets = alloc_bucket_n(alloc, other.cap);
			p_bitmap  = alloc_bitmap_n(alloc, (other.cap + 63) >> 6);	 // ceil(cap / 64)

			copy_to(alloc, cap, other.p_buckets, other.p_bitmap, p_buckets, p_bitmap);
		}

		constexpr sparse_vector(sparse_vector&& other) noexcept
			: count{ std::exchange(other.count, 0) },
			  cap{ std::exchange(other.cap, 0) },
			  hole_idx{ std::exchange(other.hole_idx, 0) },
			  p_buckets{ std::exchange(other.p_buckets, nullptr) },
			  p_bitmap{ std::exchange(other.p_bitmap, nullptr) },
			  alloc{ std::move(other.alloc) }
		{
		}

		constexpr sparse_vector&
		operator=(const sparse_vector& other) noexcept
			requires(std::is_trivially_copyable_v<value_type>
					 or std::is_nothrow_copy_constructible_v<value_type>)
		{
			if (this == &other) { return *this; }

			destroy_all(alloc, cap, p_buckets, p_bitmap);
			c_auto old_word_count = (cap + 63) >> 6;	// ceil(cap / 64);
			dealloc_bucket_n(alloc, p_buckets, cap);
			dealloc_bitmap_n(alloc, p_bitmap, old_word_count);

			if constexpr (std::allocator_traits<allocator_type>::propagate_on_container_copy_assignment::value)
			{
				alloc = other.alloc;
			}

			count	 = other.count;
			cap		 = other.cap;
			hole_idx = other.hole_idx;

			if (cap == 0)
			{
				p_buckets = nullptr;
				p_bitmap  = nullptr;
				return *this;
			}

			p_buckets = alloc_bucket_n(alloc, cap);
			p_bitmap  = alloc_bitmap_n(alloc, (cap + 63) >> 6);	   // ceil(cap / 64);

			copy_to(alloc, cap, other.p_buckets, other.p_bitmap, p_buckets, p_bitmap);

			return *this;
		}

		constexpr sparse_vector&
		operator=(sparse_vector&& other) noexcept
		{
			if (this == &other) { return *this; }

			destroy_all(alloc, cap, p_buckets, p_bitmap);
			dealloc_bucket_n(alloc, p_buckets, cap);
			dealloc_bitmap_n(alloc, p_bitmap, (cap + 63) >> 6);	   // ceil(cap / 64);

			if constexpr (std::allocator_traits<allocator_type>::propagate_on_container_move_assignment::value)
			{
				alloc = std::move(other.alloc);
			}

			count	  = std::exchange(other.count, 0);
			cap		  = std::exchange(other.cap, 0);
			hole_idx  = std::exchange(other.hole_idx, 0);
			p_buckets = std::exchange(other.p_buckets, nullptr);
			p_bitmap  = std::exchange(other.p_bitmap, nullptr);


			return *this;
		}

		constexpr ~sparse_vector() noexcept
		{
			destroy_all(alloc, cap, p_buckets, p_bitmap);
			dealloc_bucket_n(alloc, p_buckets, cap);
			dealloc_bitmap_n(alloc, p_bitmap, (cap + 63) >> 6);	   // ceil(cap / 64);
		}

	  private:
		class gen_reserve_key { };

		constexpr sparse_vector(gen_reserve_key, size_type initial_cap) noexcept
			: cap{ initial_cap }
		{
			if (cap == size_type{ 0 })
			{
				return;
			}

			p_buckets = alloc_bucket_n(alloc, cap);
			p_bitmap  = alloc_bitmap_n(alloc, (cap + 63) >> 6);	   // ceil(cap / 64)

			for (auto i = size_type{ 0 }; i < cap; ++i)
			{
				p_buckets[i].next_hole_idx() = i + 1;
			}
		}

	  public:
		FORCE_INLINE static constexpr sparse_vector
		gen_reserve(size_type initial_cap) noexcept
		{
			return sparse_vector(gen_reserve_key{}, initial_cap);
		}

		FORCE_INLINE constexpr void
		reserve(size_type new_cap) noexcept
		{
			if (new_cap <= cap) { return; }

			c_auto new_word_count = (new_cap + 63) >> 6;

			auto* p_new_buckets = alloc_bucket_n(alloc, new_cap);
			auto* p_new_bitmap	= alloc_bitmap_n(alloc, new_word_count);

			relocate_to(alloc, cap, p_buckets, p_bitmap, p_new_buckets, p_new_bitmap);

			dealloc_bucket_n(alloc, p_buckets, cap);
			dealloc_bitmap_n(alloc, p_bitmap, (cap + 63) >> 6);

			for (auto i = cap; i + 1 < new_cap; ++i)
			{
				p_new_buckets[i].next_hole_idx() = i + 1;
			}
			p_new_buckets[new_cap - 1].next_hole_idx() = hole_idx;
			hole_idx								   = cap;

			p_buckets = p_new_buckets;
			p_bitmap  = p_new_bitmap;
			cap		  = new_cap;
		}

		FORCE_INLINE constexpr reference
		operator[](size_type idx) noexcept
		{
			AGE_ASSERT(idx < cap);
			AGE_ASSERT(((p_bitmap[idx >> 6] >> (idx & 63)) & 1) != 0);
			return p_buckets[idx].data();
		}

		FORCE_INLINE constexpr const_reference
		operator[](size_type idx) const noexcept
		{
			AGE_ASSERT(idx < cap);
			AGE_ASSERT(((p_bitmap[idx >> 6] >> (idx & 63)) & 1) != 0);
			return p_buckets[idx].data();
		}

		template <typename t_handle>
		requires requires(t_handle h) { h.id; }
		FORCE_INLINE constexpr reference
		operator[](t_handle h) noexcept
		{
			return (*this)[h.id];
		}

		template <typename t_handle>
		requires requires(t_handle h) { h.id; }
		FORCE_INLINE constexpr const_reference
		operator[](t_handle h) const noexcept
		{
			return (*this)[h.id];
		}

		FORCE_INLINE constexpr reference
		at(size_type idx) noexcept
		{
			AGE_ASSERT(idx < cap);
			AGE_ASSERT(((p_bitmap[idx >> 6] >> (idx & 63)) & 1) != 0);
			return p_buckets[idx].data();
		}

		FORCE_INLINE constexpr const_reference
		at(size_type idx) const noexcept
		{
			AGE_ASSERT(idx < cap);
			AGE_ASSERT(((p_bitmap[idx >> 6] >> (idx & 63)) & 1) != 0);
			return p_buckets[idx].data();
		}

		template <typename t_ret = size_type>
		FORCE_INLINE constexpr t_ret
		size() const noexcept
		{
			return static_cast<t_ret>(count);
		}

		template <typename t_ret = size_type>
		FORCE_INLINE constexpr t_ret
		capacity() const noexcept
		{
			return static_cast<t_ret>(cap);
		}

		FORCE_INLINE constexpr bool
		empty() const noexcept
		{
			return count == 0;
		}

		FORCE_INLINE constexpr bool
		is_empty() const noexcept
		{
			return count == 0;
		}

		FORCE_INLINE constexpr bool
		is_live(size_type idx) const noexcept
		{
			AGE_ASSERT(idx < cap);
			return ((p_bitmap[idx >> 6] >> (idx & 63)) & 1) != 0;
		}

		FORCE_INLINE constexpr allocator_type
		get_allocator() const noexcept
		{
			return alloc;
		}

		FORCE_INLINE constexpr size_type
		hole_count() const noexcept
		{
			return cap - count;
		}

	  private:
		FORCE_INLINE constexpr size_type
		front_idx() const noexcept
		{
			c_auto word_count = (cap + 63) >> 6;
			for (auto word_idx = size_type{ 0 }; word_idx < word_count; ++word_idx)
			{
				c_auto w = p_bitmap[word_idx];
				if (w != 0)
				{
					return word_idx * 64 + static_cast<size_type>(std::countr_zero(w));
				}
			}

			return cap;
		}

		FORCE_INLINE constexpr size_type
		back_idx() const noexcept
		{
			for (auto word_idx = (cap + 63) >> 6; word_idx-- > 0;)
			{
				c_auto w = p_bitmap[word_idx];
				if (w != 0)
				{
					return word_idx * 64 + 63 - static_cast<size_type>(std::countl_zero(w));
				}
			}

			return cap;
		}

		FORCE_INLINE constexpr size_type
		next_live_idx_from(size_type start_idx) const noexcept
		{
			if (start_idx >= cap) { return cap; }

			c_auto word_count = (cap + 63) >> 6;
			auto   word_idx	  = start_idx >> 6;
			c_auto bit		  = start_idx & 63;

			c_auto first_w = p_bitmap[word_idx] >> bit;
			if (first_w != 0)
			{
				return start_idx + static_cast<size_type>(std::countr_zero(first_w));
			}

			for (++word_idx; word_idx < word_count; ++word_idx)
			{
				c_auto w = p_bitmap[word_idx];
				if (w != 0)
				{
					return word_idx * 64 + static_cast<size_type>(std::countr_zero(w));
				}
			}

			return cap;
		}

		FORCE_INLINE constexpr size_type
		prev_live_idx_from(size_type start_idx) const noexcept
		{
			if (start_idx == cap) { return cap; }	 // sentinel

			auto   word_idx = start_idx >> 6;
			c_auto bit		= start_idx & 63;

			c_auto mask	   = (uint64{ 1 } << bit) - 1;
			c_auto first_w = p_bitmap[word_idx] & mask;
			if (first_w != 0)
			{
				return word_idx * 64 + 63 - static_cast<size_type>(std::countl_zero(first_w));
			}

			while (word_idx-- > 0)
			{
				c_auto w = p_bitmap[word_idx];
				if (w != 0)
				{
					return word_idx * 64 + 63 - static_cast<size_type>(std::countl_zero(w));
				}
			}

			return cap;
		}

	  public:
		FORCE_INLINE constexpr reference
		front() noexcept
		{
			AGE_ASSERT(count > 0);
			return p_buckets[front_idx()].data();
		}

		FORCE_INLINE constexpr const_reference
		front() const noexcept
		{
			AGE_ASSERT(count > 0);
			return p_buckets[front_idx()].data();
		}

		FORCE_INLINE constexpr reference
		back() noexcept
		{
			AGE_ASSERT(count > 0);
			return p_buckets[back_idx()].data();
		}

		FORCE_INLINE constexpr const_reference
		back() const noexcept
		{
			AGE_ASSERT(count > 0);
			return p_buckets[back_idx()].data();
		}

		FORCE_INLINE constexpr size_type
		emplace_back(auto&&... arg) noexcept
		{
			if (count == cap)
			{
				reserve(std::max<size_type>(4, cap * 2));
			}

			c_auto idx = hole_idx;
			hole_idx   = p_buckets[idx].next_hole_idx();

			construct_one(alloc, &p_buckets[idx].data(), FWD(arg)...);

			p_bitmap[idx >> 6] |= (uint64{ 1 } << (idx & 63));
			++count;

			return idx;
		}

		FORCE_INLINE constexpr void
		remove(size_type idx) noexcept
		{
			AGE_ASSERT(idx < cap);
			AGE_ASSERT(((p_bitmap[idx >> 6] >> (idx & 63)) & 1) != 0);
			AGE_ASSERT(count > 0);

			destroy_one(alloc, &p_buckets[idx].data());

			p_bitmap[idx >> 6] &= ~(uint64{ 1 } << (idx & 63));

			p_buckets[idx].next_hole_idx() = hole_idx;
			hole_idx					   = idx;

			--count;
		}

		template <typename t_handle>
		requires requires(t_handle h) { h.id; }
		FORCE_INLINE constexpr void
		remove(t_handle h) noexcept
		{
			remove(h.id);
		}

		FORCE_INLINE constexpr void
		clear() noexcept
		{
			if (cap == 0) { return; }

			destroy_all(alloc, cap, p_buckets, p_bitmap);

			std::memset(p_bitmap, 0, sizeof(uint64) * ((cap + 63) >> 6));

			for (size_type i = 0; i < cap; ++i)
			{
				p_buckets[i].next_hole_idx() = i + 1;
			}
			hole_idx = 0;
			count	 = 0;
		}

		FORCE_INLINE constexpr iterator
		begin() noexcept
		{
			return iterator{ this, count > 0 ? front_idx() : cap };
		}

		FORCE_INLINE constexpr const_iterator
		begin() const noexcept
		{
			return const_iterator{ this, count > 0 ? front_idx() : cap };
		}

		FORCE_INLINE constexpr const_iterator
		cbegin() const noexcept
		{
			return begin();
		}

		FORCE_INLINE constexpr iterator
		end() noexcept
		{
			return iterator{ this, cap };
		}

		FORCE_INLINE constexpr const_iterator
		end() const noexcept
		{
			return const_iterator{ this, cap };
		}

		FORCE_INLINE constexpr const_iterator
		cend() const noexcept
		{
			return end();
		}

		FORCE_INLINE constexpr reverse_iterator
		rbegin() noexcept
		{
			return reverse_iterator{ this, count > 0 ? back_idx() : cap };
		}

		FORCE_INLINE constexpr const_reverse_iterator
		rbegin() const noexcept
		{
			return const_reverse_iterator{ this, count > 0 ? back_idx() : cap };
		}

		FORCE_INLINE constexpr const_reverse_iterator
		crbegin() const noexcept
		{
			return rbegin();
		}

		FORCE_INLINE constexpr reverse_iterator
		rend() noexcept
		{
			return reverse_iterator{ this, cap };
		}

		FORCE_INLINE constexpr const_reverse_iterator
		rend() const noexcept
		{
			return const_reverse_iterator{ this, cap };
		}

		FORCE_INLINE constexpr const_reverse_iterator
		crend() const noexcept
		{
			return rend();
		}

	  private:
		FORCE_INLINE static constexpr bucket*
		alloc_bucket_n(allocator_type alloc, size_type n) noexcept
		{
			auto bucket_alloc = bucket_allocator_type{ alloc };
			return std::allocator_traits<bucket_allocator_type>::allocate(bucket_alloc, n);
		}

		FORCE_INLINE static constexpr void
		dealloc_bucket_n(allocator_type alloc, bucket* p_bucket, size_type n) noexcept
		{
			auto bucket_alloc = bucket_allocator_type{ alloc };
			std::allocator_traits<bucket_allocator_type>::deallocate(bucket_alloc, p_bucket, n);
		}

		FORCE_INLINE static constexpr uint64*
		alloc_bitmap_n(allocator_type alloc, size_type word_count) noexcept
		{
			auto  bitmap_alloc = bitmap_allocator_type{ alloc };
			auto* p_bitmap	   = std::allocator_traits<bitmap_allocator_type>::allocate(bitmap_alloc, word_count);
			std::memset(p_bitmap, 0, sizeof(uint64) * word_count);

			return p_bitmap;
		}

		FORCE_INLINE static constexpr void
		dealloc_bitmap_n(allocator_type alloc, uint64* p_bitmap, size_type word_count) noexcept
		{
			auto bitmap_alloc = bitmap_allocator_type{ alloc };
			std::allocator_traits<bitmap_allocator_type>::deallocate(bitmap_alloc, p_bitmap, word_count);
		}

		FORCE_INLINE static constexpr void
		construct_one(allocator_type alloc, value_type* p, auto&&... args) noexcept
		{
			std::allocator_traits<allocator_type>::construct(alloc, p, FWD(args)...);
		}

		FORCE_INLINE static constexpr void
		destroy_one(allocator_type alloc, value_type* p) noexcept
		{
			std::allocator_traits<allocator_type>::destroy(alloc, p);
		}

		FORCE_INLINE static constexpr void
		destroy_all(allocator_type alloc, size_type cap, bucket* p_buckets, uint64* p_bitmap) noexcept
		{
			if constexpr (std::is_trivially_destructible_v<value_type>)
			{
				return;
			}
			else
			{
				c_auto word_count = (cap + 63) >> 6;	// ceil(cap / 64);

				for (auto word_idx = size_type{ 0 }; word_idx < word_count; ++word_idx)
				{
					for (auto w = p_bitmap[word_idx]; w != 0; w &= w - 1)
					{
						c_auto idx = word_idx * 64 + static_cast<size_type>(std::countr_zero(w));

						destroy_one(alloc, &p_buckets[idx].data());
					}
				}
			}
		}

		FORCE_INLINE static constexpr void
		relocate_to(allocator_type alloc, size_type cap, bucket* p_buckets, uint64* p_bitmap, bucket* p_new_buckets, uint64* p_new_bitmap) noexcept
		{
			c_auto old_word_count = (cap + 63) >> 6;	// ceil(cap / 64);

			if constexpr (std::is_trivially_copyable_v<value_type>)
			{
				std::memcpy(p_new_buckets, p_buckets, sizeof(bucket) * cap);
			}
			else
			{
				c_auto full_word_count = cap >> 6;		 // cap / 64;
				c_auto tail_count	   = cap & 63ull;	 // cap % 64;

				for (auto word_idx = size_type{ 0 }; word_idx < full_word_count; ++word_idx)
				{
					c_auto bitmap = p_bitmap[word_idx];
					c_auto offset = word_idx * 64;

					for (auto i = 0; i < 64; ++i)
					{
						c_auto idx = offset + i;

						if (c_auto is_live = ((bitmap >> i) & 1) != 0)
						{
							construct_one(alloc, &p_new_buckets[idx].data(), std::move(p_buckets[idx].data()));
							destroy_one(alloc, &p_buckets[idx].data());
						}
						else
						{
							p_new_buckets[idx].next_hole_idx() = p_buckets[idx].next_hole_idx();
						}
					}
				}

				if (tail_count != 0)
				{
					c_auto bitmap = p_bitmap[full_word_count];
					c_auto offset = full_word_count * 64;

					for (auto i = 0; i < tail_count; ++i)
					{
						c_auto idx = offset + i;

						if (c_auto is_live = ((bitmap >> i) & 1) != 0)
						{
							construct_one(alloc, &p_new_buckets[idx].data(), std::move(p_buckets[idx].data()));
							destroy_one(alloc, &p_buckets[idx].data());
						}
						else
						{
							p_new_buckets[idx].next_hole_idx() = p_buckets[idx].next_hole_idx();
						}
					}
				}
			}

			std::memcpy(p_new_bitmap, p_bitmap, sizeof(uint64) * old_word_count);
		}

		FORCE_INLINE static constexpr void
		copy_to(allocator_type alloc, size_type cap,
				const bucket* p_src_buckets, const uint64* p_src_bitmap,
				bucket* p_dst_buckets, uint64* p_dst_bitmap) noexcept
		{
			c_auto word_count = (cap + 63) >> 6;	// ceil(cap / 64);

			if constexpr (std::is_trivially_copyable_v<value_type>)
			{
				std::memcpy(p_dst_buckets, p_src_buckets, sizeof(bucket) * cap);
			}
			else
			{
				c_auto full_word_count = cap >> 6;		 // cap / 64;
				c_auto tail_count	   = cap & 63ull;	 // cap % 64;

				for (auto word_idx = size_type{ 0 }; word_idx < full_word_count; ++word_idx)
				{
					c_auto bitmap = p_src_bitmap[word_idx];
					c_auto offset = word_idx * 64;

					for (auto i = 0; i < 64; ++i)
					{
						c_auto idx = offset + i;

						if (c_auto is_live = ((bitmap >> i) & 1) != 0)
						{
							construct_one(alloc, &p_dst_buckets[idx].data(), p_src_buckets[idx].data());
						}
						else
						{
							p_dst_buckets[idx].next_hole_idx() = p_src_buckets[idx].next_hole_idx();
						}
					}
				}

				if (tail_count != 0)
				{
					c_auto bitmap = p_src_bitmap[full_word_count];
					c_auto offset = full_word_count * 64;

					for (auto i = 0; i < tail_count; ++i)
					{
						c_auto idx = offset + i;

						if (c_auto is_live = ((bitmap >> i) & 1) != 0)
						{
							construct_one(alloc, &p_dst_buckets[idx].data(), p_src_buckets[idx].data());
						}
						else
						{
							p_dst_buckets[idx].next_hole_idx() = p_src_buckets[idx].next_hole_idx();
						}
					}
				}
			}

			std::memcpy(p_dst_bitmap, p_src_bitmap, sizeof(uint64) * word_count);
		}
	};
}	 // namespace age::inline data_structure

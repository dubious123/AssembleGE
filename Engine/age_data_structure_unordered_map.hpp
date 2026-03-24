#pragma once

namespace age::inline data_structure
{
	template <typename t_key,
			  typename t_value,
			  typename t_hash	   = std::hash<t_key>,
			  typename t_key_equal = std::equal_to<t_key>,
			  typename t_allocator = std::allocator<std::pair<const t_key, t_value>>>
	struct unordered_map
	{
		using key_type		  = t_key;
		using mapped_type	  = t_value;
		using value_type	  = std::pair<const t_key, t_value>;
		using size_type		  = std::size_t;
		using difference_type = std::ptrdiff_t;
		using hasher		  = t_hash;
		using key_equal		  = t_key_equal;
		using allocator_type  = t_allocator;
		using reference		  = value_type&;
		using const_reference = const value_type&;
		using pointer		  = std::allocator_traits<t_allocator>::pointer;
		using const_pointer	  = std::allocator_traits<t_allocator>::const_pointer;

		struct alignas(32) t_block;

		template <bool is_const>
		struct basic_iterator
		{
			using t_block_ptr = std::conditional_t<is_const, const t_block*, t_block*>;
			using t_ref		  = std::conditional_t<is_const, const value_type&, value_type&>;
			using t_ptr		  = std::conditional_t<is_const, const value_type*, value_type*>;

			t_block_ptr p_block;
			t_block_ptr p_block_end;
			uint32		header_idx;

			FORCE_INLINE constexpr t_ref
			operator*() const noexcept
			{
				return p_block->kv_arr[header_idx];
			}

			FORCE_INLINE constexpr t_ptr
			operator->() const noexcept
			{
				return std::addressof(p_block->kv_arr[header_idx]);
			}

			FORCE_INLINE constexpr basic_iterator&
			operator++() noexcept
			{
				++header_idx;
				advance();
				return *this;
			}

			FORCE_INLINE constexpr bool
			operator==(const basic_iterator& other) const noexcept
			{
				return p_block == other.p_block and header_idx == other.header_idx;
			}

			constexpr
			operator basic_iterator<true>() const noexcept
				requires(is_const is_false)
			{
				return { p_block, p_block_end, header_idx };
			}

			FORCE_INLINE constexpr void
			advance() noexcept
			{
				while (p_block != p_block_end)
				{
					if consteval
					{
						for (; header_idx < header_count_per_block; ++header_idx)
						{
							if (p_block->is_occupied(header_idx))
							{
								return;
							}
						}
					}
					else
					{
						if (header_idx < 16)
						{
							auto idx = find_next_occupied(p_block->header_arr, header_idx);
							if (idx < 16)
							{
								header_idx = idx;
								return;
							}
							header_idx = 16;
						}

						auto idx = find_next_occupied(p_block->header_arr + 16, header_idx - 16);
						if (idx < 16)
						{
							header_idx = 16 + idx;
							return;
						}
					}

					header_idx = 0;
					++p_block;
				}

				// not found, p_block == p_block_end
			}
		};

		using iterator			   = basic_iterator<false>;
		using const_iterator	   = basic_iterator<true>;
		using local_iterator	   = iterator;
		using const_local_iterator = const_iterator;
		// using reverse_iterator		 = std::reverse_iterator<iterator>;
		// using const_reverse_iterator = std::reverse_iterator<const_iterator>;


		using t_header		 = uint16;
		using key_value_pair = std::pair<const t_key, t_value>;

		static constexpr auto header_count_per_block = 32;

		struct alignas(32) t_block
		{
			t_header header_arr[header_count_per_block] = {};	 // uint16 × 32 = 64 bytes

			union
			{
				key_value_pair kv_arr[header_count_per_block];
			};	  // disable default initialization

			// Normally, empty constructor/destructor should be avoided.
			// However, having a non-trivial type inside a union causes
			// the implicit special members to be deleted by language rule.
			// The alternative is raw byte storage + std::launder, but
			// explicit empty bodies are cleaner.
			constexpr t_block() noexcept { }

			constexpr ~t_block() noexcept { }

			FORCE_INLINE constexpr bool
			is_occupied(t_header idx) const noexcept
			{
				return header_arr[idx] & 1;
			}
		};

		using block_allocator_type = typename std::allocator_traits<t_allocator>::template rebind_alloc<t_block>;


	  private:
		no_unique_addr
		allocator_type kv_alloc;

		no_unique_addr
		block_allocator_type block_alloc;

		no_unique_addr
		hasher hash;

		no_unique_addr
		key_equal equal;

		size_type count;
		size_type capacity;
		t_block*  p_block;

		// idx : [0, 16)
		// ptr : addressof header[0] or addressof header[16]
		FORCE_INLINE static uint32
		find_next_occupied(const void* p_header, uint32 idx) noexcept
		{
			auto occupied = _mm256_and_si256(*reinterpret_cast<const __m256i*>(p_header), _mm256_set1_epi16(1));
			auto cmp	  = _mm256_cmpeq_epi16(occupied, _mm256_set1_epi16(1));
			auto bits	  = _mm256_movemask_epi8(cmp);

			bits >>= (idx * 2);

			return bits ? (idx + (_tzcnt_u32(bits) >> 1)) : 16;
		}

		FORCE_INLINE static uint32
		find_first_empty(const void* p_header) noexcept
		{
			auto cmp  = _mm256_cmpeq_epi16(*reinterpret_cast<const __m256i*>(p_header), _mm256_setzero_si256());
			auto bits = _mm256_movemask_epi8(cmp);

			return bits ? (_tzcnt_u32(bits) >> 1) : 16;
		}

		FORCE_INLINE static uint32
		find_matches(const void* p_header, __m256i needle) noexcept
		{
			auto cmp = _mm256_cmpeq_epi16(*reinterpret_cast<const __m256i*>(p_header), needle);
			return _mm256_movemask_epi8(cmp);
		}

		FORCE_INLINE static constexpr uint32
		get_match_idx(uint32 matches) noexcept
		{
			return _tzcnt_u32(matches) >> 1;
		}

		FORCE_INLINE static constexpr uint32
		remove_match_idx(uint32 matches, uint32 match_idx) noexcept
		{
			return matches & ~(0x3 << (match_idx * 2));
		}

		FORCE_INLINE static t_header
		make_header(uint32 header_idx, uint32 home_index, uint64 hash) noexcept
		{
			// [distance (00 ~ 1f)][hash][occupied (0, 1)]
			c_auto distance = (header_idx - home_index) & 0x1f;	   // mod 32, rotation
			return (distance << 11) | (hash & 0x7fe) | 1;
		}

		static constexpr alignas(32) int16 needle_base_first[16] = {
			static_cast<int16>(0 << 11), static_cast<int16>(1 << 11), static_cast<int16>(2 << 11), static_cast<int16>(3 << 11),
			static_cast<int16>(4 << 11), static_cast<int16>(5 << 11), static_cast<int16>(6 << 11), static_cast<int16>(7 << 11),
			static_cast<int16>(8 << 11), static_cast<int16>(9 << 11), static_cast<int16>(10 << 11), static_cast<int16>(11 << 11),
			static_cast<int16>(12 << 11), static_cast<int16>(13 << 11), static_cast<int16>(14 << 11), static_cast<int16>(15 << 11)
		};

		static constexpr alignas(32) int16 needle_base_second[16] = {
			static_cast<int16>(16 << 11), static_cast<int16>(17 << 11), static_cast<int16>(18 << 11), static_cast<int16>(19 << 11),
			static_cast<int16>(20 << 11), static_cast<int16>(21 << 11), static_cast<int16>(22 << 11), static_cast<int16>(23 << 11),
			static_cast<int16>(24 << 11), static_cast<int16>(25 << 11), static_cast<int16>(26 << 11), static_cast<int16>(27 << 11),
			static_cast<int16>(28 << 11), static_cast<int16>(29 << 11), static_cast<int16>(30 << 11), static_cast<int16>(31 << 11)
		};

		FORCE_INLINE static __m256i
		make_first_needle(uint16 home_idx, uint64 hash)
		{
			auto lower = static_cast<uint16>((hash & 0x7fe) | 0x01);
			auto m	   = _mm256_load_si256(reinterpret_cast<const __m256i*>(needle_base_first));

			m = _mm256_sub_epi16(m, _mm256_set1_epi16(home_idx << 11));
			return _mm256_or_si256(m, _mm256_set1_epi16(lower));
		}

		FORCE_INLINE static __m256i
		make_second_needle(uint16 home_idx, uint64 hash)
		{
			auto lower = static_cast<uint16>((hash & 0x7fe) | 0x01);
			auto m	   = _mm256_load_si256(reinterpret_cast<const __m256i*>(needle_base_second));
			m		   = _mm256_sub_epi16(m, _mm256_set1_epi16(home_idx << 11));
			return _mm256_or_si256(m, _mm256_set1_epi16(lower));
		}

	  public:
		constexpr unordered_map() noexcept
			: kv_alloc{ allocator_type{} },
			  block_alloc{ block_allocator_type{} },
			  hash{ hasher{} },
			  equal{ key_equal{} },
			  count{ 0 },
			  capacity{ 32u },
			  p_block{ alloc_block_n(block_alloc, block_count()) }
		{
			construct_block_n(block_alloc, p_block, block_count());
		}

		constexpr unordered_map(const unordered_map& other) noexcept
			: kv_alloc{ other.get_allocator() },
			  block_alloc{ other.get_block_allocator() },
			  hash{ other.hash_function() },
			  equal{ other.key_eq() },
			  count{ other.count },
			  capacity{ other.capacity },
			  p_block{ alloc_block_n(other.get_block_allocator(), other.block_count()) }

		{
			copy_block_n(block_alloc, kv_alloc, p_block, other.p_block, other.block_count());
		}

		constexpr unordered_map(unordered_map&& other) noexcept
			: kv_alloc{ other.get_allocator() },
			  block_alloc{ other.get_block_allocator() },
			  hash{ other.hash_function() },
			  equal{ other.key_eq() },
			  count{ std::exchange(other.count, 0) },
			  capacity{ std::exchange(other.capacity, 0) },
			  p_block{ std::exchange(other.p_block, nullptr) } { }

		template <typename t_input_it>
		constexpr unordered_map(t_input_it first, t_input_it last) noexcept
			: kv_alloc{ allocator_type{} },
			  block_alloc{ block_allocator_type{} },
			  hash{ hasher{} },
			  equal{ key_equal{} },
			  count{ 0 }
		{
			if constexpr (std::random_access_iterator<t_input_it>)
			{
				capacity = next_power_of_2(static_cast<size_type>(last - first));
			}
			else if constexpr (std::forward_iterator<t_input_it>)
			{
				capacity = next_power_of_2(static_cast<size_type>(std::distance(first, last)));
			}
			else
			{
				static_assert(false, "not supported yet");
			}

			p_block = alloc_block_n(block_alloc, block_count());
			construct_block_n(block_alloc, p_block, block_count());

			for (; first != last; ++first)
			{
				insert(*first);
			}
		}

		constexpr unordered_map(std::initializer_list<value_type> init) noexcept
			: unordered_map(init.begin(), init.end())
		{
			static_assert(std::contiguous_iterator<decltype(init.begin())> and std::is_trivially_copyable_v<key_value_pair>);
		}

		template <std::ranges::input_range r>
		constexpr unordered_map(std::from_range_t, r&& rg) noexcept
			: kv_alloc{ allocator_type{} },
			  block_alloc{ block_allocator_type{} },
			  hash{ hasher{} },
			  equal{ key_equal{} },
			  count{ 0 },
			  capacity{ 32 },
			  p_block{ alloc_block_n(block_alloc, 1) }
		{
			static_assert(meta::emplace_constructible<value_type, unordered_map, allocator_type, decltype(*std::ranges::begin(rg))>);
			static_assert(
				std::ranges::sized_range<r>
				or std::ranges::forward_range<r>
				or meta::move_insertable<value_type, unordered_map, allocator_type, decltype(*std::ranges::begin(rg))>);

			construct_block_n(block_alloc, p_block, 1);
			insert_range(FWD(rg));
		}

		constexpr unordered_map&
		operator=(const unordered_map& other) noexcept
		{
			if (this == &other) { return *this; }

			if (p_block is_not_nullptr)
			{
				dealloc_block_n(block_alloc, kv_alloc, p_block, block_count());
			}

			capacity = other.capacity;
			p_block	 = alloc_block_n(block_alloc, block_count());
			count	 = other.count;

			kv_alloc	= other.kv_alloc;
			block_alloc = other.block_alloc;
			hash		= other.hash;
			equal		= other.equal;

			construct_block_n(block_alloc, p_block, block_count());
			copy_block_n(block_alloc, kv_alloc, p_block, other.p_block, block_count());

			return *this;
		}

		constexpr unordered_map&
		operator=(unordered_map&& other) noexcept
		{
			if (this == &other) { return *this; }

			if (p_block is_not_nullptr)
			{
				dealloc_block_n(block_alloc, kv_alloc, p_block, block_count());
			}

			capacity = std::exchange(other.capacity, 0);
			count	 = std::exchange(other.count, 0);
			p_block	 = std::exchange(other.p_block, nullptr);

			kv_alloc	= std::move(other.kv_alloc);
			block_alloc = std::move(other.block_alloc);
			hash		= std::move(other.hash);
			equal		= std::move(other.equal);

			return *this;
		}

		FORCE_INLINE constexpr ~unordered_map() noexcept
		{
			if (p_block is_not_nullptr)
			{
				dealloc_block_n(block_alloc, kv_alloc, p_block, block_count());
			}
		}

		FORCE_INLINE constexpr allocator_type
		get_allocator() const
		{
			return kv_alloc;
		}

		FORCE_INLINE constexpr block_allocator_type
		get_block_allocator() const
		{
			return block_alloc;
		}

		FORCE_INLINE constexpr hasher
		hash_function() const
		{
			return hash;
		}

		FORCE_INLINE constexpr key_equal
		key_eq() const
		{
			return equal;
		}

		template <typename t = size_type>
		FORCE_INLINE constexpr t
		block_count() const
		{
			return static_cast<t>(capacity / header_count_per_block);
		}

	  private:
		FORCE_INLINE constexpr void
		rehash_from(t_block* p_old_block, size_type old_block_count) noexcept
		{
			count = 0;

			if consteval
			{
				for (auto i = 0; i < old_block_count; ++i)
				{
					for (auto h_idx = 0; h_idx < header_count_per_block; ++h_idx)
					{
						if (p_old_block[i].is_occupied(h_idx))
							insert_new(std::move(p_old_block[i].kv_arr[h_idx]));
					}
				}
			}
			else
			{
				for (auto i = 0; i < old_block_count; ++i)
				{
					auto matches = find_matches(p_old_block[i].header_arr + 0, _mm256_or_si256(*reinterpret_cast<__m256i*>(p_old_block[i].header_arr + 0), _mm256_set1_epi16(1)));

					while (matches)
					{
						c_auto idx = get_match_idx(matches);

						insert_new(std::move(p_old_block[i].kv_arr[idx]));

						matches = remove_match_idx(matches, idx);
					}


					matches = find_matches(p_old_block[i].header_arr + 16, _mm256_or_si256(*reinterpret_cast<__m256i*>(p_old_block[i].header_arr + 16), _mm256_set1_epi16(1)));

					while (matches)
					{
						c_auto idx = get_match_idx(matches);

						insert_new(std::move(p_old_block[i].kv_arr[idx]));

						matches = remove_match_idx(matches, idx);
					}
				}
			}

			dealloc_block_n(block_alloc, kv_alloc, p_old_block, old_block_count);
		}

	  public:
		FORCE_INLINE constexpr void
		reserve(size_type n) noexcept
		{
			if (n <= capacity) { return; }

			auto* p_old_block	  = p_block;
			auto  old_block_count = block_count();

			capacity = next_power_of_2(n);
			p_block	 = alloc_block_n(block_alloc, block_count());
			construct_block_n(block_alloc, p_block, block_count());
			rehash_from(p_old_block, old_block_count);
		}

		std::pair<iterator, bool>
		insert(const value_type& kv) noexcept
		{
			return emplace(kv);
		}

		std::pair<iterator, bool>
		insert(value_type&& kv) noexcept
		{
			return emplace(std::move(kv));
		}

		template <std::ranges::input_range r>
		constexpr void
		insert_range(r&& rg) noexcept
		{
			if constexpr (std::ranges::sized_range<r>)
			{
				reserve(count + std::ranges::size(rg));
			}
			else if constexpr (std::ranges::forward_range<r>)
			{
				reserve(count + std::ranges::distance(rg));
			}

			for (auto&& elem : rg)
			{
				emplace(FWD(elem));
			}
		}

		FORCE_INLINE constexpr std::pair<iterator, bool>
		emplace(auto&&... args) noexcept
		{
			auto   key_value = key_value_pair(FWD(args)...);
			c_auto h		 = hash(key_value.first);

			// capacity is always power of 2
			// this is same as
			// home = hash % capacity;
			c_auto home = h & (capacity - 1);
			// header count is 32
			// block_idx = home / 32
			c_auto block_idx = home >> 5;
			c_auto home_idx	 = home & (32 - 1);

			auto* p_block_target = p_block + block_idx;
			if consteval
			{
				// find existing
				for (uint32 i = 0; i < header_count_per_block; ++i)
				{
					if (p_block_target->is_occupied(i) and equal(p_block_target->kv_arr[i].first, key_value.first))
					{
						return { iterator{
									 .p_block	  = p_block_target,
									 .p_block_end = p_block + block_count(),
									 .header_idx  = i },
								 false };
					}
				}

				// find empty
				for (uint32 i = 0; i < header_count_per_block; ++i)
				{
					if (p_block_target->is_occupied(i) is_false)
					{
						p_block_target->header_arr[i] = make_header(i, home_idx, h);

						std::allocator_traits<allocator_type>::construct(
							kv_alloc,
							std::addressof(p_block_target->kv_arr[i]),
							std::move(key_value));

						++count;

						return { iterator{
									 .p_block	  = p_block_target,
									 .p_block_end = p_block + block_count(),
									 .header_idx  = i },
								 true };
					}
				}
			}
			else
			{
				// find existing first 16
				auto first_needle = make_first_needle(home_idx, h);
				auto matches	  = find_matches(p_block_target->header_arr, first_needle);

				while (matches)
				{
					c_auto idx = get_match_idx(matches);
					if (equal(p_block_target->kv_arr[idx].first, key_value.first))
					{
						return { iterator{
									 .p_block	  = p_block_target,
									 .p_block_end = p_block + block_count(),
									 .header_idx  = idx },
								 false };
					}
					matches = remove_match_idx(matches, idx);
				}

				// find existing second 16
				auto second_needle = make_second_needle(home_idx, h);
				matches			   = find_matches(p_block_target->header_arr + 16, second_needle);

				while (matches)
				{
					c_auto idx = get_match_idx(matches);
					if (equal(p_block_target->kv_arr[16 + idx].first, key_value.first))
					{
						return { iterator{
									 .p_block	  = p_block_target,
									 .p_block_end = p_block + block_count(),
									 .header_idx  = 16 + idx },
								 false };
					}
					matches = remove_match_idx(matches, idx);
				}

				// find empty slot first 16
				auto empty_idx = find_first_empty(p_block_target->header_arr);

				if (empty_idx >= 16)
				{
					// find empty slot second 16
					empty_idx = 16 + find_first_empty(p_block_target->header_arr + 16);
				}

				if (empty_idx < 32)
				{
					p_block_target->header_arr[empty_idx] = make_header(empty_idx, home_idx, h);
					std::allocator_traits<allocator_type>::construct(
						kv_alloc,
						std::addressof(p_block_target->kv_arr[empty_idx]),
						std::move(key_value));

					++count;

					return { iterator{
								 .p_block	  = p_block_target,
								 .p_block_end = p_block + block_count(),
								 .header_idx  = empty_idx },
							 true };
				}
			}

			// block is full
			// resize and retry
			resize();
			return insert(std::move(key_value));
		}

		template <typename... t_args>
		FORCE_INLINE constexpr std::pair<iterator, bool>
		try_emplace(const key_type& key, t_args&&... args) noexcept
		{
			c_auto h		 = hash(key);
			c_auto home		 = h & (capacity - 1);
			c_auto block_idx = home >> 5;
			c_auto home_idx	 = home & (32 - 1);

			auto* p_block_target = p_block + block_idx;

			if consteval
			{
				for (uint32 i = 0; i < header_count_per_block; ++i)
				{
					if (p_block_target->is_occupied(i)
						and equal(p_block_target->kv_arr[i].first, key))
					{
						return { iterator{
									 .p_block	  = p_block_target,
									 .p_block_end = p_block + block_count(),
									 .header_idx  = i },
								 false };
					}
				}

				for (uint32 i = 0; i < header_count_per_block; ++i)
				{
					if (not p_block_target->is_occupied(i))
					{
						p_block_target->header_arr[i] = make_header(i, home_idx, h);
						std::allocator_traits<allocator_type>::construct(
							kv_alloc,
							std::addressof(p_block_target->kv_arr[i]),
							std::piecewise_construct,
							std::forward_as_tuple(key),
							std::forward_as_tuple(FWD(args)...));

						++count;

						return { iterator{
									 .p_block	  = p_block_target,
									 .p_block_end = p_block + block_count(),
									 .header_idx  = i },
								 true };
					}
				}
			}
			else
			{
				// find existing: first 16
				auto first_needle = make_first_needle(home_idx, h);
				auto matches	  = find_matches(p_block_target->header_arr, first_needle);

				while (matches)
				{
					c_auto idx = get_match_idx(matches);
					if (equal(p_block_target->kv_arr[idx].first, key))
					{
						return { iterator{
									 .p_block	  = p_block_target,
									 .p_block_end = p_block + block_count(),
									 .header_idx  = idx },
								 false };
					}
					matches = remove_match_idx(matches, idx);
				}

				// find existing: second 16
				auto second_needle = make_second_needle(home_idx, h);
				matches			   = find_matches(p_block_target->header_arr + 16, second_needle);

				while (matches)
				{
					c_auto idx = get_match_idx(matches);
					if (equal(p_block_target->kv_arr[16 + idx].first, key))
					{
						return { iterator{
									 .p_block	  = p_block_target,
									 .p_block_end = p_block + block_count(),
									 .header_idx  = 16 + idx },
								 false };
					}
					matches = remove_match_idx(matches, idx);
				}

				// find empty
				auto empty_idx = find_first_empty(p_block_target->header_arr);

				if (empty_idx >= 16)
				{
					empty_idx = 16 + find_first_empty(p_block_target->header_arr + 16);
				}

				if (empty_idx < 32)
				{
					p_block_target->header_arr[empty_idx] = make_header(empty_idx, home_idx, h);
					std::allocator_traits<allocator_type>::construct(
						kv_alloc,
						std::addressof(p_block_target->kv_arr[empty_idx]),
						std::piecewise_construct,
						std::forward_as_tuple(key),
						std::forward_as_tuple(FWD(args)...));

					++count;

					return { iterator{
								 .p_block	  = p_block_target,
								 .p_block_end = p_block + block_count(),
								 .header_idx  = empty_idx },
							 true };
				}
			}

			// block is full
			// resize and retry
			resize();
			return try_emplace(key, FWD(args)...);
		}

		FORCE_INLINE constexpr mapped_type&
		operator[](const key_type& key) noexcept
		{
			return try_emplace(key).first->second;
		}

	  private:
		FORCE_INLINE constexpr void
		insert_new(key_value_pair&& key_value) noexcept
		{
			c_auto h		 = hash(key_value.first);
			c_auto home		 = h & (capacity - 1);
			c_auto block_idx = home >> 5;
			c_auto home_idx	 = home & (32 - 1);

			auto* p_target = p_block + block_idx;

			if consteval
			{
				for (uint32 i = 0; i < header_count_per_block; ++i)
				{
					if (not p_target->is_occupied(i))
					{
						p_target->header_arr[i] = make_header(i, home_idx, h);
						std::allocator_traits<allocator_type>::construct(
							kv_alloc,
							std::addressof(p_target->kv_arr[i]),
							std::move(key_value));

						++count;

						return;
					}
				}
			}
			else
			{
				auto empty_idx = find_first_empty(p_target->header_arr);

				if (empty_idx >= 16)
				{
					empty_idx = 16 + find_first_empty(p_target->header_arr + 16);
				}

				if (empty_idx < 32)
				{
					p_target->header_arr[empty_idx] = make_header(empty_idx, home_idx, h);
					std::allocator_traits<allocator_type>::construct(
						kv_alloc,
						std::addressof(p_target->kv_arr[empty_idx]),
						std::move(key_value));

					++count;
					return;
				}
			}

			// block full, resize and retry
			resize();
			insert_new(std::move(key_value));
		}

	  public:
		FORCE_INLINE constexpr size_type
		erase(const key_type& key) noexcept
		{
			auto it = find(key);
			if (it == end())
			{
				return 0;
			}

			if constexpr (std::is_trivially_destructible_v<key_value_pair>)
			{
			}
			else if constexpr (std::is_nothrow_destructible_v<key_value_pair>)
			{
				std::allocator_traits<allocator_type>::destroy(
					kv_alloc, std::addressof(it.p_block->kv_arr[it.header_idx]));
			}
			else
			{
				static_assert(false, "throwable element is not supported yet");
			}

			it.p_block->header_arr[it.header_idx] = 0;
			--count;
			return 1;
		}

		FORCE_INLINE constexpr void
		clear() noexcept
		{
			if constexpr (std::is_trivially_destructible_v<key_value_pair>)
			{
			}
			else if constexpr (std::is_nothrow_destructible_v<key_value_pair>)
			{
				for (auto it = begin(); it != end(); ++it)
				{
					std::allocator_traits<allocator_type>::destroy(
						kv_alloc, std::addressof(*it));
				}
			}
			else
			{
				static_assert(false, "throwable element is not supported yet");
			}

			for (auto i = 0; i < block_count(); ++i)
			{
				std::fill_n(p_block[i].header_arr, header_count_per_block, t_header{ 0 });
			}

			count = 0;
		}

		FORCE_INLINE constexpr void
		resize() noexcept
		{
			auto* p_old_block		= p_block;
			auto  old_block_count	= block_count();
			capacity			  <<= 1;
			p_block					= alloc_block_n(block_alloc, block_count());
			construct_block_n(block_alloc, p_block, block_count());
			rehash_from(p_old_block, old_block_count);
		}

		FORCE_INLINE constexpr iterator
		begin() noexcept
		{
			auto it = iterator{ .p_block = p_block, .p_block_end = p_block + block_count(), .header_idx = 0 };
			it.advance();
			return it;
		}

		FORCE_INLINE constexpr const_iterator
		begin() const noexcept
		{
			auto it = const_iterator{ .p_block = p_block, .p_block_end = p_block + block_count(), .header_idx = 0 };
			it.advance();
			return it;
		}

		FORCE_INLINE constexpr const_iterator
		cbegin() const noexcept
		{
			return begin();
		}

		FORCE_INLINE constexpr iterator
		end() noexcept
		{
			return iterator{ .p_block = p_block + block_count(), .p_block_end = p_block + block_count(), .header_idx = 0 };
		}

		FORCE_INLINE constexpr const_iterator
		end() const noexcept
		{
			return const_iterator{ .p_block = p_block + block_count(), .p_block_end = p_block + block_count(), .header_idx = 0 };
		}

		FORCE_INLINE constexpr const_iterator
		cend() const noexcept
		{
			return end();
		}

		/*FORCE_INLINE constexpr reverse_iterator
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
		}*/

		FORCE_INLINE iterator
		find(const key_type& key) noexcept
		{
			c_auto h		 = hash(key);
			c_auto home		 = h & (capacity - 1);
			c_auto block_idx = home >> 5;
			c_auto home_idx	 = home & (32 - 1);

			auto* p_target = p_block + block_idx;

			if consteval
			{
				for (uint32 i = 0; i < header_count_per_block; ++i)
				{
					if (p_target->is_occupied(i)
						and equal(p_target->kv_arr[i].first, key))
					{
						return iterator{
							.p_block	 = p_target,
							.p_block_end = p_block + block_count(),
							.header_idx	 = i
						};
					}
				}
			}
			else
			{
				auto first_needle = make_first_needle(home_idx, h);
				auto matches	  = find_matches(p_target->header_arr, first_needle);

				while (matches)
				{
					c_auto idx = get_match_idx(matches);
					if (equal(p_target->kv_arr[idx].first, key))
					{
						return iterator{
							.p_block	 = p_target,
							.p_block_end = p_block + block_count(),
							.header_idx	 = idx
						};
					}
					matches = remove_match_idx(matches, idx);
				}

				auto second_needle = make_second_needle(home_idx, h);
				matches			   = find_matches(p_target->header_arr + 16, second_needle);

				while (matches)
				{
					c_auto idx = get_match_idx(matches);
					if (equal(p_target->kv_arr[16 + idx].first, key))
					{
						return iterator{
							.p_block	 = p_target,
							.p_block_end = p_block + block_count(),
							.header_idx	 = 16 + idx
						};
					}
					matches = remove_match_idx(matches, idx);
				}
			}

			return end();
		}

		FORCE_INLINE const_iterator
		find(const key_type& key) const noexcept
		{
			return const_cast<unordered_map*>(this)->find(key);
		}

		FORCE_INLINE constexpr mapped_type&
		at(const key_type& key) noexcept
		{
			return find(key)->second;
		}

		FORCE_INLINE constexpr const mapped_type&
		at(const key_type& key) const noexcept
		{
			return find(key)->second;
		}

		FORCE_INLINE constexpr bool
		contains(const key_type& key) const noexcept
		{
			return find(key).p_block != p_block + block_count();
		}

		template <typename t_res = size_type>
		FORCE_INLINE constexpr t_res
		size() const noexcept
		{
			return static_cast<t_res>(count);
		}

		FORCE_INLINE constexpr bool
		empty() const noexcept
		{
			return count == 0;
		}

		FORCE_INLINE constexpr bool
		is_empty() const noexcept
		{
			return empty();
		}


	  private:
		FORCE_INLINE static constexpr size_type
		next_power_of_2(size_type n) noexcept
		{
			if (n <= header_count_per_block) return header_count_per_block;

			--n;
			n |= n >> 1;
			n |= n >> 2;
			n |= n >> 4;
			n |= n >> 8;
			n |= n >> 16;
			n |= n >> 32;
			return ++n;
		}

		FORCE_INLINE static constexpr t_block*
		alloc_block_n(block_allocator_type block_alloc, const size_type block_count) noexcept
		{
			return std::allocator_traits<block_allocator_type>::allocate(block_alloc, block_count);
		}

		FORCE_INLINE static constexpr void
		construct_block_n(block_allocator_type block_alloc, t_block* p_block, const size_type block_count) noexcept
		{
			for (auto i = 0; i < block_count; ++i)
			{
				std::allocator_traits<block_allocator_type>::construct(block_alloc, p_block + i);
			}
		}

		FORCE_INLINE static constexpr void
		dealloc_block_n(block_allocator_type block_alloc, allocator_type kv_alloc, t_block* p_block, const size_type block_count) noexcept
		{
			if constexpr (std::is_trivially_destructible_v<key_value_pair> is_false)
			{
				for (auto i = 0; i < block_count; ++i)
				{
					for (auto h_idx = 0; h_idx < header_count_per_block; ++h_idx)
					{
						if (p_block[i].is_occupied(h_idx))
						{
							std::allocator_traits<allocator_type>::destroy(kv_alloc, std::addressof(p_block[i].kv_arr[h_idx]));
						}
					}
				}
			}

			std::allocator_traits<block_allocator_type>::deallocate(block_alloc, p_block, block_count);
		}

		FORCE_INLINE static constexpr void
		copy_block_n(block_allocator_type block_alloc, allocator_type kv_alloc, t_block* p_dst, const t_block* p_src, const size_type block_count) noexcept
		{
			if consteval
			{
				for (auto i = 0; i < block_count; ++i)
				{
					for (auto h_idx = 0; h_idx < header_count_per_block; ++h_idx)
					{
						p_dst[i].header_arr[h_idx] = p_src[i].header_arr[h_idx];

						if (p_src[i].is_occupied(h_idx))
						{
							std::allocator_traits<allocator_type>::construct(kv_alloc, std::addressof(p_dst[i].kv_arr[h_idx]), p_src[i].kv_arr[h_idx]);
						}
					}
				}
			}
			else
			{
				if constexpr (std::is_trivially_copyable_v<value_type>)
				{
					std::memcpy(p_dst, p_src, sizeof(t_block) * block_count);
				}
				else if constexpr (std::is_nothrow_copy_constructible_v<value_type>)
				{
					for (auto i = 0; i < block_count; ++i)
					{
						std::memcpy(p_dst[i].header_arr, p_src[i].header_arr, sizeof(t_header) * header_count_per_block);

						for (auto h_idx = 0; h_idx < header_count_per_block; ++h_idx)
						{
							if (p_src[i].is_occupied(h_idx))
							{
								std::allocator_traits<allocator_type>::construct(kv_alloc, std::addressof(p_dst[i].kv_arr[h_idx]), p_src[i].kv_arr[h_idx]);
							}
						}
					}
				}
				else
				{
					static_assert(false, "throwable element not supported yet");
				}
			}
		}
	};
}	 // namespace age::inline data_structure
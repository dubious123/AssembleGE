#pragma once

namespace age::inline data_structure
{
	namespace byte_buffer_cx
	{
		template <typename t>
		concept trivial =
			std::is_trivially_destructible_v<std::remove_cvref_t<t>>
			and std::is_standard_layout_v<std::remove_cvref_t<t>>
			and std::is_trivially_copyable_v<std::remove_cvref_t<t>>;

		template <typename t>
		concept custom = requires(t arg, void* p_base, std::size_t& offset) {
			{ arg.byte_size() } -> std::convertible_to<std::size_t>;
			{ arg.write_to(p_base, offset) };
			{ std::remove_cvref_t<t>::read_from(p_base, offset) } -> std::same_as<std::remove_cvref_t<t>>;
		};

		template <typename t>
		concept value = trivial<t> or custom<t>;
	}	 // namespace byte_buffer_cx

	template <typename t_allocator = std::allocator<std::byte>>
	struct byte_buffer
	{
		using value_type	 = std::byte;
		using allocator_type = typename std::allocator_traits<t_allocator>::template rebind_alloc<value_type>;
		using size_type		 = std::size_t;
		using reference		 = value_type&;

	  private:
		size_type	cap		  = {};
		size_type	write_pos = {};
		size_type	read_pos  = {};
		value_type* p_data	  = nullptr;

		no_unique_addr
		allocator_type alloc;

	  public:
		constexpr byte_buffer() noexcept
			: alloc{ allocator_type{} } { };

		constexpr byte_buffer(const byte_buffer& other) noexcept
			: cap{ other.capacity() },
			  write_pos{ other.size() },
			  read_pos{ 0 },
			  p_data{ allocate(other.get_allocator(), other.cap) },
			  alloc{ other.get_allocator() }

		{
			std::memcpy(p_data, other.data(), other.size());
		}

		constexpr byte_buffer(byte_buffer&& other) noexcept
			: cap{ std::exchange(other.cap, 0) },
			  write_pos{ std::exchange(other.write_pos, 0) },
			  read_pos{ std::exchange(other.read_pos, 0) },
			  p_data{ std::exchange(other.p_data, nullptr) },
			  alloc{ other.get_allocator() }
		{
		}

		constexpr byte_buffer&
		operator=(const byte_buffer& other) noexcept
		{
			if (this == &other) { return *this; }

			if (cap < other.size())
			{
				deallocate(alloc, p_data, cap);
				p_data = allocate(alloc, other.size());
				cap	   = other.size();
			}

			std::memcpy(p_data, other.data(), other.size());

			write_pos = other.write_pos;
			read_pos  = other.read_pos;

			return *this;
		}

		constexpr byte_buffer&
		operator=(byte_buffer&& other) noexcept
		{
			if (this == &other) { return *this; }

			clear();
			deallocate(alloc, p_data, cap);

			cap		  = std::exchange(other.cap, 0);
			write_pos = std::exchange(other.write_pos, 0);
			read_pos  = std::exchange(other.read_pos, 0);
			p_data	  = std::exchange(other.p_data, nullptr);
			alloc	  = other.get_allocator();

			return *this;
		}

		FORCE_INLINE constexpr ~byte_buffer() noexcept
		{
			deallocate(alloc, p_data, cap);
		}

		FORCE_INLINE static constexpr byte_buffer
		gen_reserved(size_type n, void* p_hint = nullptr) noexcept
		{
			auto buf	  = byte_buffer{};
			buf.cap		  = n;
			buf.read_pos  = 0;
			buf.write_pos = 0;
			buf.p_data	  = allocate(buf.alloc, n, p_hint);
			return buf;
		}

		// access
		FORCE_INLINE constexpr allocator_type
		get_allocator() const
		{
			return alloc;
		}

		template <typename t_ret = size_type>
		FORCE_INLINE constexpr decltype(auto)
		capacity() const noexcept
		{
			return static_cast<t_ret>(cap);
		}

		template <typename t_ret = size_type>
		FORCE_INLINE constexpr decltype(auto)
		size() const noexcept
		{
			return static_cast<t_ret>(write_pos);
		}

		template <typename t_ret = size_type>
		FORCE_INLINE constexpr decltype(auto)
		read_amount() const noexcept
		{
			return read_pos;
		}

		FORCE_INLINE constexpr reference
		operator[](size_type i) noexcept
		{
			AGE_ASSERT(i < write_pos);
			return p_data[i];
		}

		FORCE_INLINE constexpr const value_type&
		operator[](size_type i) const noexcept
		{
			AGE_ASSERT(i < write_pos);
			return p_data[i];
		}

		FORCE_INLINE constexpr const value_type*
		data() const noexcept
		{
			return p_data;
		}

		FORCE_INLINE constexpr value_type*
		data() noexcept
		{
			return p_data;
		}

		FORCE_INLINE constexpr bool
		has_remaining() const noexcept
		{
			return read_pos < write_pos;
		}

		// capacity
		FORCE_INLINE constexpr void
		reserve(size_type new_cap, void* p_hint = nullptr) noexcept
		{
			if (cap >= new_cap)
			{
				return;
			}

			resize(new_cap, p_hint);
		}

		FORCE_INLINE constexpr void
		resize(size_type new_cap, void* p_hint = nullptr) noexcept
		{
			AGE_ASSERT(write_pos <= new_cap);
			auto* p_new_data = allocate(alloc, new_cap, p_hint);

			if (p_data is_not_nullptr)
			{
				std::memcpy(p_new_data, p_data, write_pos);
				deallocate(alloc, p_data, cap);
			}

			p_data = p_new_data;
			cap	   = new_cap;
		}

	  private:
		template <byte_buffer_cx::value t>
		FORCE_INLINE static constexpr size_type
		calc_one_size(const t& val) noexcept
		{
			if constexpr (byte_buffer_cx::trivial<t>)
			{
				return static_cast<size_type>(sizeof(t));
			}
			else
			{
				return val.byte_size();
			}
		}

	  public:
		template <byte_buffer_cx::trivial... t>
		FORCE_INLINE constexpr size_type
		calc_write_size() const noexcept
		{
			return calc_write_size_at<t...>(write_pos);
		}

		template <byte_buffer_cx::trivial... t>
		FORCE_INLINE constexpr size_type
		calc_write_size_at(size_type offset) const noexcept
		{
			c_auto start = offset;
			((offset = age::util::align_up(offset, alignof(t)) + sizeof(t)), ...);
			return offset - start;
		}

		template <byte_buffer_cx::value... t>
		FORCE_INLINE constexpr size_type
		calc_write_size(const t&... arg) const noexcept
		{
			auto   offset = write_pos;
			c_auto start  = offset;
			((offset = age::util::align_up(offset, byte_buffer_cx::trivial<t> ? alignof(t) : 1) + calc_one_size(arg)), ...);
			return offset - start;
		}

		FORCE_INLINE constexpr bool
		is_empty() const noexcept
		{
			return write_pos == 0;
		}

		FORCE_INLINE constexpr bool
		empty() const noexcept
		{
			return write_pos == 0;
		}

		// modifiers
		FORCE_INLINE constexpr void
		clear() noexcept
		{
			write_pos = 0;
			read_pos  = 0;
		}

		FORCE_INLINE constexpr void
		reset_read() noexcept
		{
			read_pos = 0;
		}

	  private:
		FORCE_INLINE constexpr void
		write_one(byte_buffer_cx::value auto&& val) noexcept
		{
			using raw_t = BARE_OF(val);

			if constexpr (byte_buffer_cx::custom<raw_t>)
			{
				val.write_to(p_data, write_pos);
			}
			else
			{
				write_pos = age::util::align_up(write_pos, alignof(raw_t));
				std::memcpy(p_data + write_pos, &val, sizeof(raw_t));
				write_pos += sizeof(raw_t);
			}
		}

	  public:
		FORCE_INLINE constexpr void
		write(byte_buffer_cx::value auto&&... arg) noexcept
		{
			c_auto cap_required = write_pos + calc_write_size(arg...);
			if (cap < cap_required)
			{
				resize(std::max(cap_required, cap * 2));
			}

			(write_one(FWD(arg)), ...);
		}

	  private:
		template <byte_buffer_cx::value t>
		FORCE_INLINE constexpr t
		read_one() noexcept
		{
			if constexpr (byte_buffer_cx::custom<t>)
			{
				return t::read_from(p_data, read_pos);
			}
			else
			{
				read_pos = age::util::align_up<size_type>(read_pos, alignof(t));
				t val;
				std::memcpy(&val, p_data + read_pos, sizeof(t));
				read_pos += sizeof(t);
				return val;
			}
		}

	  public:
		template <byte_buffer_cx::value... t>
		requires(sizeof...(t) > 0)
		FORCE_INLINE constexpr decltype(auto)
		read() noexcept
		{
			if constexpr (sizeof...(t) == 1)
			{
				return read_one<t...>();
			}
			else
			{
				return std::tuple<t...>{ read_one<t>()... };
			}
		}


	  private:
		FORCE_INLINE static constexpr value_type*
		allocate(allocator_type alloc, size_type cap, void* p_hint = nullptr) noexcept
		{
			return std::allocator_traits<allocator_type>::allocate(alloc, cap, p_hint);
		}

		FORCE_INLINE static constexpr void
		deallocate(allocator_type alloc, value_type* p_data, size_type cap) noexcept
		{
			std::allocator_traits<allocator_type>::deallocate(alloc, p_data, cap);
		}
	};

	using byte_buf = byte_buffer<>;
}	 // namespace age::inline data_structure
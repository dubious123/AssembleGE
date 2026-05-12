#pragma once

namespace age::util
{
	template <uint32 n_bits>
	requires(n_bits % 32 == 0)
	struct bitset
	{
		static constexpr uint32 n_words = n_bits / 32;
		uint32					words[n_words];

		struct range
		{
			uint32 min = get_invalid_idx<uint32>();
			uint32 max;

			FORCE_INLINE constexpr explicit
			operator bool() const
			{
				return AGE_IS_INVALID_IDX(min) is_false;
			}
		};

		FORCE_INLINE bool
		test(uint32 idx) const noexcept
		{
			return (words[idx / 32] >> (idx % 32)) & 1;
		}

		FORCE_INLINE void
		set(uint32 idx) noexcept
		{
			words[idx / 32] |= (1u << (idx % 32));
		}

		FORCE_INLINE void
		reset(uint32 idx) noexcept
		{
			words[idx / 32] &= ~(1u << (idx % 32));
		}

		FORCE_INLINE void
		flip(uint32 idx) noexcept
		{
			words[idx / 32] ^= (1u << (idx % 32));
		}

		FORCE_INLINE void
		set() noexcept
		{
			for (auto& w : words)
			{
				w = ~0u;
			}
		}

		FORCE_INLINE void
		set_range(int32 min, int32 max) noexcept
		{
			AGE_ASSERT(min <= max);
			AGE_ASSERT(min >= 0 and max >= 0);
			AGE_ASSERT(max < n_bits);

			if (min / 32 == max / 32)
			{
				words[min / 32] |= (0xffff'ffff << (min % 32)) & ((1u << (max % 32 + 1)) - 1);
				return;
			}

			words[min / 32] |= (0xffff'ffff << (min % 32));

			for (auto i = min / 32 + 1; i < max / 32; ++i)
			{
				words[i] = ~0u;
			}

			words[max / 32] |= ((1u << (max % 32)) - 1);
		}

		FORCE_INLINE range
		calc_set_range() const noexcept
		{
			auto res = range{};

			for (auto&& [i, w] : words | std::views::enumerate)
			{
				if (w)
				{
					res.min = static_cast<uint32>(i) * 32 + std::countr_zero(w);
					break;
				}
			}

			for (auto&& [i, w] : words | std::views::enumerate | std::views::reverse)
			{
				if (w)
				{
					res.max = static_cast<uint32>(i) * 32 + std::bit_width(w) - 1;
					break;
				}
			}

			return res;
		}

		FORCE_INLINE void
		set_range(auto min, auto max) noexcept
			requires(std::is_same_v<BARE_OF(min), int32> or std::is_same_v<BARE_OF(max), int32>)
		{
			return set_range(static_cast<int32>(min), static_cast<int32>(max));
		}

		FORCE_INLINE void
		reset() noexcept
		{
			for (auto& w : words)
			{
				w = 0u;
			}
		}

		FORCE_INLINE void
		flip() noexcept
		{
			for (auto& w : words)
			{
				w = ~w;
			}
		}

		FORCE_INLINE bool
		any() const noexcept
		{
			for (auto w : words)
			{
				if (w) { return true; }
			}
			return false;
		}

		FORCE_INLINE bool
		none() const noexcept
		{
			return !any();
		}

		FORCE_INLINE bool
		all() const noexcept
		{
			for (auto w : words)
			{
				if (w != ~0u) { return false; }
			}
			return true;
		}

		FORCE_INLINE uint32
		count() const noexcept
		{
			uint32 c = 0;
			for (auto w : words)
			{
				c += popcount(w);
			}
			return c;
		}

		FORCE_INLINE static constexpr uint32
		size() noexcept
		{
			return n_bits;
		}

		FORCE_INLINE bool
		operator==(const bitset&) const noexcept = default;

		FORCE_INLINE bitset
		operator&(const bitset& rhs) const noexcept
		{
			bitset result;
			for (uint32 i = 0; i < n_words; ++i)
			{
				result.words[i] = words[i] & rhs.words[i];
			}
			return result;
		}

		FORCE_INLINE bitset
		operator|(const bitset& rhs) const noexcept
		{
			bitset result;
			for (uint32 i = 0; i < n_words; ++i)
			{
				result.words[i] = words[i] | rhs.words[i];
			}
			return result;
		}

		FORCE_INLINE bitset
		operator^(const bitset& rhs) const noexcept
		{
			bitset result;
			for (uint32 i = 0; i < n_words; ++i)
			{
				result.words[i] = words[i] ^ rhs.words[i];
			}
			return result;
		}

		FORCE_INLINE bitset
		operator~() const noexcept
		{
			bitset result;
			for (uint32 i = 0; i < n_words; ++i)
			{
				result.words[i] = ~words[i];
			}
			return result;
		}

		FORCE_INLINE bitset&
		operator&=(const bitset& rhs) noexcept
		{
			for (uint32 i = 0; i < n_words; ++i)
			{
				words[i] &= rhs.words[i];
			}
			return *this;
		}

		FORCE_INLINE bitset&
		operator|=(const bitset& rhs) noexcept
		{
			for (uint32 i = 0; i < n_words; ++i)
			{
				words[i] |= rhs.words[i];
			}
			return *this;
		}

		FORCE_INLINE bitset&
		operator^=(const bitset& rhs) noexcept
		{
			for (uint32 i = 0; i < n_words; ++i)
			{
				words[i] ^= rhs.words[i];
			}
			return *this;
		}

		template <typename t_fn>
		FORCE_INLINE void
		foreach_set(t_fn&& fn) const noexcept
		{
			for (uint32 i = 0; i < n_words; ++i)
			{
				auto w = words[i];
				while (w)
				{
					c_auto bit = std::countr_zero(w);
					fn(i * 32 + bit);
					w &= w - 1;	   // clear lowest set bit
				}
			}
		}

		template <typename t = uint32>
		FORCE_INLINE t
		extract() const noexcept
		{
			static_assert(std::is_unsigned_v<t>);

			if constexpr (sizeof(t) <= 4)
			{
				return static_cast<t>(words[0]);
			}
			else if constexpr (sizeof(t) == 8)
			{
				return static_cast<t>(words[0]) | (static_cast<t>(words[1]) << 32);
			}
		}

		template <typename t = uint32>
		FORCE_INLINE t
		extract(uint32 bit_offset) const noexcept
		{
			static_assert(std::is_unsigned_v<t>);
			constexpr uint32 t_bits = sizeof(t) * 8;

			c_auto word_idx = bit_offset / 32;
			c_auto bit_idx	= bit_offset % 32;

			if constexpr (t_bits <= 32)
			{
				if (bit_idx + t_bits <= 32)
				{
					return static_cast<t>((words[word_idx] >> bit_idx) & ((1u << t_bits) - 1));
				}
				else
				{
					c_auto lo = words[word_idx] >> bit_idx;
					c_auto hi = words[word_idx + 1] << (32 - bit_idx);
					return static_cast<t>((lo | hi) & ((1u << t_bits) - 1));
				}
			}
			else if constexpr (t_bits == 64)
			{
				if (bit_idx == 0)
				{
					return static_cast<t>(words[word_idx]) | (static_cast<t>(words[word_idx + 1]) << 32);
				}
				else
				{
					c_auto lo  = static_cast<t>(words[word_idx]) >> bit_idx;
					c_auto mid = static_cast<t>(words[word_idx + 1]) << (32 - bit_idx);
					c_auto hi  = static_cast<t>(words[word_idx + 2]) << (64 - bit_idx);
					return (lo | mid | hi) & (~t(0));
				}
			}
		}
	};
}	 // namespace age::util
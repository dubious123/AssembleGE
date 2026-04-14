#pragma once

namespace age::util
{
	template <uint32 n_bits>
	requires(n_bits % 32 == 0)
	struct bitset
	{
		static constexpr uint32 n_words = n_bits / 32;
		uint32					words[n_words];

		FORCE_INLINE bool
		test(uint32 idx) const noexcept
		{
			return (words[idx / 32] >> (idx % 32)) & 1;
		}

		FORCE_INLINE bitset&
		set(uint32 idx) noexcept
		{
			words[idx / 32] |= (1u << (idx % 32));
			return *this;
		}

		FORCE_INLINE bitset&
		reset(uint32 idx) noexcept
		{
			words[idx / 32] &= ~(1u << (idx % 32));
			return *this;
		}

		FORCE_INLINE bitset&
		flip(uint32 idx) noexcept
		{
			words[idx / 32] ^= (1u << (idx % 32));
			return *this;
		}

		FORCE_INLINE bitset&
		set() noexcept
		{
			for (auto& w : words)
			{
				w = ~0u;
			}
			return *this;
		}

		FORCE_INLINE bitset&
		reset() noexcept
		{
			for (auto& w : words)
			{
				w = 0u;
			}
			return *this;
		}

		FORCE_INLINE bitset&
		flip() noexcept
		{
			for (auto& w : words)
			{
				w = ~w;
			}
			return *this;
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
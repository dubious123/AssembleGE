#pragma once

namespace age::util
{
	template <std::unsigned_integral t>
	FORCE_INLINE constexpr t
	set_bit(t value, uint32 bit_idx, bool flag) noexcept
	{
		return (value & ~(t(1) << bit_idx)) | (t(flag) << bit_idx);
	}

	template <std::unsigned_integral t>
	FORCE_INLINE constexpr bool
	get_bit(t value, uint32 bit_idx) noexcept
	{
		return (value >> bit_idx) & t(1);
	}

	template <std::unsigned_integral t>
	FORCE_INLINE constexpr t
	set_bits(t value, uint32 bit_idx, uint32 bit_count, t new_bits) noexcept
	{
		c_auto mask = ((t(1) << bit_count) - t(1)) << bit_idx;
		return (value & ~mask) | ((new_bits << bit_idx) & mask);
	}

	template <std::unsigned_integral t>
	FORCE_INLINE constexpr t
	get_bits(t value, uint32 bit_idx, uint32 bit_count) noexcept
	{
		return (value >> bit_idx) & ((t(1) << bit_count) - t(1));
	}
}	 // namespace age::util
#pragma once
#include "age.hpp"

namespace age::buffer
{
	namespace detail
	{
		FORCE_INLINE std::size_t
		calc_size(auto&& v) noexcept
		{
			if constexpr (std::ranges::range<BARE_OF(v)>)
			{
				return sizeof(std::ranges::range_value_t<BARE_OF(v)>) * static_cast<std::size_t>(std::ranges::size(v));
			}
			else
			{
				return sizeof(v);
			}
		}
	}	 // namespace detail

	template <typename t>
	FORCE_INLINE void
	write_bytes(t* p_dst, auto&& v) noexcept
		requires std::is_trivially_copyable_v<BARE_OF(v)>
	{
		std::memcpy(p_dst, &v, sizeof(v));
	}

	template <typename t>
	FORCE_INLINE std::size_t
	write_bytes(t* p_dst, std::ranges::range auto&& rg) noexcept
		requires meta::cx_byte_writable_contiguous_range<decltype(rg)>
	{
		const std::size_t byte_size =
			sizeof(std::ranges::range_value_t<decltype(rg)>) * static_cast<std::size_t>(std::ranges::size(FWD(rg)));

		std::memcpy(p_dst, std::ranges::data(rg), byte_size);
		return byte_size;
	}

	template <typename t>
	FORCE_INLINE std::size_t
	write_bytes(t* p_dst, auto&&... v) noexcept
		requires(sizeof...(v) > 1)
			and ((std::is_trivially_copyable_v<BARE_OF(v)> or meta::cx_byte_writable_contiguous_range<decltype(v)>) and ...)
	{
		auto offset = 0ull;
		((offset += write_bytes((std::byte*)p_dst + offset, v)), ...);
		return offset;
	}

	FORCE_INLINE decltype(auto)
	write_bytes(auto&&... v) noexcept
		requires(sizeof...(v) > 0)
			and ((std::is_trivially_copyable_v<BARE_OF(v)> or meta::cx_byte_writable_contiguous_range<decltype(v)>) and ...)
	{

		const std::size_t byte_size = (detail::calc_size(v) + ...);

		auto arr = age::dynamic_array<std::byte>::gen_sized_default(byte_size);


		auto offset = 0ull;
		((offset += write_bytes(arr.data() + offset, v)), ...);

		AGE_ASSERT(offset == byte_size);

		return arr;
	}
}	 // namespace age::buffer
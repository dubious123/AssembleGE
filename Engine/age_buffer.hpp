#pragma once
#include "age.hpp"

namespace age::buffer
{
	namespace detail
	{
		FORCE_INLINE constexpr std::size_t
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

		template <std::size_t align>
		inline constexpr bool check_align = std::has_single_bit(align) or align == 1;
	}	 // namespace detail

	template <std::size_t align = 1, typename t>
	FORCE_INLINE std::size_t
	write_bytes(t* p_dst, auto&& v) noexcept
		requires std::is_trivially_copyable_v<BARE_OF(v)>
	{
		static_assert(detail::check_align<align>, "align must be power of 2");

		std::memcpy(p_dst, &v, sizeof(v));
		return util::align_up(sizeof(v), align);
	}

	template <std::size_t align = 1, typename t>
	FORCE_INLINE std::size_t
	write_bytes(t* p_dst, std::ranges::range auto&& rg) noexcept
		requires meta::cx_byte_writable_contiguous_range<decltype(rg)>
	{
		static_assert(detail::check_align<align>, "align must be power of 2");

		const auto byte_size =
			sizeof(std::ranges::range_value_t<decltype(rg)>) * static_cast<std::size_t>(std::ranges::size(FWD(rg)));

		std::memcpy(p_dst, std::ranges::data(rg), byte_size);

		return util::align_up(byte_size, align);
	}

	template <std::size_t align = 1, typename t>
	FORCE_INLINE std::size_t
	write_bytes(t* p_dst, auto&&... v) noexcept
		requires(sizeof...(v) > 1)
			and ((std::is_trivially_copyable_v<BARE_OF(v)> or meta::cx_byte_writable_contiguous_range<decltype(v)>) and ...)
	{
		static_assert(detail::check_align<align>, "align must be power of 2");

		auto offset = 0ull;
		((offset += write_bytes<align>((std::byte*)p_dst + offset, FWD(v))), ...);
		return offset;
	}

	template <std::size_t align = 1>
	FORCE_INLINE decltype(auto)
	write_bytes(auto&&... v) noexcept
		requires(sizeof...(v) > 0)
			and ((std::is_trivially_copyable_v<BARE_OF(v)> or meta::cx_byte_writable_contiguous_range<decltype(v)>) and ...)
	{
		static_assert(detail::check_align<align>, "align must be power of 2");

		const auto byte_size = (util::align_up(detail::calc_size(v), align) + ...);

		auto arr = age::dynamic_array<std::byte>::gen_sized_default(byte_size);

		auto offset = 0ull;
		((offset += write_bytes<align>(arr.data() + offset, FWD(v))), ...);

		AGE_ASSERT(offset == byte_size);

		return arr;
	}
}	 // namespace age::buffer

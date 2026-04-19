#pragma once

// string -> value
namespace age::util
{
	consteval std::size_t
	str_to_uint64(std::string_view str)
	{
		auto res = std::size_t{ 0 };

		for (auto c : str)
		{
			if (c < '0' or c > '9')
			{
				throw "invalid character in decimal literal";
			}

			c_auto digit =
				static_cast<std::uint64_t>(c - '0');

			if (res > (std::numeric_limits<std::uint64_t>::max() - digit) / 10)
				throw "str_to_uint64: overflow";

			res = res * 10 + digit;
		}

		return res;
	}
}	 // namespace age::util

// value -> string
namespace age::util
{
	namespace detail
	{
		template <std::size_t n, std::size_t overhead>
		consteval float
		threshold() noexcept
		{
			if constexpr (n - overhead >= 38)
			{
				return std::numeric_limits<float>::max();
			}
			else
			{
				auto v = 1.f;
				for (std::size_t i = 0; i < n - overhead; ++i)
				{
					v *= 10.f;
				}
				return v;
			}
		}
	}	 // namespace detail

	template <std::size_t n, int32 precision = 2>
	constexpr void
	float_to_str(char (&buf)[n], float value) noexcept
	{
		static_assert(n >= 12, "buffer too small for scientific notation");

		constexpr auto fixed_overhead = 1 + 1 + precision + 1;	  // sign + dot + decimals + null

		c_auto fmt = (std::abs(value) < detail::threshold<n, fixed_overhead>())
					   ? std::chars_format::fixed
					   : std::chars_format::scientific;

		auto [p_char, ec] = std::to_chars(buf, buf + n, value, fmt, precision);
		AGE_ASSERT(ec == std::errc{});

		while (p_char > buf + 2 and *(p_char - 1) == '0')
		{
			--p_char;
		}
		if (*(p_char - 1) == '.')
		{
			++p_char;
		}

		*p_char = '\0';
	}

	template <std::size_t n, int32 base = 10>
	constexpr void
	integral_to_str(char (&buf)[n], std::integral auto value) noexcept
	{
		if constexpr (sizeof(value) <= sizeof(uint32))
		{
			static_assert(n >= 12, "buffer too small");
		}
		else if constexpr (sizeof(value) <= sizeof(uint64))
		{
			static_assert(n >= 21, "buffer too small");
		}

		auto [ptr, ec] = std::to_chars(buf, buf + n - 1, value, base);
		AGE_ASSERT(ec == std::errc{});
		*ptr = '\0';
	}
}	 // namespace age::util

namespace age::util
{
	template <typename t, std::size_t n>
	constexpr void
	to_str(char (&buf)[n], t value) noexcept
	{
		if constexpr (std::is_floating_point_v<t>)
		{
			float_to_str(buf, value);
		}
		else
		{
			integral_to_str(buf, value);
		}
	}

	// template <int32 base = 10, typename t, std::size_t n, std::size_t p>
	// constexpr void
	// to_str(char (&buf)[n], t value, const char (&prefix)[p]) noexcept
	//{
	//	constexpr auto len = p - 1;
	//	std::memcpy(buf, prefix, len);

	//	if constexpr (std::is_floating_point_v<t>)
	//	{
	//		auto [ptr, ec] = std::to_chars(buf + len, buf + n - 1, value);
	//		AGE_ASSERT(ec == std::errc{});
	//		*ptr = '\0';
	//	}
	//	else
	//	{
	//		auto [ptr, ec] = std::to_chars(buf + len, buf + n - 1, value, base);
	//		AGE_ASSERT(ec == std::errc{});
	//		*ptr = '\0';
	//	}
	//}

	template <int32 base = 10, std::size_t digits = 0, typename t, std::size_t n, std::size_t p>
	constexpr void
	to_str(char (&buf)[n], t value, const char (&prefix)[p]) noexcept
	{
		constexpr auto len = p - 1;
		std::memcpy(buf, prefix, len);

		char tmp[24];
		auto [ptr, ec] = std::to_chars(tmp, tmp + sizeof(tmp), value, base);
		AGE_ASSERT(ec == std::errc{});

		if constexpr (digits > 0)
		{
			c_auto written = static_cast<std::size_t>(ptr - tmp);
			c_auto pad	   = (digits > written) ? digits - written : 0;
			std::memset(buf + len, '0', pad);
			std::memcpy(buf + len + pad, tmp, written);
			buf[len + pad + written] = '\0';
		}
		else
		{
			c_auto written = static_cast<std::size_t>(ptr - tmp);
			std::memcpy(buf + len, tmp, written);
			buf[len + written] = '\0';
		}
	}

	template <typename t, std::size_t n>
	constexpr bool
	from_str(char (&buf)[n], t& value) noexcept
	{
		auto result		= t{};
		c_auto[ptr, ec] = std::from_chars(buf, buf + std::strlen(buf), result);

		if (ec == std::errc{})
		{
			value = result;
			return true;
		}

		return false;
	}
}	 // namespace age::util

namespace age::util
{
	FORCE_INLINE constexpr uint32
	encode_utf8(char* p_dst, uint32 unicode)
	{
		if (unicode < 0x80)
		{
			p_dst[0] = static_cast<char>(unicode);
			return 1;
		}
		else if (unicode < 0x800)
		{
			p_dst[0] = static_cast<char>(0xC0 | (unicode >> 6));
			p_dst[1] = static_cast<char>(0x80 | (unicode & 0x3F));
			return 2;
		}
		else if (unicode < 0x10000)
		{
			p_dst[0] = static_cast<char>(0xE0 | (unicode >> 12));
			p_dst[1] = static_cast<char>(0x80 | ((unicode >> 6) & 0x3F));
			p_dst[2] = static_cast<char>(0x80 | (unicode & 0x3F));
			return 3;
		}
		else
		{
			p_dst[0] = static_cast<char>(0xF0 | (unicode >> 18));
			p_dst[1] = static_cast<char>(0x80 | ((unicode >> 12) & 0x3F));
			p_dst[2] = static_cast<char>(0x80 | ((unicode >> 6) & 0x3F));
			p_dst[3] = static_cast<char>(0x80 | (unicode & 0x3F));
			return 4;
		}
	}

	template <std::size_t n_dst, std::size_t n_src>
	FORCE_INLINE constexpr uint32
	encode_utf8(char (&dst)[n_dst], const uint32 (&src)[n_src], uint32 count)
	{
		static_assert(n_dst >= n_src * 4 + 1, "dst too small for worst case");
		AGE_ASSERT(count <= n_src);

		auto written = 0u;
		for (auto i : std::views::iota(0u) | std::views::take(count))
		{
			written += encode_utf8(dst + written, src[i]);
		}
		dst[written] = '\0';
		return written;
	}

	FORCE_INLINE constexpr std::tuple<uint8, uint16>
	decode_utf8(const char* p)
	{
		auto c = static_cast<uint8>(p[0]);

		if (c < 0x80)			   // 1 byte (ASCII)
		{
			return { 1, c };
		}
		if ((c & 0xE0) == 0xC0)	   // 2 bytes
		{
			uint16 unicode	= (c & 0x1F) << 6;
			unicode		   |= (static_cast<uint8>(p[1]) & 0x3F);
			return { 2, unicode };
		}
		if ((c & 0xF0) == 0xE0)	   // 3 bytes
		{
			uint16 unicode	= (c & 0x0F) << 12;
			unicode		   |= (static_cast<uint8>(p[1]) & 0x3F) << 6;
			unicode		   |= (static_cast<uint8>(p[2]) & 0x3F);
			return { 3, unicode };
		}
		// if ((c & 0xF8) == 0xF0)	   // 4 bytes
		//{
		//	uint32 cp  = (c & 0x07) << 18;
		//	cp		  |= (static_cast<uint8>(*++p) & 0x3F) << 12;
		//	cp		  |= (static_cast<uint8>(*++p) & 0x3F) << 6;
		//	cp		  |= (static_cast<uint8>(*++p) & 0x3F);
		//	++p;
		//	return cp;
		// }

		return { 1, 0xFFFD };	 // replacement character
	}
}	 // namespace age::util

namespace age::util
{
	template <std::size_t len, std::size_t n>
	consteval auto
	to_fixed_str(const char (&str)[n])
	{
		static_assert(n <= len, "string exceeds max length");

		auto res = std::array<char, len>{};

		for (std::size_t i = 0; i < n; ++i)
		{
			res[i] = str[i];
		}

		return res;
	}

	template <std::size_t len>
	constexpr auto
	to_fixed_str(std::string_view sv) noexcept
	{
		AGE_ASSERT(sv.size() < len);
		auto res = std::array<char, len>{};
		std::ranges::copy_n(sv.data(), sv.size(), res.begin());
		res[sv.size()] = '\0';
		return res;
	}

	template <std::size_t len, std::size_t... n>
	consteval auto
	to_fixed_str_arr(const char (&... strs)[n])
	{
		return std::array{ to_fixed_str<len>(strs)... };
	}

	template <std::size_t len>
	consteval auto
	to_fixed_str_arr()
	{
		return std::array<std::array<const char, len>, 0>{};
	}
}	 // namespace age::util

namespace age::util
{
	template <std::size_t n>
	struct nttp_string_holder
	{
		// char data[n];

		consteval nttp_string_holder(const char (&str)[n])
		{
			// std::copy_n(str, n, data);
		}

		// constexpr
		// operator std::string_view() const
		//{
		//	return { data, n - 1 };
		//}
	};

	template <std::size_t n>
	nttp_string_holder(const char (&)[n]) -> nttp_string_holder<n>;
}	 // namespace age::util
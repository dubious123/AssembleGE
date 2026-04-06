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
			auto v = 1.f;
			for (std::size_t i = 0; i < n - overhead; ++i)
			{
				v *= 10.f;
			}
			return v;
		}
	}	 // namespace detail

	template <std::size_t n, int32 precision = 2>
	constexpr void
	float_to_str(char (&buf)[n], float value)
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
	integral_to_str(char (&buf)[n], std::integral auto value)
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
	to_str(char (&buf)[n], t value)
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
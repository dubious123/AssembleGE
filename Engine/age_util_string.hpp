#pragma once

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
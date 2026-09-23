#pragma once

namespace age::util
{
	template <std::size_t n, typename... t_args>
	[[nodiscard]] age::array<char, n>
	fixed_format(std::format_string<t_args...> fmt, t_args&&... args) noexcept
	{
		auto   res		  = age::array<char, n>{};
		c_auto format_res = std::format_to_n(res.data(), n - 1, fmt, FWD(args)...);


		res[std::min(static_cast<std::size_t>(format_res.size), n - 1)] = '\0';
		return res;
	}
}	 // namespace age::util
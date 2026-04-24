#pragma once

namespace std
{
	template <>
	struct hash<std::array<char, age::config::max_asset_path_len>>
	{
		size_t
		operator()(c_auto& arr) const noexcept
		{
			c_auto len = ::strnlen(arr.data(), arr.size());
			return std::hash<std::string_view>{}({ arr.data(), len });
		}
	};


}	 // namespace std

namespace age
{
	template <typename t>
	struct hash : public std::hash<t>
	{ };

	template <>
	struct hash<std::string_view>
	{
		constexpr std::size_t
		operator()(std::string_view sv) const noexcept
		{
			constexpr std::size_t fnv_offset = 14695981039346656037ULL;
			constexpr std::size_t fnv_prime	 = 1099511628211ULL;

			std::size_t h = fnv_offset;
			for (char c : sv)
			{
				h ^= static_cast<std::size_t>(static_cast<unsigned char>(c));
				h *= fnv_prime;
			}
			return h;
		}
	};
}	 // namespace age
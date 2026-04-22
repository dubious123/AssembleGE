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

namespace age::util
{

}
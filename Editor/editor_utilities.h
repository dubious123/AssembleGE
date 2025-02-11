#pragma once

namespace editor::utilities
{
	inline constexpr auto deref_view = std::views::transform([](auto ptr) -> decltype(*ptr) { return *ptr; });

	std::string read_file(const std::filesystem::path path);
	void		create_file(const std::filesystem::path path, const std::string content);

	std::vector<std::string> split_string(const std::string& str, const std::string& delims);

	std::wstring str_to_wstr(std::string);
}	 // namespace editor::utilities
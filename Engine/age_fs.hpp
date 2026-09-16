#pragma once
#include "age.hpp"

namespace age::fs
{
	bool
	dir_exists(std::string_view path) noexcept;

	bool
	file_exists(std::string_view path) noexcept;

	bool
	is_dir_empty(std::string_view path) noexcept;

	bool
	rename(std::string_view from, std::string_view to) noexcept;

	bool
	create_dir(std::string_view path) noexcept;

	// also create file
	bool
	write_file(std::string_view path, std::span<const std::byte> bytes) noexcept;

	bool
	write_file(std::string_view path, const age::byte_buf& buf) noexcept;

	bool
	remove_dir(std::string_view path) noexcept;

	bool
	remove_file(std::string_view path) noexcept;

	// if exists(path) is_false, return true
	bool
	can_overwrite_file(std::string_view path) noexcept;
}	 // namespace age::fs

// util
namespace age::fs
{
	// "a/b/name.exe" -> "name.exe",  "name.exe" -> "name.exe",  "a/b/" -> ""
	constexpr std::string_view
	get_file_name(std::string_view path) noexcept
	{
		c_auto sep = path.find_last_of("/\\");
		return sep == std::string_view::npos ? path : path.substr(sep + 1);
	}

	// "a/b/name.exe" -> "name",  "a/b/.hidden" -> ".hidden",  "a/b/name" -> "name"
	constexpr std::string_view
	get_file_stem(std::string_view path) noexcept
	{
		c_auto name = get_file_name(path);
		c_auto dot	= name.find_last_of('.');
		return dot == std::string_view::npos or dot == 0 ? name : name.substr(0, dot);
	}

	// "a/b/name.exe" -> ".exe",  "a/b/name" -> "",  "a/b/.hidden" -> ""
	constexpr std::string_view
	get_file_extension(std::string_view path) noexcept
	{
		c_auto name = get_file_name(path);
		c_auto dot	= name.find_last_of('.');
		return dot == std::string_view::npos or dot == 0 ? std::string_view{} : name.substr(dot);
	}
}	 // namespace age::fs
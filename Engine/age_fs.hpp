#pragma once
#include "age.hpp"

namespace age::fs
{
	bool
	exists(std::string_view path) noexcept;

	bool
	dir_exists(std::string_view path) noexcept;

	bool
	file_exists(std::string_view path) noexcept;

	// false if the path does not exist or is not a directory
	bool
	is_dir_empty(std::string_view path) noexcept;

	// size in bytes of a regular file. nullopt if path is missing, a dir, or the query fails
	std::tuple<bool, uint64>
	get_file_size(std::string_view path) noexcept;

	bool
	rename(std::string_view from, std::string_view to) noexcept;

	bool
	create_dir(std::string_view path) noexcept;

	// also create file
	bool
	write_file(std::string_view path, std::span<const std::byte> bytes) noexcept;

	bool
	write_file(std::string_view path, const age::byte_buf& buf) noexcept;

	byte_buf
	read_file(std::string_view path) noexcept;

	std::tuple<bool, uint64>
	read_file(std::string_view path, std::span<std::byte> buf, uint64 file_size) noexcept;

	bool
	remove_dir(std::string_view path) noexcept;

	bool
	remove_file(std::string_view path) noexcept;

	// if exists(path) is_false, return true
	bool
	can_overwrite_file(std::string_view path) noexcept;

	std::tuple<bool, std::chrono::file_clock::time_point>
	get_last_write_time(std::string_view path) noexcept;

	struct file_entry
	{
		std::string_view					path;	 // utf8, '/' separators. valid only during the callback
		std::string_view					name;	 // file name part of path
		std::chrono::file_clock::time_point last_write_time;
		uint64								size;
	};

	void
	for_each_file(std::string_view dir, util::function_ref<void(const file_entry&)> fn) noexcept;

	void
	for_each_file_recursive(std::string_view dir, util::function_ref<void(const file_entry&)> fn) noexcept;

	struct entry
	{
		std::string_view path;	  // utf8, '/' separators. valid only during the callback
		std::string_view name;
		bool			 is_dir;
	};

	void
	for_each_entry(std::string_view dir, util::function_ref<void(const entry&)> fn) noexcept;	 // files and dirs, not recursive
	std::string
	get_current_dir() noexcept;
}	 // namespace age::fs

namespace age::fs::detail
{
#if defined(AGE_PLATFORM_WINDOW)

	void
	to_utf16(std::string_view sv, AGE_OUT std::wstring& res) noexcept;
	std::wstring
	to_utf16(std::string_view sv) noexcept;
	void
	to_utf8(std::wstring_view wide, bool to_generic, AGE_OUT std::string& res) noexcept;
	std::string
	to_utf8(std::wstring_view wide, bool to_generic = true) noexcept;
#endif

	// appends rhs to res with one '/' between. same rules as join(lhs, rhs)
	void
	append_path(AGE_INOUT std::string& res, std::string_view rhs) noexcept;
}	 // namespace age::fs::detail

// util
namespace age::fs
{
	std::string
	normalize_path(std::string_view path) noexcept;

	bool
	is_absolute(std::string_view path) noexcept;

	std::string
	join(std::string_view lhs, std::string_view rhs) noexcept;

	std::string
	join(auto&&... args) noexcept
		requires(sizeof...(args) > 2)
	{
		auto res = std::string{};
		res.reserve((std::string_view{ args }.size() + ...) + sizeof...(args));
		(detail::append_path(AGE_INOUT res, std::string_view{ args }), ...);
		return res;
	}

	// "a/b/name.exe" -> "name.exe",  "name.exe" -> "name.exe",  "a/b/" -> ""
	constexpr std::string_view
	get_file_name(std::string_view path) noexcept
	{
		c_auto sep = path.find_last_of("/\\");
		return sep == std::string_view::npos ? path : path.substr(sep + 1);
	}

	// "a/b/c" -> "a/b",  "a/b/" -> "a/b",  "a" -> "",  "/a" -> "/",  "C:/a" -> "C:/",  "C:/" -> "C:/"
	constexpr std::string_view
	get_parent_path(std::string_view path) noexcept
	{
		// ignore one trailing separator, "a/b/" -> "a/b"
		if (path.size() > 1 and (path.back() == '/' or path.back() == '\\'))
		{
			path.remove_suffix(1);
		}

		c_auto sep = path.find_last_of("/\\");
		if (sep == std::string_view::npos)
		{
			return {};
		}

		// keep the root separator : "/a" -> "/",  "C:/a" -> "C:/"
		c_auto is_root = sep == 0 or (sep == 2 and path[1] == ':');
		return path.substr(0, is_root ? sep + 1 : sep);
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
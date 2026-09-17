#include "age_pch.hpp"
#include "age.hpp"

namespace age::fs::detail
{
	namespace unicode
	{
		inline constexpr uint32 replacement			= 0xFFFD;	   // "<?>", decode failed indicator
		inline constexpr uint32 max_code_point		= 0x10FFFF;	   // unicode_code_point_max
		inline constexpr uint32 bmp_end				= 0x10000;	   // end of Basic Multilingual Plane, less than bmp : 1 wchar_t, greater or equal than bmp : 2 wchar_t
		inline constexpr uint32 high_surrogate_base = 0xD800;	   // D800..DBFF
		inline constexpr uint32 low_surrogate_base	= 0xDC00;	   // DC00..DFFF
		inline constexpr uint32 surrogate_begin		= 0xD800;
		inline constexpr uint32 surrogate_end		= 0xDFFF;

		struct utf8_char
		{
			uint32 code_point;
			uint32 byte_count;
			bool   is_valid;
		};

		struct utf16_char
		{
			uint32 code_point;
			uint32 unit_count;	  // count of char16_t
			bool   is_valid;
		};

		// utf8 -> code_point
		// safer version than util::decode_utf8, also support len 4
		constexpr utf8_char
		decode_utf8(std::string_view sv) noexcept
		{
			c_auto res_failed = utf8_char{ replacement, 1, false };

			if (sv.empty()) { return res_failed; }

			c_auto c = static_cast<unsigned char>(sv[0]);

			// 0xxxxxxx : 1, 110xxxxx : 2, 1110xxxx : 3, 11110xxx : 4
			uint32 code_point;
			uint32 min_code_point;
			uint32 byte_count;

			if (c < 0b1000'0000)
			{
				return { c, 1, true };
			}
			else if ((c & 0b1110'0000) == 0b1100'0000)
			{
				byte_count	   = 2;
				min_code_point = 0x80;
				code_point	   = c & 0b0001'1111;
			}
			else if ((c & 0b1111'0000) == 0b1110'0000)
			{
				byte_count	   = 3;
				min_code_point = 0x800;
				code_point	   = c & 0b0000'1111;
			}
			else if ((c & 0b1111'1000) == 0b1111'0000)
			{
				byte_count	   = 4;
				min_code_point = bmp_end;
				code_point	   = c & 0b0000'0111;
			}
			else
			{
				return res_failed;
			}

			if (sv.size() < byte_count)
			{
				return res_failed;
			}

			for (auto i = 1u; i < byte_count; ++i)
			{
				c_auto c_i = static_cast<unsigned char>(sv[i]);
				if ((c_i & 0b1100'0000) != 0b1000'0000)
				{
					return res_failed;
				}

				code_point = (code_point << 6) | (c_i & 0b0011'1111);
			}

			if ((code_point >= surrogate_begin and code_point <= surrogate_end)
				or code_point < min_code_point
				or code_point > max_code_point) { return res_failed; }

			return { code_point, byte_count, true };
		}

		// utf16 -> code_point
		// lone or reversed surrogates -> replacement
		constexpr utf16_char
		decode_utf16(std::u16string_view sv) noexcept
		{
			c_auto res_failed = utf16_char{ replacement, 1, false };

			if (sv.empty()) { return res_failed; }

			c_auto c = static_cast<uint32>(sv[0]);

			// not a surrogate
			if (c < surrogate_begin or c > surrogate_end)
			{
				return { c, 1, true };
			}

			// low surrogate without a preceding high surrogate
			if (c >= low_surrogate_base)
			{
				return res_failed;
			}

			if (sv.size() < 2)
			{
				return res_failed;
			}

			c_auto c_lo = static_cast<uint32>(sv[1]);
			if (c_lo < low_surrogate_base or c_lo > surrogate_end)
			{
				return res_failed;
			}

			return { bmp_end + ((c - high_surrogate_base) << 10) + (c_lo - low_surrogate_base), 2, true };
		}

		// safer version than util::encode_utf8
		void
		encode_utf8(uint32 code_point, AGE_OUT std::string& res) noexcept
		{
			// 1 byte : 0xxxxxxx
			if (code_point < 0x80)
			{
				res.push_back(static_cast<char>(code_point));
				return;
			}

			// 2 bytes : 110xxxxx 10xxxxxx
			if (code_point < 0x800)
			{
				res.push_back(static_cast<char>(0b1100'0000 | (code_point >> 6)));
				res.push_back(static_cast<char>(0b1000'0000 | (code_point & 0b0011'1111)));
				return;
			}

			// surrogate or out of range code point -> replacement
			if ((code_point >= surrogate_begin and code_point <= surrogate_end) or code_point > max_code_point)
			{
				code_point = replacement;
			}

			// 3 bytes : 1110xxxx 10xxxxxx 10xxxxxx
			if (code_point < bmp_end)
			{
				res.push_back(static_cast<char>(0b1110'0000 | (code_point >> 12)));
				res.push_back(static_cast<char>(0b1000'0000 | ((code_point >> 6) & 0b0011'1111)));
				res.push_back(static_cast<char>(0b1000'0000 | (code_point & 0b0011'1111)));
				return;
			}

			// 4 bytes : 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
			res.push_back(static_cast<char>(0b1111'0000 | (code_point >> 18)));
			res.push_back(static_cast<char>(0b1000'0000 | ((code_point >> 12) & 0b0011'1111)));
			res.push_back(static_cast<char>(0b1000'0000 | ((code_point >> 6) & 0b0011'1111)));
			res.push_back(static_cast<char>(0b1000'0000 | (code_point & 0b0011'1111)));
		}

		void
		encode_utf16(uint32 code_point, AGE_OUT std::wstring& res) noexcept
		{
			if (code_point < bmp_end)
			{
				res.push_back(static_cast<wchar_t>(code_point));
				return;
			}

			c_auto v = code_point - bmp_end;
			res.push_back(static_cast<wchar_t>(high_surrogate_base | (v >> 10)));	  // high_surrogate_base | upper 10 bits
			res.push_back(static_cast<wchar_t>(low_surrogate_base | (v & 0x3FF)));	  // low_surrogate_base  | lower 10 bits
		}
	}	 // namespace unicode

#if defined(AGE_PLATFORM_WINDOW)
	void
	to_utf16(std::string_view sv, AGE_OUT std::wstring& res) noexcept
	{
		static_assert(sizeof(wchar_t) == 2, "utf16 wchar_t expected");

		res.clear();
		res.reserve(sv.size());

		while (sv.empty() is_false)
		{
			c_auto ch = unicode::decode_utf8(sv);
			sv.remove_prefix(ch.byte_count);
			unicode::encode_utf16(ch.code_point, AGE_OUT res);
		}
	}

	std::wstring
	to_utf16(std::string_view sv) noexcept
	{
		auto res = std::wstring{};
		to_utf16(sv, AGE_OUT res);
		return res;
	}

	void
	to_utf8(std::wstring_view wide, bool to_generic, AGE_OUT std::string& res) noexcept
	{
		static_assert(sizeof(wchar_t) == 2, "utf16 wchar_t expected");

		res.clear();
		res.reserve(wide.size() * 3);	 // worst case, 1 utf16 -> 3 utf8

		for (auto sv_u16 = std::u16string_view{ reinterpret_cast<const char16_t*>(wide.data()), wide.size() };
			 sv_u16.empty() is_false;)
		{
			c_auto ch = unicode::decode_utf16(sv_u16);
			sv_u16.remove_prefix(ch.unit_count);
			if (to_generic and ch.code_point == U'\\')
			{
				res.push_back('/');
			}
			else
			{
				unicode::encode_utf8(ch.code_point, res);
			}
		}
	}

	std::string
	to_utf8(std::wstring_view wide, bool to_generic) noexcept
	{
		auto res = std::string{};
		to_utf8(wide, to_generic, AGE_OUT res);
		return res;
	}
#endif

	// path -> utf8 with '/' separators, into a reused string
	void
	to_utf8_generic(const std::filesystem::path& p, AGE_OUT std::string& res) noexcept
	{
#if defined(AGE_PLATFORM_WINDOW)
		to_utf8(p.native(), true, AGE_OUT res);
#else
		res = p.native();
#endif
	}

	// path -> utf8 with '/' separators, into a reused string
	std::string
	to_utf8_generic(const std::filesystem::path& p) noexcept
	{
#if defined(AGE_PLATFORM_WINDOW)
		return to_utf8(p.native(), true);
#else
		return p.native();
#endif
	}

	template <std::size_t slot>
	const std::filesystem::path&
	get_scratch_path(std::string_view sv) noexcept
	{
		thread_local auto wide = std::wstring{};
		thread_local auto p	   = std::filesystem::path{};
#if defined(AGE_PLATFORM_WINDOW)
		to_utf16(sv, AGE_OUT wide);
		p.assign(wide.begin(), wide.end());
#else
		p.assign(sv.begin(), sv.end());
#endif
		return p;
	}

	void
	append_path(AGE_INOUT std::string& res, std::string_view rhs) noexcept
	{
		if (rhs.empty())
		{
			return;
		}
		if (res.empty())
		{
			res.assign(rhs);
			return;
		}
		if (res.back() != '/' and res.back() != '\\')
		{
			res += '/';
		}
		res += rhs;
	}
}	 // namespace age::fs::detail

namespace age::fs
{
	bool
	exists(std::string_view path) noexcept
	{
		auto ec = std::error_code{};
		return std::filesystem::exists(detail::get_scratch_path<0>(path), ec);
	}

	bool
	dir_exists(std::string_view path) noexcept
	{
		auto   ec	  = std::error_code{};
		c_auto status = std::filesystem::status(detail::get_scratch_path<0>(path), ec);
		if (ec) { return false; }

		return std::filesystem::is_directory(status);
	}

	bool
	file_exists(std::string_view path) noexcept
	{
		auto ec = std::error_code{};
		return std::filesystem::is_regular_file(detail::get_scratch_path<0>(path), ec);
	}

	// false if the path does not exist or is not a directory
	bool
	is_dir_empty(std::string_view path) noexcept
	{
		auto	ec = std::error_code{};
		c_auto& p  = detail::get_scratch_path<0>(path);
		if (std::filesystem::is_directory(p, ec) is_false or ec) { return false; }

		c_auto empty = std::filesystem::is_empty(p, ec);
		if (ec) { return false; }

		return empty;
	}

	// size in bytes of a regular file. nullopt if path is missing, a dir, or the query fails
	std::tuple<bool, uint64>
	get_file_size(std::string_view path) noexcept
	{
		auto   ec	= std::error_code{};
		c_auto size = std::filesystem::file_size(detail::get_scratch_path<0>(path), ec);
		if (ec)
		{
			return { false, 0uz };
		}
		return { true, static_cast<uint64>(size) };
	}

	bool
	rename(std::string_view from, std::string_view to) noexcept
	{
		auto ec = std::error_code{};
		std::filesystem::rename(detail::get_scratch_path<0>(from), detail::get_scratch_path<1>(to), ec);
		return not ec;
	}

	bool
	create_dir(std::string_view path) noexcept
	{
		auto ec = std::error_code{};
		std::filesystem::create_directories(detail::get_scratch_path<0>(path), ec);
		return not ec;
	}

	bool
	write_file(std::string_view path, std::span<const std::byte> bytes) noexcept
	{
		if (c_auto parent = get_parent_path(path); parent.empty() is_false)
		{
			auto ec = std::error_code{};
			std::filesystem::create_directories(detail::get_scratch_path<1>(parent), ec);
			if (ec) { return false; }
		}

		auto file = std::ofstream{ detail::get_scratch_path<0>(path), std::ios::binary | std::ios::trunc };
		if (file.is_open() is_false) { return false; }

		file.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));

		file.close();
		return file.good();
	}

	bool
	write_file(std::string_view path, std::span<const std::span<const std::byte>> chunks) noexcept
	{
		if (c_auto parent = get_parent_path(path); parent.empty() is_false)
		{
			auto ec = std::error_code{};
			std::filesystem::create_directories(detail::get_scratch_path<1>(parent), ec);
			if (ec) { return false; }
		}

		auto file = std::ofstream{ detail::get_scratch_path<0>(path), std::ios::binary | std::ios::trunc };
		if (file.is_open() is_false) { return false; }

		for (c_auto& chunk : chunks)
		{
			file.write(reinterpret_cast<const char*>(chunk.data()), static_cast<std::streamsize>(chunk.size()));
		}

		file.close();
		return file.good();
	}

	bool
	write_file(std::string_view path, std::initializer_list<std::span<const std::byte>> chunks) noexcept
	{
		return write_file(path, std::span{ chunks.begin(), chunks.size() });
	}

	bool
	write_file(std::string_view path, const age::byte_buf& buf) noexcept
	{
		return write_file(path, std::span<const std::byte>{ buf.data(), buf.size() });
	}

	byte_buf
	read_file(std::string_view path) noexcept
	{
		auto buf = byte_buf{};

		c_auto[success, file_size] = fs::get_file_size(path);

		if (success is_false or file_size == 0)
		{
			return buf;
		}

		auto file = std::ifstream{ detail::get_scratch_path<0>(path), std::ios::in | std::ios::binary };
		if (file.is_open() is_false)
		{
			return buf;
		}

		buf.resize(file_size);

		file.read(reinterpret_cast<char*>(buf.data()), file_size);
		if (static_cast<uint64>(file.gcount()) != file_size)
		{
			buf.clear();
			return buf;	   // nrvo
		}

		buf.move_write_pos(file_size);

		return buf;		   // nrvo
	}

	std::tuple<bool, uint64>
	read_file(std::string_view path, std::span<std::byte> buf, uint64 file_size) noexcept
	{
		AGE_ASSERT(file_size > 0);

		auto file = std::ifstream{ detail::get_scratch_path<0>(path), std::ios::binary };
		if (file.is_open() is_false)
		{
			return { false, 0 };
		}

		file.read(reinterpret_cast<char*>(buf.data()), static_cast<std::streamsize>(file_size));
		if (static_cast<uint64>(file.gcount()) != file_size)
		{
			return { false, 0 };
		}

		return { true, file_size };
	}

	bool
	remove_dir(std::string_view path) noexcept
	{
		auto& p	 = detail::get_scratch_path<0>(path);
		auto  ec = std::error_code{};

		if (std::filesystem::exists(p, ec) and std::filesystem::is_directory(p, ec) is_false)
		{
			return false;
		}

		std::filesystem::remove_all(p, ec);
		return not ec;
	}

	bool
	remove_file(std::string_view path) noexcept
	{
		auto& p	 = detail::get_scratch_path<0>(path);
		auto  ec = std::error_code{};

		if (std::filesystem::is_directory(p, ec))
		{
			return false;
		}

		std::filesystem::remove(p, ec);
		return not ec;
	}

	namespace detail
	{
#if defined(AGE_PLATFORM_WINDOW)
		bool
		can_replace_file(std::string_view path) noexcept
		{
			c_auto& p = get_scratch_path<0>(path);
			c_auto	h = ::CreateFileW(p.c_str(), GENERIC_WRITE | DELETE, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
			if (h == INVALID_HANDLE_VALUE) { return false; }
			::CloseHandle(h);
			return true;
		}
#else
		bool
		can_replace_file(std::string_view path) noexcept
		{
			c_auto sep	  = path.find_last_of('/');
			c_auto parent = sep == std::string_view::npos ? std::string_view{ "." }
						  : sep == 0					  ? std::string_view{ "/" }
														  : path.substr(0, sep);

			return ::access(get_scratch_path<1>(parent).c_str(), W_OK) == 0;
		}
#endif
	}	 // namespace detail

	bool
	can_overwrite_file(std::string_view path) noexcept
	{
		auto& p	 = detail::get_scratch_path<0>(path);
		auto  ec = std::error_code{};

		c_auto st = std::filesystem::status(p, ec);
		if (ec) { return false; }
		if (std::filesystem::exists(st) is_false) { return true; }
		if (std::filesystem::is_directory(st)) { return false; }

		return detail::can_replace_file(path);
	}

	std::tuple<bool, std::chrono::file_clock::time_point>
	get_last_write_time(std::string_view path) noexcept
	{
		auto   ec = std::error_code{};
		c_auto t  = std::filesystem::last_write_time(detail::get_scratch_path<0>(path), ec);
		if (ec)
		{
			return { false, std::move(t) };
		}
		return { true, std::move(t) };
	}

	namespace detail
	{
		// shared body. t_iterator : directory_iterator or recursive_directory_iterator
		template <typename t_iterator>
		void
		for_each_file_impl(std::string_view dir, util::function_ref<void(const file_entry&)> fn) noexcept
		{
			auto path_utf8 = std::string{};	   // reused for every entry, one allocation total
			auto ec		   = std::error_code{};

			for (auto it = t_iterator{ get_scratch_path<0>(dir), ec }; ec.value() == 0 and it != t_iterator{}; it.increment(ec))
			{
				c_auto& entry = *it;

				auto entry_ec = std::error_code{};
				if (entry.is_regular_file(entry_ec) is_false)
				{
					continue;
				}

				detail::to_utf8_generic(entry.path(), path_utf8);
				fn(file_entry{
					.path			 = path_utf8,
					.name			 = get_file_name(path_utf8),
					.last_write_time = entry.last_write_time(entry_ec),
					.size			 = static_cast<uint64>(entry.file_size(entry_ec)) });
			}
		}
	}	 // namespace detail

	void
	for_each_file(std::string_view dir, util::function_ref<void(const file_entry&)> fn) noexcept
	{
		detail::for_each_file_impl<std::filesystem::directory_iterator>(dir, fn);
	}

	void
	for_each_file_recursive(std::string_view dir, util::function_ref<void(const file_entry&)> fn) noexcept
	{
		detail::for_each_file_impl<std::filesystem::recursive_directory_iterator>(dir, fn);
	}

	// files and dirs directly under dir. not recursive
	void
	for_each_entry(std::string_view dir, util::function_ref<void(const entry&)> fn) noexcept
	{
		auto path_utf8 = std::string{};	   // reused for every entry, one allocation total
		auto ec		   = std::error_code{};

		for (auto it = std::filesystem::directory_iterator{ detail::get_scratch_path<0>(dir), ec };
			 ec.value() == 0 and it != std::filesystem::directory_iterator{};
			 it.increment(ec))
		{
			auto entry_ec = std::error_code{};
			detail::to_utf8_generic(it->path(), path_utf8);

			fn(entry{
				.path	= path_utf8,
				.name	= get_file_name(path_utf8),
				.is_dir = it->is_directory(entry_ec) });
		}
	}

	// utf8, '/' separators, no trailing '/'. empty on failure
	std::string
	get_current_dir() noexcept
	{
		auto   ec = std::error_code{};
		c_auto p  = std::filesystem::current_path(ec);
		if (ec)
		{
			return {};
		}

		return detail::to_utf8_generic(p);
	}
}	 // namespace age::fs

// util
namespace age::fs
{
	void
	normalize_path(std::string_view path, AGE_OUT std::string& res) noexcept
	{
		c_auto normalized = detail::get_scratch_path<0>(path).lexically_normal();

		detail::to_utf8_generic(normalized, AGE_OUT res);
	}

	std::string
	normalize_path(std::string_view path) noexcept
	{
		auto res = std::string{};
		normalize_path(path, AGE_OUT res);
		return res;
	}

	bool
	is_absolute(std::string_view path) noexcept
	{
		return detail::get_scratch_path<0>(path).is_absolute();
	}
}	 // namespace age::fs
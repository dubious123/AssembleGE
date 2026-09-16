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
	to_utf16(std::string_view sv, std::wstring& res) noexcept
	{
		static_assert(sizeof(wchar_t) == 2, "utf16 wchar_t expected");

		res.clear();
		res.reserve(sv.size());

		while (sv.empty() is_false)
		{
			c_auto ch = unicode::decode_utf8(sv);
			unicode::encode_utf16(ch.code_point, AGE_OUT res);
			sv.remove_prefix(ch.byte_count);
		}
	}
#endif

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
}	 // namespace age::fs::detail

namespace age::fs
{
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
		if (c_auto sep = path.find_last_of("/\\");
			sep != std::string_view::npos and sep > 0)
		{
			auto ec = std::error_code{};
			std::filesystem::create_directories(detail::get_scratch_path<1>(path.substr(0, sep)), ec);
			if (ec) { return false; }
		}

		auto file = std::ofstream{ detail::get_scratch_path<0>(path), std::ios::binary | std::ios::trunc };
		if (file.is_open() is_false) { return false; }

		file.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());

		file.close();
		return file.good();
	}

	bool
	write_file(std::string_view path, const age::byte_buf& buf) noexcept
	{
		return write_file(path, std::span<const std::byte>{ buf.data(), buf.size() });
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
}	 // namespace age::fs
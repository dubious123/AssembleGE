#include "age_pch.hpp"
#include "age.hpp"

namespace age::asset
{
	namespace detail
	{
		bool
		validate_header_common(const file_header& header) noexcept
		{
			static_assert(sizeof(file_header) == 24);

			auto res = true;

			res &= (header.magic == g::asset_header_magic);
			AGE_ASSERT(res);

			res &= (header.header_size >= sizeof(file_header));
			AGE_ASSERT(res);

			res &= (header.file_size >= sizeof(file_header));
			AGE_ASSERT(res);

			res &= (header.version_major == age::config::version_major);
			res &= (header.version_minor == age::config::version_minor);
			AGE_ASSERT(res);

			res &= validate_header(header.asset_kind, header);

			return res;
		}
	}	 // namespace detail

	file_data_aligned
	read_asset_file(std::string_view full_path) noexcept
	{
		auto file_data = file_data_aligned{ asset::file_header{}, aligned_byte_buf{ aligned_byte_allocator{} } };

		auto   ec		 = std::error_code{};
		c_auto file_size = std::filesystem::file_size(full_path, ec);
		if (ec or file_size == 0)
		{
			return file_data;
		}

		auto file = std::ifstream{ std::filesystem::path{ full_path }, std::ios::in | std::ios::binary };
		if (file.is_open() is_false)
		{
			return file_data;
		}

		AGE_ASSERT(sizeof(file_header) <= file_size);

		file.read(reinterpret_cast<char*>(&file_data.header), sizeof(file_header));


		AGE_ASSERT(file_data.header.file_size == file_size);
		AGE_ASSERT(file_data.header.header_size <= file_size);
		if (detail::validate_header_common(file_data.header) is_false)
		{
			return file_data;
		}

		c_auto blob_size = file_size - file_data.header.header_size;
		c_auto align	 = 1ull << file_data.header.blob_alignment_log2;
		file_data.buf	 = aligned_byte_buf::gen_reserved(blob_size, aligned_byte_allocator(align));

		file.read(reinterpret_cast<char*>(file_data.buf.data()), blob_size);

		file_data.buf.move_write_pos(blob_size);

		return file_data;	 // nrvo
	}

	file_data_aligned
	read_asset_file(const age::array<char, config::max_asset_path_len>& full_path) noexcept
	{
		return read_asset_file(full_path.data());
	}

	byte_buf
	read_raw_file(std::string_view full_path) noexcept
	{
		auto buf = byte_buf{};

		auto   ec		 = std::error_code{};
		c_auto file_size = std::filesystem::file_size(full_path, ec);
		if (ec or file_size == 0)
		{
			return buf;
		}

		auto file = std::ifstream{ std::filesystem::path{ full_path }, std::ios::in | std::ios::binary };
		if (file.is_open() is_false)
		{
			return buf;
		}

		buf.resize(file_size);

		file.read(reinterpret_cast<char*>(buf.data()), file_size);

		buf.move_write_pos(file_size);

		return buf;	   // nrvo
	}

	void
	write_asset_file(const std::filesystem::path& file_path, const file_header& header, const void* p_src) noexcept
	{
		auto   ec	  = std::error_code{};
		c_auto parent = file_path.parent_path();
		if (parent.empty() is_false)
		{
			std::filesystem::create_directories(parent, ec);
			AGE_ASSERT(!ec);
		}

		auto file = std::ofstream(file_path, std::ios::out | std::ios::binary | std::ios::trunc);
		AGE_ASSERT(file.is_open());
		AGE_ASSERT(header.file_size > header.header_size);

		file.write(reinterpret_cast<const char*>(&header), header.header_size);
		file.write(reinterpret_cast<const char*>(p_src), header.file_size - header.header_size);

		file.close();
	}

	bool
	write_raw_file(std::string_view full_path, const byte_buf& buf) noexcept
	{
		auto file_path = std::filesystem::path{ full_path };
		auto ec		   = std::error_code{};

		if (c_auto parent = file_path.parent_path();
			parent.empty() is_false)
		{
			std::filesystem::create_directories(parent, ec);
			if (ec) { return false; }
		}


		auto file = std::ofstream(file_path, std::ios::out | std::ios::binary | std::ios::trunc);

		if (file.is_open() is_false) { return false; }

		file.write(reinterpret_cast<const char*>(buf.data()), buf.size());

		return file.good();
	}

	namespace detail
	{
		bool
		is_allowed_char(char c) noexcept
		{
			if (util::is_char_english(c)) { return true; }
			if (util::is_char_number(c)) { return true; }
			if (std::ranges::contains(age::array{ '\\', '_', '/', '.', '-' }, c)) { return true; }
			return false;
		}

		bool
		is_reserved_word(std::string_view sv) noexcept
		{
			for (auto str_reserved : std::array{ "con", "prn", "aux", "nul" })
			{
				if (sv == str_reserved) { return true; }
			}

			if (sv.size() == 4)
			{
				if (sv.starts_with("com") and util::is_char_number(sv.back())) { return true; }
				if (sv.starts_with("lpt") and util::is_char_number(sv.back())) { return true; }
			}

			return false;
		}
	}	 // namespace detail

	void
	normalize_asset_path(e::kind e_kind, age::array<char, config::max_asset_path_len>& full_path) noexcept
	{
		c_auto		   asset_tag = asset::visit(e_kind, []<e::kind k> { return std::string_view{ asset::get_asset_tag<k>() }; });
		constexpr auto asset_ext = std::string_view{ config::asset_extension };

		c_auto full_path_len  = static_cast<uint32>(std::ranges::find(full_path, '\0') - full_path.begin());
		auto   full_path_span = std::span{ const_cast<char*>(full_path.data()), full_path_len };
		// replace char
		for (auto& c : full_path_span)
		{
			if (c == '\\')
			{
				c = '/';
			}
			else if (util::is_char_english(c))
			{
				c = util::to_lower_ascii(c);
			}
			else if (detail::is_allowed_char(c))
			{
				// number, ., -,
			}
			else
			{
				c = '_';
			}
		}

		auto full_path_body_sv = std::string_view{ full_path_span.data(), full_path_span.size() };
		for (auto _ : views::loop(2))
		{
			{
				c_auto slash_pos = full_path_body_sv.rfind('/');
				c_auto dot_pos	 = full_path_body_sv.rfind('.');
				if (dot_pos != std::string_view::npos and (slash_pos == std::string_view::npos or dot_pos > slash_pos))
				{
					full_path_body_sv = full_path_body_sv.substr(0, dot_pos);
				}
			}
		}


		auto res_buf = age::make_filled_array<char, config::max_asset_path_len>('\0');
		auto res_len = 0u;

		for (c_auto& [ i, sv ] : full_path_body_sv
									 | std::views::split('/')
									 | std::views::transform([](auto sv) { return util::trim(std::string_view{ sv }, '.'); })
									 | std::views::filter([](c_auto sv) { return sv.empty() is_false; })
									 | views::enumerate<uint32>)
		{
			if (i > 0) { res_buf[res_len++] = '/'; }

			std::ranges::copy(sv, res_buf.begin() + res_len);

			std::ranges::replace(res_buf.begin() + res_len, res_buf.begin() + res_len + sv.size(), '.', '_');

			if (detail::is_reserved_word(std::string_view{ res_buf.data() + res_len, sv.size() }))
			{
				std::ranges::fill(res_buf.begin() + res_len, res_buf.begin() + res_len + sv.size(), '_');
			}

			res_len += static_cast<uint32>(sv.size());
		}


		{
			constexpr auto invalid_asset_name = std::string_view{ "invalid_asset_name" };
			c_auto		   max_body_len		  = static_cast<uint32>(res_buf.size() - 1 - asset_tag.size() - asset_ext.size());
			AGE_ASSERT(invalid_asset_name.size() <= max_body_len);

			if (res_len == 0u or res_len > max_body_len)
			{
				std::ranges::copy(invalid_asset_name, res_buf.begin());
				res_len = static_cast<uint32>(invalid_asset_name.size());
			}
		}

		std::ranges::copy(asset_tag, res_buf.data() + res_len);
		res_len += static_cast<uint32>(asset_tag.size());
		std::ranges::copy(config::asset_extension, res_buf.data() + res_len);
		res_len += static_cast<uint32>(asset_ext.size());

		res_buf[res_len] = '\0';
		full_path		 = res_buf;
	}

	e::asset_path_error_kind
	validate_asset_path(asset::e::kind e_kind, asset::handle h_asset, const age::array<char, config::max_asset_path_len>& new_path) noexcept
	{
		c_auto null_it = std::ranges::find(new_path, '\0');
		if (null_it == new_path.end()) { return e::asset_path_error_kind::not_null_terminated; }

		c_auto sv = std::string_view{ new_path.data(), static_cast<uint32>(null_it - new_path.begin()) };

		if (sv.empty()) { return e::asset_path_error_kind::empty; }

		if (sv.find("..") != std::string_view::npos) { return e::asset_path_error_kind::parent_ref; }

		auto tmp = new_path;
		normalize_asset_path(e_kind, tmp);

		constexpr auto asset_ext = std::string_view{ config::asset_extension };
		if (sv.ends_with(asset_ext) is_false) { return e::asset_path_error_kind::invalid_asset_extension; }

		c_auto sv_without_ext = sv.substr(0, sv.size() - asset_ext.size());

		if (c_auto asset_tag = asset::visit(e_kind, []<e::kind k> { return std::string_view{ asset::get_asset_tag<k>() }; });
			sv_without_ext.ends_with(asset_tag) is_false)
		{
			return e::asset_path_error_kind::invalid_asset_tag;
		}

		if (c_auto sv_normalized = std::string_view{ tmp.data() };
			sv_normalized != sv)
		{
			return e::asset_path_error_kind::not_normalized;
		}

		if (sv == asset::visit(e_kind, [h_asset]<e::kind k> { return std::string_view{ h_asset.get_path<k>() }; })) { return e::asset_path_error_kind::path_unchanged; }

		if (auto h_other = asset::find(e_kind, sv);
			runtime::is_handle_invalid(h_other) is_false and h_other != h_asset) { return e::asset_path_error_kind::asset_exists; }

		if (auto ec = std::error_code{};
			std::filesystem::exists(sv, ec)) { return e::asset_path_error_kind::file_exists; }

		return e::asset_path_error_kind::none;
	}

	std::string_view
	get_path_error_msg(e::asset_path_error_kind e_kind) noexcept
	{
		switch (e_kind)
		{
		case e::asset_path_error_kind::none:
		{
			return "success";
		}
		case e::asset_path_error_kind::empty:
		{
			return "name is empty";
		}
		case e::asset_path_error_kind::parent_ref:
		{
			return "'..' is not allowed in an asset path";
		}
		case e::asset_path_error_kind::asset_exists:
		{
			return "another asset already uses this path";
		}
		case e::asset_path_error_kind::file_exists:
		{
			return "a file already exists at this path";
		}
		case e::asset_path_error_kind::path_unchanged:
		{
			return "same as the current path";
		}
		case e::asset_path_error_kind::io_failed:
		{
			return "file io failed, rename canceled";
		}
		case e::asset_path_error_kind::fixable_by_normalize_begin:
		{
			AGE_UNREACHABLE("fixable_by_normalize_begin is a sentinel, not an error");
			return {};
		}
		case e::asset_path_error_kind::not_null_terminated:
		{
			return "path buffer is not null terminated";
		}
		case e::asset_path_error_kind::invalid_asset_extension:
		{
			return "wrong asset extension - press normalize";
		}
		case e::asset_path_error_kind::invalid_asset_tag:
		{
			return "wrong asset tag - press normalize";
		}
		case e::asset_path_error_kind::not_normalized:
		{
			return "path is not normalized - press normalize button";
		}
		case e::asset_path_error_kind::too_long:
		{
			return "path is too long - normalize will discard the name";
		}
		default:
		{
			AGE_UNREACHABLE("invalid_id {}", to_idx(e_kind));
			return {};
		}
		}
	}

	handle
	find(e::kind e_kind, std::string_view full_path) noexcept
	{
		return find(e_kind, util::to_fixed_str<config::max_asset_path_len>(full_path));
	}

	handle
	find(e::kind e_kind, const age::array<char, config::max_asset_path_len>& full_path) noexcept
	{
		auto it = g::path_to_handle_map[to_idx(e_kind)].find(full_path);

		if (it == g::path_to_handle_map[to_idx(e_kind)].end())
		{
			return handle{};
		}

		return it->second;
	}
}	 // namespace age::asset

namespace age::asset
{
	void
	deinit() noexcept
	{
		// Known hole: once an entry is gone (registry clear or manual erase),
		// child handles to it cannot be validated. Most assets are registered,
		// and unregistered ones rarely reference other assets, so this should
		// hold. If it ever breaks, it breaks here - future me will deal with it.
		if constexpr (config::debug_mode)
		{
			for_each_kind(AGE_LAMBDA(
				<e::kind e_kind>(),
				{
					auto& pool = pool_of<e_kind>();
					for (auto it = pool.begin(); it != pool.end(); ++it)
					{
						AGE_ASSERT(is_any_loaded(*it) is_false);
					}
				}));
		}

		for_each_kind(AGE_LAMBDA(
			<e::kind e_kind>(),
			{
				auto& pool = pool_of<e_kind>();
				for (auto it = pool.begin(); it != pool.end(); ++it)
				{
					auto h = handle::make<e_kind>(it.idx<uint32>());
					destroy_entry<e_kind>(h);
				}
			}));

		AGE_ASSERT(g::path_vec.is_empty());

		for (auto& vec : g::registry_map)
		{
			vec.clear();
		}

		for (auto& map : g::path_to_handle_map)
		{
			map.clear();
		}
	}
}	 // namespace age::asset
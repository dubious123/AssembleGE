#include "age_pch.hpp"
#include "age.hpp"

namespace age::asset
{
	template <>
	handle
	create_entry<e::kind::font>(std::string_view asset_path) noexcept
	{
		AGE_ASSERT(asset_path.size() < config::max_asset_path_len);
		// auto& path_to_handle = g::registry_path_to_handle_map[to_idx(e::kind::font)];
		// if (auto it = path_to_handle.find(asset_path); it != path_to_handle.end())
		//{
		//	AGE_ASSERT(false);
		//	return it->second;
		// }

		auto& pool = pool_of<e::kind::font>();

		auto idx = pool.emplace_back(entry<e::kind::font>{
			.path_id = static_cast<uint32>(
				g::path_vec.emplace_back(util::to_fixed_str<config::max_asset_path_len>(asset_path))),
		});

		return handle::make<e::kind::font>(idx);
	}

	template <>
	void
	destroy_entry<e::kind::font>(handle& h_font) noexcept
	{
		auto& entry = h_font.get_entry<e::kind::font>();
		AGE_ASSERT(entry.is_loaded() is_false);

		g::path_vec.remove(entry.path_id);

		auto& pool = pool_of<e::kind::font>();

		pool.remove(h_font.get_idx());

		h_font.id = age::get_invalid_id<t_asset_id>();
	}

	std::array<char, config::max_asset_path_len>&
	entry<e::kind::font>::get_path() const noexcept
	{
		return g::path_vec[path_id];
	}

	bool
	entry<e::kind::font>::is_loaded() const noexcept
	{
		return AGE_IS_INVALID_ID(atlas_id) is_false;
	}

	std::span<const font::glyph_data>
	entry<e::kind::font>::get_glyph() const noexcept
	{
		return std::span<const font::glyph_data>{ reinterpret_cast<const font::glyph_data*>(p_blob), glyph_count };
	}

	std::span<const uint16>
	entry<e::kind::font>::get_extra_unicode() const noexcept
	{
		return std::span<const uint16>{ reinterpret_cast<uint16*>(p_blob + sizeof(font::glyph_data) * glyph_count), extra_unicode_count };
	}

	const font::glyph_data&
	entry<e::kind::font>::get_glyph_data(uint16 unicode) const noexcept
	{
		auto   glyphs = get_glyph();
		uint32 idx	  = 0;

		if (has_all(charset_flag, e::font_charset_flag::ascii))
		{
			if (unicode >= 0x20 && unicode <= 0x7E)
			{
				return glyphs[idx + (unicode - 0x20)];
			}
			idx += (0x7E - 0x20 + 1);
		}

		if (has_all(charset_flag, e::font_charset_flag::hangul))
		{
			if (unicode >= 0xAC00 && unicode <= 0xD7A3)
			{
				return glyphs[idx + (unicode - 0xAC00)];
			}
			idx += (0xD7A3 - 0xAC00 + 1);
		}

		auto extra = get_extra_unicode();
		auto it	   = std::ranges::lower_bound(extra, unicode);
		if (it != extra.end() && *it == unicode)
		{
			return glyphs[idx + static_cast<uint32>(it - extra.begin())];
		}

		AGE_UNREACHABLE();	  // glyph not found
		return glyphs[0];
	}
}	 // namespace age::asset

namespace age::asset::font::detail
{
	float
	read_space_advance_ttf(const char* p_path) noexcept
	{
		auto file		 = std::ifstream(p_path, std::ios::binary);
		auto buf		 = std::vector<uint8>(std::istreambuf_iterator<char>(file), {});
		auto read_uint16 = [&](uint32 offset) { return uint16(buf[offset] << 8 | buf[offset + 1]); };
		auto read_uint32 = [&](uint32 offset) { return uint32(buf[offset]) << 24 | uint32(buf[offset + 1]) << 16 | uint32(buf[offset + 2]) << 8 | buf[offset + 3]; };

		// find table offsets
		uint32 head = 0, cmap = 0, hmtx = 0, hhea = 0;
		for (uint16 i = 0; i < read_uint16(4); ++i)
		{
			c_auto table_idx = 12 + i * 16;
			c_auto offset	 = read_uint32(table_idx + 8);
			if (std::memcmp(&buf[table_idx], "head", 4) == 0) { head = offset; };
			if (std::memcmp(&buf[table_idx], "cmap", 4) == 0) { cmap = offset; };
			if (std::memcmp(&buf[table_idx], "hmtx", 4) == 0) { hmtx = offset; };
			if (std::memcmp(&buf[table_idx], "hhea", 4) == 0) { hhea = offset; };
		}

		c_auto upm		 = read_uint16(head + 18);
		c_auto h_metrics = read_uint16(hhea + 34);

		// find format 4 subtable
		uint32 f4 = 0;
		for (uint16 i = 0; i < read_uint16(cmap + 2); ++i)
		{
			c_auto e = cmap + 4 + i * 8;
			if (read_uint16(e) == 3 && read_uint16(e + 2) == 1)
			{
				f4 = cmap + read_uint32(e + 4);
				break;
			}
		}

		// walk segments for U+0020
		c_auto seg_count = read_uint16(f4 + 6) / 2;
		c_auto ends		 = f4 + 14;
		c_auto starts	 = ends + seg_count * 2 + 2;
		c_auto deltas	 = starts + seg_count * 2;
		c_auto ranges	 = deltas + seg_count * 2;

		uint16 gid = 0;
		for (uint16 i = 0; i < seg_count; ++i)
		{
			if (0x20 > read_uint16(ends + i * 2)) { continue; }
			if (0x20 < read_uint16(starts + i * 2)) { break; }

			c_auto ro = read_uint16(ranges + i * 2);
			if (ro == 0)
			{
				gid = uint16((0x20 + int16(read_uint16(deltas + i * 2))) & 0xFFFF);
			}
			else
			{
				gid = read_uint16(ranges + i * 2 + ro + (0x20 - read_uint16(starts + i * 2)) * 2);
				break;
			}
		}

		c_auto adv = read_uint16(hmtx + std::min(gid, uint16(h_metrics - 1)) * 4);
		return float(adv) / float(upm);
	}
}	 // namespace age::asset::font::detail

namespace age::asset::font::detail
{
	std::string_view
	extract_asset_name(std::string_view full_name) noexcept
	{
		// "font_name.xxx.age_asset" -> "font_name"
		if (full_name.ends_with(config::asset_extension))
		{
			full_name.remove_suffix(std::size(config::asset_extension) - 1);
		}
		if (full_name.ends_with(config::font_asset_tag))
		{
			full_name.remove_suffix(std::size(config::font_asset_tag) - 1);
		}
		return full_name;
	}

	void
	read_entry(asset::entry<e::kind::font>& ntry, byte_buf& buf) noexcept
	{
		static_assert(alignof(glyph_data) >= alignof(uint16));
		buf.read(
			ntry.glyph_count,
			ntry.charset_flag,
			ntry.ascent,
			ntry.descent,
			ntry.space_advance,
			ntry.line_height,
			ntry.em_size,
			ntry.px_range,
			ntry.atlas_width,
			ntry.atlas_height,
			ntry.extra_unicode_count,
			ntry.atlas_channel_count);

		ntry.p_blob = (std::byte*)::operator new(sizeof(glyph_data) * ntry.glyph_count + sizeof(uint16) * ntry.extra_unicode_count,
												 std::align_val_t{ alignof(glyph_data) });

		buf.read<glyph_data>(ntry.p_blob, ntry.glyph_count);
		buf.read<uint16>(ntry.p_blob + ntry.glyph_count * sizeof(glyph_data), ntry.extra_unicode_count);
	}

	bool
	need_rebuild(const asset::entry<e::kind::font>& ntry, e::font_charset_flag flag, std::span<uint16> extra_unicode) noexcept
	{
		if (e::has_all(ntry.charset_flag, flag) is_false)
		{
			return true;
		}

		if (std::ranges::includes(ntry.get_extra_unicode(), extra_unicode) is_false)
		{
			return true;
		}

		return false;
	}

	void
	rebuild_font(std::string_view font_name, e::font_charset_flag flag, std::span<uint16> extra_unicode_span) noexcept
	{
		for (auto font_extension : { ".ttf" /*, ".otf"*/ })
		{
			auto font_path = std::format("{}{}", font_name, font_extension);
			auto res	   = external::msdfgen::bake_font(font_path.c_str(),
														  "font_bake_temp.bin",
														  "font_bake_temp.csv",
														  "font_bake_temp.json",
														  flag,
														  extra_unicode_span);

			if (res is_false)
			{
				std::remove("font_bake_temp.bin");
				std::remove("font_bake_temp.csv");
				std::remove("font_bake_temp.json");
				continue;
			}

			auto ntry = entry<e::kind::font>{};

			{
				auto file = std::ifstream("font_bake_temp.json");
				AGE_ASSERT(file.is_open());

				auto text = std::string(std::istreambuf_iterator<char>(file), {});

				auto find_float = [&](const char* key) -> float {
					return std::stof(text.substr(text.find(key) + strlen(key) + 1));
				};

				auto find_uint = [&](const char* key) -> uint32 {
					return static_cast<uint32>(std::stoul(text.substr(text.find(key) + strlen(key) + 1)));
				};

				ntry.ascent		   = find_float("\"ascender\"");
				ntry.descent	   = find_float("\"descender\"");
				ntry.space_advance = detail::read_space_advance_ttf(font_path.c_str());
				ntry.line_height   = find_float("\"lineHeight\"");
				ntry.em_size	   = find_float("\"emSize\"");
				ntry.atlas_width   = find_uint("\"width\"");
				ntry.atlas_height  = find_uint("\"height\"");

				ntry.charset_flag		 = flag;
				ntry.px_range			 = 8.0f;
				ntry.extra_unicode_count = static_cast<uint16>(extra_unicode_span.size());
				ntry.atlas_channel_count = uint8{ 4 };
			}

			auto buf = byte_buf{};
			buf.write(
				ntry.glyph_count,
				ntry.charset_flag,
				ntry.ascent,
				ntry.descent,
				ntry.space_advance,
				ntry.line_height,
				ntry.em_size,
				ntry.px_range,
				ntry.atlas_width,
				ntry.atlas_height,
				ntry.extra_unicode_count,
				ntry.atlas_channel_count);


			{
				// reserve index 0 for space (not in CSV, outline-less glyph)
				{
					buf.write(glyph_data{
						.advance = ntry.space_advance,
					});

					ntry.glyph_count = 1;
				}

				auto file = std::ifstream("font_bake_temp.csv");
				AGE_ASSERT(file.is_open());

				auto line = std::string();
				std::getline(file, line);	 // header skip

				for (; std::getline(file, line); ++ntry.glyph_count)
				{
					uint32 unicode;
					float  advance, plane_l, plane_t, plane_r, plane_b, atlas_rect_l, atlas_rect_t, atlas_rect_r, atlas_rect_b;

					sscanf_s(line.c_str(), "%u,%f,%f,%f,%f,%f,%f,%f,%f,%f",
							 &unicode, &advance,
							 &plane_l, &plane_t, &plane_r, &plane_b,
							 &atlas_rect_l, &atlas_rect_t, &atlas_rect_r, &atlas_rect_b);

					c_auto width  = static_cast<float>(ntry.atlas_width);
					c_auto height = static_cast<float>(ntry.atlas_height);

					buf.write(glyph_data{
						.advance	  = advance,
						.offset		  = { plane_l, plane_t - ntry.ascent },
						.size		  = { plane_r - plane_l, plane_b - plane_t },
						.atlas_uv_min = { atlas_rect_l / width, atlas_rect_t / height },
						.atlas_uv_max = { atlas_rect_r / width, atlas_rect_b / height },
					});
				}
				{
					buf.write_at(0, ntry.glyph_count);
				}

				{
					for (auto c : extra_unicode_span)
					{
						buf.write(c);
					}
				}
			}

			{
				auto file = std::ifstream("font_bake_temp.bin", std::ios::in | std::ios::binary);
				AGE_ASSERT(file.is_open());

				c_auto file_size = std::filesystem::file_size("font_bake_temp.bin");

				c_auto atlas_size = sizeof(uint8_4) * ntry.atlas_width * ntry.atlas_height;

				AGE_ASSERT(file_size == atlas_size);

				buf.reserve(buf.size() + atlas_size);

				file.read(reinterpret_cast<char*>(buf.data() + buf.size()), atlas_size);

				buf.move_write_pos(buf.size() + atlas_size);
			}

			std::remove("font_bake_temp.bin");
			std::remove("font_bake_temp.csv");
			std::remove("font_bake_temp.json");

			c_auto f_header = get_default_file_header<e::kind::font>(buf.size());

			write_asset_file(std::format("{}{}{}", font_name, config::font_asset_tag, config::asset_extension), f_header, buf.data());
			return;
		}

		AGE_ASSERT(false);
	}
}	 // namespace age::asset::font::detail
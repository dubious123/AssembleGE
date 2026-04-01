#include "age_pch.hpp"
#include "age.hpp"

namespace age::asset::font
{
	std::span<uint16>
	asset_header::get_extra_unicode() noexcept
	{
		auto* ptr = reinterpret_cast<uint16*>(reinterpret_cast<uint8*>(this) + sizeof(asset_header));
		return { ptr, extra_unicode_count };
	}

	std::span<glyph_data>
	asset_header::get_glyph() noexcept
	{
		auto* ptr = reinterpret_cast<glyph_data*>(reinterpret_cast<uint8*>(this) + glyph_data_offset);
		return { ptr, glyph_count };
	}

	std::span<const uint8>
	asset_header::get_atlas() const noexcept
	{
		auto* ptr = reinterpret_cast<const uint8*>(this) + atlas_offset;
		return { ptr, atlas_width * atlas_height * atlas_channel_count };
	}

	const glyph_data&
	asset_header::get_glyph_data(uint16 unicode) noexcept
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

	file_header
	get_file_header(uint64 file_size) noexcept
	{
		return file_header{
			.magic		   = g::asset_header_magic,
			.header_size   = sizeof(file_header),
			.file_size	   = file_size + sizeof(file_header),
			.version_major = config::version_major,
			.version_minor = config::version_minor,
			.asset_kind	   = e::kind::font,
		};
	}
}	 // namespace age::asset::font

namespace age::asset::font
{
	handle
	load(const std::string_view& font_name, e::font_charset_flag flag, std::span<uint16> extra_unicode_span) noexcept
	{
		auto h_asset = load_from_file(font_name);
		if (age::runtime::is_handle_valid(h_asset))
		{
			auto& font_header = h_asset->get_asset_header<e::kind::font>();

			auto no_rebuild = true;

			no_rebuild &= e::has_all(font_header.charset_flag, flag);

			std::ranges::sort(extra_unicode_span);

			no_rebuild &= std::ranges::includes(font_header.get_extra_unicode(), extra_unicode_span);

			if (no_rebuild)
			{
				return h_asset;
			}

			unload(h_asset);
		}


		for (auto font_extension : { ".ttf", ".otf" })
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

			float  ascent, descent, line_height, line_gap, em_size;
			uint32 atlas_width, atlas_height;
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

				ascent		 = find_float("\"ascender\"");
				descent		 = find_float("\"descender\"");
				line_height	 = find_float("\"lineHeight\"");
				em_size		 = find_float("\"emSize\"");
				atlas_width	 = find_uint("\"width\"");
				atlas_height = find_uint("\"height\"");

				line_gap = line_height - (ascent - descent);
			}

			c_auto extra_unicode_offset = sizeof(asset_header);
			c_auto glyph_data_offset	= extra_unicode_offset + sizeof(uint16) * extra_unicode_span.size();
			c_auto atlas_offset			= glyph_data_offset + sizeof(glyph_data) * (calc_unicode_count(flag) + extra_unicode_span.size());
			c_auto atlas_size			= sizeof(uint8_4) * atlas_width * atlas_height;
			c_auto blob_size			= atlas_offset + atlas_size;
			auto*  p_blob				= (std::byte*)::operator new(blob_size, asset::detail::get_alignment(e::kind::font));

			{
				std::memcpy(p_blob + extra_unicode_offset, extra_unicode_span.data(), sizeof(uint16) * extra_unicode_span.size());
			}

			{
				auto file = std::ifstream("font_bake_temp.csv");
				AGE_ASSERT(file.is_open());

				auto line = std::string();
				std::getline(file, line);	 // header skip

				// reserve index 0 for space (not in CSV, outline-less glyph)
				for (auto* ptr = reinterpret_cast<glyph_data*>(p_blob + glyph_data_offset) + 1;
					 std::getline(file, line);
					 ++ptr)
				{
					uint32 unicode;
					float  advance, plane_l, plane_b, plane_r, plane_t, atlas_rect_l, atlas_rect_b, atlas_rect_r, atlas_rect_t;

					sscanf_s(line.c_str(), "%u,%f,%f,%f,%f,%f,%f,%f,%f,%f",
							 &unicode, &advance,
							 &plane_l, &plane_b, &plane_r, &plane_t,
							 &atlas_rect_l, &atlas_rect_b, &atlas_rect_r, &atlas_rect_t);

					c_auto width  = static_cast<float>(atlas_width);
					c_auto height = static_cast<float>(atlas_height);

					auto data = glyph_data{
						.advance	  = advance,
						.offset		  = { plane_l, ascent - plane_t },
						.size		  = { plane_r - plane_l, plane_t - plane_b },
						.atlas_uv_min = { atlas_rect_l / width, atlas_rect_b / height },
						.atlas_uv_max = { atlas_rect_r / width, atlas_rect_t / height },
					};

					std::memcpy(ptr, &data, sizeof(glyph_data));
				}
			}

			{
				auto file = std::ifstream("font_bake_temp.bin", std::ios::in | std::ios::binary);
				AGE_ASSERT(file.is_open());

				c_auto file_size = std::filesystem::file_size("font_bake_temp.bin");

				AGE_ASSERT(file_size == atlas_size);

				for (auto* p_dst : std::views::iota(p_blob + atlas_offset) | std::views::stride(sizeof(uint8_4) * atlas_width) | std::views::take(atlas_height) | std::views::reverse)
				{
					file.read(reinterpret_cast<char*>(p_dst), sizeof(uint8_4) * atlas_width);
				}
			}

			{
				auto header = asset_header{
					.charset_flag		 = flag,
					.glyph_data_offset	 = glyph_data_offset,
					.atlas_offset		 = atlas_offset,
					.ascent				 = ascent,
					.descent			 = descent,
					.line_height		 = line_height,
					.em_size			 = em_size,
					.px_range			 = 2.0f,
					.atlas_width		 = atlas_width,
					.atlas_height		 = atlas_height,
					.glyph_count		 = static_cast<uint16>(calc_unicode_count(flag) + extra_unicode_span.size()),
					.extra_unicode_count = static_cast<uint16>(extra_unicode_span.size()),
					.atlas_channel_count = 4,
				};
				std::memcpy(p_blob, &header, sizeof(header));
			}

			std::remove("font_bake_temp.bin");
			std::remove("font_bake_temp.csv");
			std::remove("font_bake_temp.json");

			c_auto f_header = get_file_header(blob_size);

			write_to_file(font_name, f_header, *p_blob);

			return load_from_blob(font_name, f_header, *p_blob);
		}

		AGE_ASSERT(false);

		return h_asset;
	}
}	 // namespace age::asset::font
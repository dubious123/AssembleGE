#pragma once
#include "age.hpp"

namespace age::asset::font::detail
{
	void
	read_entry(asset::entry<e::kind::font>&, byte_buf&) noexcept;

	bool
	need_rebuild(const asset::entry<e::kind::font>& ntry,
				 e::font_charset_flag				flag,
				 std::span<uint16>					extra_unicode) noexcept;

	void
	rebuild_font(std::string_view	  font_name,
				 e::font_charset_flag flag,
				 std::span<uint16>	  extra_unicode_span) noexcept;


}	 // namespace age::asset::font::detail

namespace age::asset::font
{
	void
	unload(handle h_font, auto& renderer) noexcept
	{
		auto& entry = h_font.get_entry<e::kind::font>();
		if (entry.is_loaded())
		{
			::operator delete(entry.p_blob, std::align_val_t{ alignof(glyph_data) });
			renderer.release_texture(entry.atlas_id);
		}

		AGE_ASSERT(entry.is_loaded() is_false);
	}

	void
	load(handle h_font, auto& renderer, e::font_charset_flag flag, std::span<uint16> extra_unicode) noexcept
	{
		std::ranges::sort(extra_unicode);

		auto& entry = h_font.get_entry<e::kind::font>();

		if (entry.is_loaded())
		{
			if (detail::need_rebuild(entry, flag, extra_unicode) is_false)
			{
				return;
			}
			else
			{
				unload(h_font, renderer);
			}
		}
		else if (auto buf = asset::read_asset_file(entry.get_path().data());
				 buf.empty() is_false)
		{
			detail::read_entry(entry, buf);

			if (detail::need_rebuild(entry, flag, extra_unicode) is_false)
			{
				entry.atlas_id = renderer.upload_texture(buf.data() + buf.read_amount(),
														 { .width = entry.atlas_width, .height = entry.atlas_height },
														 age::graphics::e::texture_format::rgba8_unorm);
				return;
			}
		}

		detail::rebuild_font(detail::extract_asset_name<e::kind::font>(entry.get_path()), flag, extra_unicode);

		if (auto buf = asset::read_asset_file(entry.get_path().data());
			buf.empty() is_false)
		{
			detail::read_entry(entry, buf);

			entry.atlas_id = renderer.upload_texture(buf.data() + buf.read_amount(),
													 { .width = entry.atlas_width, .height = entry.atlas_height },
													 age::graphics::e::texture_format::rgba8_unorm);
		}
		else
		{
			AGE_ASSERT(false);
		}
	}

	handle
	load(std::string_view font_name, auto& renderer, e::font_charset_flag flag, std::span<uint16> extra_unicode) noexcept
	{
		auto full_path = std::format("{}{}{}", font_name, config::font_asset_tag, config::asset_extension);
		auto h_asset   = registry::find<e::kind::font>(font_name.data());

		if (AGE_IS_INVALID_ID(h_asset.id))
		{
			h_asset = asset::create_entry<e::kind::font>(full_path);
		}

		load(h_asset, renderer, flag, extra_unicode);

		return h_asset;
	}
}	 // namespace age::asset::font
#include "age_pch.hpp"
#include "age.hpp"

namespace age::ui::font
{
	namespace detail
	{
		FORCE_INLINE uint32
		find_idx(t_hash h) noexcept
		{
			for (auto&& [idx, key] : g::font_vec | std::views::keys | std::views::enumerate)
			{
				if (key == h)
				{
					return static_cast<uint32>(idx);
				}
			}

			return age::get_invalid_id<uint32>();
		}
	}	 // namespace detail

	void
	load(const char* p_font_name, asset::e::font_charset_flag flag, std::span<uint16> extra_unicode) noexcept
	{
		c_auto h   = ui::detail::hash(p_font_name);
		c_auto idx = detail::find_idx(h);
		if (idx == age::get_invalid_id<uint32>())
		{
			g::font_vec.emplace_back(h, asset::font::load(p_font_name, flag, extra_unicode));
		}
	}

	void
	set_default(const char* p_font_name) noexcept
	{
		c_auto h   = ui::detail::hash(p_font_name);
		c_auto idx = detail::find_idx(h);

		AGE_ASSERT(idx != age::get_invalid_id<uint32>());

		g::current_font_idx = idx;
	}

	void
	set_default_size(float size) noexcept
	{
		g::current_font_size = size;
	}

	void
	unload(const char* p_font_name) noexcept
	{
		c_auto h   = ui::detail::hash(p_font_name);
		c_auto idx = detail::find_idx(h);

		AGE_ASSERT(idx != age::get_invalid_id<uint32>());

		g::font_vec[idx] = g::font_vec.back();
		g::font_vec.pop_back();

		g::current_font_idx = std::min(g::current_font_idx, g::font_vec.size<uint32>() - 1);
	}

	float
	get_height(float font_size) noexcept
	{
		auto  h_font = g::font_vec[g::current_font_idx].second;
		auto& header = h_font->get_asset_header<asset::e::kind::font>();
		return header.line_height * font_size;
	}

	float
	get_advance(const uint16& unicode, float font_size) noexcept
	{
		auto  h_font = g::font_vec[g::current_font_idx].second;
		auto& header = h_font->get_asset_header<asset::e::kind::font>();

		auto& data = header.get_glyph_data(unicode);

		return data.advance * font_size;
	}
}	 // namespace age::ui::font
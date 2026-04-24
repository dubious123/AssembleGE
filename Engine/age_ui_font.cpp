#include "age_pch.hpp"
#include "age.hpp"

namespace age::ui::font
{
	namespace detail
	{
		uint32
		find_idx(t_hash h) noexcept
		{
			for (auto&& [idx, key] : g::font_data_vec | std::views::keys | std::views::enumerate)
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
	set_default(const char* p_font_name) noexcept
	{
		c_auto h   = ui::detail::hash(p_font_name);
		c_auto idx = detail::find_idx(h);

		AGE_ASSERT(idx != age::get_invalid_id<uint32>());

		g::current_font_idx = idx;
	}

	float
	get_line_height(float font_size, uint32 font_idx) noexcept
	{
		auto  h_font = g::font_data_vec[font_idx].second.h_font;
		auto& entry	 = h_font.get_entry<asset::e::kind::font>();
		return entry.line_height * font_size;
	}

	float
	get_advance(uint16 unicode, float font_size, uint32 font_idx) noexcept
	{
		auto  h_font = g::font_data_vec[font_idx].second.h_font;
		auto& entry	 = h_font.get_entry<asset::e::kind::font>();

		auto& data = entry.get_glyph_data(unicode);

		return data.advance * font_size;
	}

	float
	get_space_advance(float font_size, uint32 font_idx) noexcept
	{
		auto  h_font = g::font_data_vec[font_idx].second.h_font;
		auto& entry	 = h_font.get_entry<asset::e::kind::font>();

		return entry.space_advance * font_size;
	}

	const asset::font::glyph_data&
	get_glyph_data(uint16 unicode, uint32 font_idx) noexcept
	{
		auto  h_font = g::font_data_vec[font_idx].second.h_font;
		auto& entry	 = h_font.get_entry<asset::e::kind::font>();

		return entry.get_glyph_data(unicode);
	}

	const asset::font::glyph_data&
	get_glyph_data(uint16 unicode, t_hash font_hash) noexcept
	{
		return get_glyph_data(unicode, detail::find_idx(font_hash));
	}
}	 // namespace age::ui::font
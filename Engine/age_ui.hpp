#pragma once
#include "age.hpp"

namespace age::ui
{
	FORCE_INLINE t_hash
	new_id() noexcept;

	FORCE_INLINE id_ctx
	id_begin() noexcept;

	t_hash
	hash(const char*) noexcept;
}	 // namespace age::ui

namespace age::ui
{
	void
	init() noexcept;

	void
	begin_frame(platform::window_handle h_window) noexcept;

	void
	end_frame(age::vector<render_data>&, age::vector<util::range>&) noexcept;

	void
	deinit(auto& renderer) noexcept;
}	 // namespace age::ui

namespace age::ui::font
{
	void
	load(const char* p_font_name, uint32 atlas_id, asset::e::font_charset_flag flag = asset::e::font_charset_flag::ascii, std::span<uint16> extra_unicode = {}) noexcept;

	void
	load(const char* p_font_name, auto& renderer, asset::e::font_charset_flag flag = asset::e::font_charset_flag::ascii, std::span<uint16> extra_unicode = {}) noexcept;

	void
	set_default(const char* p_font_name) noexcept;

	void
	set_scale(float scale) noexcept;

	void
	unload(const char* p_font_name) noexcept;

	float
	get_line_height(float font_size, uint32 font_idx = g::current_font_idx) noexcept;

	float
	get_advance(uint16 unicode, float font_size, uint32 font_idx = g::current_font_idx) noexcept;

	float
	get_space_advance(float font_size, uint32 font_idx = g::current_font_idx) noexcept;

	const asset::font::glyph_data&
	get_glyph_data(uint16 unicode, uint32 font_idx = g::current_font_idx) noexcept;

	const asset::font::glyph_data&
	get_glyph_data(uint16 unicode, t_hash font_hash) noexcept;
}	 // namespace age::ui::font

// widgets
namespace age::ui::widget
{
	widget_ctx
	begin(widget_desc&& desc) noexcept;
}	 // namespace age::ui::widget

// defaults
namespace age::ui::size_mode
{
	FORCE_INLINE constexpr widget_size_mode
	fixed(auto value) noexcept;

	FORCE_INLINE constexpr widget_size_mode
	grow(auto min, auto max) noexcept;

	FORCE_INLINE constexpr widget_size_mode
	fit(auto min, auto max) noexcept;

	FORCE_INLINE constexpr widget_size_mode
	text(auto min, auto max) noexcept;
}	 // namespace age::ui::size_mode

namespace age::ui::brush_data
{
	FORCE_INLINE constexpr ui_brush_data
	color(float r, float g, float b, float a = 1.f) noexcept;
}	 // namespace age::ui::brush_data

namespace age::ui::font
{
	void
	load(const char* p_font_name, auto& renderer, asset::e::font_charset_flag flag, std::span<uint16> extra_unicode) noexcept
	{
		c_auto h   = ui::hash(p_font_name);
		auto   idx = age::get_invalid_id<uint32>();

		for (auto&& [i, key] : g::font_data_vec | std::views::keys | std::views::enumerate)
		{
			if (key == h)
			{
				idx = static_cast<uint32>(i);
				break;
			}
		}

		if (idx == age::get_invalid_id<uint32>())
		{
			auto h_font = asset::font::load(p_font_name, flag, extra_unicode);

			c_auto& font_header = h_font->get_asset_header<asset::e::kind::font>();

			c_auto atlas_id = renderer.upload_texture(font_header.get_atlas().data(), { .width = font_header.atlas_width, .height = font_header.atlas_height }, age::graphics::e::texture_format::rgba8_unorm);

			g::font_data_vec.emplace_back(std::pair{
				h,
				font_data{ .atlas_id = atlas_id, .h_font = h_font } });
		}
	}

}	 // namespace age::ui::font

namespace age::ui
{
	void
	deinit(auto& renderer) noexcept
	{
		g::widget_state_map.clear();

		for (auto&& [hash, font_data] : g::font_data_vec)
		{
			renderer.release_texture(font_data.atlas_id);
			asset::unload(font_data.h_font);
		}

		g::font_data_vec.clear();

		g::id_stack.clear();
		g::layout_size_data_stack.clear();
		g::element_layout_pos_data_vec.clear();
		g::element_render_data_vec.clear();

		g::text_data_vec.clear();
		g::word_data_vec.clear();
		g::char_data_vec.clear();
		g::char_pos_data_vec.clear();
	}
}	 // namespace age::ui
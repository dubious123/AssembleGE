#pragma once
#include "age.hpp"

namespace age::ui
{
	t_hash
	new_id() noexcept;

	id_ctx
	id_begin() noexcept;

	t_hash
	hash(const char*) noexcept;
}	 // namespace age::ui

namespace age::ui
{
	void
	init() noexcept;

	void
	begin_frame(platform::window_handle h_window, const float3& cam_world_pos = {}, const float4x4& cam_view_proj_inv = {}) noexcept;

	void
	end_frame(std::tuple<age::vector<ui::render_data>&,
						 age::vector<util::range>&,
						 age::vector<util::range>&,
						 age::array<age::vector<ui::root_graphics_data>, ui::e::space_mode_kind_size>&>
				  gpu_sink) noexcept;

	void
	clear() noexcept;	 // cancel render

	void
	deinit(auto& renderer) noexcept;
}	 // namespace age::ui

namespace age::ui::font
{
	void
	load(const char* p_font_name, auto& renderer, asset::e::font_charset_flag flag = asset::e::font_charset_flag::ascii, std::span<uint16> extra_unicode = {}) noexcept;

	void
	set_default(const char* p_font_name) noexcept;

	void
	unload(const char* p_font_name, auto& renderer) noexcept;

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
	FORCE_INLINE widget_ctx
	begin(auto&& desc) noexcept;
}	 // namespace age::ui::widget

namespace age::ui
{
	root_ctx
	root_begin(const root_desc& desc) noexcept;
	// todo.
	__declspec(noinline) void
	draw_direct(auto&& mod) noexcept;
}	 // namespace age::ui

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

namespace age::ui::shape_data
{
	FORCE_INLINE constexpr ui_shape_data
	roundness(float r) noexcept;
}

namespace age::ui::detail
{
	void
	widget_end() noexcept;

	void
	gen_text_data(widget_desc& desc) noexcept;

	cursor_data
	screen_offset_to_cursor(uint32 text_data_idx, float2 screen_offset, float width) noexcept;

	cursor_data
	byte_offset_to_cursor(uint32 text_data_idx, uint32 byte_offset, float width) noexcept;

	void
	update_text_buf(char* p_buf, uint32 buf_byte_size, cursor_data& cursor) noexcept;

	float
	calc_line_width(const char*& p_buf, float font_size, uint32 font_idx) noexcept;

	FORCE_INLINE root_data&
	get_current_root() noexcept;
}	 // namespace age::ui::detail

namespace age::ui
{
	template <std::size_t n>
	FORCE_INLINE constexpr widget_ctx_impl<n>::~widget_ctx_impl() noexcept
	{
		if (hash_id != age::get_invalid_id<t_hash>())
		{
			for (auto _ : std::views::iota(0u, n))
			{
				detail::widget_end();
				g::id_stack.pop_back();
			}
		}
	}

	FORCE_INLINE constexpr root_ctx::~root_ctx() noexcept
	{
		if (hash_id != age::get_invalid_id<t_hash>())
		{
			detail::widget_end();
			g::id_stack.pop_back();
		}
	};
}	 // namespace age::ui
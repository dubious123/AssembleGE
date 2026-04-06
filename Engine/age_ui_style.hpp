#pragma once
#include "age.hpp"

namespace age::ui::theme
{
	template <e::theme_token_kind e_theme_token>
	consteval FORCE_INLINE auto
	get_theme_token_data() noexcept
	{
		if constexpr (e_theme_token == e::theme_token_kind::bg_panel)
		{
			return g::bg_panel;
		}
		else if constexpr (e_theme_token == e::theme_token_kind::bg_surface)
		{
			return g::bg_surface;
		}
		else if constexpr (e_theme_token == e::theme_token_kind::bg_interactive)
		{
			return g::bg_interactive;
		}
		else if constexpr (e_theme_token == e::theme_token_kind::bg_accent)
		{
			return g::bg_accent;
		}
		else if constexpr (e_theme_token == e::theme_token_kind::bg_popup)
		{
			return g::bg_popup;
		}
		else if constexpr (e_theme_token == e::theme_token_kind::border_default)
		{
			return g::border_default;
		}
		else if constexpr (e_theme_token == e::theme_token_kind::border_accent)
		{
			return g::border_accent;
		}
		else if constexpr (e_theme_token == e::theme_token_kind::text_primary)
		{
			return g::text_primary;
		}
		else if constexpr (e_theme_token == e::theme_token_kind::text_secondary)
		{
			return g::text_secondary;
		}
		else if constexpr (e_theme_token == e::theme_token_kind::text_tertiary)
		{
			return g::text_tertiary;
		}
		else if constexpr (e_theme_token == e::theme_token_kind::text_hint)
		{
			return g::text_hint;
		}
		else if constexpr (e_theme_token == e::theme_token_kind::text_disabled)
		{
			return g::text_disabled;
		}
		else if constexpr (e_theme_token == e::theme_token_kind::text_accent)
		{
			return g::text_accent;
		}
		else if constexpr (e_theme_token == e::theme_token_kind::text_positive)
		{
			return g::text_positive;
		}
		else if constexpr (e_theme_token == e::theme_token_kind::text_negative)
		{
			return g::text_negative;
		}
		else if constexpr (e_theme_token == e::theme_token_kind::text_amber)
		{
			return g::text_amber;
		}
		else if constexpr (e_theme_token == e::theme_token_kind::text_interactive)
		{
			return g::text_interactive;
		}
		else if constexpr (e_theme_token == e::theme_token_kind::separator)
		{
			return g::separator;
		}
		else if constexpr (e_theme_token == e::theme_token_kind::toggle_off)
		{
			return g::toggle_off;
		}
		else if constexpr (e_theme_token == e::theme_token_kind::toggle_on)
		{
			return g::toggle_on;
		}
		else if constexpr (e_theme_token == e::theme_token_kind::slider_fill)
		{
			return g::slider_fill;
		}
		else if constexpr (e_theme_token == e::theme_token_kind::slider_track)
		{
			return g::slider_track;
		}
		else if constexpr (e_theme_token == e::theme_token_kind::slider_thumb)
		{
			return g::slider_thumb;
		}
		else if constexpr (e_theme_token == e::theme_token_kind::slider_thumb_ring)
		{
			return g::slider_thumb_ring;
		}
		else if constexpr (e_theme_token == e::theme_token_kind::scroll_thumb)
		{
			return g::scroll_thumb;
		}
		else if constexpr (e_theme_token == e::theme_token_kind::select_accent)
		{
			return g::select_accent;
		}
		else
		{
			AGE_UNREACHABLE();
		}
	}

	template <e::theme_token_kind e_theme_token>
	constexpr FORCE_INLINE float4
	color(e::style_state e_state = e::style_state::idle) noexcept
	{
		return float4{ g::theme_color[e::to_idx(get_theme_token_data<e_theme_token>().color)], g::theme_opacity[get_theme_token_data<e_theme_token>().opacity[e::to_idx(e_state)]] };
	}

	template <e::theme_token_kind e_theme_token>
	constexpr FORCE_INLINE float4
	color(e::theme_color_kind e_color, e::style_state e_state = e::style_state::idle) noexcept
	{
		return float4{ g::theme_color[e::to_idx(e_color)], g::theme_opacity[get_theme_token_data<e_theme_token>().opacity[e::to_idx(e_state)]] };
	}

	constexpr FORCE_INLINE float3
	color(e::theme_color_kind e_color) noexcept
	{
		return g::theme_color[e::to_idx(e_color)];
	}

	template <e::theme_token_kind e_theme_token>
	constexpr FORCE_INLINE float
	opacity(e::style_state e_state = e::style_state::idle) noexcept
	{
		return g::theme_opacity[get_theme_token_data<e_theme_token>().opacity[e::to_idx(e_state)]];
	}

	template <e::theme_token_kind e_theme_token>
	constexpr FORCE_INLINE float4
	font_size() noexcept
	{
		return g::theme_font_size[e::to_idx(get_theme_token_data<e_theme_token>().font_size)];
	}
}	 // namespace age::ui::theme

namespace age::ui::theme::colors
{
	FORCE_INLINE float4
	bg_panel(e::style_state state = e::style_state::idle) noexcept
	{
		return color<e::theme_token_kind::bg_panel>(state);
	}

	FORCE_INLINE float4
	bg_surface(e::style_state state = e::style_state::idle) noexcept
	{
		return color<e::theme_token_kind::bg_surface>(state);
	}

	FORCE_INLINE float4
	bg_interactive(e::style_state state = e::style_state::idle) noexcept
	{
		return color<e::theme_token_kind::bg_interactive>(state);
	}

	FORCE_INLINE float4
	bg_accent(e::style_state state = e::style_state::idle) noexcept
	{
		return color<e::theme_token_kind::bg_accent>(state);
	}

	FORCE_INLINE float4
	bg_popup(e::style_state state = e::style_state::idle) noexcept
	{
		return color<e::theme_token_kind::bg_popup>(state);
	}

	FORCE_INLINE float4
	border_default(e::style_state state = e::style_state::idle) noexcept
	{
		return color<e::theme_token_kind::border_default>(state);
	}

	FORCE_INLINE float4
	border_accent(e::style_state state = e::style_state::idle) noexcept
	{
		return color<e::theme_token_kind::border_accent>(state);
	}

	FORCE_INLINE float4
	border_interactive(e::style_state state = e::style_state::idle) noexcept
	{
		if (state == e::style_state::idle)
		{
			return color<e::theme_token_kind::border_default>(e::style_state::idle);
		}
		else
		{
			return color<e::theme_token_kind::border_accent>(state);
		}
	}

	FORCE_INLINE float4
	text_primary(e::style_state state = e::style_state::idle) noexcept
	{
		return color<e::theme_token_kind::text_primary>(state);
	}

	FORCE_INLINE float4
	text_secondary(e::style_state state = e::style_state::idle) noexcept
	{
		return color<e::theme_token_kind::text_secondary>(state);
	}

	FORCE_INLINE float4
	text_tertiary(e::style_state state = e::style_state::idle) noexcept
	{
		return color<e::theme_token_kind::text_tertiary>(state);
	}

	FORCE_INLINE float4
	text_hint(e::style_state state = e::style_state::idle) noexcept
	{
		return color<e::theme_token_kind::text_hint>(state);
	}

	FORCE_INLINE float4
	text_disabled(e::style_state state = e::style_state::idle) noexcept
	{
		return color<e::theme_token_kind::text_disabled>(state);
	}

	FORCE_INLINE float4
	text_accent(e::style_state state = e::style_state::idle) noexcept
	{
		return color<e::theme_token_kind::text_accent>(state);
	}

	FORCE_INLINE float4
	text_positive(e::style_state state = e::style_state::idle) noexcept
	{
		return color<e::theme_token_kind::text_positive>(state);
	}

	FORCE_INLINE float4
	text_negative(e::style_state state = e::style_state::idle) noexcept
	{
		return color<e::theme_token_kind::text_negative>(state);
	}

	FORCE_INLINE float4
	text_amber(e::style_state state = e::style_state::idle) noexcept
	{
		return color<e::theme_token_kind::text_amber>(state);
	}

	FORCE_INLINE float4
	text_interactive(e::style_state state = e::style_state::idle) noexcept
	{
		return color<e::theme_token_kind::text_interactive>(state);
	}

	FORCE_INLINE float4
	separator(e::style_state state = e::style_state::idle) noexcept
	{
		return color<e::theme_token_kind::separator>(state);
	}

	FORCE_INLINE float4
	toggle_off(e::style_state state = e::style_state::idle) noexcept
	{
		return color<e::theme_token_kind::toggle_off>(state);
	}

	FORCE_INLINE float4
	toggle_on(e::style_state state = e::style_state::idle) noexcept
	{
		return color<e::theme_token_kind::toggle_on>(state);
	}

	FORCE_INLINE float4
	slider_fill(e::style_state state = e::style_state::idle) noexcept
	{
		return color<e::theme_token_kind::slider_fill>(state);
	}

	FORCE_INLINE float4
	slider_track(e::style_state state = e::style_state::idle) noexcept
	{
		return color<e::theme_token_kind::slider_track>(state);
	}

	FORCE_INLINE float4
	slider_thumb(e::style_state state = e::style_state::idle) noexcept
	{
		return color<e::theme_token_kind::slider_thumb>(state);
	}

	FORCE_INLINE float4
	slider_thumb_ring(e::style_state state = e::style_state::idle) noexcept
	{
		return color<e::theme_token_kind::slider_thumb_ring>(state);
	}

	FORCE_INLINE float4
	scroll_thumb(e::style_state state = e::style_state::idle) noexcept
	{
		return color<e::theme_token_kind::scroll_thumb>(state);
	}

	FORCE_INLINE float4
	select_accent(e::style_state state = e::style_state::idle) noexcept
	{
		return color<e::theme_token_kind::select_accent>(state);
	}
}	 // namespace age::ui::theme::colors

namespace age::ui::theme::size
{
	FORCE_INLINE float
	text_primary() noexcept
	{
		return g::theme_font_size[e::to_idx(g::text_primary.font_size)];
	}

	FORCE_INLINE float
	text_secondary() noexcept
	{
		return g::theme_font_size[e::to_idx(g::text_secondary.font_size)];
	}

	FORCE_INLINE float
	text_tertiary() noexcept
	{
		return g::theme_font_size[e::to_idx(g::text_tertiary.font_size)];
	}

	FORCE_INLINE float
	text_hint() noexcept
	{
		return g::theme_font_size[e::to_idx(g::text_hint.font_size)];
	}

	FORCE_INLINE float
	text_disabled() noexcept
	{
		return g::theme_font_size[e::to_idx(g::text_disabled.font_size)];
	}

	FORCE_INLINE float
	text_accent() noexcept
	{
		return g::theme_font_size[e::to_idx(g::text_accent.font_size)];
	}

	FORCE_INLINE float
	text_positive() noexcept
	{
		return g::theme_font_size[e::to_idx(g::text_positive.font_size)];
	}

	FORCE_INLINE float
	text_negative() noexcept
	{
		return g::theme_font_size[e::to_idx(g::text_negative.font_size)];
	}

	FORCE_INLINE float
	text_amber() noexcept
	{
		return g::theme_font_size[e::to_idx(g::text_amber.font_size)];
	}

	FORCE_INLINE float
	text_interactive() noexcept
	{
		return g::theme_font_size[e::to_idx(g::text_interactive.font_size)];
	}
}	 // namespace age::ui::theme::size

namespace age::ui::theme
{
	FORCE_INLINE float
	slider_track_height() noexcept
	{
		return g::theme_slider_track_height;
	}

	FORCE_INLINE float
	slider_thumb_size(e::style_state e_state = e::style_state::idle) noexcept
	{
		return g::theme_slider_thumb_size[e::to_idx(e_state)];
	}

	FORCE_INLINE float
	slider_thumb_border_thickness(e::style_state e_state = e::style_state::idle) noexcept
	{
		return g::theme_slider_thumb_border_thickness[e::to_idx(e_state)];
	}
}	 // namespace age::ui::theme

namespace age::ui::detail
{
	template <typename t>
	concept cx_modifier = requires(t mod, widget_desc& desc) {{ mod.apply(desc) } -> std::same_as<void>; };
}	 // namespace age::ui::detail

namespace age::ui::detail
{
	FORCE_INLINE widget_desc
	operator|(detail::cx_modifier auto&& mod_l, detail::cx_modifier auto&& mod_r) noexcept
	{
		auto desc = widget_desc{};
		mod_l.apply(desc);
		mod_r.apply(desc);
		return desc;
	}

	FORCE_INLINE widget_desc&&
	operator|(widget_desc&& desc, detail::cx_modifier auto&& mod) noexcept
	{
		mod.apply(desc);
		return std::move(desc);
	}
}	 // namespace age::ui::detail

namespace age::ui
{
	// clang-format off
#define X(name) decltype(widget_desc::name) name
#define Y(name) desc.name = name
#define DEF(name, ...)                                \
	namespace detail                                  \
	{                                                 \
		struct mod_##name                             \
		{                                             \
			FOR_EACH (X, __VA_ARGS__);                \
                                                      \
			FORCE_INLINE constexpr void               \
			apply(widget_desc& desc) const noexcept   \
			{                                         \
				FOR_EACH (Y, __VA_ARGS__);            \
			}                                         \
			FORCE_INLINE constexpr                    \
			operator widget_desc() const noexcept     \
			{                                         \
				widget_desc desc{};                   \
				apply(desc);                          \
				return desc;                          \
			}                                         \
		};                                            \
	}                                                 \
	FORCE_INLINE constexpr decltype(auto)             \
	set_##name(FOR_EACH_ARG(X, __VA_ARGS__)) noexcept \
	{                                                 \
		return detail::mod_##name{ __VA_ARGS__ };     \
	}
	// clang-format on

	DEF(draw, draw)
	DEF(layout, layout)
	DEF(overflow, overflow)
	DEF(align, align)

	DEF(z_offset, z_offset)
	DEF(offset, offset)
	DEF(child_gap, child_gap)
	DEF(padding, padding_left, padding_right, padding_top, padding_bottom)
	DEF(padding_left, padding_left)
	DEF(padding_right, padding_right)
	DEF(padding_top, padding_top)
	DEF(padding_bottom, padding_bottom)
	DEF(pivot_uv, pivot_uv)
	DEF(rotation, rotation)
	DEF(shape, shape_kind, shape_data)
	DEF(shape_kind, shape_kind)
	DEF(shape_data, shape_data)
	DEF(body_brush, body_brush_kind, body_brush_data)
	DEF(body_brush_kind, body_brush_kind)
	DEF(body_brush_data, body_brush_data)
	DEF(border_thickness, border_thickness)
	DEF(border_brush, border_brush_kind, border_brush_data)
	DEF(border_brush_kind, border_brush_kind)
	DEF(border_brush_data, border_brush_data)
	DEF(border, border_thickness, border_brush_kind, border_brush_data)

	DEF(interact, interact)
	DEF(save_state, save_state)

	FORCE_INLINE constexpr decltype(auto)
	set_offset(float x, float y) noexcept
	{
		return detail::mod_offset{ float2{ x, y } };
	}

	FORCE_INLINE constexpr decltype(auto)
	set_body_brush_data(float4 color) noexcept
	{
		return detail::mod_body_brush_data{ brush_data::color(color) };
	}

	FORCE_INLINE constexpr decltype(auto)
	set_body_brush_data(float r, float g, float b, float a) noexcept
	{
		return detail::mod_body_brush_data{ brush_data::color(r, g, b, a) };
	}

	FORCE_INLINE constexpr decltype(auto)
	set_body_brush_data(float3 rgb, float a) noexcept
	{
		return detail::mod_body_brush_data{ brush_data::color(rgb.x, rgb.y, rgb.z, a) };
	}

	FORCE_INLINE constexpr decltype(auto)
	set_border_brush_data(float4 color) noexcept
	{
		return detail::mod_border_brush_data{ brush_data::color(color) };
	}

	FORCE_INLINE constexpr decltype(auto)
	set_border_brush_data(float r, float g, float b, float a) noexcept
	{
		return detail::mod_border_brush_data{ brush_data::color(r, g, b, a) };
	}

	FORCE_INLINE constexpr decltype(auto)
	set_border_brush_data(float3 rgb, float a) noexcept
	{
		return detail::mod_border_brush_data{ brush_data::color(rgb.x, rgb.y, rgb.z, a) };
	}

	FORCE_INLINE constexpr decltype(auto)
	set_horizontal() noexcept
	{
		return set_layout(e::widget_layout::horizontal);
	}

	FORCE_INLINE constexpr decltype(auto)
	set_vertical() noexcept
	{
		return set_layout(e::widget_layout::vertical);
	}

	namespace detail
	{
		struct mod_size
		{
			float width_min;
			float width_max;

			float height_min;
			float height_max;

			e::size_mode_kind width_size_mode;
			e::size_mode_kind height_size_mode;

			FORCE_INLINE constexpr void
			apply(widget_desc& desc) const noexcept
			{
				desc.width_min		  = width_min;
				desc.width_max		  = width_max;
				desc.height_min		  = height_min;
				desc.height_max		  = height_max;
				desc.width_size_mode  = width_size_mode;
				desc.height_size_mode = height_size_mode;
			}

			FORCE_INLINE constexpr
			operator widget_desc() const noexcept
			{
				widget_desc desc{};
				apply(desc);
				return desc;
			}
		};

		struct mod_width
		{
			widget_size_mode size_mode;

			FORCE_INLINE constexpr void
			apply(widget_desc& desc) const noexcept
			{
				desc.width_min		 = size_mode.min;
				desc.width_max		 = size_mode.max;
				desc.width_size_mode = size_mode.size_mode;
			}

			FORCE_INLINE constexpr
			operator widget_desc() const noexcept
			{
				widget_desc desc{};
				apply(desc);
				return desc;
			}
		};

		struct mod_height
		{
			widget_size_mode size_mode;

			FORCE_INLINE constexpr void
			apply(widget_desc& desc) const noexcept
			{
				desc.height_min		  = size_mode.min;
				desc.height_max		  = size_mode.max;
				desc.height_size_mode = size_mode.size_mode;
			}

			FORCE_INLINE constexpr
			operator widget_desc() const noexcept
			{
				widget_desc desc{};
				apply(desc);
				return desc;
			}
		};
	}	 // namespace detail

	FORCE_INLINE constexpr decltype(auto)
	set_size(widget_size_mode size_width, widget_size_mode size_height) noexcept
	{
		return detail::mod_size{ size_width.min,
								 size_width.max,
								 size_height.min,
								 size_height.max,
								 size_width.size_mode,
								 size_height.size_mode };
	}

	FORCE_INLINE constexpr decltype(auto)
	set_width(widget_size_mode size_width) noexcept
	{
		return detail::mod_width{ size_width };
	}

	FORCE_INLINE constexpr decltype(auto)
	set_height(widget_size_mode size_height) noexcept
	{
		return detail::mod_height{ size_height };
	}

#undef X
#undef Y


#define X(name) decltype(widget_desc::text.name) name
#define Y(name) desc.text.name = name

	DEF(font_size, font_size)
	DEF(font_idx, font_idx)
	DEF(text, p_str)

	FORCE_INLINE constexpr decltype(auto)
	set_font_size(e::font_size_kind e_font_size) noexcept
	{
		return detail::mod_font_size{ g::theme_font_size[e::to_idx(e_font_size)] };
	}

#undef X
#undef Y
#undef DEF
}	 // namespace age::ui

// wrappers
namespace age::ui::style
{
	namespace detail
	{
		FORCE_INLINE constexpr widget_desc
		layout_base() noexcept
		{
			return set_draw(false)
				 | set_align(e::widget_align::begin)
				 | set_z_offset(0)
				 | set_padding(0.f, 0.f, 0.f, 0.f)
				 | set_border_thickness(0.f);
		}
	}	 // namespace detail

	FORCE_INLINE constexpr widget_desc
	vertical() noexcept
	{
		return detail::layout_base()
			 | set_layout(e::widget_layout::vertical)
			 | set_size(size_mode::grow(), size_mode::fit());
	}

	FORCE_INLINE constexpr widget_desc
	vertical(widget_size_mode width, widget_size_mode height) noexcept
	{
		return detail::layout_base()
			 | set_layout(e::widget_layout::vertical)
			 | set_size(width, height);
	}

	FORCE_INLINE constexpr widget_desc
	horizontal() noexcept
	{
		return detail::layout_base()
			 | set_layout(e::widget_layout::horizontal)
			 | set_size(size_mode::fit(), size_mode::grow());
	}

	FORCE_INLINE constexpr widget_desc
	horizontal(widget_size_mode width, widget_size_mode height) noexcept
	{
		return detail::layout_base()
			 | set_layout(e::widget_layout::horizontal)
			 | set_size(width, height);
	}

	FORCE_INLINE constexpr widget_desc
	layout(e::widget_layout e_layout) noexcept
	{
		if (e_layout == e::widget_layout::vertical)
		{
			return vertical();
		}
		else if (e_layout == e::widget_layout::horizontal)
		{
			return horizontal();
		}
		else
		{
			AGE_UNREACHABLE();
		}
	}
}	 // namespace age::ui::style

namespace age::ui::style
{
	namespace detail
	{
		FORCE_INLINE constexpr widget_desc
		frame_base() noexcept
		{
			return set_draw(true)
				 | set_layout(e::widget_layout::vertical)
				 | set_align(e::widget_align::begin)
				 | set_size(size_mode::fit(), size_mode::fit())
				 | set_z_offset(1)
				 | set_padding(8.f, 8.f, 3.f, 3.f)
				 | set_body_brush_kind(e::brush_kind::color)
				 | set_border_brush_kind(e::brush_kind::color)
				 | set_border_thickness(1.f);
		}
	}	 // namespace detail

	FORCE_INLINE constexpr widget_desc
	frame(e::style_state state = e::style_state::idle) noexcept
	{
		return detail::frame_base()
			 | set_body_brush_data(theme::colors::bg_panel(state))
			 | set_border_brush_data(theme::colors::border_default(state));
	}

	FORCE_INLINE constexpr widget_desc
	frame_interactive(e::style_state state = e::style_state::idle) noexcept
	{
		return detail::frame_base()
			 | set_body_brush_data(theme::colors::bg_interactive(state))
			 | set_border_brush_data(theme::colors::border_interactive(state));
	}

	FORCE_INLINE constexpr widget_desc
	seperator() noexcept
	{
		return set_draw(true)
			 | set_layout(e::widget_layout::vertical)
			 | set_align(e::widget_align::center)
			 | set_size(size_mode::grow(), size_mode::fixed(1.f))
			 | set_z_offset(1)
			 | set_padding(0.f, 0.f, 0.f, 0.f)
			 | set_body_brush_kind(e::brush_kind::color)
			 | set_body_brush_data(theme::colors::text_primary())
			 | set_border_thickness(0.f);
	}
}	 // namespace age::ui::style

namespace age::ui::style
{
	namespace detail
	{
		FORCE_INLINE constexpr widget_desc
		text_base() noexcept
		{
			return set_align(e::widget_align::center)
				 | set_shape_kind(e::shape_kind::text)
				 | set_padding(0.f, 0.f, 0.f, 0.f)
				 | set_border_thickness(0.f);
		}
	}	 // namespace detail

	FORCE_INLINE constexpr widget_desc
	text(const char* p_text) noexcept
	{
		return detail::text_base()
			 | set_text(p_text)
			 | set_font_idx(g::current_font_idx)
			 | set_font_size(e::font_size_kind::normal)
			 | set_body_brush_data(theme::colors::text_secondary(e::style_state::idle));
	}

	// heading, selected item text
	FORCE_INLINE constexpr widget_desc
	text_primary(const char* p_text, e::style_state state = e::style_state::idle) noexcept
	{
		return text(p_text)
			 | set_font_size(g::text_primary.font_size)
			 | set_body_brush_data(theme::colors::text_primary(state));
	}

	// input value, normal body
	FORCE_INLINE constexpr widget_desc
	text_secondary(const char* p_text, e::style_state state = e::style_state::idle) noexcept
	{
		return text(p_text)
			 | set_font_size(g::text_secondary.font_size)
			 | set_body_brush_data(theme::colors::text_secondary(state));
	}

	// section header, label
	FORCE_INLINE constexpr widget_desc
	text_tertiary(const char* p_text, e::style_state state = e::style_state::idle) noexcept
	{
		return text(p_text)
			 | set_font_size(g::text_tertiary.font_size)
			 | set_body_brush_data(theme::colors::text_tertiary(state));
	}

	// input label, placeholder
	FORCE_INLINE constexpr widget_desc
	text_hint(const char* p_text, e::style_state state = e::style_state::idle) noexcept
	{
		return text(p_text)
			 | set_font_size(g::text_hint.font_size)
			 | set_body_brush_data(theme::colors::text_hint(state));
	}

	// disabled widget text
	FORCE_INLINE constexpr widget_desc
	text_disabled(const char* p_text, e::style_state state = e::style_state::idle) noexcept
	{
		return text(p_text)
			 | set_font_size(g::text_disabled.font_size)
			 | set_body_brush_data(theme::colors::text_disabled(state));
	}

	// link, asset reference, clickable path
	FORCE_INLINE constexpr widget_desc
	text_accent(const char* p_text, e::style_state state = e::style_state::idle) noexcept
	{
		return text(p_text)
			 | set_font_size(g::text_accent.font_size)
			 | set_body_brush_data(theme::colors::text_accent(state));
	}

	// success msg, y axis, fps ok
	FORCE_INLINE constexpr widget_desc
	text_positive(const char* p_text, e::style_state state = e::style_state::idle) noexcept
	{
		return text(p_text)
			 | set_font_size(g::text_positive.font_size)
			 | set_body_brush_data(theme::colors::text_positive(state));
	}

	// error msg, x axis, warning
	FORCE_INLINE constexpr widget_desc
	text_negative(const char* p_text, e::style_state state = e::style_state::idle) noexcept
	{
		return text(p_text)
			 | set_font_size(g::text_negative.font_size)
			 | set_body_brush_data(theme::colors::text_negative(state));
	}

	// button text, list item text
	FORCE_INLINE constexpr widget_desc
	text_interactive(const char* p_text, e::style_state state = e::style_state::idle) noexcept
	{
		return text(p_text)
			 | set_font_size(g::text_interactive.font_size)
			 | set_body_brush_data(theme::colors::text_interactive(state));
	}
}	 // namespace age::ui::style

namespace age::ui::style
{
	namespace detail
	{
		FORCE_INLINE constexpr widget_desc
		slider_track_base() noexcept
		{
			return set_align(e::widget_align::center)
				 | set_height(size_mode::fixed(theme::slider_track_height()))
				 | set_shape_kind(e::shape_kind::rect)
				 | set_body_brush_kind(e::brush_kind::color)
				 | set_padding(0.f, 0.f, 0.f, 0.f)
				 | set_border_thickness(0.f)
				 | set_z_offset(1);
		}

		FORCE_INLINE constexpr widget_desc
		slider_thumb_base() noexcept
		{
			return set_align(e::widget_align::center)
				 | set_shape_kind(e::shape_kind::circle)
				 | set_body_brush_kind(e::brush_kind::color)
				 | set_border_brush_kind(e::brush_kind::color)
				 | set_padding(0.f, 0.f, 0.f, 0.f)
				 | set_z_offset(2);
		}
	}	 // namespace detail

	FORCE_INLINE constexpr widget_desc
	slider_track(e::style_state state = e::style_state::idle) noexcept
	{
		return detail::slider_track_base()
			 | set_body_brush_data(theme::colors::slider_track(state));
	}

	FORCE_INLINE constexpr widget_desc
	slider_fill(e::style_state state = e::style_state::idle) noexcept
	{
		return detail::slider_track_base()
			 | set_body_brush_data(theme::colors::slider_fill(state));
	}

	FORCE_INLINE constexpr widget_desc
	slider_thumb(e::style_state state = e::style_state::idle) noexcept
	{
		return detail::slider_thumb_base()
			 | set_size(size_mode::fixed(theme::slider_thumb_size(state)), size_mode::fixed(theme::slider_thumb_size(state)))
			 | set_border_thickness(theme::slider_thumb_border_thickness(state))
			 | set_body_brush_data(theme::colors::slider_thumb(state))
			 | set_border_brush_data(theme::colors::slider_thumb_ring(state));
	}
}	 // namespace age::ui::style

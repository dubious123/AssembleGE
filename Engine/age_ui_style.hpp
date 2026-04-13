#pragma once
#include "age.hpp"

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

	DEF(width_size_mode, width_size_mode)
	DEF(height_size_mode, height_size_mode)

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
	set_align_begin() noexcept
	{
		return detail::mod_align{ e::widget_align::begin };
	}

	FORCE_INLINE constexpr decltype(auto)
	set_align_center() noexcept
	{
		return detail::mod_align{ e::widget_align::center };
	}

	FORCE_INLINE constexpr decltype(auto)
	set_align_end() noexcept
	{
		return detail::mod_align{ e::widget_align::end };
	}

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
	set_body_brush_data(float3 rgb, float a = 1.f) noexcept
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
	set_border_brush_data(float3 rgb, float a = 1.f) noexcept
	{
		return detail::mod_border_brush_data{ brush_data::color(rgb.x, rgb.y, rgb.z, a) };
	}

	FORCE_INLINE constexpr decltype(auto)
	set_shape_data(float r) noexcept
	{
		return detail::mod_shape_data{ shape_data::roundness(r) };
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

	FORCE_INLINE constexpr decltype(auto)
	set_horizontal_inv() noexcept
	{
		return set_layout(e::widget_layout::horizontal_inv);
	}

	FORCE_INLINE constexpr decltype(auto)
	set_vertical_inv() noexcept
	{
		return set_layout(e::widget_layout::vertical_inv);
	}

	FORCE_INLINE constexpr decltype(auto)
	set_padding(float4 padding) noexcept
	{
		return set_padding(padding.x, padding.y, padding.z, padding.w);
	}

	FORCE_INLINE constexpr decltype(auto)
	set_padding(float padding) noexcept
	{
		return set_padding(padding, padding, padding, padding);
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

	FORCE_INLINE constexpr decltype(auto)
	set_width_fixed(float width) noexcept
	{
		return detail::mod_width{ size_mode::fixed(width) };
	}

	FORCE_INLINE constexpr decltype(auto)
	set_height_fixed(float height) noexcept
	{
		return detail::mod_height{ size_mode::fixed(height) };
	}

	FORCE_INLINE constexpr decltype(auto)
	set_width_grow() noexcept
	{
		return detail::mod_width{ size_mode::grow() };
	}

	FORCE_INLINE constexpr decltype(auto)
	set_height_grow() noexcept
	{
		return detail::mod_height{ size_mode::grow() };
	}

	FORCE_INLINE constexpr decltype(auto)
	set_width_fit() noexcept
	{
		return detail::mod_width{ size_mode::fit() };
	}

	FORCE_INLINE constexpr decltype(auto)
	set_height_fit() noexcept
	{
		return detail::mod_height{ size_mode::fit() };
	}

#undef X
#undef Y


#define X(name) decltype(widget_desc::text.name) name
#define Y(name) desc.text.name = name

	DEF(font_size, font_size)
	DEF(font_idx, font_idx)
	DEF(text, p_str)

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
				 | set_padding(0.f)
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
	vertical_inv() noexcept
	{
		return detail::layout_base()
			 | set_layout(e::widget_layout::vertical_inv)
			 | set_size(size_mode::grow(), size_mode::fit());
	}

	FORCE_INLINE constexpr widget_desc
	vertical_inv(widget_size_mode width, widget_size_mode height) noexcept
	{
		return detail::layout_base()
			 | set_layout(e::widget_layout::vertical_inv)
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
	horizontal_inv() noexcept
	{
		return detail::layout_base()
			 | set_layout(e::widget_layout::horizontal_inv)
			 | set_size(size_mode::fit(), size_mode::grow());
	}

	FORCE_INLINE constexpr widget_desc
	horizontal_inv(widget_size_mode width, widget_size_mode height) noexcept
	{
		return detail::layout_base()
			 | set_layout(e::widget_layout::horizontal_inv)
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
		else if (e_layout == e::widget_layout::vertical_inv)
		{
			return vertical_inv();
		}
		else if (e_layout == e::widget_layout::horizontal_inv)
		{
			return horizontal_inv();
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
		panel_base() noexcept
		{
			return set_draw(true)
				 | set_layout(e::widget_layout::vertical)
				 | set_height_grow()
				 | set_align(e::widget_align::begin)
				 | set_z_offset(1)
				 | set_body_brush_kind(e::brush_kind::color)
				 | set_border_brush_kind(e::brush_kind::color);
		}
	}	 // namespace detail

	FORCE_INLINE constexpr widget_desc
	panel() noexcept
	{
		return detail::panel_base()
			 | set_padding(theme::panel_padding())
			 | set_child_gap(theme::panel_child_gap())
			 | set_border_thickness(theme::panel_border_thickness())
			 | set_body_brush_data(theme::panel_color_bg())
			 | set_border_brush_data(theme::panel_color_border_idle());
	}

	FORCE_INLINE constexpr widget_desc
	panel_focus() noexcept
	{
		return detail::panel_base()
			 | set_padding(theme::panel_padding())
			 | set_child_gap(theme::panel_child_gap())
			 | set_border_thickness(theme::panel_border_thickness())
			 | set_body_brush_data(theme::panel_color_bg())
			 | set_border_brush_data(theme::panel_color_border_focus());
	}
}	 // namespace age::ui::style

namespace age::ui::style
{
	namespace detail
	{
		FORCE_INLINE constexpr widget_desc
		section_base() noexcept
		{
			return set_draw(true)
				 | set_layout(e::widget_layout::vertical)
				 | set_align(e::widget_align::begin)
				 | set_height_fit()
				 | set_width_grow()
				 | set_z_offset(1)
				 | set_body_brush_kind(e::brush_kind::color)
				 | set_border_brush_kind(e::brush_kind::color);
		}
	}	 // namespace detail

	FORCE_INLINE constexpr widget_desc
	section() noexcept
	{
		return detail::section_base()
			 | set_padding(theme::section_padding())
			 | set_child_gap(theme::section_child_gap())
			 | set_border_thickness(theme::section_border_thickness())
			 | set_body_brush_data(theme::section_color_bg())
			 | set_border_brush_data(theme::section_color_border());
	}

	FORCE_INLINE constexpr widget_desc
	section_focus() noexcept
	{
		return detail::section_base()
			 | set_padding(theme::section_padding())
			 | set_child_gap(theme::section_child_gap())
			 | set_border_thickness(theme::section_border_thickness())
			 | set_body_brush_data(theme::section_color_bg())
			 | set_border_brush_data(theme::section_color_border_focus());
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
				 | set_size(size_mode::grow(), size_mode::fit())
				 | set_z_offset(1)
				 | set_shape_kind(e::shape_kind::rounded_rect)
				 | set_body_brush_kind(e::brush_kind::color)
				 | set_border_brush_kind(e::brush_kind::color);
		}
	}	 // namespace detail

	FORCE_INLINE constexpr widget_desc
	frame_idle() noexcept
	{
		return detail::frame_base()
			 | set_padding(theme::frame_padding())
			 | set_child_gap(theme::frame_child_gap())
			 | set_border_thickness(theme::frame_border_thickness())
			 | set_shape_data(theme::frame_roundness())
			 | set_body_brush_data(theme::frame_color_bg())
			 | set_border_brush_data(theme::frame_color_border());
	}

	FORCE_INLINE constexpr widget_desc
	frame_hover() noexcept
	{
		return detail::frame_base()
			 | set_padding(theme::frame_padding())
			 | set_child_gap(theme::frame_child_gap())
			 | set_border_thickness(theme::frame_border_thickness())
			 | set_shape_data(theme::frame_roundness())
			 | set_body_brush_data(theme::frame_color_bg_hover())
			 | set_border_brush_data(theme::frame_color_border_hover());
	}

	FORCE_INLINE constexpr widget_desc
	frame_focus() noexcept
	{
		return detail::frame_base()
			 | set_padding(theme::frame_padding())
			 | set_child_gap(theme::frame_child_gap())
			 | set_border_thickness(theme::frame_border_thickness())
			 | set_shape_data(theme::frame_roundness())
			 | set_body_brush_data(theme::frame_color_bg_focus())
			 | set_border_brush_data(theme::frame_color_border_focus());
	}

	FORCE_INLINE constexpr widget_desc
	frame(e::style_state state = e::style_state::idle) noexcept
	{
		if (state == e::style_state::idle) { return frame_idle(); }
		if (state == e::style_state::hover) { return frame_hover(); }
		if (state == e::style_state::active) { return frame_focus(); }

		AGE_UNREACHABLE();
	}
}	 // namespace age::ui::style

namespace age::ui::style
{
	namespace detail
	{
		FORCE_INLINE constexpr widget_desc
		header_bar_base() noexcept
		{
			return set_draw(true)
				 | set_layout(e::widget_layout::horizontal)
				 | set_align(e::widget_align::center)
				 | set_size(size_mode::grow(), size_mode::fit())
				 | set_z_offset(1)
				 | set_shape_kind(e::shape_kind::rounded_rect)
				 | set_body_brush_kind(e::brush_kind::color)
				 | set_border_brush_kind(e::brush_kind::color);
		}
	}	 // namespace detail

	FORCE_INLINE constexpr widget_desc
	header_bar_idle() noexcept
	{
		return detail::header_bar_base()
			 | set_padding(theme::header_bar_padding())
			 | set_child_gap(theme::header_bar_child_gap())
			 | set_border_thickness(theme::header_bar_border_thickness())
			 | set_shape_data(theme::header_bar_roundness())
			 | set_body_brush_data(theme::header_bar_color_bg());
	}

	FORCE_INLINE constexpr widget_desc
	header_bar_hover() noexcept
	{
		return detail::header_bar_base()
			 | set_padding(theme::header_bar_padding())
			 | set_child_gap(theme::header_bar_child_gap())
			 | set_border_thickness(theme::header_bar_border_thickness())
			 | set_shape_data(theme::header_bar_roundness())
			 | set_body_brush_data(theme::header_bar_color_bg_hover());
	}

	FORCE_INLINE constexpr widget_desc
	header_bar_focus() noexcept
	{
		return detail::header_bar_base()
			 | set_padding(theme::header_bar_padding())
			 | set_child_gap(theme::header_bar_child_gap())
			 | set_border_thickness(theme::header_bar_border_thickness())
			 | set_shape_data(theme::header_bar_roundness())
			 | set_body_brush_data(theme::header_bar_color_bg_active());
	}

	FORCE_INLINE constexpr widget_desc
	header_bar(e::style_state state = e::style_state::idle) noexcept
	{
		if (state == e::style_state::idle) { return header_bar_idle(); }
		if (state == e::style_state::hover) { return header_bar_hover(); }
		if (state == e::style_state::active) { return header_bar_focus(); }
		AGE_UNREACHABLE();
	}
}	 // namespace age::ui::style

namespace age::ui::style
{
	namespace detail
	{
		FORCE_INLINE constexpr widget_desc
		item_base() noexcept
		{
			return set_draw(true)
				 | set_layout(e::widget_layout::horizontal)
				 | set_align(e::widget_align::center)
				 | set_size(size_mode::grow(), size_mode::fit())
				 | set_z_offset(1)
				 | set_shape_kind(e::shape_kind::rounded_rect)
				 | set_body_brush_kind(e::brush_kind::color)
				 | set_border_brush_kind(e::brush_kind::color);
		}
	}	 // namespace detail

	FORCE_INLINE constexpr widget_desc
	item_selected_idle() noexcept
	{
		return detail::item_base()
			 | set_border_thickness(theme::item_border_thickness())
			 | set_body_brush_data(theme::item_color_bg_selected())
			 | set_border_brush_data(theme::item_color_border_selected())
			 | set_shape_data(theme::item_roundness());
	}

	FORCE_INLINE constexpr widget_desc
	item_selected_hover() noexcept
	{
		return detail::item_base()
			 | set_border_thickness(theme::item_border_thickness())
			 | set_body_brush_data(theme::item_color_bg_selected_hover())
			 | set_border_brush_data(theme::item_color_border_selected_hover())
			 | set_shape_data(theme::item_roundness());
	}

	FORCE_INLINE constexpr widget_desc
	item_selected_active() noexcept
	{
		return detail::item_base()
			 | set_border_thickness(theme::item_border_thickness())
			 | set_body_brush_data(theme::item_color_bg_selected_active())
			 | set_border_brush_data(theme::item_color_border_selected_active())
			 | set_shape_data(theme::item_roundness());
	}

	FORCE_INLINE constexpr widget_desc
	item_idle() noexcept
	{
		return detail::item_base()
			 | set_border_thickness(theme::item_border_thickness())
			 | set_body_brush_data(theme::item_color_bg())
			 | set_border_brush_data(theme::item_color_border())
			 | set_shape_data(theme::item_roundness());
	}

	FORCE_INLINE constexpr widget_desc
	item_hover() noexcept
	{
		return detail::item_base()
			 | set_border_thickness(theme::item_border_thickness())
			 | set_body_brush_data(theme::item_color_bg_hover())
			 | set_border_brush_data(theme::item_color_border_hover())
			 | set_shape_data(theme::item_roundness());
	}

	FORCE_INLINE constexpr widget_desc
	item_active() noexcept
	{
		return detail::item_base()
			 | set_border_thickness(theme::item_border_thickness())
			 | set_body_brush_data(theme::item_color_bg_active())
			 | set_border_brush_data(theme::item_color_border_active())
			 | set_shape_data(theme::item_roundness());
	}

	FORCE_INLINE constexpr widget_desc
	item_selected(e::style_state state = e::style_state::idle) noexcept
	{
		if (state == e::style_state::idle) { return item_selected_idle(); }
		if (state == e::style_state::hover) { return item_selected_hover(); }
		if (state == e::style_state::active) { return item_selected_active(); }
		AGE_UNREACHABLE();
	}

	FORCE_INLINE constexpr widget_desc
	item_not_selected(e::style_state state = e::style_state::idle) noexcept
	{
		if (state == e::style_state::idle) { return item_idle(); }
		if (state == e::style_state::hover) { return item_hover(); }
		if (state == e::style_state::active) { return item_active(); }
		AGE_UNREACHABLE();
	}

	FORCE_INLINE constexpr widget_desc
	item(bool is_selected, e::style_state state = e::style_state::idle) noexcept
	{
		if (is_selected)
		{
			return item_selected(state);
		}
		else
		{
			return item_not_selected(state);
		}
	}
}	 // namespace age::ui::style

namespace age::ui::style
{
	namespace detail
	{
		FORCE_INLINE constexpr widget_desc
		toggle_box_base() noexcept
		{
			return set_align(e::widget_align::center)
				 | set_draw(true)
				 | set_shape_kind(e::shape_kind::rounded_rect)
				 | set_body_brush_kind(e::brush_kind::color)
				 | set_border_brush_kind(e::brush_kind::color)
				 | set_padding(0.f, 0.f, 0.f, 0.f);
		}
	}	 // namespace detail

	FORCE_INLINE constexpr widget_desc
	toggle_box_on_idle() noexcept
	{
		return detail::toggle_box_base()
			 | set_border_thickness(theme::toggle_box_border_thickness())
			 | set_body_brush_data(theme::toggle_box_color_bg_on())
			 | set_border_brush_data(theme::toggle_box_color_border_on())
			 | set_shape_data(theme::toggle_box_roundness());
	}

	FORCE_INLINE constexpr widget_desc
	toggle_box_on_hover() noexcept
	{
		return detail::toggle_box_base()
			 | set_border_thickness(theme::toggle_box_border_thickness())
			 | set_body_brush_data(theme::toggle_box_color_bg_on_hover())
			 | set_border_brush_data(theme::toggle_box_color_border_on_hover())
			 | set_shape_data(theme::toggle_box_roundness());
	}

	FORCE_INLINE constexpr widget_desc
	toggle_box_on_active() noexcept
	{
		return detail::toggle_box_base()
			 | set_border_thickness(theme::toggle_box_border_thickness())
			 | set_body_brush_data(theme::toggle_box_color_bg_on_active())
			 | set_border_brush_data(theme::toggle_box_color_border_on_active())
			 | set_shape_data(theme::toggle_box_roundness());
	}

	FORCE_INLINE constexpr widget_desc
	toggle_box_off_idle() noexcept
	{
		return detail::toggle_box_base()
			 | set_border_thickness(theme::toggle_box_border_thickness())
			 | set_body_brush_data(theme::toggle_box_color_bg_off())
			 | set_border_brush_data(theme::toggle_box_color_border_off())
			 | set_shape_data(theme::toggle_box_roundness());
	}

	FORCE_INLINE constexpr widget_desc
	toggle_box_off_hover() noexcept
	{
		return detail::toggle_box_base()
			 | set_border_thickness(theme::toggle_box_border_thickness())
			 | set_body_brush_data(theme::toggle_box_color_bg_off_hover())
			 | set_border_brush_data(theme::toggle_box_color_border_off_hover())
			 | set_shape_data(theme::toggle_box_roundness());
	}

	FORCE_INLINE constexpr widget_desc
	toggle_box_off_active() noexcept
	{
		return detail::toggle_box_base()
			 | set_border_thickness(theme::toggle_box_border_thickness())
			 | set_body_brush_data(theme::toggle_box_color_bg_off_active())
			 | set_border_brush_data(theme::toggle_box_color_border_off_active())
			 | set_shape_data(theme::toggle_box_roundness());
	}

	FORCE_INLINE constexpr widget_desc
	toggle_box_on(e::style_state state = e::style_state::idle) noexcept
	{
		if (state == e::style_state::idle) { return toggle_box_on_idle(); }
		if (state == e::style_state::hover) { return toggle_box_on_hover(); }
		if (state == e::style_state::active) { return toggle_box_on_active(); }
		AGE_UNREACHABLE();
	}

	FORCE_INLINE constexpr widget_desc
	toggle_box_off(e::style_state state = e::style_state::idle) noexcept
	{
		if (state == e::style_state::idle) { return toggle_box_off_idle(); }
		if (state == e::style_state::hover) { return toggle_box_off_hover(); }
		if (state == e::style_state::active) { return toggle_box_off_active(); }
		AGE_UNREACHABLE();
	}

	FORCE_INLINE constexpr widget_desc
	toggle_box(bool is_on, e::style_state state = e::style_state::idle) noexcept
	{
		if (is_on)
		{
			return toggle_box_on();
		}
		else
		{
			return toggle_box_off();
		}
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
				 | set_width_size_mode(e::size_mode_kind::grow)
				 | set_height_size_mode(e::size_mode_kind::text)
				 | set_padding(0.f, 0.f, 0.f, 0.f)
				 | set_border_thickness(0.f);
		}
	}	 // namespace detail

	FORCE_INLINE constexpr widget_desc
	text_title(const char* p_text) noexcept
	{
		return detail::text_base()
			 | set_text(p_text)
			 | set_font_idx(g::current_font_idx)
			 | set_font_size(theme::text_title_font_size())
			 | set_body_brush_data(theme::text_title_color());
	}

	FORCE_INLINE constexpr widget_desc
	text_title_disabled(const char* p_text) noexcept
	{
		return detail::text_base()
			 | set_text(p_text)
			 | set_font_idx(g::current_font_idx)
			 | set_font_size(theme::text_title_font_size())
			 | set_body_brush_data(theme::text_title_color_disabled());
	}

	FORCE_INLINE constexpr widget_desc
	text_heading(const char* p_text) noexcept
	{
		return detail::text_base()
			 | set_text(p_text)
			 | set_font_idx(g::current_font_idx)
			 | set_font_size(theme::text_heading_font_size())
			 | set_body_brush_data(theme::text_heading_color());
	}

	FORCE_INLINE constexpr widget_desc
	text_heading_disabled(const char* p_text) noexcept
	{
		return detail::text_base()
			 | set_text(p_text)
			 | set_font_idx(g::current_font_idx)
			 | set_font_size(theme::text_heading_font_size())
			 | set_body_brush_data(theme::text_heading_color_disabled());
	}

	FORCE_INLINE constexpr widget_desc
	text(const char* p_text) noexcept
	{
		return detail::text_base()
			 | set_text(p_text)
			 | set_font_idx(g::current_font_idx)
			 | set_font_size(theme::text_font_size())
			 | set_body_brush_data(theme::text_color());
	}

	FORCE_INLINE constexpr widget_desc
	text_hover(const char* p_text) noexcept
	{
		return detail::text_base()
			 | set_text(p_text)
			 | set_font_idx(g::current_font_idx)
			 | set_font_size(theme::text_font_size())
			 | set_body_brush_data(theme::text_color_hover());
	}

	FORCE_INLINE constexpr widget_desc
	text_active(const char* p_text) noexcept
	{
		return detail::text_base()
			 | set_text(p_text)
			 | set_font_idx(g::current_font_idx)
			 | set_font_size(theme::text_font_size())
			 | set_body_brush_data(theme::text_color_active());
	}

	FORCE_INLINE constexpr widget_desc
	text_disabled(const char* p_text) noexcept
	{
		return detail::text_base()
			 | set_text(p_text)
			 | set_font_idx(g::current_font_idx)
			 | set_font_size(theme::text_font_size())
			 | set_body_brush_data(theme::text_color_disabled());
	}

	FORCE_INLINE constexpr widget_desc
	text_label(const char* p_text) noexcept
	{
		return detail::text_base()
			 | set_text(p_text)
			 | set_font_idx(g::current_font_idx)
			 | set_font_size(theme::text_label_font_size())
			 | set_body_brush_data(theme::text_label_color());
	}

	FORCE_INLINE constexpr widget_desc
	text_label_hover(const char* p_text) noexcept
	{
		return detail::text_base()
			 | set_text(p_text)
			 | set_font_idx(g::current_font_idx)
			 | set_font_size(theme::text_label_font_size())
			 | set_body_brush_data(theme::text_label_color_hover());
	}

	FORCE_INLINE constexpr widget_desc
	text_label_active(const char* p_text) noexcept
	{
		return detail::text_base()
			 | set_text(p_text)
			 | set_font_idx(g::current_font_idx)
			 | set_font_size(theme::text_label_font_size())
			 | set_body_brush_data(theme::text_label_color_active());
	}

	FORCE_INLINE constexpr widget_desc
	text_label_disabled(const char* p_text) noexcept
	{
		return detail::text_base()
			 | set_text(p_text)
			 | set_font_idx(g::current_font_idx)
			 | set_font_size(theme::text_label_font_size())
			 | set_body_brush_data(theme::text_label_color_disabled());
	}

	FORCE_INLINE constexpr widget_desc
	text_hint(const char* p_text) noexcept
	{
		return detail::text_base()
			 | set_text(p_text)
			 | set_font_idx(g::current_font_idx)
			 | set_font_size(theme::text_hint_font_size())
			 | set_body_brush_data(theme::text_hint_color());
	}

	FORCE_INLINE constexpr widget_desc
	text_hint_disabled(const char* p_text) noexcept
	{
		return detail::text_base()
			 | set_text(p_text)
			 | set_font_idx(g::current_font_idx)
			 | set_font_size(theme::text_hint_font_size())
			 | set_body_brush_data(theme::text_hint_color_disabled());
	}

	FORCE_INLINE constexpr widget_desc
	text_button(const char* p_text) noexcept
	{
		return detail::text_base()
			 | set_text(p_text)
			 | set_font_idx(g::current_font_idx)
			 | set_font_size(theme::text_button_font_size())
			 | set_body_brush_data(theme::text_button_color());
	}

	FORCE_INLINE constexpr widget_desc
	text_button_disabled(const char* p_text) noexcept
	{
		return detail::text_base()
			 | set_text(p_text)
			 | set_font_idx(g::current_font_idx)
			 | set_font_size(theme::text_button_font_size())
			 | set_body_brush_data(theme::text_button_color_disabled());
	}
}	 // namespace age::ui::style

namespace age::ui::style
{
	namespace detail
	{
		FORCE_INLINE constexpr widget_desc
		slider_track_base_h() noexcept
		{
			return set_align(e::widget_align::center)
				 | set_shape_kind(e::shape_kind::rounded_rect)
				 | set_body_brush_kind(e::brush_kind::color)
				 | set_width_grow()
				 | set_padding(0.f, 0.f, 0.f, 0.f)
				 | set_border_thickness(0.f)
				 | set_z_offset(1);
		}

		FORCE_INLINE constexpr widget_desc
		slider_track_base_v() noexcept
		{
			return set_align(e::widget_align::center)
				 | set_shape_kind(e::shape_kind::rounded_rect)
				 | set_body_brush_kind(e::brush_kind::color)
				 | set_height_grow()
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
	slider_track_h() noexcept
	{
		return detail::slider_track_base_h()
			 | set_height_fixed(theme::slider_track_size())
			 | set_body_brush_data(theme::slider_track_color_bg());
	}

	FORCE_INLINE constexpr widget_desc
	slider_track_fill_h_idle() noexcept
	{
		return detail::slider_track_base_h()
			 | set_height_fixed(theme::slider_track_size())
			 | set_body_brush_data(theme::slider_track_color_fill());
	}

	FORCE_INLINE constexpr widget_desc
	slider_track_fill_h_hover() noexcept
	{
		return detail::slider_track_base_h()
			 | set_height_fixed(theme::slider_track_size())
			 | set_body_brush_data(theme::slider_track_color_fill_hover());
	}

	FORCE_INLINE constexpr widget_desc
	slider_track_fill_h_active() noexcept
	{
		return detail::slider_track_base_h()
			 | set_height_fixed(theme::slider_track_size())
			 | set_body_brush_data(theme::slider_track_color_fill_active());
	}

	FORCE_INLINE constexpr widget_desc
	slider_track_v() noexcept
	{
		return detail::slider_track_base_v()
			 | set_width_fixed(theme::slider_track_size())
			 | set_body_brush_data(theme::slider_track_color_bg());
	}

	FORCE_INLINE constexpr widget_desc
	slider_track_fill_v_idle() noexcept
	{
		return detail::slider_track_base_v()
			 | set_width_fixed(theme::slider_track_size())
			 | set_body_brush_data(theme::slider_track_color_fill());
	}

	FORCE_INLINE constexpr widget_desc
	slider_track_fill_v_hover() noexcept
	{
		return detail::slider_track_base_v()
			 | set_width_fixed(theme::slider_track_size())
			 | set_body_brush_data(theme::slider_track_color_fill_hover());
	}

	FORCE_INLINE constexpr widget_desc
	slider_track_fill_v_active() noexcept
	{
		return detail::slider_track_base_v()
			 | set_width_fixed(theme::slider_track_size())
			 | set_body_brush_data(theme::slider_track_color_fill_active());
	}

	FORCE_INLINE constexpr widget_desc
	slider_thumb_idle() noexcept
	{
		return detail::slider_thumb_base()
			 | set_size(size_mode::fixed(theme::slider_thumb_size()), size_mode::fixed(theme::slider_thumb_size()))
			 | set_body_brush_data(theme::slider_thumb_color());
	}

	FORCE_INLINE constexpr widget_desc
	slider_thumb_hover() noexcept
	{
		return detail::slider_thumb_base()
			 | set_size(size_mode::fixed(theme::slider_thumb_size_hover()), size_mode::fixed(theme::slider_thumb_size_hover()))
			 | set_body_brush_data(theme::slider_thumb_color_hover());
	}

	FORCE_INLINE constexpr widget_desc
	slider_thumb_active() noexcept
	{
		return detail::slider_thumb_base()
			 | set_size(size_mode::fixed(theme::slider_thumb_size_active()), size_mode::fixed(theme::slider_thumb_size_active()))
			 | set_body_brush_data(theme::slider_thumb_color_active());
	}

	FORCE_INLINE constexpr widget_desc
	slider_thumb(e::style_state state = e::style_state::idle) noexcept
	{
		if (state == e::style_state::idle) { return slider_thumb_idle(); }
		if (state == e::style_state::hover) { return slider_thumb_hover(); }
		if (state == e::style_state::active) { return slider_thumb_active(); }

		AGE_UNREACHABLE();
	}

	FORCE_INLINE constexpr widget_desc
	slider_track_fill_h(e::style_state state = e::style_state::idle) noexcept
	{
		if (state == e::style_state::idle) { return slider_track_fill_h_idle(); }
		if (state == e::style_state::hover) { return slider_track_fill_h_hover(); }
		if (state == e::style_state::active) { return slider_track_fill_h_active(); }

		AGE_UNREACHABLE();
	}

	FORCE_INLINE constexpr widget_desc
	slider_tack_fill_v(e::style_state state = e::style_state::idle) noexcept
	{
		if (state == e::style_state::idle) { return slider_track_fill_v_idle(); }
		if (state == e::style_state::hover) { return slider_track_fill_v_hover(); }
		if (state == e::style_state::active) { return slider_track_fill_v_active(); }

		AGE_UNREACHABLE();
	}

}	 // namespace age::ui::style

namespace age::ui::style
{
	namespace detail
	{
		FORCE_INLINE constexpr widget_desc
		scroll_thumb_base() noexcept
		{
			return set_align(e::widget_align::begin)
				 | set_shape_kind(e::shape_kind::rounded_rect)
				 | set_body_brush_kind(e::brush_kind::color)
				 | set_border_brush_kind(e::brush_kind::color)
				 | set_padding(0.f, 0.f, 0.f, 0.f)
				 | set_z_offset(1);
		}
	}	 // namespace detail

	FORCE_INLINE constexpr widget_desc
	scroll_thumb_h_idle() noexcept
	{
		return detail::scroll_thumb_base()
			 | set_height_fixed(theme::scroll_thumb_size())
			 | set_shape_data(theme::scroll_thumb_roundness())
			 | set_body_brush_data(theme::scroll_thumb_color());
	}

	FORCE_INLINE constexpr widget_desc
	scroll_thumb_h_hover() noexcept
	{
		return detail::scroll_thumb_base()
			 | set_height_fixed(theme::scroll_thumb_size())
			 | set_shape_data(theme::scroll_thumb_roundness())
			 | set_body_brush_data(theme::scroll_thumb_color_hover());
	}

	FORCE_INLINE constexpr widget_desc
	scroll_thumb_h_active() noexcept
	{
		return detail::scroll_thumb_base()
			 | set_height_fixed(theme::scroll_thumb_size())
			 | set_shape_data(theme::scroll_thumb_roundness())
			 | set_body_brush_data(theme::scroll_thumb_color_active());
	}

	FORCE_INLINE constexpr widget_desc
	scroll_thumb_v_idle() noexcept
	{
		return detail::scroll_thumb_base()
			 | set_width_fixed(theme::scroll_thumb_size())
			 | set_shape_data(theme::scroll_thumb_roundness())
			 | set_body_brush_data(theme::scroll_thumb_color());
	}

	FORCE_INLINE constexpr widget_desc
	scroll_thumb_v_hover() noexcept
	{
		return detail::scroll_thumb_base()
			 | set_width_fixed(theme::scroll_thumb_size())
			 | set_shape_data(theme::scroll_thumb_roundness())
			 | set_body_brush_data(theme::scroll_thumb_color_hover());
	}

	FORCE_INLINE constexpr widget_desc
	scroll_thumb_v_active() noexcept
	{
		return detail::scroll_thumb_base()
			 | set_width_fixed(theme::scroll_thumb_size())
			 | set_shape_data(theme::scroll_thumb_roundness())
			 | set_body_brush_data(theme::scroll_thumb_color_active());
	}

	FORCE_INLINE constexpr widget_desc
	scroll_thumb_h(e::style_state state = e::style_state::idle) noexcept
	{
		if (state == e::style_state::idle) { return scroll_thumb_h_idle(); }
		if (state == e::style_state::hover) { return scroll_thumb_h_hover(); }
		if (state == e::style_state::active) { return scroll_thumb_h_active(); }

		AGE_UNREACHABLE();
	}

	FORCE_INLINE constexpr widget_desc
	scroll_thumb_v(e::style_state state = e::style_state::idle) noexcept
	{
		if (state == e::style_state::idle) { return scroll_thumb_v_idle(); }
		if (state == e::style_state::hover) { return scroll_thumb_v_hover(); }
		if (state == e::style_state::active) { return scroll_thumb_v_active(); }

		AGE_UNREACHABLE();
	}
}	 // namespace age::ui::style

namespace age::ui::style
{
	namespace detail
	{
		FORCE_INLINE constexpr widget_desc
		separator_base() noexcept
		{
			return set_draw(true)
				 | set_z_offset(1)
				 | set_padding(0.f, 0.f, 0.f, 0.f)
				 | set_body_brush_kind(e::brush_kind::color)
				 | set_border_thickness(0.f);
		}
	}	 // namespace detail

	FORCE_INLINE constexpr widget_desc
	separator_v() noexcept
	{
		return detail::separator_base()
			 | set_width_grow()
			 | set_height_fixed(theme::separator_thickness());
	}

	FORCE_INLINE constexpr widget_desc
	separator_h() noexcept
	{
		return detail::separator_base()
			 | set_height_grow()
			 | set_width_fixed(theme::separator_thickness());
	}
}	 // namespace age::ui::style

namespace age::ui::style
{
	namespace detail
	{
		FORCE_INLINE constexpr widget_desc
		resize_handle_base() noexcept
		{
			return set_draw(true)
				 | set_z_offset(1)
				 | set_padding(0.f, 0.f, 0.f, 0.f)
				 | set_shape_kind(e::shape_kind::rect)
				 | set_body_brush_kind(e::brush_kind::color)
				 | set_border_thickness(0.f);
		}
	}	 // namespace detail

	FORCE_INLINE constexpr widget_desc
	resize_handle_h_idle() noexcept
	{
		return detail::resize_handle_base()
			 | set_height_grow()
			 | set_body_brush_data(theme::resize_handle_color())
			 | set_width_fixed(theme::resize_handle_size());
	}

	FORCE_INLINE constexpr widget_desc
	resize_handle_h_hover() noexcept
	{
		return detail::resize_handle_base()
			 | set_height_grow()
			 | set_body_brush_data(theme::resize_handle_color_hover())
			 | set_width_fixed(theme::resize_handle_size());
	}

	FORCE_INLINE constexpr widget_desc
	resize_handle_h_active() noexcept
	{
		return detail::resize_handle_base()
			 | set_height_grow()
			 | set_body_brush_data(theme::resize_handle_color_active())
			 | set_width_fixed(theme::resize_handle_size());
	}

	FORCE_INLINE constexpr widget_desc
	resize_handle_v_idle() noexcept
	{
		return detail::resize_handle_base()
			 | set_width_grow()
			 | set_body_brush_data(theme::resize_handle_color())
			 | set_height_fixed(theme::resize_handle_size());
	}

	FORCE_INLINE constexpr widget_desc
	resize_handle_v_hover() noexcept
	{
		return detail::resize_handle_base()
			 | set_width_grow()
			 | set_body_brush_data(theme::resize_handle_color_hover())
			 | set_height_fixed(theme::resize_handle_size());
	}

	FORCE_INLINE constexpr widget_desc
	resize_handle_v_active() noexcept
	{
		return detail::resize_handle_base()
			 | set_width_grow()
			 | set_body_brush_data(theme::resize_handle_color_active())
			 | set_height_fixed(theme::resize_handle_size());
	}

	FORCE_INLINE constexpr widget_desc
	resize_handle_h(e::style_state state) noexcept
	{
		if (state == e::style_state::idle) { return resize_handle_h_idle(); }
		if (state == e::style_state::hover) { return resize_handle_h_hover(); }
		if (state == e::style_state::active) { return resize_handle_h_active(); }

		AGE_UNREACHABLE();
	}

	FORCE_INLINE constexpr widget_desc
	resize_handle_v(e::style_state state) noexcept
	{
		if (state == e::style_state::idle) { return resize_handle_v_idle(); }
		if (state == e::style_state::hover) { return resize_handle_v_hover(); }
		if (state == e::style_state::active) { return resize_handle_v_active(); }

		AGE_UNREACHABLE();
	}
}	 // namespace age::ui::style
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
	DEF(size, size_mode_width, size_mode_height)
	DEF(width, size_mode_width)
	DEF(height, size_mode_height)
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

#undef X
#undef Y

#define X(name) decltype(widget_desc::text.name) name
#define Y(name) desc.text.name = name

	DEF(font_size, font_size)
	DEF(font_idx, font_idx)

	namespace detail
	{
		struct mod_text
		{
			decltype(widget_desc::text.p_str) p_str;

			FORCE_INLINE constexpr void
			apply(widget_desc& desc) const noexcept
			{
				desc.text.p_str = p_str;
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

	FORCE_INLINE constexpr widget_desc
	set_text(decltype(widget_desc::text.p_str) p_text) noexcept
	{
		return detail::mod_text{ p_text }
			 | set_align(e::widget_align::center)
			 | set_shape_kind(e::shape_kind::text)
			 | set_font_idx(g::current_font_idx)
			 | set_font_size(g::current_font_size)
			 | set_body_brush_data(brush_data::color(1.f, 1.f, 1.f, 1.f))
			 | set_border_thickness(0.f)
			 | set_z_offset(0);
	}

#undef X
#undef Y
#undef DEF
}	 // namespace age::ui

#pragma once
#include "age.hpp"

// layout
namespace age::ui::widget
{
	FORCE_INLINE widget_ctx
	layout(e::widget_layout e_layout) noexcept
	{
		return widget::begin(style::layout(e_layout));
	}

	FORCE_INLINE widget_ctx
	vertical() noexcept
	{
		return widget::begin(style::vertical());
	}

	FORCE_INLINE widget_ctx
	horizontal() noexcept
	{
		return widget::begin(style::horizontal());
	}
}	 // namespace age::ui::widget

// text
namespace age::ui::widget
{
	FORCE_INLINE widget_ctx
	text_primary(const char* p_str, e::style_state state = e::style_state::idle) noexcept
	{
		return widget::begin(style::text_primary(p_str, state));
	}

	FORCE_INLINE widget_ctx
	text_secondary(const char* p_str, e::style_state state = e::style_state::idle) noexcept
	{
		return widget::begin(style::text_secondary(p_str, state));
	}

	FORCE_INLINE widget_ctx
	text_tertiary(const char* p_str, e::style_state state = e::style_state::idle) noexcept
	{
		return widget::begin(style::text_tertiary(p_str, state));
	}

	FORCE_INLINE widget_ctx
	text_hint(const char* p_str, e::style_state state = e::style_state::idle) noexcept
	{
		return widget::begin(style::text_hint(p_str, state));
	}

	FORCE_INLINE widget_ctx
	text_disabled(const char* p_str, e::style_state state = e::style_state::idle) noexcept
	{
		return widget::begin(style::text_disabled(p_str, state));
	}

	FORCE_INLINE widget_ctx
	text_accent(const char* p_str, e::style_state state = e::style_state::idle) noexcept
	{
		return widget::begin(style::text_accent(p_str, state));
	}

	FORCE_INLINE widget_ctx
	text_positive(const char* p_str, e::style_state state = e::style_state::idle) noexcept
	{
		return widget::begin(style::text_positive(p_str, state));
	}

	FORCE_INLINE widget_ctx
	text_negative(const char* p_str, e::style_state state = e::style_state::idle) noexcept
	{
		return widget::begin(style::text_negative(p_str, state));
	}

	FORCE_INLINE widget_ctx
	text_interactive(const char* p_str, e::style_state state = e::style_state::idle) noexcept
	{
		return widget::begin(style::text_interactive(p_str, state));
	}
}	 // namespace age::ui::widget

// button
namespace age::ui::widget
{
	FORCE_INLINE widget_ctx
	button(const char* p_str) noexcept
	{
		using enum input::e::key_kind;

		if (auto btn = widget::begin(style::layout(e::widget_layout::vertical)
									 | set_interact(true)
									 | set_width(size_mode::fit())))
		{
			auto state = e::style_state::idle;
			if (btn.pressed<mouse_left>())
			{
				state = e::style_state::active;
			}
			else if (btn.hovered())
			{
				state = e::style_state::hover;
			}

			if (auto _ = widget::begin(style::frame_interactive(state)))
			{
				widget::text_interactive(p_str, state);
			}

			return btn;
		}
		else
		{
			return {};
		}
	}
}	 // namespace age::ui::widget

// collapsible
namespace age::ui::widget
{
	FORCE_INLINE widget_ctx
	collapsible_header(const char* p_str) noexcept
	{
		using enum input::e::key_kind;

		if (auto res = widget::begin(style::layout(e::widget_layout::vertical)
									 | set_size(size_mode::fit(), size_mode::fit())))
		{
			auto is_open = false;

			if (auto header = widget::begin(style::frame()
											| set_layout(e::widget_layout::horizontal)
											| set_align(e::widget_align::center)
											| set_interact(true)
											| set_border_thickness(0.f)))
			{
				if (header.clicked<mouse_left>())
				{
					header.toggle();
				}

				is_open = header.is_toggled();

				c_auto rotation = is_open
									? 90 * math::g::degree_to_radian
									: 0.f;

				widget::begin(set_align(e::widget_align::center)
							  | set_size(size_mode::fixed(22), size_mode::fixed(22))
							  | set_border_thickness(0.f)
							  | set_rotation(rotation)
							  | set_shape_kind(e::shape_kind::arrow_right)
							  | set_body_brush_data(theme::text_primary()));

				widget::text_primary(p_str);
			}

			if (is_open)
			{
				return res;
			}
		}

		return {};
	}
}	 // namespace age::ui::widget
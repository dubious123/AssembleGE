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

// frame
namespace age::ui::widget
{
	FORCE_INLINE widget_ctx
	frame(e::style_state state = e::style_state::idle) noexcept
	{
		return widget::begin(style::frame(state));
	}

	FORCE_INLINE widget_ctx
	frame_interactive(e::style_state state = e::style_state::idle) noexcept
	{
		return widget::begin(style::frame_interactive(state));
	}

	FORCE_INLINE widget_ctx
	seperator() noexcept
	{
		return widget::begin(style::seperator());
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
	button(const char* p_str, auto&&... modifier) noexcept
	{
		using enum input::e::key_kind;

		if (auto btn = widget::begin(((style::layout(e::widget_layout::vertical)
									   | set_interact(true)
									   | set_width(size_mode::fit()))
									  | ... | modifier)))
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

			if (auto _ = widget::begin(style::frame_interactive(state) | set_layout(e::widget_layout::horizontal)))
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
									 | set_size(size_mode::grow(), size_mode::fit())))
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

				c_auto chevelon_size = font::get_line_height(theme::text_primary_font_size());

				widget::begin(set_align(e::widget_align::center)
							  | set_size(size_mode::fixed(chevelon_size), size_mode::fixed(chevelon_size))
							  | set_border_thickness(0.f)
							  | set_rotation(rotation)
							  | set_shape_kind(e::shape_kind::arrow_right)
							  | set_body_brush_data(theme::text_primary()));

				widget::text_primary(p_str);
			}

			if (is_open)
			{
				widget::seperator();
				return res;
			}
		}

		return {};
	}

	FORCE_INLINE widget_ctx
	tree_node(const char* p_str) noexcept
	{
		using enum input::e::key_kind;

		c_auto chevelon_size = font::get_line_height(theme::text_secondary_font_size());

		if (auto res = widget::begin(style::layout(e::widget_layout::vertical)
									 | set_padding_left(chevelon_size)
									 | set_size(size_mode::fit(), size_mode::fit())))
		{
			auto is_open = false;

			if (auto header = widget::begin(style::frame_interactive()
											| set_interact(true)
											| set_layout(e::widget_layout::horizontal)
											| set_align(e::widget_align::begin)))
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
							  | set_size(size_mode::fixed(chevelon_size), size_mode::fixed(chevelon_size))
							  | set_border_thickness(0.f)
							  | set_rotation(rotation)
							  | set_shape_kind(e::shape_kind::arrow_right)
							  | set_body_brush_data(theme::text_secondary()));

				widget::text_secondary(p_str);
			}

			if (is_open)
			{
				return res;
			}
		}

		return {};
	}
}	 // namespace age::ui::widget

// slider
namespace age::ui::widget
{
	FORCE_INLINE widget_ctx
	slider(float& value, float min, float max) noexcept
	{
		using enum input::e::key_kind;

		c_auto max_thumb_size = theme::slider_thumb_size(e::style_state::active);

		if (auto h_slider = widget::begin(style::layout(e::widget_layout::horizontal)
										  | set_interact(true)
										  | set_save_state(true)
										  | set_child_gap(0)
										  | set_size(size_mode::grow(), size_mode::fixed(max_thumb_size))))
		{
			c_auto& slider_widget_state = h_slider.get_state();
			c_auto	width				= std::max(slider_widget_state.width, max_thumb_size) - max_thumb_size;
			c_auto	padding				= max_thumb_size * 0.5f;

			auto slider_style_state = e::style_state::idle;
			if (h_slider.pressed<mouse_left>())
			{
				slider_style_state = e::style_state::active;

				c_auto x_min = slider_widget_state.pos.x + padding;
				c_auto x_max = x_min + width;

				c_auto new_ratio = (std::clamp(g::p_input_ctx->mouse_pos.x, x_min, x_max) - x_min) / width;

				value = (max - min) * new_ratio;
			}
			else if (h_slider.hovered())
			{
				slider_style_state = e::style_state::hover;
			}

			c_auto ratio = std::clamp(value, min, max) / (max - min);

			c_auto thumb_offset = float2{ padding - theme::slider_thumb_size(slider_style_state) * 0.5f, 0 };

			widget::begin(style::slider_fill(slider_style_state) | set_width(size_mode::fixed(width * ratio)) | set_offset(padding, 0.f));

			widget::begin(style::slider_thumb(slider_style_state) | set_offset(thumb_offset));

			c_auto track_offset = float2{ padding - theme::slider_thumb_size(slider_style_state), 0 };

			widget::begin(style::slider_track(slider_style_state) | set_width(size_mode::fixed(width * (1.f - ratio))) | set_offset(track_offset));


			return h_slider;
		}

		return {};
	}
}	 // namespace age::ui::widget

// numeric field
namespace age::ui::widget
{
	FORCE_INLINE widget_ctx
	numeric_field(float& value, float min, float max) noexcept
	{
		using enum input::e::key_kind;
		if (auto h_interact = widget::begin(style::horizontal(size_mode::grow(), size_mode::fit())
											| set_interact(true)))
		{
			auto state = e::style_state::idle;
			if (h_interact.pressed<mouse_left>())
			{
				state  = e::style_state::active;
				value += g::p_input_ctx->mouse_delta.x * 0.01f;
			}
			else if (h_interact.hovered())
			{
				state = e::style_state::hover;
			}

			if (auto _ = widget::begin(style::frame_interactive(state)
									   | set_horizontal()
									   | set_width(size_mode::grow())))
			{
				char char_buf[16];

				util::float_to_str(char_buf, value);

				widget::text_hint("X", state);
				widget::text_secondary(char_buf, state);
			}

			return h_interact;
		}
		else
		{
			return {};
		}
	}
}	 // namespace age::ui::widget

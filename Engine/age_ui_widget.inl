#pragma once
#include "age.hpp"

// layout
namespace age::ui::widget
{
	FORCE_INLINE widget_ctx
	layout(e::widget_layout e_layout, auto&&... modifier) noexcept
	{
		return widget::begin((style::layout(e_layout) | ... | FWD(modifier)));
	}

	FORCE_INLINE widget_ctx
	vertical(auto&&... modifier) noexcept
	{
		return widget::begin((style::vertical() | ... | FWD(modifier)));
	}

	FORCE_INLINE widget_ctx
	horizontal(auto&&... modifier) noexcept
	{
		return widget::begin((style::horizontal() | ... | FWD(modifier)));
	}

	FORCE_INLINE widget_ctx
	vertical_inv(auto&&... modifier) noexcept
	{
		return widget::begin((style::vertical_inv() | ... | FWD(modifier)));
	}

	FORCE_INLINE widget_ctx
	horizontal_inv(auto&&... modifier) noexcept
	{
		return widget::begin((style::horizontal_inv() | ... | FWD(modifier)));
	}
}	 // namespace age::ui::widget

// frame
namespace age::ui::widget
{
	FORCE_INLINE widget_ctx
	frame(e::style_state state = e::style_state::idle, auto&&... modifier) noexcept
	{
		return widget::begin((style::frame(state) | ... | FWD(modifier)));
	}

	FORCE_INLINE widget_ctx
	separator_h(auto&&... modifier) noexcept
	{
		return widget::begin((style::separator_h() | ... | FWD(modifier)));
	}

	FORCE_INLINE widget_ctx
	separator_v(auto&&... modifier) noexcept
	{
		return widget::begin((style::separator_v() | ... | FWD(modifier)));
	}
}	 // namespace age::ui::widget

// panel
namespace age::ui::widget
{
	FORCE_INLINE widget_ctx
	panel(auto&&... modifier) noexcept
	{
		return widget::begin((style::panel() | ... | FWD(modifier)));
	}

	FORCE_INLINE widget_ctx_impl<3>
	panel_resizable_h(float min, float max) noexcept
	{
		using enum input::e::key_kind;

		if (auto h_panel = widget::begin(style::panel()
										 | set_padding_right(0)
										 | set_horizontal_inv()
										 | set_size(size_mode::fit(min, max), size_mode::grow())
										 | set_child_gap(0)))
		{
			auto width = 0.f;

			if (auto h_handle = widget::begin(style::vertical()
											  | set_size(size_mode::fit(), size_mode::grow())
											  | set_padding(0, 0, 0, 0)
											  | set_interact(true)))
			{
				auto  style_state  = e::style_state::idle;
				auto& widget_state = h_handle.get_state();

				if (h_handle.pressed<mouse_left>())
				{
					style_state = e::style_state::active;

					widget_state.drag_x += g::p_input_ctx->mouse_delta.x;
				}

				else if (h_handle.hovered())
				{
					style_state = e::style_state::hover;
				}

				widget_state.drag_x = std::clamp(widget_state.drag_x, min, max - theme::resize_handle_size());

				width = widget_state.drag_x;

				widget::begin(style::resize_handle_h(style_state));
			}

			return widget_ctx_impl{ std::move(h_panel),
									widget::begin(style::vertical() | set_size(size_mode::fixed(width), size_mode::grow())),
									widget::vertical() };
		}

		return {};
	}

	FORCE_INLINE widget_ctx_impl<3>
	panel_resizable_v(float min, float max) noexcept
	{
		using enum input::e::key_kind;

		if (auto h_panel = widget::begin(style::panel()
										 | set_padding_bottom(0)
										 | set_vertical_inv()
										 | set_size(size_mode::grow(), size_mode::fit(min, max))
										 | set_child_gap(0)))
		{
			auto height = 0.f;

			if (auto h_handle = widget::begin(style::vertical()
											  | set_size(size_mode::grow(), size_mode::fit())
											  | set_padding(0, 0, 0, 0)
											  | set_interact(true)))
			{
				auto  style_state  = e::style_state::idle;
				auto& widget_state = h_handle.get_state();

				if (h_handle.pressed<mouse_left>())
				{
					style_state = e::style_state::active;

					widget_state.drag_y += g::p_input_ctx->mouse_delta.y;
				}

				else if (h_handle.hovered())
				{
					style_state = e::style_state::hover;
				}

				widget_state.drag_y = std::clamp(widget_state.drag_y, min, max - theme::resize_handle_size());

				height = widget_state.drag_y;

				widget::begin(style::resize_handle_v(style_state));
			}

			return widget_ctx_impl{ std::move(h_panel),
									widget::begin(style::vertical() | set_size(size_mode::grow(), size_mode::fixed(height))),
									widget::vertical() };
		}

		return {};
	}
}	 // namespace age::ui::widget

// scroll
namespace age::ui::widget
{
	FORCE_INLINE widget_ctx_impl<3>
	scroll_area_v() noexcept
	{
		using enum input::e::key_kind;
		if (auto h_panel = widget::begin(style::horizontal_inv() | set_size(size_mode::grow(), size_mode::grow())))
		{
			auto track_id	   = t_hash{};
			auto scroll_offset = 0.f;
			if (auto h_track = widget::begin(style::vertical() | set_save_state(true) | set_interact(true) | set_width_fit() | set_height_grow()))
			{
				track_id = h_track.hash_id;

				auto& track_state	 = h_track.get_state();
				auto& thumb_offset_y = track_state.drag_y;

				c_auto thumb_height		  = track_state.drag_x;
				c_auto thumb_width		  = thumb_height > 0.f ? theme::scroll_thumb_size() : 0.f;
				c_auto thumb_y_offset_max = std::max(track_state.height - thumb_height, 0.f);

				auto style_state = e::style_state::idle;

				if (h_track.pressed<mouse_left>())
				{
					style_state = e::style_state::active;

					c_auto thumb_y_min = track_state.pos.y + thumb_offset_y;
					c_auto thumb_y_max = thumb_y_min + thumb_height;

					if (g::p_input_ctx->mouse_pos.y < thumb_y_min or g::p_input_ctx->mouse_pos.y > thumb_y_max)
					{
						thumb_offset_y = g::p_input_ctx->mouse_pos.y - track_state.pos.y - thumb_height * 0.5f;
					}
					else
					{
						track_state.drag_y += g::p_input_ctx->mouse_delta.y;
					}
				}
				else if (h_track.hovered())
				{
					style_state = e::style_state::hover;
				}

				if (g::p_input_ctx->is_alt_down())
				{
					track_state.drag_y -= g::p_input_ctx->wheel_delta * 40.f;
				}
				else
				{
					track_state.drag_y -= g::p_input_ctx->wheel_delta * 20.f;
				}

				track_state.drag_y = std::clamp(track_state.drag_y, 0.f, thumb_y_offset_max);

				widget::begin(style::scroll_thumb_v(style_state) | set_offset(0, thumb_offset_y) | set_width_fixed(thumb_width) | set_height_fixed(thumb_height));

				scroll_offset = (thumb_offset_y / (thumb_y_offset_max)) * track_state.drag_z;
			}

			if (auto h_content = widget::panel(set_width_grow(), set_height_grow()))
			{
				if (auto h_scroll_panel = widget::begin(style::vertical()
														| set_draw(true)
														| set_align(e::widget_align::begin)
														| set_offset(0, -scroll_offset)
														| set_save_state(true)
														| set_size(size_mode::grow(), size_mode::fit())))
				{
					auto& widget_state = h_scroll_panel.get_state();

					auto& track_state = g::widget_state_map[track_id];

					if (c_auto need_scroll = widget_state.height > widget_state.clip_height + math::g::epsilon_1e4)
					{
						track_state.drag_x = std::max((widget_state.clip_height / widget_state.height) * widget_state.clip_height, 20.f);
						track_state.drag_z = widget_state.height - widget_state.clip_height;
					}
					else
					{
						track_state.drag_x = 0;
						track_state.drag_z = 0;
					}

					return widget_ctx_impl{ FWD(h_panel), FWD(h_content), FWD(h_scroll_panel) };
				}
			}


			AGE_UNREACHABLE();
		}

		return {};
	}

	FORCE_INLINE widget_ctx_impl<3>
	scroll_area_h() noexcept
	{
		using enum input::e::key_kind;
		if (auto h_panel = widget::begin(style::vertical_inv() | set_size(size_mode::grow(), size_mode::grow())))
		{
			auto track_id	   = t_hash{};
			auto scroll_offset = 0.f;
			if (auto h_track = widget::begin(style::horizontal() | set_save_state(true) | set_interact(true) | set_height_fit() | set_width_grow()))
			{
				track_id = h_track.hash_id;

				auto& track_state	 = h_track.get_state();
				auto& thumb_offset_x = track_state.drag_y;

				c_auto thumb_width		  = track_state.drag_x;
				c_auto thumb_height		  = thumb_width > 0.f ? theme::scroll_thumb_size() : 0.f;
				c_auto thumb_x_offset_max = std::max(track_state.width - thumb_width, 0.f);

				auto style_state = e::style_state::idle;

				if (h_track.pressed<mouse_left>())
				{
					style_state = e::style_state::active;

					c_auto thumb_x_min = track_state.pos.x + thumb_offset_x;
					c_auto thumb_x_max = thumb_x_min + thumb_width;

					if (g::p_input_ctx->mouse_pos.x < thumb_x_min or g::p_input_ctx->mouse_pos.x > thumb_x_max)
					{
						thumb_offset_x = g::p_input_ctx->mouse_pos.x - track_state.pos.x - thumb_width * 0.5f;
					}
					else
					{
						track_state.drag_y += g::p_input_ctx->mouse_delta.x;
					}
				}
				else if (h_track.hovered())
				{
					style_state = e::style_state::hover;
				}

				if (g::p_input_ctx->is_alt_down())
				{
					track_state.drag_y -= g::p_input_ctx->wheel_delta * 40.f;
				}
				else
				{
					track_state.drag_y -= g::p_input_ctx->wheel_delta * 20.f;
				}

				track_state.drag_y = std::clamp(track_state.drag_y, 0.f, thumb_x_offset_max);

				widget::begin(style::scroll_thumb_h(style_state) | set_offset(thumb_offset_x, 0) | set_width_fixed(thumb_width) | set_height_fixed(thumb_height));

				scroll_offset = (thumb_offset_x / (thumb_x_offset_max)) * track_state.drag_z;
			}

			if (auto h_content = widget::panel(set_width_grow(), set_height_grow()))
			{
				if (auto h_scroll_panel = widget::begin(style::horizontal()
														| set_draw(true)
														| set_align(e::widget_align::begin)
														| set_offset(-scroll_offset, 0)
														| set_save_state(true)
														| set_size(size_mode::fit(), size_mode::grow())))
				{
					auto& widget_state = h_scroll_panel.get_state();

					auto& track_state = g::widget_state_map[track_id];

					if (c_auto need_scroll = widget_state.width > widget_state.clip_width + math::g::epsilon_1e4)
					{
						track_state.drag_x = std::max((widget_state.clip_width / widget_state.width) * widget_state.clip_width, 20.f);
						track_state.drag_z = widget_state.width - widget_state.clip_width;
					}
					else
					{
						track_state.drag_x = 0;
						track_state.drag_z = 0;
					}

					return widget_ctx_impl{ FWD(h_panel), FWD(h_content), FWD(h_scroll_panel) };
				}
			}
			AGE_UNREACHABLE();
		}

		return {};
	}
}	 // namespace age::ui::widget

// text
namespace age::ui::widget
{
	FORCE_INLINE constexpr widget_ctx
	text_title(const char* p_text, bool enabled = true) noexcept
	{
		if (enabled)
		{
			return widget::begin(style::text_title(p_text));
		}
		else
		{
			return widget::begin(style::text_title_disabled(p_text));
		}
	}

	FORCE_INLINE constexpr widget_ctx
	text_heading(const char* p_text, bool enabled = true) noexcept
	{
		if (enabled)
		{
			return widget::begin(style::text_heading(p_text));
		}
		else
		{
			return widget::begin(style::text_heading_disabled(p_text));
		}
	}

	FORCE_INLINE widget_ctx
	text(const char* p_text, e::style_state state = e::style_state::idle, bool enabled = true) noexcept
	{
		if (enabled)
		{
			if (state == e::style_state::idle) { return widget::begin(style::text(p_text)); }
			if (state == e::style_state::hover) { return widget::begin(style::text_hover(p_text)); }
			if (state == e::style_state::active) { return widget::begin(style::text_active(p_text)); }

			AGE_UNREACHABLE();
		}
		else
		{
			return widget::begin(style::text_disabled(p_text));
		}
	}

	FORCE_INLINE widget_ctx
	text_label(const char* p_text, e::style_state state = e::style_state::idle, bool enabled = true) noexcept
	{
		if (enabled)
		{
			if (state == e::style_state::idle) { return widget::begin(style::text_label(p_text)); }
			if (state == e::style_state::hover) { return widget::begin(style::text_label_hover(p_text)); }
			if (state == e::style_state::active) { return widget::begin(style::text_label_active(p_text)); }

			AGE_UNREACHABLE();
		}
		else
		{
			return widget::begin(style::text_label_disabled(p_text));
		}
	}

	FORCE_INLINE constexpr widget_ctx
	text_hint(const char* p_text, bool enabled = true) noexcept
	{
		if (enabled)
		{
			return widget::begin(style::text_hint(p_text));
		}
		else
		{
			return widget::begin(style::text_hint_disabled(p_text));
		}
	}

	FORCE_INLINE constexpr widget_ctx
	text_button(const char* p_text, bool enabled = true) noexcept
	{
		if (enabled)
		{
			return widget::begin(style::text_button(p_text));
		}
		else
		{
			return widget::begin(style::text_button_disabled(p_text));
		}
	}
}	 // namespace age::ui::widget

// button
namespace age::ui::widget
{
	FORCE_INLINE widget_ctx
	button(const char* p_str, auto&&... mod) noexcept
	{
		using enum input::e::key_kind;

		if (auto btn = widget::begin(((style::layout(e::widget_layout::vertical)
									   | set_z_offset(1)
									   | set_interact(true)
									   | set_size(size_mode::fit(), size_mode::fit()))
									  | ... | FWD(mod))))
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

			if (auto _ = widget::frame(state, set_size(size_mode::fit(), size_mode::fit()), FWD(mod)...))
			{
				widget::text_button(p_str);
			}

			return btn;
		}
		else
		{
			return {};
		}
	}

	FORCE_INLINE widget_ctx
	checkbox(const char* p_label, bool& value, auto&&... modifier) noexcept
	{
		using enum input::e::key_kind;

		if (auto btn = widget::begin(((style::layout(e::widget_layout::horizontal)
									   | set_align_center()
									   | set_z_offset(1)
									   | set_interact(true)
									   | set_size(size_mode::fit(), size_mode::fit()))
									  | ... | FWD(modifier))))
		{
			auto state = e::style_state::idle;

			if (btn.clicked())
			{
				value = !value;
			}

			if (btn.pressed<mouse_left>())
			{
				state = e::style_state::active;
			}
			else if (btn.hovered())
			{
				state = e::style_state::hover;
			}


			c_auto box_size = font::get_line_height(theme::text_button_font_size());

			if (auto _ = widget::begin(style::toggle_box(value, state)
									   | set_width_fixed(box_size)
									   | set_height_fixed(box_size)))
			{
				if (value)
				{
					widget::begin(set_align(e::widget_align::center)
								  | set_border_thickness(0.f)
								  | set_body_brush_data(theme::color_white())
								  | set_width_grow()
								  | set_height_grow()
								  | set_shape_kind(e::shape_kind::check));
				}
			}


			widget::text_button(p_label);


			return btn;
		}
		else
		{
			return {};
		}
	}

	FORCE_INLINE widget_ctx
	selectable(const char* p_label, const bool& selected, auto&&... mod) noexcept
	{
		using enum input::e::key_kind;

		if (auto btn = widget::begin(((style::layout(e::widget_layout::vertical)
									   | set_z_offset(1)
									   | set_draw(false)
									   | set_interact(true)
									   | set_size(size_mode::grow(), size_mode::fit()))
									  | ... | FWD(mod))))
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

			if (auto _ = widget::begin(style::item(selected, state)))
			{
				widget::text(p_label, state);
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
	collapsible_header(const char* p_str, auto&&... modifier) noexcept
	{
		using enum input::e::key_kind;

		if (auto res = widget::begin(style::layout(e::widget_layout::vertical)
									 | set_size(size_mode::grow(), size_mode::fit())))
		{
			auto is_open = false;

			if (auto header = widget::begin(style::header_bar() | set_interact(true)))
			{
				if (header.clicked<mouse_left>())
				{
					header.toggle();
				}

				is_open = header.is_toggled();

				c_auto rotation = is_open
									? 90 * math::g::degree_to_radian
									: 0.f;

				c_auto chevelon_size = font::get_line_height(theme::text_heading_font_size());

				widget::begin(set_align(e::widget_align::center)
							  | set_size(size_mode::fixed(chevelon_size), size_mode::fixed(chevelon_size))
							  | set_border_thickness(0.f)
							  | set_rotation(rotation)
							  | set_shape_kind(e::shape_kind::arrow_right)
							  | set_body_brush_data(theme::color_white()));

				widget::text_heading(p_str);
			}

			if (is_open)
			{
				widget::separator_v();
				return res;
			}
		}

		return {};
	}

	FORCE_INLINE widget_ctx
	collapsible_header2(const char* p_str, auto&&... modifier) noexcept
	{
		using enum input::e::key_kind;

		if (auto res = widget::begin(style::layout(e::widget_layout::vertical)
									 | set_size(size_mode::grow(), size_mode::fit())))
		{
			auto is_open = false;

			if (auto header = widget::begin(style::header_bar() | set_interact(true)))
			{
				if (header.clicked<mouse_left>())
				{
					header.toggle();
				}

				is_open = header.is_toggled();

				c_auto disclosure_indicator_size = font::get_line_height(theme::text_heading_font_size());

				if (auto _ = widget::begin(style::horizontal() | set_size(size_mode::fixed(disclosure_indicator_size), size_mode::fixed(disclosure_indicator_size)) | set_padding(theme::padding_small()) | set_align_center()))
				{
					if (is_open)
					{
						widget::begin(set_align(e::widget_align::center)
									  | set_size(size_mode::grow(), size_mode::grow())
									  | set_border_thickness(0.f)
									  // | set_rotation(180 * math::g::degree_to_radian)
									  | set_padding(theme::padding_medium())
									  | set_shape_kind(e::shape_kind::triangle)
									  | set_body_brush_data(theme::color_text_gray()));
					}
					else
					{
						widget::begin(set_align(e::widget_align::center)
									  | set_size(size_mode::grow(), size_mode::fixed(theme::thickness_thick()))
									  | set_padding(theme::padding_medium())
									  | set_border_thickness(0.f)
									  | set_shape_kind(e::shape_kind::rect)
									  | set_body_brush_data(theme::color_text_gray_dark()));
					}
				}


				widget::text_heading(p_str);
			}

			if (is_open)
			{
				widget::separator_v();
				return res;
			}
		}

		return {};
	}

	FORCE_INLINE widget_ctx_impl<2>
	tree_node(const char* p_str) noexcept
	{
		using enum input::e::key_kind;

		c_auto chevelon_size = font::get_line_height(theme::text_font_size());
		c_auto child_padding = chevelon_size + theme::frame_padding().x + theme::frame_padding().y + theme::gap_small();

		if (auto res = widget::begin(style::layout(e::widget_layout::vertical)
									 | set_size(size_mode::grow(), size_mode::fit())))
		{
			auto is_open = false;

			if (auto header = widget::begin(style::frame()
											| set_interact(true)
											| set_width(size_mode::grow())
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
							  | set_body_brush_data(theme::color_white()));

				widget::text(p_str);
			}

			if (is_open)
			{
				return widget_ctx_impl{ std::move(res),
										widget::begin(style::layout(e::widget_layout::vertical)
													  | set_padding_left(child_padding)
													  | set_size(size_mode::grow(), size_mode::fit())) };
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

		c_auto max_thumb_size = std::max(std::max(theme::slider_thumb_size(), theme::slider_thumb_size_hover()), theme::slider_thumb_size_active());

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

			c_auto thumb_size = slider_style_state == e::style_state::idle
								  ? theme::slider_thumb_size()
							  : slider_style_state == e::style_state::hover
								  ? theme::slider_thumb_size_hover()
								  : theme::slider_thumb_size_active();

			c_auto thumb_offset = float2{ padding - thumb_size * 0.5f, 0 };

			widget::begin(style::slider_track_fill_h(slider_style_state) | set_width(size_mode::fixed(width * ratio)) | set_offset(padding, 0.f));

			widget::begin(style::slider_thumb(slider_style_state) | set_offset(thumb_offset));

			c_auto track_offset = float2{ padding - thumb_size, 0 };

			widget::begin(style::slider_track_h() | set_width(size_mode::fixed(width * (1.f - ratio))) | set_offset(track_offset));


			return h_slider;
		}

		return {};
	}
}	 // namespace age::ui::widget

// text input
namespace age::ui::widget
{
	namespace detail
	{
		FORCE_INLINE void
		handle_text_click(widget_state& state, const char* p_buf, widget_desc& text_desc,
						  float width, float2 mouse_offset, uint8 click_count) noexcept
		{
			ui::detail::gen_text_data(text_desc);

			auto res = ui::detail::screen_offset_to_cursor(text_desc.text.text_data_idx, mouse_offset, width);

			state.cursor.offset_x = res.offset.x;
			state.cursor.offset_y = res.offset.y;

			if (click_count == 3)
			{
				state.cursor.anchor_byte_pos = res.line_byte_offset;
				state.cursor.byte_pos		 = res.line_byte_offset + res.line_byte_size;
			}
			else if (click_count == 2)
			{
				state.cursor.anchor_byte_pos = res.word_byte_offset;
				state.cursor.byte_pos		 = res.word_byte_offset + res.word_byte_size;
			}
			else
			{
				state.cursor.byte_pos		 = res.byte_offset;
				state.cursor.anchor_byte_pos = g::p_input_ctx->is_shift_down()
												 ? state.cursor.anchor_byte_pos
												 : res.byte_offset;
			}
		}

		FORCE_INLINE void
		handle_text_edit(widget_state& state, char* p_buf, uint32 buf_size,
						 widget_desc& text_desc, float width) noexcept
		{
			using enum input::e::key_kind;

			c_auto font_size		= text_desc.text.font_size;
			c_auto text_line_height = font::get_line_height(font_size, g::current_font_idx);

			auto regen_cursor = false;

			if (g::p_input_ctx->is_pressed_or_repeat(key_up))
			{
				state.cursor.offset_y -= text_line_height - math::g::epsilon_1e4;
				regen_cursor		   = !regen_cursor;
			}

			if (g::p_input_ctx->is_pressed_or_repeat(key_down))
			{
				state.cursor.offset_y += text_line_height + math::g::epsilon_1e4;
				regen_cursor		   = !regen_cursor;
			}

			if (regen_cursor)
			{
				ui::detail::gen_text_data(text_desc);
				auto res = ui::detail::screen_offset_to_cursor(text_desc.text.text_data_idx,
															   float2{ state.cursor.offset_x, state.cursor.offset_y }, width);

				state.cursor.offset_x = res.offset.x;
				state.cursor.offset_y = res.offset.y;
				state.cursor.byte_pos = res.byte_offset;

				if (g::p_input_ctx->is_shift_down() is_false)
				{
					state.cursor.anchor_offset_x = res.offset.x;
					state.cursor.anchor_offset_y = res.offset.y;
					state.cursor.anchor_byte_pos = res.byte_offset;
				}
			}

			ui::detail::gen_text_data(text_desc);

			auto cursor = ui::detail::byte_offset_to_cursor(text_desc.text.text_data_idx, state.cursor.byte_pos, width);
			auto anchor = ui::detail::byte_offset_to_cursor(text_desc.text.text_data_idx, state.cursor.anchor_byte_pos, width);

			cursor.anchor_byte_offset = anchor.byte_offset;
			ui::detail::update_text_buf(p_buf, buf_size, cursor);
			anchor.byte_offset = cursor.anchor_byte_offset;

			state.cursor.offset_x		 = cursor.offset.x;
			state.cursor.offset_y		 = cursor.offset.y;
			state.cursor.byte_pos		 = cursor.byte_offset;
			state.cursor.anchor_offset_x = anchor.offset.x;
			state.cursor.anchor_offset_y = anchor.offset.y;
			state.cursor.anchor_byte_pos = anchor.byte_offset;
		}

		FORCE_INLINE void
		draw_text_cursor_and_selection(const widget_state& state, const cursor_data& cursor,
									   const cursor_data& anchor, widget_desc& text_desc,
									   float width, float text_line_height) noexcept
		{
			if (c_auto has_selection = state.cursor.byte_pos != state.cursor.anchor_byte_pos)
			{
				c_auto min_x = std::min(state.cursor.offset_x, state.cursor.anchor_offset_x);
				c_auto max_x = std::max(state.cursor.offset_x, state.cursor.anchor_offset_x);
				c_auto min_y = std::min(state.cursor.offset_y, state.cursor.anchor_offset_y);
				c_auto max_y = std::max(state.cursor.offset_y, state.cursor.anchor_offset_y);

				if (state.cursor.offset_y == state.cursor.anchor_offset_y)
				{
					draw_direct(style::vertical()
								| set_draw(true)
								| set_align(e::widget_align::begin)
								| set_size(size_mode::fixed(max_x - min_x), size_mode::fixed(text_line_height))
								| set_z_offset(0)
								| set_offset(min_x, min_y)
								| set_body_brush_data(theme::color_blue(), theme::opacity_medium()));
				}
				else
				{
					auto&& [first_x, last_x, first_line_width] =
						state.cursor.offset_y < state.cursor.anchor_offset_y
							? std::tuple{ state.cursor.offset_x, state.cursor.anchor_offset_x, cursor.line_width }
							: std::tuple{ state.cursor.anchor_offset_x, state.cursor.offset_x, anchor.line_width };

					draw_direct(style::vertical()
								| set_draw(true)
								| set_align(e::widget_align::begin)
								| set_size(size_mode::fixed(first_line_width - first_x), size_mode::fixed(text_line_height))
								| set_z_offset(0)
								| set_offset(first_x, min_y)
								| set_body_brush_data(theme::color_blue(), theme::opacity_medium()));

					for (auto i : views::loop(static_cast<uint32>((max_y - min_y - math::g::epsilon_1e4) / text_line_height)))
					{
						c_auto offset	 = float2{ 0.f, min_y + (i + 1) * text_line_height };
						auto   line_info = ui::detail::screen_offset_to_cursor(text_desc.text.text_data_idx, offset, width);

						draw_direct(style::vertical()
									| set_draw(true)
									| set_align(e::widget_align::begin)
									| set_size(size_mode::fixed(line_info.line_width), size_mode::fixed(text_line_height))
									| set_z_offset(0)
									| set_offset(offset)
									| set_body_brush_data(theme::color_blue(), theme::opacity_medium()));
					}

					draw_direct(style::vertical()
								| set_draw(true)
								| set_align(e::widget_align::begin)
								| set_size(size_mode::fixed(last_x), size_mode::fixed(text_line_height))
								| set_z_offset(0)
								| set_offset(0, max_y)
								| set_body_brush_data(theme::color_blue(), theme::opacity_medium()));
				}
			}

			draw_direct(style::vertical()
						| set_draw(true)
						| set_align(e::widget_align::begin)
						| set_size(size_mode::fixed(theme::cursor_thickness()), size_mode::fixed(text_line_height))
						| set_z_offset(0)
						| set_offset(state.cursor.offset_x, state.cursor.offset_y)
						| set_body_brush_data(theme::color_white()));
		}
	}	 // namespace detail

	FORCE_INLINE widget_ctx
	text_input(char* p_buf, uint32 buf_size, auto&&... modifier) noexcept
	{
		if (auto h = widget::begin(style::frame()
								   | set_horizontal()
								   | set_child_gap(0)
								   | set_interact(true)
								   | set_save_state(true)
								   | set_width_grow()
								   | set_height_fit()))
		{
			c_auto font_size		= theme::text_font_size();
			c_auto text_line_height = font::get_line_height(font_size, g::current_font_idx);

			auto text_desc = ((style::text(p_buf)
							   | set_align(e::widget_align::begin)
							   | set_font_size(font_size))
							  | ... | FWD(modifier));

			auto& state = h.get_state();

			c_auto width		= state.width - (theme::frame_padding().x + theme::frame_padding().y);
			c_auto mouse_offset = g::p_input_ctx->mouse_pos - state.pos - float2{ theme::frame_padding().x, theme::frame_padding().z };

			if (h.triple_clicked())
			{
				detail::handle_text_click(state, p_buf, text_desc, width, mouse_offset, 3);
			}
			else if (h.double_clicked())
			{
				detail::handle_text_click(state, p_buf, text_desc, width, mouse_offset, 2);
			}
			else if (h.clicked())
			{
				detail::handle_text_click(state, p_buf, text_desc, width, mouse_offset, 1);
			}
			else if (h.focused() and not h.pressed())
			{
				detail::handle_text_edit(state, p_buf, buf_size, text_desc, width);

				auto cursor = ui::detail::byte_offset_to_cursor(text_desc.text.text_data_idx, state.cursor.byte_pos, width);
				auto anchor = ui::detail::byte_offset_to_cursor(text_desc.text.text_data_idx, state.cursor.anchor_byte_pos, width);

				detail::draw_text_cursor_and_selection(state, cursor, anchor, text_desc, width, text_line_height);
			}

			widget::begin(std::move(text_desc));
			return h;
		}

		return {};
	}
}	 // namespace age::ui::widget

// numeric field
namespace age::ui::widget
{
	template <meta::cx_arithmetic t>
	FORCE_INLINE widget_ctx
	numeric_field(t&		  value,
				  const char* p_label		   = nullptr,
				  t			  min			   = std::numeric_limits<t>::lowest(),
				  t			  max			   = std::numeric_limits<t>::max(),
				  float4	  text_label_color = theme::text_label_color(),
				  float		  step			   = 0.1f) noexcept
	{
		using enum input::e::key_kind;
		if (auto h_interact = widget::begin(style::horizontal(size_mode::grow(), size_mode::fit())
											| set_interact(true)
											| set_save_state(true)))
		{
			auto& state		 = h_interact.get_state();
			auto& is_editing = state.toggled;

			step *= g::step_scale_table[g::p_input_ctx->is_ctrl_down()][g::p_input_ctx->is_shift_down()];

			auto style_state = e::style_state::idle;

			if (h_interact.double_clicked())
			{
				is_editing = true;
				util::to_str(g::numeric_field_text_edit_buf, value);

				// select all
				state.cursor.anchor_byte_pos = 0;
				state.cursor.byte_pos		 = static_cast<uint32>(std::strlen(g::numeric_field_text_edit_buf));
			}

			if (is_editing)
			{
				style_state = e::style_state::active;

				if (g::p_input_ctx->is_pressed(key_enter))
				{
					is_editing = false;
					if (util::from_str(g::numeric_field_text_edit_buf, value))
					{
						value = std::clamp(value, min, max);
					}
				}
				else if (g::p_input_ctx->is_pressed(key_escape))
				{
					is_editing = false;
				}
			}
			else
			{
				if constexpr (std::is_floating_point_v<t>)
				{
					if (h_interact.pressed<mouse_left>())
					{
						style_state	 = e::style_state::active;
						value		+= g::p_input_ctx->mouse_delta.x * step;
						value		 = std::clamp(value, min, max);
					}
					else if (h_interact.hovered())
					{
						style_state = e::style_state::hover;
					}
				}
				else
				{
					if (h_interact.clicked<mouse_left>())
					{
						style_state	 = e::style_state::active;
						state.drag_x = 0.f;
					}
					else if (h_interact.pressed<mouse_left>())
					{
						style_state = e::style_state::active;

						state.drag_x += g::p_input_ctx->mouse_delta.x * step;

						auto delta = static_cast<std::make_signed_t<t>>(state.drag_x);

						delta = std::clamp(delta,
										   static_cast<std::make_signed_t<t>>(min) - static_cast<std::make_signed_t<t>>(value),
										   static_cast<std::make_signed_t<t>>(max) - static_cast<std::make_signed_t<t>>(value));


						// value  = std::clamp(value, min + std::abs(delta), max - std::abs(delta));
						value += delta;

						state.drag_x -= static_cast<float>(delta);
					}
					else if (h_interact.hovered())
					{
						style_state = e::style_state::hover;
					}
				}
			}

			if (auto _ = widget::horizontal(set_size(size_mode::grow(), size_mode::fit())))
			{
				if (p_label is_not_nullptr)
				{
					widget::begin(style::text_label(p_label)
								  | set_body_brush_data(text_label_color));
				}

				if (is_editing)
				{
					if (auto _ = widget::begin(style::frame(e::style_state::active)
											   | set_horizontal()
											   | set_width(size_mode::grow())))
					{
						c_auto font_size		= theme::text_font_size();
						c_auto text_line_height = font::get_line_height(font_size, g::current_font_idx);

						auto text_desc = style::text_active(g::numeric_field_text_edit_buf)
									   | set_align(e::widget_align::begin)
									   | set_font_size(font_size);

						c_auto width = state.width - (theme::frame_padding().x, theme::frame_padding().y);

						detail::handle_text_edit(state, g::numeric_field_text_edit_buf, 65, text_desc, width);

						auto cursor = ui::detail::byte_offset_to_cursor(text_desc.text.text_data_idx, state.cursor.byte_pos, width);
						auto anchor = ui::detail::byte_offset_to_cursor(text_desc.text.text_data_idx, state.cursor.anchor_byte_pos, width);

						detail::draw_text_cursor_and_selection(state, cursor, anchor, text_desc, width, text_line_height);

						widget::begin(std::move(text_desc));
					}
				}
				else
				{
					if (auto _ = widget::begin(style::frame(style_state)
											   | set_horizontal()
											   | set_width(size_mode::grow())))
					{
						char char_buf[21];
						util::to_str(char_buf, value);
						widget::text(char_buf, style_state);
					}
				}
			}

			return h_interact;
		}

		return {};
	}

	template <meta::cx_arithmetic t>
	FORCE_INLINE widget_ctx
	numeric_field(t&		  value,
				  const char* p_label,
				  t			  min,
				  t			  max,
				  float3	  text_label_color,
				  float		  step = 0.1f) noexcept
	{
		return numeric_field(value, p_label, min, max, float4{ text_label_color, 1.f }, step);
	}

	template <template <typename> typename t_vec, meta::cx_arithmetic t>
	requires(not requires(t_vec<t> m) { m[0][0]; })
	FORCE_INLINE widget_ctx
	numeric_field(t_vec<t>&		  value,
				  const char*	  p_label		   = nullptr,
				  const t_vec<t>& min			   = t_vec<t>{ std::numeric_limits<t>::lowest() },
				  const t_vec<t>& max			   = t_vec<t>{ std::numeric_limits<t>::max() },
				  float4		  text_label_color = theme::text_label_color(),
				  float			  step			   = 0.1f) noexcept
	{
		if (auto row = widget::horizontal(set_size(size_mode::grow(), size_mode::fit())))
		{
			if (p_label is_not_nullptr)
			{
				if (auto _ = widget::vertical(set_align_center(), set_size(size_mode::grow(), size_mode::fit())))
				{
					widget::begin(style::text_label(p_label)
								  | set_align(e::widget_align::begin)
								  | set_body_brush_data(text_label_color));
				}
			}

			if constexpr (requires { value.x; })
			{
				numeric_field(value.x, "X", min.x, max.x, theme::color_text_red(), step);
			}

			if constexpr (requires { value.y; })
			{
				numeric_field(value.y, "Y", min.y, max.y, theme::color_text_green(), step);
			}

			if constexpr (requires { value.z; })
			{
				numeric_field(value.z, "Z", min.z, max.z, theme::color_text_blue(), step);
			}

			if constexpr (requires { value.w; })
			{
				numeric_field(value.w, "W", min.w, max.w, theme::color_text_amber(), step);
			}
		}
		return {};
	}

	template <template <typename> typename t_vec, meta::cx_arithmetic t>
	requires(not requires(t_vec<t> m) { m[0][0]; })
	FORCE_INLINE widget_ctx
	numeric_field(t_vec<t>&		  value,
				  const char*	  p_label,
				  const t_vec<t>& min,
				  const t_vec<t>& max,
				  float3		  text_label_color,
				  float			  step = 0.1f) noexcept
	{
		return numeric_field(value, p_label, min, max, text_label_color, step);
	}

	template <template <typename> typename t_mat, meta::cx_arithmetic t>
	requires requires(t_mat<t> m) { m[0][0]; }
	FORCE_INLINE widget_ctx
	numeric_field(t_mat<t>&	  value,
				  const char* p_label		   = nullptr,
				  const t	  min			   = std::numeric_limits<t>::lowest(),
				  const t	  max			   = std::numeric_limits<t>::max(),
				  float4	  text_label_color = theme::text_label_color(),
				  float		  step			   = 0.1f) noexcept
	{
		if (auto col = widget::begin(style::vertical(size_mode::grow(), size_mode::fit())))
		{
			if (p_label is_not_nullptr)
			{
				widget::begin(style::text_label(p_label)
							  | set_align(e::widget_align::begin)
							  | set_body_brush_data(text_label_color));
			}
			if constexpr (requires { value.r0; })
			{
				using row_t = BARE_OF(value.r0);
				numeric_field(value.r0, "Row 0", row_t{ min }, row_t{ max }, theme::text_label_color(), step);
			}

			if constexpr (requires { value.r1; })
			{
				using row_t = BARE_OF(value.r1);
				numeric_field(value.r1, "Row 1", row_t{ min }, row_t{ max }, theme::text_label_color(), step);
			}

			if constexpr (requires { value.r2; })
			{
				using row_t = BARE_OF(value.r2);
				numeric_field(value.r2, "Row 2", row_t{ min }, row_t{ max }, theme::text_label_color(), step);
			}

			if constexpr (requires { value.r3; })
			{
				using row_t = BARE_OF(value.r3);
				numeric_field(value.r3, "Row 3", row_t{ min }, row_t{ max }, theme::text_label_color(), step);
			}

			return col;
		}
		return {};
	}

	template <template <typename> typename t_mat, meta::cx_arithmetic t>
	requires requires(t_mat<t> m) { m[0][0]; }
	FORCE_INLINE widget_ctx
	numeric_field(t_mat<t>&	  value,
				  const char* p_label,
				  const t	  min,
				  const t	  max,
				  float3	  text_label_color,
				  float		  step = 0.1f) noexcept
	{
		return numeric_field(value, p_label, min, max, float4{ text_label_color, 1.f }, step);
	}

	FORCE_INLINE widget_ctx
	rotation_field(float4&	   value,
				   const char* p_label			= nullptr,
				   float4	   text_label_color = theme::text_label_color(),
				   float	   step				= 0.1f) noexcept
	{
		if (auto h = widget::begin(style::horizontal(size_mode::grow(), size_mode::fit())
								   | set_save_state(true)))
		{
			auto& state = h.get_state();

			c_auto quat_now = float4(value);

			if (not state.toggled or state.rotation_field.quat != quat_now)
			{
				state.rotation_field.euler = age::math::quat_to_euler_deg(value);
				state.rotation_field.quat  = quat_now;
				state.toggled			   = true;
			}

			c_auto euler_prev = state.rotation_field.euler;
			auto   euler	  = state.rotation_field.euler;

			numeric_field(euler, p_label, float3{ -180.f }, float3{ 180.f }, text_label_color, step);

			if (euler != euler_prev)
			{
				value				   = age::math::normalize(age::math::euler_deg_to_quat(euler));
				auto& s				   = h.get_state();
				s.rotation_field.euler = euler;
				s.rotation_field.quat  = float4(value);
			}

			return h;
		}
		return {};
	}

	FORCE_INLINE widget_ctx
	rotation_field(float4&	   value,
				   const char* p_label,
				   float3	   text_label_color,
				   float	   step = 0.1f) noexcept
	{
		return rotation_field(value, p_label, text_label_color, step);
	}
}	 // namespace age::ui::widget

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
	frame_interactive(e::style_state state = e::style_state::idle, auto&&... modifier) noexcept
	{
		return widget::begin((style::frame_interactive(state) | ... | FWD(modifier)));
	}

	FORCE_INLINE widget_ctx
	seperator(auto&&... modifier) noexcept
	{
		return widget::begin((style::seperator() | ... | FWD(modifier)));
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
	panel_resizable_h(float min, float max, auto&&... modifier) noexcept
	{
		using enum input::e::key_kind;

		if (auto h_panel = widget::begin(style::panel()
										 | set_padding(0, 0, 0, 0)
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

				widget_state.drag_x = std::clamp(widget_state.drag_x, min, max - g::theme_resize_handle_thickness);

				width = widget_state.drag_x;

				widget::begin(style::vertical()
							  | set_draw(true)
							  | set_body_brush_kind(e::brush_kind::color)
							  | set_body_brush_data(theme::color<e::theme_token_kind::resize_handle>(style_state))
							  | set_size(size_mode::fixed(g::theme_resize_handle_thickness), size_mode::grow()));
			}

			return widget_ctx_impl{ std::move(h_panel),
									widget::begin(style::vertical() | set_size(size_mode::fixed(width), size_mode::grow())),
									widget::begin((style::vertical() | ... | FWD(modifier))) };
		}

		return {};
	}

	FORCE_INLINE widget_ctx_impl<3>
	panel_resizable_v(float min, float max, auto&&... modifier) noexcept
	{
		using enum input::e::key_kind;

		if (auto h_panel = widget::begin(style::panel()
										 | set_padding(0, 0, 0, 0)
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

				widget_state.drag_y = std::clamp(widget_state.drag_y, min, max - g::theme_resize_handle_thickness);

				height = widget_state.drag_y;

				widget::begin(style::vertical()
							  | set_draw(true)
							  | set_body_brush_kind(e::brush_kind::color)
							  | set_body_brush_data(theme::color<e::theme_token_kind::resize_handle>(style_state))
							  | set_size(size_mode::grow(), size_mode::fixed(g::theme_resize_handle_thickness)));
			}

			return widget_ctx_impl{ std::move(h_panel),
									widget::begin(style::vertical() | set_size(size_mode::grow(), size_mode::fixed(height))),
									widget::begin((style::vertical() | ... | FWD(modifier))) };
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
				c_auto thumb_width		  = thumb_height > 0.f ? g::theme_scroll_thumb_thickness : 0.f;
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

					track_state.drag_y = std::clamp(track_state.drag_y, 0.f, thumb_y_offset_max);
				}
				else if (h_track.hovered())
				{
					style_state = e::style_state::hover;
				}

				widget::begin(style::scroll_thumb(style_state) | set_offset(0, thumb_offset_y) | set_width_fixed(thumb_width) | set_height_fixed(thumb_height));

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
				c_auto thumb_height		  = thumb_width > 0.f ? g::theme_scroll_thumb_thickness : 0.f;
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

					track_state.drag_y = std::clamp(track_state.drag_y, 0.f, thumb_x_offset_max);
				}
				else if (h_track.hovered())
				{
					style_state = e::style_state::hover;
				}

				widget::begin(style::scroll_thumb(style_state) | set_offset(thumb_offset_x, 0) | set_width_fixed(thumb_width) | set_height_fixed(thumb_height));

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
									   | set_z_offset(1)
									   | set_interact(true)
									   | set_size(size_mode::fit(), size_mode::fit()))
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

				c_auto chevelon_size = font::get_line_height(theme::size::text_primary());

				widget::begin(set_align(e::widget_align::center)
							  | set_size(size_mode::fixed(chevelon_size), size_mode::fixed(chevelon_size))
							  | set_border_thickness(0.f)
							  | set_rotation(rotation)
							  | set_shape_kind(e::shape_kind::arrow_right)
							  | set_body_brush_data(theme::colors::text_primary()));

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

	FORCE_INLINE widget_ctx_impl<2>
	tree_node(const char* p_str) noexcept
	{
		using enum input::e::key_kind;

		c_auto chevelon_size = font::get_line_height(theme::size::text_secondary());
		c_auto child_padding = chevelon_size + 12.f;

		if (auto res = widget::begin(style::layout(e::widget_layout::vertical)
									 | set_size(size_mode::grow(), size_mode::fit())))
		{
			auto is_open = false;

			if (auto header = widget::begin(style::frame_interactive()
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
							  | set_body_brush_data(theme::colors::text_secondary()));

				widget::text_secondary(p_str);
			}

			if (is_open)
			{
				return widget_ctx_impl{ std::move(res), widget::begin(style::layout(e::widget_layout::vertical)
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

// text input
namespace age::ui::widget
{
	FORCE_INLINE widget_ctx
	text_input(char* p_buf, uint32 buf_size) noexcept
	{
		if (auto h = widget::begin(style::frame()
								   | set_horizontal()
								   | set_padding_left(g::theme_frame_padding_left - g::theme_cursor_thickness)
								   | set_child_gap(0)
								   | set_interact(true)
								   | set_save_state(true)
								   | set_width_grow()
								   | set_height_fit()))
		{
			using enum input::e::key_kind;

			// c_auto font_size = theme::font_size<e::theme_token_kind::text_interactive>();
			c_auto font_size = 22.f;

			auto text_desc = style::text_secondary(p_buf) | set_align(e::widget_align::begin) | set_font_size(font_size);

			auto& state = h.get_state();

			c_auto draw_cursor = h.focused() and not h.pressed();

			if (h.clicked())
			{
				ui::detail::gen_text_data(text_desc);

				auto screen_offset = g::p_input_ctx->mouse_pos - state.pos - float2{ g::theme_frame_padding_left, g::theme_frame_padding_top };

				auto res = ui::detail::screen_offset_to_cursor(text_desc.text.text_data_idx, screen_offset, state.width);


				// std::println(
				//	"offset = {},                      \n"
				//	"word_min_x = {},				   \n"
				//	"word_max_x = {},				   \n"
				//	"line_width = {},				   \n"
				//	"byte_offset = {},				   \n"
				//	"word_byte_offset = {},			   \n"
				//	"word_byte_size = {},			   \n"
				//	"line_byte_offset = {},			   \n"
				//	"line_byte_size = {},			   \n"
				//	"screen_offset = {}",
				//	res.offset,
				//	res.word_min_x,
				//	res.word_max_x,
				//	res.line_width,
				//	res.byte_offset,
				//	res.word_byte_offset,
				//	res.word_byte_size,
				//	res.line_byte_offset,
				//	res.line_byte_size,
				//	screen_offset);

				state.cursor.offset_x		 = res.offset.x;
				state.cursor.offset_y		 = res.offset.y;
				state.cursor.byte_pos		 = res.byte_offset;
				state.cursor.anchor_byte_pos = res.byte_offset;


				// reinterpret_cast<uint32&>(state.drag_x) = cursor_x_byte_idx;
				// state.cursor.offset_x							= cursor_x_screen_offset;
			}
			else if (draw_cursor)
			{
				auto regen_cursor = false;
				// cursor up
				if (g::p_input_ctx->is_pressed_or_repeat(key_up))
				{
					state.cursor.offset_y -= font::get_line_height(font_size, g::current_font_idx) - math::g::epsilon_1e4;
					regen_cursor		   = !regen_cursor;
				}

				// cursor down
				if (g::p_input_ctx->is_pressed_or_repeat(key_down))
				{
					state.cursor.offset_y += font::get_line_height(font_size, g::current_font_idx) + math::g::epsilon_1e4;
					regen_cursor		   = !regen_cursor;
				}

				if (regen_cursor)
				{
					ui::detail::gen_text_data(text_desc);
					auto res			  = ui::detail::screen_offset_to_cursor(text_desc.text.text_data_idx, float2{ state.cursor.offset_x, state.cursor.offset_y }, state.width);
					state.cursor.offset_x = res.offset.x;
					state.cursor.offset_y = res.offset.y;
					state.cursor.byte_pos = res.byte_offset;
				}

				ui::detail::gen_text_data(text_desc);

				auto res = ui::detail::byte_offset_to_cursor(text_desc.text.text_data_idx, state.cursor.byte_pos, state.width);


				ui::detail::update_text_buf(p_buf, buf_size, res);

				// std::println("{}", cursor_byte_idx);


				std::println(
					"offset = {},                      \n"
					"word_min_x = {},				   \n"
					"word_max_x = {},				   \n"
					"line_width = {},				   \n"
					"byte_offset = {},				   \n"
					"word_byte_offset = {},			   \n"
					"word_byte_size = {},			   \n"
					"line_byte_offset = {},			   \n"
					"line_byte_size = {},			   \n",
					res.offset,
					res.word_min_x,
					res.word_max_x,
					res.line_width,
					res.byte_offset,
					res.word_byte_offset,
					res.word_byte_size,
					res.line_byte_offset,
					res.line_byte_size);

				state.cursor.offset_x		 = res.offset.x;
				state.cursor.offset_y		 = res.offset.y;
				state.cursor.byte_pos		 = res.byte_offset;
				state.cursor.anchor_byte_pos = res.anchor_byte_offset;
			}

			if (state.cursor.anchor_byte_pos != state.cursor.byte_pos)
			{
			}

			widget::begin(style::vertical()
						  | set_align(e::widget_align::begin)
						  | set_size(size_mode::fixed(g::theme_cursor_thickness), size_mode::fixed(font::get_line_height(font_size, g::current_font_idx)))
						  | set_draw(draw_cursor)
						  | set_offset(state.cursor.offset_x, state.cursor.offset_y)
						  | set_body_brush_data(theme::colors::text_interactive()));


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
	numeric_field(t& value, t min = std::numeric_limits<t>::min(), t max = std::numeric_limits<t>::max(),
				  const char*		  p_hint	 = nullptr,
				  e::theme_color_kind hint_color = (e::theme_color_kind)g::text_hint.color,
				  float				  step		 = 0.1f) noexcept
	{
		using enum input::e::key_kind;
		if (auto h_interact = widget::begin(style::horizontal(size_mode::grow(), size_mode::fit())
											| set_interact(true)))
		{
			step *= g::step_scale_table[g::p_input_ctx->is_ctrl_down()][g::p_input_ctx->is_shift_down()];

			auto style_state = e::style_state::idle;

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
					style_state = e::style_state::active;

					auto& state	 = h_interact.get_state();
					state.drag_x = 0.f;
				}
				else if (h_interact.pressed<mouse_left>())
				{
					style_state = e::style_state::active;

					auto& state = h_interact.get_state();

					state.drag_x += g::p_input_ctx->mouse_delta.x * step;

					auto delta = static_cast<std::make_signed_t<t>>(state.drag_x);

					value = std::clamp(value, min + std::abs(delta), max - std::abs(delta));

					value += delta;

					state.drag_x -= static_cast<float>(delta);
				}
				else if (h_interact.hovered())
				{
					style_state = e::style_state::hover;
				}
			}


			if (auto _ = widget::horizontal(set_size(size_mode::grow(), size_mode::fit())))
			{
				if (p_hint is_not_nullptr)
				{
					widget::begin(style::text_secondary(p_hint)
								  | set_body_brush_data(theme::color(hint_color), theme::opacity<e::theme_token_kind::text_secondary>(style_state)));
				}

				if (auto _ = widget::begin(style::frame_interactive(style_state)
										   | set_horizontal()
										   | set_width(size_mode::grow())))
				{
					char char_buf[21];

					util::to_str(char_buf, value);
					widget::text_secondary(char_buf, style_state);
				}
			}

			return h_interact;
		}
		else
		{
			return {};
		}
	}

	template <template <typename> typename t_vec, meta::cx_arithmetic t>
	requires(not requires(t_vec<t> m) { m[0][0]; })
	FORCE_INLINE widget_ctx
	numeric_field(t_vec<t>&			  value,
				  const char*		  p_hint	 = nullptr,
				  const t_vec<t>&	  min		 = t_vec<t>{ std::numeric_limits<t>::min() },
				  const t_vec<t>&	  max		 = t_vec<t>{ std::numeric_limits<t>::max() },
				  e::theme_color_kind hint_color = (e::theme_color_kind)g::text_hint.color,
				  float				  step		 = 0.1f) noexcept
	{
		using enum e::theme_color_kind;
		constexpr e::theme_color_kind colors[4] = { negative, positive, accent, amber };
		constexpr const char*		  labels[4] = { "X", "Y", "Z", "W" };

		if (auto row = widget::horizontal(set_size(size_mode::grow(), size_mode::fit())))
		{
			if (p_hint is_not_nullptr)
			{
				if (auto _ = widget::vertical(set_size(size_mode::grow(), size_mode::fit())))
				{
					widget::begin(style::text_secondary(p_hint)
								  | set_align(e::widget_align::begin)
								  | set_body_brush_data(theme::color(hint_color), theme::opacity<e::theme_token_kind::text_secondary>()));
				}
			}

			if constexpr (requires { value.x; })
			{
				numeric_field(value.x, min.x, max.x, labels[0], colors[0], step);
			}

			if constexpr (requires { value.y; })
			{
				numeric_field(value.y, min.y, max.y, labels[1], colors[1], step);
			}

			if constexpr (requires { value.z; })
			{
				numeric_field(value.z, min.z, max.z, labels[2], colors[2], step);
			}

			if constexpr (requires { value.w; })
			{
				numeric_field(value.w, min.w, max.w, labels[3], colors[3], step);
			}
		}
		return {};
	}

	template <template <typename> typename t_mat, meta::cx_arithmetic t>
	requires requires(t_mat<t> m) { m[0][0]; }
	FORCE_INLINE widget_ctx
	numeric_field(t_mat<t>&			  value,
				  const char*		  p_hint	 = nullptr,
				  const t			  min		 = std::numeric_limits<t>::min(),
				  const t			  max		 = std::numeric_limits<t>::max(),
				  e::theme_color_kind hint_color = (e::theme_color_kind)g::text_hint.color,
				  float				  step		 = 0.1f) noexcept
	{
		constexpr const char* row_labels[4] = { "Row 0", "Row 1", "Row 2", "Row 3" };

		if (auto col = widget::begin(style::vertical(size_mode::grow(), size_mode::fit())))
		{
			if (p_hint is_not_nullptr)
			{
				widget::begin(style::text_secondary(p_hint)
							  | set_align(e::widget_align::begin)
							  | set_body_brush_data(theme::color(hint_color), theme::opacity<e::theme_token_kind::text_secondary>()));
			}
			if constexpr (requires { value.r0; })
			{
				using row_t = BARE_OF(value.r0);
				numeric_field(value.r0, row_labels[0], row_t{ min }, row_t{ max }, e::theme_color_kind::white, step);
			}

			if constexpr (requires { value.r1; })
			{
				using row_t = BARE_OF(value.r1);
				numeric_field(value.r1, row_labels[1], row_t{ min }, row_t{ max }, e::theme_color_kind::white, step);
			}

			if constexpr (requires { value.r2; })
			{
				using row_t = BARE_OF(value.r2);
				numeric_field(value.r2, row_labels[2], row_t{ min }, row_t{ max }, e::theme_color_kind::white, step);
			}

			if constexpr (requires { value.r3; })
			{
				using row_t = BARE_OF(value.r3);
				numeric_field(value.r3, row_labels[3], row_t{ min }, row_t{ max }, e::theme_color_kind::white, step);
			}

			return col;
		}
		return {};
	}
}	 // namespace age::ui::widget

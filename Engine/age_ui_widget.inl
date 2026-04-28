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

// indicators
namespace age::ui::widget
{
	void
	indicator(e::shape_kind e_shape, float size = font::get_line_height(theme::text_heading_font_size()), auto color = theme::indicator_color()) noexcept
	{
		if (auto _ = widget::begin(style::horizontal() | set_size(size_mode::fixed(size), size_mode::fixed(size)) | set_padding(theme::padding_indicator()) | set_align_center()))
		{
			widget::begin(set_align(e::widget_align::center)
						  | set_size(size_mode::grow(), size_mode::grow())
						  | set_z_offset(1)
						  | set_border_thickness(0.f)
						  | set_shape_kind(e_shape)
						  | set_body_brush_data(color));
		}
	}

	void
	disclosure_indicator(bool is_open, float size = font::get_line_height(theme::text_heading_font_size())) noexcept;
}	 // namespace age::ui::widget

// frame
namespace age::ui::widget
{
	widget_ctx
	frame(e::style_state state = e::style_state::idle, auto&&... modifier) noexcept
	{
		return widget::begin((style::frame(state) | ... | FWD(modifier)));
	}

	widget_ctx
	separator_h(auto&&... modifier) noexcept
	{
		return widget::begin((style::separator_h() | ... | FWD(modifier)));
	}

	widget_ctx
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

	widget_ctx_impl<3>
	panel_resizable_h(float min, float max) noexcept;

	widget_ctx_impl<3>
	panel_resizable_v(float min, float max) noexcept;
}	 // namespace age::ui::widget

// scroll
namespace age::ui::widget
{
	widget_ctx_impl<3>
	scroll_area_v() noexcept;

	widget_ctx_impl<3>
	scroll_area_h() noexcept;
}	 // namespace age::ui::widget

// text
namespace age::ui::widget
{
	widget_ctx
	text_title(const char* p_text, bool enabled = true) noexcept;

	widget_ctx
	text_heading(const char* p_text, bool enabled = true) noexcept;

	widget_ctx
	text(const char* p_text, e::style_state state = e::style_state::idle, bool enabled = true) noexcept;

	widget_ctx
	text_label(const char* p_text, e::style_state state = e::style_state::idle, bool enabled = true) noexcept;

	widget_ctx
	text_hint(const char* p_text, bool enabled = true) noexcept;

	widget_ctx
	text_button(const char* p_text, bool enabled = true) noexcept;
}	 // namespace age::ui::widget

// button
namespace age::ui::widget
{
	widget_ctx
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

	widget_ctx
	button(e::shape_kind e_shape,
		   float		 size		  = font::get_line_height(theme::text_heading_font_size()),
		   float3		 color_idle	  = theme::color_text_gray_dark(),
		   float3		 color_hover  = theme::color_text_gray_dark(),
		   float3		 color_active = theme::color_text_gray_dark(),
		   auto&&... mod) noexcept
	{
		using enum input::e::key_kind;

		if (auto btn = widget::begin(style::layout(e::widget_layout::vertical)
									 | set_z_offset(1)
									 | set_interact(true)
									 | set_size(size_mode::fit(), size_mode::fit())))
		{
			auto state = e::style_state::idle;
			auto color = color_idle;
			if (btn.pressed<mouse_left>())
			{
				state = e::style_state::active;
				color = color_active;
			}
			else if (btn.hovered())
			{
				state = e::style_state::hover;
				color = color_hover;
			}

			if (auto _ = widget::frame(state, set_size(size_mode::fixed(size), size_mode::fixed(size))))
			{
				widget::begin(((set_align(e::widget_align::center)
								| set_size(size_mode::grow(), size_mode::grow())
								| set_z_offset(1)
								| set_border_thickness(0.f)
								| set_shape_kind(e_shape)
								| set_body_brush_data(color))
							   | ... | FWD(mod)));
			}

			return btn;
		}
		else
		{
			return {};
		}
	}

	widget_ctx
	toggle_button(e::shape_kind e_shape,
				  float			size		 = font::get_line_height(theme::text_heading_font_size()),
				  float3		color_idle	 = theme::color_text_gray_dark(),
				  float3		color_hover	 = theme::color_text_gray_dark(),
				  float3		color_active = theme::color_text_gray_dark(),
				  auto&&... mod) noexcept
	{
		using enum input::e::key_kind;

		if (auto btn = widget::begin(style::layout(e::widget_layout::vertical)
									 | set_z_offset(1)
									 | set_interact(true)
									 | set_size(size_mode::fit(), size_mode::fit())))
		{
			auto state = e::style_state::idle;
			auto color = color_idle;

			if (btn.pressed<mouse_left>())
			{
				state = e::style_state::active;
				color = color_active;
			}
			else if (btn.hovered())
			{
				state = e::style_state::hover;
				color = color_hover;
			}

			if (btn.clicked())
			{
				btn.toggle();
			}

			c_auto is_toggled = btn.is_toggled();

			if (auto _ = widget::begin(style::frame(state) | set_padding(theme::frame_padding().x) | set_width_fixed(size) | set_height_fixed(size) | set_border_brush_data(is_toggled ? theme::frame_color_border_focus() : float4::zero())))
			{
				widget::begin(((set_align(e::widget_align::center)
								| set_size(size_mode::grow(), size_mode::grow())
								| set_z_offset(1)
								| set_border_thickness(0.f)
								| set_shape_kind(e_shape)
								| set_body_brush_data(color))
							   | ... | FWD(mod)));
			}

			return btn;
		}

		return {};
	}

	widget_ctx
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

	widget_ctx
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
	widget_ctx
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

	widget_ctx
	collapsible_header2(const char* p_str, bool default_open = true, auto&&... modifier) noexcept
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

				is_open = header.is_toggled() != default_open;

				c_auto disclosure_indicator_size = font::get_line_height(theme::text_heading_font_size());

				if (auto _ = widget::begin(style::horizontal() | set_size(size_mode::fixed(disclosure_indicator_size), size_mode::fixed(disclosure_indicator_size)) | set_padding(theme::padding_small() + 1.f) | set_align_center()))
				{
					if (is_open)
					{
						widget::begin(set_align(e::widget_align::center)
									  | set_size(size_mode::grow(), size_mode::grow())
									  | set_border_thickness(0.f)
									  // | set_rotation(180 * math::g::degree_to_radian)
									  | set_padding(theme::padding_medium())
									  | set_shape_kind(e::shape_kind::triangle)
									  | set_body_brush_data(theme::color_text_gray_dark()));
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

	widget_ctx_impl<2>
	tree_node(const char* p_str) noexcept;
}	 // namespace age::ui::widget

// slider
namespace age::ui::widget
{
	widget_ctx
	slider(float& value, float min, float max) noexcept;
}	 // namespace age::ui::widget

// text input
namespace age::ui::widget
{
	namespace detail
	{
		void
		handle_text_click(widget_state& state, const char* p_buf, widget_desc& text_desc,
						  float width, float2 mouse_offset, uint8 click_count) noexcept;

		void
		handle_text_edit(widget_state& state, char* p_buf, uint32 buf_size,
						 widget_desc& text_desc, float width) noexcept;

		void
		draw_text_cursor_and_selection(const widget_state& state, const cursor_data& cursor,
									   const cursor_data& anchor, widget_desc& text_desc,
									   float width, float text_line_height) noexcept;
	}	 // namespace detail

	widget_ctx
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

			auto text_desc = widget_desc::apply(
				((style::text(p_buf)
				  | set_align(e::widget_align::begin)
				  | set_font_size(font_size))
				 | ... | FWD(modifier)));

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

	template <auto n>
	widget_ctx
	text_input(std::array<char, n>& arr, auto&&... mod) noexcept
	{
		return text_input(arr.data(), n, FWD(mod)...);
	}

	widget_ctx
	text_input2(char* p_buf, uint32 buf_size, auto&& frame_mod, auto&& text_mod) noexcept
	{
		if (auto h = widget::begin(style::horizontal()
								   | set_horizontal()
								   | set_child_gap(0)
								   | set_interact(true)
								   | set_save_state(true)
								   | set_width_fit()
								   | set_height_fit()))
		{
			if (auto _ = widget::begin(style::frame()
									   | set_draw(h.focused())
									   | set_horizontal()
									   | set_child_gap(0)
									   | set_width_fit()
									   | set_height_fit()
									   | FWD(frame_mod)))
			{
				auto text_desc = widget_desc::apply(style::text(p_buf)
													| set_align(e::widget_align::begin)
													| FWD(text_mod));

				c_auto font_size		= text_desc.text.font_size;
				c_auto text_line_height = font::get_line_height(font_size, g::current_font_idx);

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
			}

			return h;
		}

		return {};
	}

	inline widget_ctx
	text_input2(char* p_buf, uint32 buf_size) noexcept
	{
		return text_input2(p_buf, buf_size, ui::detail::mod_empty{}, ui::detail::mod_empty{});
	}

	template <auto n>
	widget_ctx
	text_input2(std::array<char, n>& arr) noexcept
	{
		return text_input2(arr.data(), n);
	}
}	 // namespace age::ui::widget

// numeric field
namespace age::ui::widget
{
	namespace detail
	{
		template <typename t>
		consteval decltype(auto)
		default_max() noexcept
		{
			using value_type = std::remove_cvref_t<t>;
			if constexpr (std::is_integral_v<value_type>)
			{
				if constexpr (std::is_unsigned_v<value_type>)
				{
					return std::numeric_limits<std::make_signed_t<value_type>>::max();
				}
				else
				{
					return std::numeric_limits<value_type>::max();
				}
			}
			else
			{
				return std::numeric_limits<value_type>::max();
			}
		}
	}	 // namespace detail

	template <meta::cx_arithmetic t>
	widget_ctx
	numeric_field(t&		  value,
				  const char* p_label		   = nullptr,
				  t			  min			   = std::numeric_limits<t>::lowest(),
				  t			  max			   = detail::default_max<t>(),
				  float4	  text_label_color = theme::text_label_color(),
				  float		  step			   = 0.1f) noexcept
	{
		if constexpr (std::is_integral_v<t>)
		{
			if constexpr (std::is_unsigned_v<t>)
			{
				AGE_ASSERT(max <= static_cast<t>(detail::default_max<t>()));
			}
		}
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

						auto text_desc = widget_desc::apply(style::text_active(g::numeric_field_text_edit_buf)
															| set_align(e::widget_align::begin)
															| set_font_size(font_size));

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
	widget_ctx
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
	widget_ctx
	numeric_field(t_vec<t>&		  value,
				  const char*	  p_label		   = nullptr,
				  const t_vec<t>& min			   = t_vec<t>{ std::numeric_limits<t>::lowest() },
				  const t_vec<t>& max			   = t_vec<t>{ detail::default_max<t>() },
				  float4		  text_label_color = theme::text_label_color(),
				  float			  step			   = 0.1f) noexcept
	{
		static constexpr c_auto labels = std::array{ "X", "Y", "Z", "W" };
		c_auto					colors = std::array{
			theme::color_text_red(),
			theme::color_text_green(),
			theme::color_text_blue(),
			theme::color_text_amber(),
		};


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

			for (auto i : views::loop(t_vec<t>::size()))
			{
				numeric_field(value[i], labels[i], min[i], max[i], colors[i], step);
			}
		}
		return {};
	}

	template <template <typename> typename t_vec, meta::cx_arithmetic t>
	requires(not requires(t_vec<t> m) { m[0][0]; })
	widget_ctx
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
	widget_ctx
	numeric_field(t_mat<t>&	  value,
				  const char* p_label		   = nullptr,
				  const t	  min			   = std::numeric_limits<t>::lowest(),
				  const t	  max			   = detail::default_max<t>(),
				  float4	  text_label_color = theme::text_label_color(),
				  float		  step			   = 0.1f) noexcept
	{
		static constexpr c_auto row_labels = std::array{ "Row 0", "Row 1", "Row 2", "Row 3" };

		if (auto col = widget::begin(style::vertical(size_mode::grow(), size_mode::fit())))
		{
			if (p_label is_not_nullptr)
			{
				widget::begin(style::text_label(p_label)
							  | set_align(e::widget_align::begin)
							  | set_body_brush_data(text_label_color));
			}

			using row_t = BARE_OF(value.r0);

			for (auto i : views::loop(t_mat<t>::rows()))
			{
				numeric_field(value[i], row_labels[i], row_t{ min }, row_t{ max }, theme::text_label_color(), step);
			}
			return col;
		}
		return {};
	}

	template <template <typename> typename t_mat, meta::cx_arithmetic t>
	requires requires(t_mat<t> m) { m[0][0]; }
	widget_ctx
	numeric_field(t_mat<t>&	  value,
				  const char* p_label,
				  const t	  min,
				  const t	  max,
				  float3	  text_label_color,
				  float		  step = 0.1f) noexcept
	{
		return numeric_field(value, p_label, min, max, float4{ text_label_color, 1.f }, step);
	}

	widget_ctx
	rotation_field(float4&	   value,
				   const char* p_label			= nullptr,
				   float4	   text_label_color = theme::text_label_color(),
				   float	   step				= 0.1f) noexcept;

	widget_ctx
	rotation_field(float4&	   value,
				   const char* p_label,
				   float3	   text_label_color,
				   float	   step = 0.1f) noexcept;
}	 // namespace age::ui::widget

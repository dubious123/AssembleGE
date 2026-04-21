#include "age_pch.hpp"
#include "age.hpp"

namespace age::ui::detail
{
	FORCE_INLINE constexpr bool
	height_depends_on_width(e::size_mode_kind kind) noexcept
	{
		return kind == e::size_mode_kind::text
			or kind == e::size_mode_kind::aspect_ratio;
	}

	FORCE_INLINE constexpr float
	calc_height_depends_on_width(e::size_mode_kind mode_kind, layout_pos_data& pos_data) noexcept
	{
		if (mode_kind == e::size_mode_kind::text)
		{
			c_auto& text_data		  = g::text_data_vec[pos_data.text.idx];
			c_auto	char_pos_data_idx = g::char_pos_data_vec.size<uint32>();
			pos_data.text.idx		  = char_pos_data_idx;

			auto line_offset = 0u;
			auto wrap_count	 = 0u;
			auto cursor		 = float2{ 0.f, 0.f };

			g::char_pos_data_vec.reserve(g::char_pos_data_vec.size() + text_data.char_data_count);

			for (auto char_offset = text_data.char_data_offset;
				 c_auto& word_data : std::span(g::word_data_vec.data() + text_data.word_data_offset, text_data.word_data_count))
			{
				c_auto leading_space = word_data.leading_space_count * text_data.space_advance;

				auto wrapped = false;

				if (word_data.line_offset + wrap_count > line_offset)
				{
					line_offset = word_data.line_offset + wrap_count;
					cursor.x	= 0.f;
				}
				else if (cursor.x > 0.f and cursor.x + leading_space + word_data.width > pos_data.width + math::g::epsilon_1e4)
				{
					++line_offset;
					++wrap_count;
					cursor.x = 0.f;

					wrapped = true;
				}

				if (wrapped is_false)
				{
					cursor.x += leading_space;
				}

				cursor.y = line_offset * text_data.line_height;
				for (c_auto& char_data : std::span(g::char_data_vec.data() + char_offset, word_data.char_count))
				{
					g::char_pos_data_vec.emplace_back(char_pos_data{
						.offset		  = cursor + char_data.offset,
						.size		  = char_data.size,
						.atlas_uv_min = char_data.atlas_uv_min,
						.atlas_uv_max = char_data.atlas_uv_max,
					});

					cursor.x += char_data.advance;
				}

				char_offset += word_data.char_count;
			}

			return (line_offset + 1) * text_data.line_height;
		}
		else if (mode_kind == e::size_mode_kind::aspect_ratio)
		{
			c_auto aspect_ratio = std::bit_cast<float>(static_cast<uint32>(pos_data.extra));
			return pos_data.width * aspect_ratio;
		}
		else
		{
			AGE_UNREACHABLE();
		}
	}
}	 // namespace age::ui::detail

namespace age::ui::detail
{
	FORCE_INLINE uint64
	grow_sort_event(float value, bool is_leave, uint32 idx) noexcept
	{
		return (static_cast<uint64>(std::bit_cast<uint32>(value)) << 32) | (static_cast<uint64>(is_leave) << 31) | static_cast<uint64>(idx);
	}

	FORCE_INLINE float
	grow_sort_event_value(uint64 sort_event) noexcept
	{
		return std::bit_cast<float>(static_cast<uint32>(sort_event >> 32));
	}

	FORCE_INLINE bool
	grow_sort_event_is_leave(uint64 sort_event) noexcept
	{
		return static_cast<bool>((sort_event >> 31) & 1);
	}

	FORCE_INLINE uint32
	grow_sort_event_idx(uint64 sort_event) noexcept
	{
		return static_cast<uint32>(sort_event & 0x7fff'ffff);
	}
}	 // namespace age::ui::detail

namespace age::ui::detail
{
	// template <bool is_root = false>
	// FORCE_INLINE t_hash
	// widget_begin(widget_desc&& desc) noexcept
	//{
	//	c_auto id = new_id();

	//	auto z_offset		   = 0u;
	//	auto render_data_count = 0u;
	//	auto padding_sum	   = 0.f;
	//	auto atlas_id		   = 0u;

	//	if constexpr (is_root is_false)
	//	{
	//		auto& size_data_parent = g::layout_size_data_stack[g::layout_size_data_current_idx];
	//		auto& pos_data_parent  = g::layout_pos_data_vec[size_data_parent.pos_data_idx];
	//		++pos_data_parent.child_count;

	//		z_offset = pos_data_parent.z_offset + desc.z_offset;

	//		if (desc.draw)
	//		{
	//			if (desc.shape_kind == e::shape_kind::text)
	//			{
	//				if (AGE_IS_INVALID_IDX(desc.text.text_data_idx))
	//				{
	//					detail::gen_text_data(desc);
	//				}

	//				auto& text_data	  = g::text_data_vec[desc.text.text_data_idx];
	//				render_data_count = text_data.char_data_count;

	//				atlas_id = g::font_data_vec[desc.text.font_idx].second.atlas_id;
	//			}
	//			else
	//			{
	//				render_data_count = 1;
	//			}
	//		}

	//		padding_sum = desc.layout == e::widget_layout::vertical or desc.layout == e::widget_layout::vertical_inv
	//						? desc.padding_left + desc.padding_right
	//						: desc.padding_top + desc.padding_bottom;
	//	}


	//	g::layout_size_data_stack.emplace_back(layout_size_data{
	//		.layout				= desc.layout,
	//		.width_mode			= desc.width_size_mode,
	//		.height_mode		= desc.height_size_mode,
	//		.child_subtree_size = 0,
	//		.parent_idx			= g::layout_size_data_current_idx,
	//		.pos_data_idx		= g::layout_pos_data_vec.size<uint32>(),
	//		.child_gap			= desc.child_gap,
	//		.width_min			= desc.width_min,
	//		.width_max			= desc.width_max,
	//		.width_final		= 0.f,
	//		.height_min			= desc.height_min,
	//		.height_max			= desc.height_max,
	//		.height_final		= 0.f,
	//	});

	//	g::layout_pos_data_vec.emplace_back(layout_pos_data{
	//		.id				   = id,
	//		.render_data_idx   = g::render_data_vec.size<uint32>(),
	//		.render_data_count = render_data_count,
	//		.layout			   = desc.layout,
	//		.align			   = desc.align,
	//		.z_offset		   = static_cast<uint16>(z_offset),
	//		.offset			   = desc.offset,
	//		.child_gap		   = desc.child_gap,
	//		.padding_left	   = desc.padding_left,
	//		.padding_right	   = desc.padding_right,
	//		.padding_top	   = desc.padding_top,
	//		.padding_bottom	   = desc.padding_bottom,
	//		.interact		   = desc.interact,
	//		.save_state		   = desc.save_state,
	//		.direct_draw	   = false,
	//		.text			   = { .idx = desc.text.text_data_idx, .atlas_id = atlas_id },
	//	});

	//	if (desc.draw)
	//	{
	//		g::render_data_vec.emplace_back(render_data{
	//			.pivot_uv		   = desc.pivot_uv,
	//			.rotation		   = desc.rotation,
	//			.border_thickness  = desc.border_thickness,
	//			.shape_kind		   = desc.shape_kind,
	//			.body_brush_kind   = desc.body_brush_kind,
	//			.border_brush_kind = desc.border_brush_kind,
	//			.shape_data		   = desc.shape_data,
	//			.body_brush_data   = desc.body_brush_data,
	//			.border_brush_data = desc.border_brush_data,
	//		});
	//	}

	//	// handle z_order
	//	{
	//		if (z_offset >= g::z_order_count_vec.size())
	//		{
	//			c_auto before_size = g::z_order_count_vec.size();
	//			g::z_order_count_vec.resize(z_offset + 1);
	//			std::ranges::fill(g::z_order_count_vec.begin() + before_size, g::z_order_count_vec.end(), 0u);
	//		}

	//		g::z_order_count_vec[z_offset] += render_data_count;
	//	}

	//	g::layout_size_data_current_idx = g::layout_size_data_stack.size<uint32>() - 1;

	//	return id;
	//}

	template <bool is_width>
	FORCE_INLINE bool
	is_cross(e::widget_layout e_layout) noexcept
	{
		if constexpr (is_width)
		{
			return e_layout == e::widget_layout::vertical or e_layout == e::widget_layout::vertical_inv;
		}
		else
		{
			return e_layout == e::widget_layout::horizontal or e_layout == e::widget_layout::horizontal_inv;
		}
	}

	template <bool is_width>
	FORCE_INLINE float
	padding_sum(const layout_pos_data& pos_data) noexcept
	{
		if constexpr (is_width)
		{
			return pos_data.padding_left + pos_data.padding_right;
		}
		else
		{
			return pos_data.padding_top + pos_data.padding_bottom;
		}
	}

	template <bool is_width>
	void
	finalize_fixed(layout_size_data& size_data, uint32 idx, float final_size) noexcept;

	template <bool is_width>
	void
	finalize_fit(layout_size_data& size_data, uint32 idx) noexcept;

	template <bool is_width>
	void
	submit_size(layout_size_data& size_data, uint32 idx, float final_size) noexcept
	{
		auto& pos_data = g::layout_pos_data_vec[size_data.pos_data_idx];

		size_data.size_final<is_width>() = final_size;

		pos_data.set_size<is_width>(final_size);

		if constexpr (is_width)
		{
			if (height_depends_on_width(size_data.height_mode))
			{
				AGE_ASSERT(size_data.height_final < 0.f);
				c_auto height = calc_height_depends_on_width(size_data.height_mode, pos_data);
				finalize_fixed<false>(size_data, idx, std::clamp(height, size_data.height_min, size_data.height_max));
			}
		}
	}

	template <bool is_width>
	void
	finalize_fixed(layout_size_data& size_data, uint32 idx, float final_size) noexcept
	{
		auto& pos_data = g::layout_pos_data_vec[size_data.pos_data_idx];

		if (is_cross<is_width>(size_data.layout))
		{
			c_auto child_size = final_size - padding_sum<is_width>(pos_data);
			for (auto child_idx = idx + 1; child_idx <= idx + size_data.child_subtree_size;)
			{
				auto& child = g::layout_size_data_stack[child_idx];

				if (child.size_final<is_width>() < 0.f)
				{
					if constexpr (is_width)
					{
						finalize_fixed<is_width>(child, child_idx, std::clamp(child_size, child.size_min<is_width>(), child.size_max<is_width>()));
					}
					else
					{
						if (child.height_mode == e::size_mode_kind::fit)
						{
							finalize_fit<false>(child, child_idx);
						}
						else
						{
							finalize_fixed<false>(child, child_idx, std::clamp(child_size, child.size_min<is_width>(), child.size_max<is_width>()));
						}
					}
				}

				child_idx += child.child_subtree_size + 1;
			}

			submit_size<is_width>(size_data, idx, final_size);
		}
		else
		{
			auto fixed		= size_data.child_gap * std::max(pos_data.child_count - 1, 0) + padding_sum<is_width>(pos_data);
			auto begin_size = g::element_layout_grow_event_vec.size();

			g::element_layout_grow_event_vec.reserve(begin_size + size_data.child_subtree_size * 2);

			for (auto child_idx = idx + 1; child_idx <= idx + size_data.child_subtree_size;)
			{
				auto& child = g::layout_size_data_stack[child_idx];

				if (child.size_final<is_width>() >= 0.f)
				{
					fixed += child.size_final<is_width>();
				}
				else if (child.size_mode<is_width>() != e::size_mode_kind::grow)
				{
					finalize_fit<is_width>(child, child_idx);
					fixed += child.size_final<is_width>();
				}
				else
				{
					fixed += child.size_min<is_width>();
					g::element_layout_grow_event_vec.emplace_back(detail::grow_sort_event(child.size_min<is_width>(), false, child_idx));
					g::element_layout_grow_event_vec.emplace_back(detail::grow_sort_event(child.size_max<is_width>(), true, child_idx));
				}

				child_idx += child.child_subtree_size + 1;
			}

			auto available = final_size - fixed;
			auto level	   = 0.f;

			if (g::element_layout_grow_event_vec.size() > begin_size)
			{
				std::ranges::sort(g::element_layout_grow_event_vec);

				auto active_count = 0;
				level			  = detail::grow_sort_event_value(g::element_layout_grow_event_vec[begin_size]);

				for (auto e : g::element_layout_grow_event_vec | std::views::drop(begin_size))
				{
					c_auto value	 = detail::grow_sort_event_value(e);
					c_auto is_leave	 = detail::grow_sort_event_is_leave(e);
					c_auto child_idx = detail::grow_sort_event_idx(e);
					c_auto cost		 = (value - level) * active_count;
					if (cost > available)
					{
						level += available / static_cast<float>(active_count);
						break;
					}

					available	 -= cost;
					active_count += 1 - 2 * is_leave;
					level		  = value;
				}
			}

			g::element_layout_grow_event_vec.resize(begin_size);
			for (auto child_idx = idx + 1; child_idx <= idx + size_data.child_subtree_size;)
			{
				auto& child = g::layout_size_data_stack[child_idx];

				if (child.size_mode<is_width>() == e::size_mode_kind::grow)
				{
					finalize_fixed<is_width>(child, child_idx, std::clamp(level, child.size_min<is_width>(), child.size_max<is_width>()));
				}

				child_idx += child.child_subtree_size + 1;
			}

			submit_size<is_width>(size_data, idx, final_size);
		}
	}

	template <bool is_width>
	void
	finalize_fit(layout_size_data& size_data, uint32 idx) noexcept
	{
		auto& pos_data = g::layout_pos_data_vec[size_data.pos_data_idx];

		c_auto size_mode = size_data.size_mode<is_width>();

		if (size_mode == e::size_mode_kind::fixed)
		{
			finalize_fixed<is_width>(size_data, idx, size_data.size_min<is_width>());
		}
		else if (is_cross<is_width>(size_data.layout))
		{
			auto final_size = 0.f;

			for (auto child_idx = idx + 1; child_idx <= idx + size_data.child_subtree_size;)
			{
				auto& child = g::layout_size_data_stack[child_idx];

				if (child.size_final<is_width>() >= 0.f)
				{
					final_size = std::max(child.size_final<is_width>(), final_size);
				}
				else if (child.size_mode<is_width>() != e::size_mode_kind::grow)
				{
					finalize_fit<is_width>(child, child_idx);
					final_size = std::max(child.size_final<is_width>(), final_size);
				}
				else
				{
					if (child.size_max<is_width>() == std::numeric_limits<float>::max())
					{
						final_size = std::max(child.size_min<is_width>(), final_size);
					}
					else
					{
						final_size = std::max(child.size_max<is_width>(), final_size);
					}
				}

				child_idx += child.child_subtree_size + 1;
			}

			final_size = std::clamp(final_size + padding_sum<is_width>(pos_data), size_data.size_min<is_width>(), size_data.size_max<is_width>());

			c_auto child_size = final_size - padding_sum<is_width>(pos_data);

			for (auto child_idx = idx + 1; child_idx <= idx + size_data.child_subtree_size;)
			{
				auto& child = g::layout_size_data_stack[child_idx];

				if (child.size_final<is_width>() < 0.f)
				{
					finalize_fixed<is_width>(child, child_idx, std::clamp(child_size, child.size_min<is_width>(), child.size_max<is_width>()));
				}

				child_idx += child.child_subtree_size + 1;
			}

			submit_size<is_width>(size_data, idx, final_size);
		}
		else
		{
			auto fixed		= size_data.child_gap * std::max(pos_data.child_count - 1, 0) + padding_sum<is_width>(pos_data);
			auto grow_size	= 0.f;
			auto begin_size = g::element_layout_grow_event_vec.size();

			g::element_layout_grow_event_vec.reserve(begin_size + size_data.child_subtree_size * 2);

			for (auto child_idx = idx + 1; child_idx <= idx + size_data.child_subtree_size;)
			{
				auto& child = g::layout_size_data_stack[child_idx];

				if (child.size_final<is_width>() >= 0.f)
				{
					fixed += child.size_final<is_width>();
				}
				else if (child.size_mode<is_width>() != e::size_mode_kind::grow)
				{
					finalize_fit<is_width>(child, child_idx);
					fixed += child.size_final<is_width>();
				}
				else
				{
					grow_size += child.size_max<is_width>();
					// fixed	  += child.size_min<is_width>();
					g::element_layout_grow_event_vec.emplace_back(detail::grow_sort_event(child.size_min<is_width>(), false, child_idx));
					g::element_layout_grow_event_vec.emplace_back(detail::grow_sort_event(child.size_max<is_width>(), true, child_idx));
				}

				child_idx += child.child_subtree_size + 1;
			}

			auto final_size = std::clamp(fixed + grow_size, size_data.size_min<is_width>(), size_data.size_max<is_width>());
			auto available	= final_size - fixed;
			auto level		= 0.f;

			if (g::element_layout_grow_event_vec.size() > begin_size)
			{
				std::ranges::sort(g::element_layout_grow_event_vec);

				auto active_count = 0;
				level			  = detail::grow_sort_event_value(g::element_layout_grow_event_vec[begin_size]);

				for (auto e : g::element_layout_grow_event_vec | std::views::drop(begin_size))
				{
					c_auto value	 = detail::grow_sort_event_value(e);
					c_auto is_leave	 = detail::grow_sort_event_is_leave(e);
					c_auto child_idx = detail::grow_sort_event_idx(e);
					c_auto cost		 = (value - level) * active_count;
					if (cost > available)
					{
						level += available / static_cast<float>(active_count);
						break;
					}

					available	 -= cost;
					active_count += 1 - 2 * is_leave;
					level		  = value;
				}
			}

			g::element_layout_grow_event_vec.resize(begin_size);
			for (auto child_idx = idx + 1; child_idx <= idx + size_data.child_subtree_size;)
			{
				auto& child = g::layout_size_data_stack[child_idx];

				if (child.size_mode<is_width>() == e::size_mode_kind::grow)
				{
					finalize_fixed<is_width>(child, child_idx, std::clamp(level, child.size_min<is_width>(), child.size_max<is_width>()));
				}

				child_idx += child.child_subtree_size + 1;
			}

			submit_size<is_width>(size_data, idx, final_size);
		}
	}

	void
	widget_end() noexcept
	{
		auto&  size_data		= g::layout_size_data_stack[g::layout_size_data_current_idx];
		c_auto parent_idx		= size_data.parent_idx;
		auto&  size_data_parent = g::layout_size_data_stack[parent_idx];

		c_auto can_finalize_width = size_data.width_mode == e::size_mode_kind::fixed
								 or size_data.width_mode == e::size_mode_kind::fit;

		c_auto can_finalize_height = size_data.height_mode == e::size_mode_kind::fixed
								  or size_data.height_mode == e::size_mode_kind::fit;

		if (can_finalize_width)
		{
			finalize_fit<true>(size_data, g::layout_size_data_current_idx);
		}
		else
		{
			size_data.width_final = -1.f;
		}

		if (can_finalize_width and can_finalize_height)
		{
			finalize_fit<false>(size_data, g::layout_size_data_current_idx);

			g::layout_size_data_stack.resize(g::layout_size_data_current_idx + 1);
			size_data.child_subtree_size		 = 0;
			size_data_parent.child_subtree_size += 1;
		}
		else
		{
			size_data.height_final				 = -1.f;
			size_data_parent.child_subtree_size += size_data.child_subtree_size + 1;
		}

		g::layout_size_data_current_idx = parent_idx;
	}
};	  // namespace age::ui::detail

namespace age::ui::widget::detail
{
	void
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

	void
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

	void
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
}	 // namespace age::ui::widget::detail

namespace age::ui::widget
{
	void
	disclosure_indicator(bool is_open, float size) noexcept
	{
		if (auto _ = widget::begin(style::horizontal() | set_size(size_mode::fixed(size), size_mode::fixed(size)) | set_padding(theme::padding_indicator()) | set_align_center()))
		{
			if (is_open)
			{
				widget::begin(set_align(e::widget_align::center)
							  | set_size(size_mode::grow(), size_mode::grow())
							  | set_z_offset(1)
							  | set_border_thickness(0.f)
							  | set_shape_kind(e::shape_kind::triangle)
							  | set_body_brush_data(theme::color_text_gray_dark()));
			}
			else
			{
				widget::begin(set_align(e::widget_align::center)
							  | set_size(size_mode::grow(), size_mode::fixed(theme::thickness_thick()))
							  | set_z_offset(1)
							  | set_border_thickness(0.f)
							  | set_shape_kind(e::shape_kind::rect)
							  | set_body_brush_data(theme::color_text_gray_dark()));
			}
		}
	}
}	 // namespace age::ui::widget

namespace age::ui::widget
{
	widget_ctx_impl<3>
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

	widget_ctx_impl<3>
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

namespace age::ui::widget
{
	widget_ctx_impl<3>
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

	widget_ctx_impl<3>
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

namespace age::ui::widget
{
	widget_ctx
	text_title(const char* p_text, bool enabled) noexcept
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

	widget_ctx
	text_heading(const char* p_text, bool enabled) noexcept
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

	widget_ctx
	text(const char* p_text, e::style_state state, bool enabled) noexcept
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

	widget_ctx
	text_label(const char* p_text, e::style_state state, bool enabled) noexcept
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

	widget_ctx
	text_hint(const char* p_text, bool enabled) noexcept
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

	widget_ctx
	text_button(const char* p_text, bool enabled) noexcept
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

namespace age::ui::widget
{
	widget_ctx_impl<2>
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

namespace age::ui::widget
{
	widget_ctx
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

	widget_ctx
	rotation_field(float4&	   value,
				   const char* p_label,
				   float4	   text_label_color,
				   float	   step) noexcept
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

	widget_ctx
	rotation_field(float4&	   value,
				   const char* p_label,
				   float3	   text_label_color,
				   float	   step) noexcept
	{
		return rotation_field(value, p_label, float4{ text_label_color, 1.f }, step);
	}
}	 // namespace age::ui::widget

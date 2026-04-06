#include "age_pch.hpp"
#include "age.hpp"

namespace age::ui::detail
{
	FORCE_INLINE constexpr std::tuple<uint16, uint16>
	decode_utf8(const char* p)
	{
		auto c = static_cast<uint8>(p[0]);

		if (c < 0x80)			   // 1 byte (ASCII)
		{
			return { 1, c };
		}
		if ((c & 0xE0) == 0xC0)	   // 2 bytes
		{
			uint16 unicode	= (c & 0x1F) << 6;
			unicode		   |= (static_cast<uint8>(p[1]) & 0x3F);
			return { 2, unicode };
		}
		if ((c & 0xF0) == 0xE0)	   // 3 bytes
		{
			uint16 unicode	= (c & 0x0F) << 12;
			unicode		   |= (static_cast<uint8>(p[1]) & 0x3F) << 6;
			unicode		   |= (static_cast<uint8>(p[2]) & 0x3F);
			return { 3, unicode };
		}
		// if ((c & 0xF8) == 0xF0)	   // 4 bytes
		//{
		//	uint32 cp  = (c & 0x07) << 18;
		//	cp		  |= (static_cast<uint8>(*++p) & 0x3F) << 12;
		//	cp		  |= (static_cast<uint8>(*++p) & 0x3F) << 6;
		//	cp		  |= (static_cast<uint8>(*++p) & 0x3F);
		//	++p;
		//	return cp;
		// }

		return { 1, 0xFFFD };	 // replacement character
	}

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
			auto cursor		 = float2{ 0.f, 0.f };

			g::char_pos_data_vec.reserve(g::char_pos_data_vec.size() + text_data.char_data_count);

			for (auto char_offset = text_data.char_data_offset;
				 c_auto& word_data : std::span(g::word_data_vec.data() + text_data.word_data_offset, text_data.word_data_count))
			{
				if (word_data.line_offset > line_offset)
				{
					line_offset = word_data.line_offset;
					cursor.x	= 0.f;
				}
				else if (cursor.x > 0.f and cursor.x + word_data.leading_space + word_data.width > pos_data.width)
				{
					++line_offset;
					cursor.x = 0.f;
				}

				cursor.x += word_data.leading_space;

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
	template <bool is_root = false>
	FORCE_INLINE t_hash
	widget_begin(const widget_desc& desc) noexcept
	{
		c_auto id = new_id();

		auto z_offset		   = 0u;
		auto render_data_count = 0u;
		auto padding_sum	   = 0.f;

		auto width_max	= desc.width_max;
		auto height_max = desc.height_max;

		if constexpr (is_root is_false)
		{
			auto& size_data_parent = g::layout_size_data_stack[g::layout_size_data_current_idx];
			auto& pos_data_parent  = g::layout_pos_data_vec[size_data_parent.pos_data_idx];
			++pos_data_parent.child_count;

			if (desc.draw)
			{
				z_offset = pos_data_parent.z_offset + desc.z_offset;

				if (desc.shape_kind == e::shape_kind::text)
				{
					auto& text_data	  = g::text_data_vec[desc.text.text_data_idx];
					render_data_count = text_data.char_data_count;
				}
				else
				{
					render_data_count = 1;
				}
			}

			padding_sum = desc.layout == e::widget_layout::vertical
							? desc.padding_left + desc.padding_right
							: desc.padding_top + desc.padding_bottom;
		}


		g::layout_size_data_stack.emplace_back(layout_size_data{
			.layout				= desc.layout,
			.width_mode			= desc.width_size_mode,
			.height_mode		= desc.height_size_mode,
			.child_subtree_size = 0,
			.parent_idx			= g::layout_size_data_current_idx,
			.pos_data_idx		= g::layout_pos_data_vec.size<uint32>(),
			.child_gap			= desc.child_gap,
			.width_min			= desc.width_min,
			.width_max			= width_max,
			.width_final		= 0.f,
			.height_min			= desc.height_min,
			.height_max			= height_max,
			.height_final		= 0.f,
		});

		g::layout_pos_data_vec.emplace_back(layout_pos_data{
			.id				   = id,
			.render_data_idx   = g::render_data_vec.size<uint32>(),
			.render_data_count = render_data_count,
			.layout			   = desc.layout,
			.align			   = desc.align,
			.z_offset		   = static_cast<uint16>(z_offset),
			.offset			   = desc.offset,
			.child_gap		   = desc.child_gap,
			.padding_left	   = desc.padding_left,
			.padding_right	   = desc.padding_right,
			.padding_top	   = desc.padding_top,
			.padding_bottom	   = desc.padding_bottom,
			.interact		   = desc.interact,
			.save_state		   = desc.save_state,
			.text			   = { .idx = desc.text.text_data_idx, .atlas_id = g::font_data_vec[desc.text.font_idx].second.atlas_id },
		});

		if (desc.draw)
		{
			g::render_data_vec.emplace_back(render_data{
				.pivot_uv		   = desc.pivot_uv,
				.rotation		   = desc.rotation,
				.border_thickness  = desc.border_thickness,
				.shape_kind		   = desc.shape_kind,
				.body_brush_kind   = desc.body_brush_kind,
				.border_brush_kind = desc.border_brush_kind,
				.shape_data		   = desc.shape_data,
				.body_brush_data   = desc.body_brush_data,
				.border_brush_data = desc.border_brush_data,
			});
		}

		// handle z_order
		{
			if (z_offset >= g::z_order_count_vec.size())
			{
				c_auto before_size = g::z_order_count_vec.size();
				g::z_order_count_vec.resize(z_offset + 1);
				std::ranges::fill(g::z_order_count_vec.begin() + before_size, g::z_order_count_vec.end(), 0u);
			}

			g::z_order_count_vec[z_offset] += render_data_count;
		}

		g::layout_size_data_current_idx = g::layout_size_data_stack.size<uint32>() - 1;

		return id;
	}

	template <bool is_width>
	FORCE_INLINE bool
	is_cross(e::widget_layout e_layout) noexcept
	{
		if constexpr (is_width)
		{
			return e_layout == e::widget_layout::vertical;
		}
		else
		{
			return e_layout == e::widget_layout::horizontal;
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
					finalize_fixed<is_width>(child, child_idx, std::clamp(child_size, child.size_min<is_width>(), child.size_max<is_width>()));
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
					final_size = std::max(child.size_max<is_width>(), final_size);
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

namespace age::ui::detail
{
	FORCE_INLINE void
	handle_text(widget_desc& desc) noexcept
	{
		c_auto* p_str		  = desc.text.p_str;
		c_auto	font_size	  = desc.text.font_size;
		c_auto	font_idx	  = desc.text.font_idx;
		c_auto	text_data_idx = g::text_data_vec.size<uint32>();

		auto text_data = ui::text_data{
			.char_data_offset = g::char_data_vec.size<uint32>(),
			.word_data_offset = g::word_data_vec.size<uint32>(),
			.line_height	  = ui::font::get_line_height(font_size, font_idx),
			.space_advance	  = ui::font::get_space_advance(font_size, font_idx)
		};

		auto word_width_min		= 0.0f;
		auto max_width			= 0.f;
		auto line_width			= 0.f;
		auto word_count			= 0u;
		auto char_count			= 0u;
		auto word_char_count	= 0u;
		auto word_leading_space = 0.f;
		auto line_offset		= 0u;
		auto word_width			= 0.0f;

		for (auto c = *p_str;; c = *p_str)
		{
			if (c == ' ' or c == '\n' or c == '\0')
			{
				if (word_char_count > 0)
				{
					g::word_data_vec.emplace_back(word_data{
						.char_count	   = word_char_count,
						.width		   = word_width,
						.leading_space = word_leading_space,
						.line_offset   = line_offset });

					word_width_min	= std::max(word_width_min, word_width + word_leading_space);
					line_width	   += word_width + word_leading_space;

					word_width		   = 0.0f;
					word_char_count	   = 0;
					word_leading_space = 0.0f;
					++word_count;
				}
			}
			if (c == ' ')
			{
				word_leading_space += text_data.space_advance;
				++p_str;
				continue;
			}
			if (c == '\n')
			{
				word_leading_space = 0.f;
				max_width		   = std::max(line_width, max_width);
				line_width		   = 0.f;
				++line_offset;
				++p_str;
				continue;
			}
			if (c == '\0')
			{
				max_width = std::max(line_width, max_width);
				break;
			}

			++char_count;
			++word_char_count;

			auto&& [byte_count, unicode]  = ui::detail::decode_utf8(p_str);
			p_str						 += byte_count;

			c_auto& glyph_data = ui::font::get_glyph_data(unicode, font_idx);

			word_width += glyph_data.advance * font_size;

			g::char_data_vec.emplace_back(ui::char_data{
				.advance	  = glyph_data.advance * font_size,
				.offset		  = glyph_data.offset * font_size,
				.size		  = glyph_data.size * font_size,
				.atlas_uv_min = glyph_data.atlas_uv_min,
				.atlas_uv_max = glyph_data.atlas_uv_max,
			});
		}

		text_data.word_data_count = word_count;
		text_data.char_data_count = char_count;

		g::text_data_vec.emplace_back(std::move(text_data));

		c_auto height_min	 = (line_offset + 1u) * text_data.line_height;
		c_auto height_max	 = (line_offset + 1u + word_count) * text_data.line_height;
		c_auto padding_sum_h = desc.padding_left + desc.padding_right;
		c_auto padding_sum_v = desc.padding_top + desc.padding_bottom;

		desc.width_size_mode  = e::size_mode_kind::grow;
		desc.height_size_mode = e::size_mode_kind::text;
		desc.width_min		  = word_width_min + padding_sum_h;
		desc.width_max		  = max_width + padding_sum_h;
		desc.height_min		  = height_min + padding_sum_v;
		desc.height_max		  = height_max + padding_sum_v;

		desc.text.text_data_idx = text_data_idx;
	}
}	 // namespace age::ui::detail

namespace age::ui::widget
{
	widget_ctx
	begin(widget_desc&& desc) noexcept
	{
		if (desc.shape_kind == e::shape_kind::text)
		{
			detail::handle_text(desc);
		}
		return widget_ctx{ detail::widget_begin(std::move(desc)) };
	}
}	 // namespace age::ui::widget

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
			c_auto& text_data		  = g::text_data_vec[pos_data.text.offset];
			c_auto	char_pos_data_idx = g::char_pos_data_vec.size<uint32>();
			pos_data.text.offset	  = char_pos_data_idx;

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
	widget_begin(const char* p_str, const widget_desc& desc) noexcept
	{
		c_auto hash_id = age::ui::new_id(p_str);

		auto z_offset		   = 0u;
		auto render_data_count = 0u;

		if constexpr (is_root)
		{
			g::element_layout_data_common_stack.emplace_back(layout_data_common{
				.child_count						  = 0,
				.parent_h_idx						  = g::layout_h_current_idx,
				.parent_v_idx						  = g::layout_v_current_idx,
				.child_gap							  = desc.child_gap,
				.z_offset							  = static_cast<uint16>(z_offset),
				.child_height_depends_on_width_solved = true,
			});
		}
		else
		{
			++g::element_layout_data_common_stack.back().child_count;

			if (desc.draw)
			{
				z_offset = g::element_layout_data_common_stack.back().z_offset + 1;

				if (desc.shape_kind == e::shape_kind::text)
				{
					auto& text_data	  = g::text_data_vec[desc.text.offset];
					render_data_count = text_data.char_data_count;
				}
				else
				{
					render_data_count = 1;
				}
			}

			auto padding_sum = desc.layout == e::widget_layout::vertical
								 ? desc.padding.x + desc.padding.y
								 : desc.padding.z + desc.padding.w;

			g::element_layout_data_common_stack.emplace_back(layout_data_common{
				.child_count						  = 0,
				.parent_h_idx						  = g::layout_h_current_idx,
				.parent_v_idx						  = g::layout_v_current_idx,
				.padding_sum						  = padding_sum,
				.child_gap							  = desc.child_gap,
				.z_offset							  = static_cast<uint16>(z_offset),
				.child_height_depends_on_width_solved = true,
			});
		}

		g::layout_h_current_idx = static_cast<uint32>(g::element_layout_data_h_stack.size());
		g::layout_v_current_idx = static_cast<uint32>(g::element_layout_data_v_stack.size());

		g::element_layout_data_h_stack.emplace_back(layout_data_h{
			.layout			  = desc.layout,
			.mode			  = desc.size_mode_width.size_mode,
			.pos_data_idx	  = static_cast<uint32>(g::element_layout_pos_data_vec.size()),
			.grow_child_count = 0,
			.width			  = desc.padding.x + desc.padding.y,
			.width_min		  = desc.size_mode_width.min,
			.width_max		  = desc.size_mode_width.max,
		});

		g::element_layout_data_v_stack.emplace_back(layout_data_v{
			.layout			  = desc.layout,
			.mode			  = desc.size_mode_height.size_mode,
			.pos_data_idx	  = static_cast<uint32>(g::element_layout_pos_data_vec.size()),
			.grow_child_count = 0,
			.height			  = desc.padding.z + desc.padding.w,
			.height_min		  = desc.size_mode_height.min,
			.height_max		  = desc.size_mode_height.max,
		});

		g::element_layout_pos_data_vec.emplace_back(layout_pos_data{
			.render_data_idx   = g::element_render_data_vec.size<uint32>(),
			.render_data_count = render_data_count,
			.layout			   = desc.layout,
			.align			   = desc.align,
			.z_offset		   = static_cast<uint16>(z_offset),
			.offset			   = desc.offset,
			.child_gap		   = desc.child_gap,
			.padding_left	   = desc.padding.x,
			.padding_right	   = desc.padding.y,
			.padding_top	   = desc.padding.z,
			.padding_bottom	   = desc.padding.w,
			.text			   = { .offset = desc.text.offset, .atlas_id = desc.text.atlas_id },
		});

		if (desc.draw)
		{
			g::element_render_data_vec.emplace_back(render_data{
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

		return hash_id;
	}

	FORCE_INLINE void
	finalize_width(layout_data_h& layout_data, uint32 layout_h_idx) noexcept
	{
		auto& pos_data	  = g::element_layout_pos_data_vec[layout_data.pos_data_idx];
		auto  available	  = layout_data.width_max - std::min(layout_data.width, layout_data.width_max);
		layout_data.width = std::clamp(layout_data.width, layout_data.width_min, layout_data.width_max);
		pos_data.width	  = layout_data.width;

		if (layout_data.grow_child_count == 0) { return; }

		c_auto padding_sum = pos_data.padding_left + pos_data.padding_right;
		if (layout_data.layout == e::widget_layout::vertical)
		{
			for (auto grow_idx = layout_h_idx + 1;
				 auto i : std::views::iota(0) | std::views::take(layout_data.grow_child_count))
			{
				auto& grow_child		  = g::element_layout_data_h_stack[grow_idx];
				auto& grow_child_pos_data = g::element_layout_pos_data_vec[grow_child.pos_data_idx];

				grow_child.width = std::clamp(layout_data.width - padding_sum, grow_child.width_min, grow_child.width_max);

				grow_child_pos_data.width = grow_child.width;

				if (height_depends_on_width(grow_child.mode))
				{
					grow_child_pos_data.height = calc_height_depends_on_width(grow_child.mode, grow_child_pos_data);
				}

				finalize_width(grow_child, grow_idx);

				grow_idx += grow_child.grow_child_count + 1;
			}

			return;
		}

		g::element_layout_grow_event_vec.clear();
		g::element_layout_grow_event_vec.reserve(layout_data.grow_child_count * 2);

		for (auto grow_idx = layout_h_idx + 1;
			 auto i : std::views::iota(0) | std::views::take(layout_data.grow_child_count))
		{
			auto& grow_child = g::element_layout_data_h_stack[grow_idx];
			g::element_layout_grow_event_vec.emplace_back(detail::grow_sort_event(grow_child.width, false, grow_idx));
			g::element_layout_grow_event_vec.emplace_back(detail::grow_sort_event(grow_child.width_max, true, grow_idx));
			grow_idx += grow_child.grow_child_count + 1;
		}

		std::ranges::sort(g::element_layout_grow_event_vec);

		auto active_count = 0;
		auto level		  = detail::grow_sort_event_value(g::element_layout_grow_event_vec[0]);

		for (auto e : g::element_layout_grow_event_vec)
		{
			c_auto value	= detail::grow_sort_event_value(e);
			c_auto is_leave = detail::grow_sort_event_is_leave(e);
			c_auto idx		= detail::grow_sort_event_idx(e);
			c_auto cost		= (value - level) * active_count;
			if (cost > available)
			{
				level += available / static_cast<float>(active_count);
				break;
			}

			available	 -= cost;
			active_count += 1 - 2 * is_leave;
			level		  = value;
		}


		for (auto grow_idx = layout_h_idx + 1;
			 auto i : std::views::iota(0) | std::views::take(layout_data.grow_child_count))
		{
			auto& grow_child		  = g::element_layout_data_h_stack[grow_idx];
			auto& grow_child_pos_data = g::element_layout_pos_data_vec[grow_child.pos_data_idx];

			grow_child.width		  = std::clamp(level, grow_child.width_min, grow_child.width_max);
			grow_child_pos_data.width = grow_child.width;

			if (height_depends_on_width(grow_child.mode))
			{
				grow_child_pos_data.height = calc_height_depends_on_width(grow_child.mode, grow_child_pos_data);
			}

			finalize_width(grow_child, grow_idx);

			grow_idx += grow_child.grow_child_count + 1;
		}
	}

	FORCE_INLINE void
	finalize_height(layout_data_v& layout_data, uint32 layout_v_idx) noexcept
	{
		auto& pos_data = g::element_layout_pos_data_vec[layout_data.pos_data_idx];

		if (layout_data.grow_child_count == 0)
		{
			layout_data.height = std::clamp(layout_data.height, layout_data.height_min, layout_data.height_max);
			pos_data.height	   = layout_data.height;
			return;
		}

		if (layout_data.layout == e::widget_layout::horizontal)
		{
			auto padding_sum = pos_data.padding_top + pos_data.padding_bottom;
			for (auto grow_idx = layout_v_idx + 1;
				 auto i : std::views::iota(0) | std::views::take(layout_data.grow_child_count))
			{
				auto& grow_child		  = g::element_layout_data_v_stack[grow_idx];
				auto& grow_child_pos_data = g::element_layout_pos_data_vec[grow_child.pos_data_idx];

				if (height_depends_on_width(grow_child.mode))
				{
					grow_child.height = grow_child_pos_data.height;

					layout_data.height = std::max(layout_data.height, grow_child.height + padding_sum);
				}
				else
				{
					grow_child.height		   = std::clamp(layout_data.height, grow_child.height, grow_child.height_max);
					grow_child_pos_data.height = grow_child.height;
				}

				finalize_height(grow_child, grow_idx);

				grow_idx += grow_child.grow_child_count + 1;
			}

			layout_data.height = std::clamp(layout_data.height, layout_data.height_min, layout_data.height_max);
			pos_data.height	   = layout_data.height;
			return;
		}

		g::element_layout_grow_event_vec.clear();
		g::element_layout_grow_event_vec.reserve(layout_data.grow_child_count * 2);

		for (auto grow_idx = layout_v_idx + 1;
			 auto i : std::views::iota(0) | std::views::take(layout_data.grow_child_count))
		{
			auto& grow_child = g::element_layout_data_v_stack[grow_idx];

			auto& grow_child_pos_data = g::element_layout_pos_data_vec[grow_child.pos_data_idx];

			if (height_depends_on_width(grow_child.mode))
			{
				grow_child.height = grow_child_pos_data.height;

				layout_data.height += grow_child.height;
			}
			else
			{
				grow_child.height		   = std::clamp(layout_data.height, grow_child.height, grow_child.height_max);
				grow_child_pos_data.height = grow_child.height;

				g::element_layout_grow_event_vec.emplace_back(detail::grow_sort_event(grow_child.height, false, grow_idx));
				g::element_layout_grow_event_vec.emplace_back(detail::grow_sort_event(grow_child.height_max, true, grow_idx));

				grow_idx += grow_child.grow_child_count + 1;
			}
		}

		auto available	   = layout_data.height_max - std::min(layout_data.height, layout_data.height_max);
		layout_data.height = std::clamp(layout_data.height, layout_data.height_min, layout_data.height_max);
		pos_data.height	   = layout_data.height;

		std::ranges::sort(g::element_layout_grow_event_vec);

		auto active_count = 0;
		auto level		  = detail::grow_sort_event_value(g::element_layout_grow_event_vec[0]);

		for (auto e : g::element_layout_grow_event_vec)
		{
			c_auto value	= detail::grow_sort_event_value(e);
			c_auto is_leave = detail::grow_sort_event_is_leave(e);
			c_auto idx		= detail::grow_sort_event_idx(e);

			c_auto cost = (value - level) * active_count;
			if (cost > available)
			{
				level += available / static_cast<float>(active_count);
				break;
			}

			available	 -= cost;
			active_count += 1 - 2 * is_leave;
			level		  = value;
		}

		for (auto grow_idx = layout_v_idx + 1;
			 auto i : std::views::iota(0) | std::views::take(layout_data.grow_child_count))
		{
			auto& grow_child		  = g::element_layout_data_v_stack[grow_idx];
			auto& grow_child_pos_data = g::element_layout_pos_data_vec[grow_child.pos_data_idx];

			if (height_depends_on_width(grow_child.mode) is_false)
			{
				grow_child.height = std::clamp(level, grow_child.height, grow_child.height_max);

				g::element_layout_pos_data_vec[grow_child.pos_data_idx].height = grow_child.height;
			}

			finalize_height(grow_child, grow_idx);

			grow_idx += grow_child.grow_child_count + 1;
		}
	}

	template <bool is_root = false>
	FORCE_INLINE void
	widget_end() noexcept
	{
		auto& layout_common_current = g::element_layout_data_common_stack.back();

		auto& layout_common_parent = AGE_LAMBDA((), {
			if constexpr (is_root)
			{
				return layout_common_current;
			}
			else
			{
				return g::element_layout_data_common_stack[g::element_layout_data_common_stack.size() - 2];
			}
		})();

		c_auto& layout_h_parent_idx = layout_common_current.parent_h_idx;
		c_auto& layout_v_parent_idx = layout_common_current.parent_v_idx;

		auto& layout_h_current = g::element_layout_data_h_stack[g::layout_h_current_idx];
		auto& layout_v_current = g::element_layout_data_v_stack[g::layout_v_current_idx];

		auto& layout_h_parent = g::element_layout_data_h_stack[layout_h_parent_idx];
		auto& layout_v_parent = g::element_layout_data_v_stack[layout_v_parent_idx];

		auto& pos_data		 = g::element_layout_pos_data_vec[layout_v_current.pos_data_idx];
		pos_data.child_count = layout_common_current.child_count;

		c_auto can_finalize_width = layout_h_current.mode == e::size_mode_kind::fixed
								 or layout_h_current.mode == e::size_mode_kind::fit;

		if (can_finalize_width)
		{
			detail::finalize_width(layout_h_current, g::layout_h_current_idx);

			layout_common_current.child_height_depends_on_width_solved = true;
		}
		else
		{
			++layout_h_parent.grow_child_count;
		}

		if (detail::height_depends_on_width(layout_v_current.mode))
		{
			AGE_ASSERT(can_finalize_width is_false);

			layout_common_parent.child_height_depends_on_width_solved = false;
		}


		c_auto can_finalize_height = (layout_v_current.mode == e::size_mode_kind::fixed
									  or layout_v_current.mode == e::size_mode_kind::fit)
								 and layout_common_current.child_height_depends_on_width_solved;

		if (can_finalize_height)
		{
			// layout_v_current.height = std::clamp(layout_v_current.height, layout_v_current.height_min, layout_v_current.height_max);

			// g::element_layout_pos_data_vec[layout_v_current.pos_data_idx].height = layout_v_current.height;

			detail::finalize_height(layout_v_current, g::layout_v_current_idx);
		}
		else
		{
			++layout_v_parent.grow_child_count;
		}

		AGE_ASSERT(layout_h_current.layout == layout_v_current.layout);
		AGE_ASSERT(layout_h_parent.layout == layout_v_parent.layout);

		if (layout_h_parent.layout == e::widget_layout::horizontal)
		{
			layout_h_parent.width  += layout_h_current.width + layout_common_parent.child_gap * (layout_common_parent.child_count > 1);
			layout_v_parent.height	= std::max(layout_v_parent.height, layout_v_current.height + layout_common_parent.padding_sum);
		}
		else if (layout_h_parent.layout == e::widget_layout::vertical)
		{
			layout_h_parent.width	= std::max(layout_h_parent.width, layout_h_current.width + layout_common_parent.padding_sum);
			layout_v_parent.height += layout_v_current.height + layout_common_parent.child_gap * (layout_common_parent.child_count > 1);
		}
		else
		{
			AGE_UNREACHABLE();
		}


		// handle z_order
		{
			if (layout_common_current.z_offset >= g::element_z_order_count_vec.size())
			{
				c_auto before_size = g::element_z_order_count_vec.size();
				g::element_z_order_count_vec.resize(layout_common_current.z_offset + 1);
				std::ranges::fill(g::element_z_order_count_vec.begin() + before_size, g::element_z_order_count_vec.end(), 0u);
			}

			g::element_z_order_count_vec[layout_common_current.z_offset] += pos_data.render_data_count;
		}

		if (can_finalize_width)
		{
			g::element_layout_data_h_stack.resize(g::layout_h_current_idx);
		}
		if (can_finalize_height)
		{
			g::element_layout_data_v_stack.resize(g::layout_v_current_idx);
		}

		g::layout_h_current_idx = layout_common_current.parent_h_idx;
		g::layout_v_current_idx = layout_common_current.parent_v_idx;
		g::element_layout_data_common_stack.pop_back();
	}
};	  // namespace age::ui::detail

namespace age::ui::widget
{
	widget_ctx
	begin(const char* p_str, const widget_desc& desc) noexcept
	{
		return widget_ctx{ .hash_id = detail::widget_begin(p_str, desc) };
	}

	widget_ctx
	begin(const widget_desc& desc) noexcept
	{
		return widget_ctx{ .hash_id = detail::widget_begin(nullptr, desc) };
	}
}	 // namespace age::ui::widget

namespace age::ui
{
	widget_ctx::~widget_ctx() noexcept
	{
		detail::widget_end();
	}
}	 // namespace age::ui

// primitives
namespace age::ui::widget
{
	widget_ctx
	layout_horizontal(widget_size_mode width,
					  widget_size_mode height,
					  float4		   padding,
					  e::widget_align  align,
					  float2		   offset) noexcept
	{
		return begin(
			nullptr,
			age::ui::widget_desc{
				.draw			   = false,
				.layout			   = age::ui::e::widget_layout::horizontal,
				.align			   = align,
				.size_mode_width   = width,
				.size_mode_height  = height,
				.z_offset		   = 0,
				.offset			   = offset,
				.child_gap		   = 10.f,
				.padding		   = padding,
				.shape_kind		   = age::ui::e::shape_kind::rect,
				.body_brush_kind   = age::ui::e::brush_kind::color,
				.border_brush_kind = age::ui::e::brush_kind::color,
				.body_brush_data   = age::ui::brush_data::color(0.15f, 0.15f, 0.15f),
				.border_brush_data = age::ui::brush_data::color(1.0f, 1.0f, 1.0f),
			});
	}

	widget_ctx
	layout_vertical(widget_size_mode width,
					widget_size_mode height,
					float4			 padding,
					e::widget_align	 align,
					float2			 offset) noexcept
	{
		return begin(
			nullptr,
			age::ui::widget_desc{
				.draw			   = false,
				.layout			   = age::ui::e::widget_layout::vertical,
				.align			   = align,
				.size_mode_width   = width,
				.size_mode_height  = height,
				.z_offset		   = 0,
				.offset			   = offset,
				.child_gap		   = 10.f,
				.padding		   = padding,
				.shape_kind		   = age::ui::e::shape_kind::rect,
				.body_brush_kind   = age::ui::e::brush_kind::color,
				.border_brush_kind = age::ui::e::brush_kind::color,
				.body_brush_data   = age::ui::brush_data::color(0.15f, 0.15f, 0.15f),
				.border_brush_data = age::ui::brush_data::color(1.0f, 1.0f, 1.0f),
			});
	}
}	 // namespace age::ui::widget

namespace age::ui::widget
{
	widget_ctx
	text(const char* p_str, float font_size) noexcept
	{
		c_auto text_data_idx = g::text_data_vec.size<uint32>();

		auto text_data = ui::text_data{
			.char_data_offset = g::char_data_vec.size<uint32>(),
			.word_data_offset = g::word_data_vec.size<uint32>(),
			.line_height	  = ui::font::get_height(font_size),
			.space_advance	  = ui::font::get_advance(' ', font_size)
		};

		auto word_width_min		= 0.0f;
		auto line_width			= 0.0f;
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

					word_width_min	= std::max(word_width_min, word_width);
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
				++line_offset;
				++p_str;
				continue;
			}
			if (c == '\0')
			{
				break;
			}

			++char_count;
			++word_char_count;

			auto&& [byte_count, unicode]  = ui::detail::decode_utf8(p_str);
			p_str						 += byte_count;

			c_auto& glyph_data = ui::font::get_glyph_data(unicode);

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

		c_auto height_min = (line_offset + 1u) * font::get_height(font_size);

		g::text_data_vec.emplace_back(std::move(text_data));

		return begin(
			p_str,
			age::ui::widget_desc{
				.draw			  = true,
				.layout			  = e::widget_layout::horizontal,
				.align			  = e::widget_align::center,
				.size_mode_width  = size_mode::text(word_width_min, line_width),
				.size_mode_height = size_mode::text(height_min, std::numeric_limits<float>::max()),
				.z_offset		  = 0,
				//.offset			   = offset,
				.child_gap = 10.f,
				//.padding		   = padding,
				.border_thickness  = 0.f,
				.shape_kind		   = e::shape_kind::text,
				.body_brush_kind   = e::brush_kind::color,
				.border_brush_kind = e::brush_kind::color,
				.body_brush_data   = brush_data::color(0.15f, 0.15f, 0.15f),
				.border_brush_data = brush_data::color(1.0f, 1.0f, 1.0f, 0.f),
				.text			   = { .offset = text_data_idx, .atlas_id = g::font_data_vec[g::current_font_idx].second.atlas_id },
			});
	}
}	 // namespace age::ui::widget
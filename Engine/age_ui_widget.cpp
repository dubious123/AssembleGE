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
	calc_height_depends_on_width(const layout_data_v& layout_data) noexcept
	{
		switch (layout_data.mode)
		{
		case e::size_mode_kind::text:
		{
			return 0.f;
		}
		case e::size_mode_kind::aspect_ratio:
		{
			auto&  data			= g::element_layout_pos_data_vec[layout_data.global_idx];
			c_auto aspect_ratio = std::bit_cast<float>(static_cast<uint32>(data.extra));
			return data.width * aspect_ratio;
		}
		default:
		{
			AGE_UNREACHABLE();
		}
		}
	}

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

	FORCE_INLINE void
	handle_child_grow_width(const layout_data_h& layout_data, uint32 layout_h_idx) noexcept
	{
		if (layout_data.grow_child_count == 0) { return; }

		if (layout_data.layout == e::widget_layout::vertical)
		{
			for (auto grow_idx = layout_h_idx + 1;
				 auto i : std::views::iota(0) | std::views::take(layout_data.grow_child_count))
			{
				auto& grow_child = g::element_layout_data_h_stack[grow_idx];

				grow_child.width = std::clamp(layout_data.width, grow_child.width, grow_child.width_max);

				g::element_layout_pos_data_vec[grow_child.global_idx].width = grow_child.width;

				handle_child_grow_width(grow_child, grow_idx);

				grow_idx += grow_child.grow_child_count + 1;
			}

			return;
		}

		g::element_layout_grow_event_vec.clear();
		g::element_layout_grow_event_vec.reserve(layout_data.grow_child_count * 2);

		auto available = layout_data.width_max - layout_data.width;

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

		for (auto grow_idx = layout_h_idx + 1;
			 auto i : std::views::iota(0) | std::views::take(layout_data.grow_child_count))
		{
			auto& grow_child = g::element_layout_data_h_stack[grow_idx];
			grow_child.width = std::clamp(level, grow_child.width, grow_child.width_max);

			g::element_layout_pos_data_vec[grow_child.global_idx].width = grow_child.width;

			handle_child_grow_width(grow_child, grow_idx);

			grow_idx += grow_child.grow_child_count + 1;
		}
	}

	FORCE_INLINE void
	handle_child_grow_height(const layout_data_v& layout_data, uint32 layout_v_idx) noexcept
	{
		if (layout_data.grow_child_count == 0) { return; }

		if (layout_data.layout == e::widget_layout::horizontal)
		{
			for (auto grow_idx = layout_v_idx + 1;
				 auto i : std::views::iota(0) | std::views::take(layout_data.grow_child_count))
			{
				auto& grow_child = g::element_layout_data_v_stack[grow_idx];

				if (height_depends_on_width(grow_child.mode))
				{
					grow_child.height = calc_height_depends_on_width(grow_child);
				}

				grow_child.height = std::clamp(layout_data.height, grow_child.height, grow_child.height_max);

				g::element_layout_pos_data_vec[grow_child.global_idx].height = grow_child.height;

				handle_child_grow_height(grow_child, grow_idx);

				grow_idx += grow_child.grow_child_count + 1;
			}

			return;
		}

		g::element_layout_grow_event_vec.clear();
		g::element_layout_grow_event_vec.reserve(layout_data.grow_child_count * 2);

		auto available = layout_data.height_max - layout_data.height;

		for (auto grow_idx = layout_v_idx + 1;
			 auto i : std::views::iota(0) | std::views::take(layout_data.grow_child_count))
		{
			auto& grow_child = g::element_layout_data_v_stack[grow_idx];

			if (height_depends_on_width(grow_child.mode))
			{
				grow_child.height	  = calc_height_depends_on_width(grow_child);
				grow_child.height_max = grow_child.height;
			}

			g::element_layout_grow_event_vec.emplace_back(detail::grow_sort_event(grow_child.height, false, grow_idx));
			g::element_layout_grow_event_vec.emplace_back(detail::grow_sort_event(grow_child.height_max, true, grow_idx));
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
			auto& grow_child  = g::element_layout_data_v_stack[grow_idx];
			grow_child.height = std::clamp(level, grow_child.height, grow_child.height_max);

			g::element_layout_pos_data_vec[grow_child.global_idx].height = grow_child.height;

			handle_child_grow_height(grow_child, grow_idx);

			grow_idx += grow_child.grow_child_count + 1;
		}
	}
}	 // namespace age::ui::detail

namespace age::ui::detail
{
	template <bool is_root = false>
	FORCE_INLINE t_hash
	widget_begin(const char* p_str, const widget_desc& desc) noexcept
	{
		c_auto hash_id = age::ui::new_id(p_str);

		if constexpr (is_root)
		{
			g::element_layout_data_common_stack.emplace_back(layout_data_common{
				.child_count						  = 0,
				.parent_h_idx						  = g::layout_h_current_idx,
				.parent_v_idx						  = g::layout_v_current_idx,
				.child_gap							  = desc.child_gap,
				.z_offset							  = desc.z_offset,
				.child_height_depends_on_width_solved = true,
			});
		}
		else
		{
			++g::element_layout_data_common_stack.back().child_count;

			g::element_layout_data_common_stack.emplace_back(layout_data_common{
				.child_count						  = 0,
				.parent_h_idx						  = g::layout_h_current_idx,
				.parent_v_idx						  = g::layout_v_current_idx,
				.child_gap							  = desc.child_gap,
				.z_offset							  = static_cast<uint16>(g::element_layout_data_common_stack.back().z_offset + desc.z_offset),
				.child_height_depends_on_width_solved = true,
			});
		}


		g::layout_h_current_idx = static_cast<uint32>(g::element_layout_data_h_stack.size());
		g::layout_v_current_idx = static_cast<uint32>(g::element_layout_data_v_stack.size());

		c_auto width  = std::max(desc.size_mode_width.min, desc.padding.x + desc.padding.y);
		c_auto height = std::max(desc.size_mode_height.min, desc.padding.z + desc.padding.w);

		g::element_layout_data_h_stack.emplace_back(layout_data_h{
			.layout			  = desc.layout,
			.mode			  = desc.size_mode_width.size_mode,
			.global_idx		  = static_cast<uint32>(g::element_render_data_vec.size()),
			.grow_child_count = 0,
			.width			  = width,
			.width_max		  = desc.size_mode_width.max,
		});

		g::element_layout_data_v_stack.emplace_back(layout_data_v{
			.layout			  = desc.layout,
			.mode			  = desc.size_mode_height.size_mode,
			.global_idx		  = static_cast<uint32>(g::element_render_data_vec.size()),
			.grow_child_count = 0,
			.height			  = height,
			.height_max		  = desc.size_mode_height.max,
		});

		g::element_layout_pos_data_vec.emplace_back(layout_pos_data{
			.draw			= desc.draw,
			.layout			= desc.layout,
			.align			= desc.align,
			.z_offset		= desc.z_offset,
			.offset			= desc.offset,
			.child_gap		= desc.child_gap,
			.padding_left	= desc.padding.x,
			.padding_right	= desc.padding.y,
			.padding_top	= desc.padding.z,
			.padding_bottom = desc.padding.w,
		});

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

		AGE_ASSERT(g::element_layout_pos_data_vec.size() == g::element_render_data_vec.size());

		return hash_id;
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


		c_auto can_finalize_width = layout_h_current.mode == e::size_mode_kind::fixed
								 or layout_h_current.mode == e::size_mode_kind::fit;

		if (can_finalize_width)
		{
			g::element_layout_pos_data_vec[layout_h_current.global_idx].width = layout_h_current.width;

			detail::handle_child_grow_width(layout_h_current, g::layout_h_current_idx);


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
			g::element_layout_pos_data_vec[layout_v_current.global_idx].height = layout_v_current.height;

			detail::handle_child_grow_height(layout_v_current, g::layout_v_current_idx);
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
			layout_v_parent.height	= std::max(layout_v_parent.height, layout_v_current.height);
		}
		else if (layout_h_parent.layout == e::widget_layout::vertical)
		{
			layout_h_parent.width	= std::max(layout_h_parent.width, layout_h_current.width);
			layout_v_parent.height += layout_v_current.height + layout_common_parent.child_gap * (layout_common_parent.child_count > 1);
		}
		else
		{
			AGE_UNREACHABLE();
		}


		AGE_ASSERT(layout_h_current.global_idx == layout_v_current.global_idx);
		g::element_layout_pos_data_vec[layout_v_current.global_idx].child_count = layout_common_current.child_count;

		// handle z_order
		if (g::element_layout_pos_data_vec[layout_v_current.global_idx].draw)
		{
			if (layout_common_current.z_offset >= g::element_z_order_count_vec.size())
			{
				c_auto before_size = g::element_z_order_count_vec.size();
				g::element_z_order_count_vec.resize(layout_common_current.z_offset + 1);
				std::ranges::fill(g::element_z_order_count_vec.begin() + before_size, g::element_z_order_count_vec.end(), 0u);
			}

			++g::element_z_order_count_vec[layout_common_current.z_offset];
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
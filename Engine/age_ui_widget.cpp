#include "age_pch.hpp"
#include "age.hpp"

namespace age::ui::detail
{
	FORCE_INLINE void
	finalize_width() noexcept
	{
	}

	FORCE_INLINE void
	finalize_height() noexcept
	{
	}

	FORCE_INLINE void
	finalize_relative_position() noexcept
	{
	}

	FORCE_INLINE void
	submit_render_data_and_cleanup_vectors() noexcept
	{
	}
}	 // namespace age::ui::detail

namespace age::ui::detail
{
	FORCE_INLINE t_hash
	widget_begin(const char* p_str, const widget_desc& desc) noexcept
	{
		c_auto hash_id = age::ui::new_id(p_str);

		++g::element_layout_data_h_stack[g::layout_h_parent_idx].child_count;
		++g::element_layout_data_v_stack[g::layout_v_parent_idx].child_count;

		g::element_layout_data_h_stack.emplace_back(layout_data_h{
			// const
			.layout		   = desc.layout,
			.align		   = desc.align,
			.mode		   = desc.size_mode_width.size_mode,
			.width_min	   = desc.size_mode_width.min,
			.width_max	   = desc.size_mode_width.max,
			.padding_left  = desc.padding.x,
			.padding_right = desc.padding.y,
		});

		g::element_layout_data_v_stack.emplace_back(layout_data_v{
			// const
			.layout			= desc.layout,
			.align			= desc.align,
			.mode			= desc.size_mode_height.size_mode,
			.height_min		= desc.size_mode_height.min,
			.height_max		= desc.size_mode_height.max,
			.padding_top	= desc.padding.z,
			.padding_bottom = desc.padding.w,
		});

		g::render_data_offset_h_stack.emplace_back(g::element_render_data_vec.size<uint32>());
		g::render_data_offset_v_stack.emplace_back(g::element_render_data_vec.size<uint32>());

		g::element_render_data_vec.emplace_back(desc.render_data);

		return hash_id;
	}

	FORCE_INLINE void
	widget_end() noexcept
	{
		auto&	layout_h_parent	   = g::render_data_offset_h_stack[g::layout_h_parent_idx];
		c_auto& layout_h_current   = g::element_layout_data_h_stack.back();
		c_auto	can_finalize_width = layout_h_current.mode == e::size_mode_kind::fixed
								  or layout_h_current.mode == e::size_mode_kind::fit;

		if (can_finalize_width)
		{
			// finalize_width;

			// handle text wrapping
			g::render_data_offset_h_stack.pop_back();
			g::element_layout_data_h_stack.resize(g::layout_h_parent_idx + 1);
			--g::layout_h_parent_idx;
		}


		auto&	layout_v_parent		= g::render_data_offset_v_stack[g::layout_v_parent_idx];
		c_auto& layout_v_current	= g::element_layout_data_v_stack.back();
		c_auto	can_finalize_height = layout_v_current.mode == e::size_mode_kind::fixed
								   or layout_v_current.mode == e::size_mode_kind::fit;

		if (can_finalize_height)
		{
			// finalize_height;
			g::render_data_offset_v_stack.pop_back();
			g::element_layout_data_v_stack.resize(g::layout_v_parent_idx + 1);
			--g::layout_v_parent_idx;
		}


		if (c_auto is_root = elem_parent.parent_idx == 0)
		{
			// finalize_relative_position_and_render_data;
			g::render_data_offset += g::element_vec.size<uint32>();
		}

		// 1. fit sizing width | child -> parent

		// 2. grow and shrink sizing width | parent -> child
		// 3. wrap text | child, based on final width
		// 4. fit sizing heights | child -> parent
		// 5. grow and shrink sizing heights | parent -> child
		// 6. calculate position
		// 7. draw
	}
};	  // namespace age::ui::detail

namespace age::ui
{
	widget_ctx
	widget(const char* p_str, const widget_desc& desc) noexcept
	{
		return widget_ctx{ .hash_id = detail::widget_begin(p_str, desc) };
	}
}	 // namespace age::ui

namespace age::ui
{
	widget_ctx::~widget_ctx() noexcept
	{
		detail::widget_end();
	}
}	 // namespace age::ui
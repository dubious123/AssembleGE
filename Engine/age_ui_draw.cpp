#include "age_pch.hpp"
#include "age.hpp"

namespace age::ui
{
	void
	draw_direct(widget_desc&& desc) noexcept
	{
		AGE_ASSERT(desc.draw);
		AGE_ASSERT(desc.interact is_false);
		AGE_ASSERT(desc.save_state is_false);

		auto& size_data_parent = g::layout_size_data_stack[g::layout_size_data_current_idx];
		auto& pos_data_parent  = g::layout_pos_data_vec[size_data_parent.pos_data_idx];

		auto render_data_count = 0u;
		auto z_offset		   = 0u;
		auto atlas_id		   = 0u;

		++pos_data_parent.child_count;
		z_offset = pos_data_parent.z_offset + desc.z_offset;

		if (desc.shape_kind == e::shape_kind::text)
		{
			if (AGE_IS_INVALID_IDX(desc.text.text_data_idx))
			{
				detail::gen_text_data(desc);
			}

			auto& text_data	  = g::text_data_vec[desc.text.text_data_idx];
			render_data_count = text_data.char_data_count;

			atlas_id = g::font_data_vec[desc.text.font_idx].second.atlas_id;
		}
		else
		{
			render_data_count = 1;
		}

		g::layout_pos_data_vec.emplace_back(layout_pos_data{
			.id				   = age::get_invalid_id<t_hash>(),
			.render_data_idx   = g::render_data_vec.size<uint32>(),
			.render_data_count = render_data_count,
			.layout			   = desc.layout,
			.align			   = desc.align,
			.z_offset		   = static_cast<uint16>(z_offset),
			.offset			   = desc.offset,
			.width			   = desc.width_min,
			.height			   = desc.height_min,
			.child_gap		   = desc.child_gap,
			.padding_left	   = desc.padding_left,
			.padding_right	   = desc.padding_right,
			.padding_top	   = desc.padding_top,
			.padding_bottom	   = desc.padding_bottom,
			.interact		   = false,
			.save_state		   = false,
			.direct_draw	   = true,
			.text			   = { .idx = desc.text.text_data_idx, .atlas_id = atlas_id },
		});

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

		{
			if (z_offset >= g::z_order_count_vec.size())
			{
				c_auto before_size = g::z_order_count_vec.size();
				g::z_order_count_vec.resize(z_offset + 1);
				std::ranges::fill(g::z_order_count_vec.begin() + before_size, g::z_order_count_vec.end(), 0u);
			}

			g::z_order_count_vec[z_offset] += render_data_count;
		}
	}
}	 // namespace age::ui
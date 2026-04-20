#pragma once
#include "age.hpp"

namespace age::ui::detail
{
	template <bool is_root = false>
	FORCE_INLINE t_hash
	widget_begin(auto&& desc) noexcept
		requires std::is_same_v<BARE_OF(desc), widget_desc>
	{
		c_auto id = new_id();

		auto z_offset		   = 0u;
		auto render_data_count = 0u;
		auto padding_sum	   = 0.f;
		auto atlas_id		   = 0u;

		if constexpr (is_root is_false)
		{
			auto& size_data_parent = g::layout_size_data_stack[g::layout_size_data_current_idx];
			auto& pos_data_parent  = g::layout_pos_data_vec[size_data_parent.pos_data_idx];
			++pos_data_parent.child_count;

			z_offset = pos_data_parent.z_offset + desc.z_offset;

			if (desc.draw)
			{
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
			}

			padding_sum = desc.layout == e::widget_layout::vertical or desc.layout == e::widget_layout::vertical_inv
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
			.width_max			= desc.width_max,
			.width_final		= 0.f,
			.height_min			= desc.height_min,
			.height_max			= desc.height_max,
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
			.direct_draw	   = false,
			.text			   = { .idx = desc.text.text_data_idx, .atlas_id = atlas_id },
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

	template <bool is_root = false>
	t_hash
	widget_begin(auto&& mod) noexcept
		requires meta::is_not_same_v<BARE_OF(mod), widget_desc>
	{
		auto desc = widget_desc{};
		FWD(mod).apply(desc);
		return widget_begin<is_root>(std::move(desc));
	}
}	 // namespace age::ui::detail

namespace age::ui::widget
{
	FORCE_INLINE widget_ctx
	begin(auto&& mod) noexcept
	{
		return widget_ctx{ ui::detail::widget_begin(FWD(mod)) };
	}
}	 // namespace age::ui::widget

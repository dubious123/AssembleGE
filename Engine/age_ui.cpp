#include "age_pch.hpp"
#include "age.hpp"

namespace age::ui
{
	void
	init() noexcept
	{
		AGE_ASSERT(g::id_stack.is_empty());
		g::current_font_idx	 = 0;
		g::current_font_size = 0;
	}

	void
	begin_frame(platform::window_handle h_window) noexcept
	{
		AGE_ASSERT(g::id_stack.is_empty());
		AGE_ASSERT(g::element_layout_data_h_stack.is_empty());
		AGE_ASSERT(g::element_layout_data_v_stack.is_empty());
		AGE_ASSERT(g::element_layout_data_common_stack.is_empty());
		AGE_ASSERT(g::element_layout_pos_data_vec.is_empty());
		AGE_ASSERT(g::element_render_data_vec.is_empty());

		g::id_stack.emplace_back(g::fnv1a_offset_basis);

		g::layout_h_current_idx = 0;
		g::layout_v_current_idx = 0;

		g::element_z_order_count_vec.clear();

		std::ranges::fill(g::element_z_order_count_vec, 0u);

		detail::widget_begin<true>(
			"__age_ui_root__",
			widget_desc{
				.draw			  = false,
				.layout			  = e::widget_layout::vertical,
				.size_mode_width  = size_mode::fixed(platform::get_client_width(h_window)),
				.size_mode_height = size_mode::fixed(platform::get_client_height(h_window)),
				.z_offset		  = 0,
				.offset			  = { 0.f, 0.f },
				.padding		  = { 0.f, 0.f, 0.f, 0.f } });

		g::element_layout_pos_data_vec[0].clip_rect = float4{ 0, 0, platform::get_client_width(h_window), platform::get_client_height(h_window) };
	}

	void
	end_frame(age::vector<render_data>& render_data_vec, age::vector<util::range>& render_data_z_range_vec) noexcept
	{
		detail::widget_end<true>();

		AGE_ASSERT(g::id_stack.size() == 1);
		AGE_ASSERT(g::element_layout_data_h_stack.size() == 0);
		AGE_ASSERT(g::element_layout_data_v_stack.size() == 0);
		AGE_ASSERT(g::element_layout_data_common_stack.size() == 0);

		render_data_vec.resize(g::element_render_data_vec.size());

		for (auto offset = 0u;
			 auto&& [i, z_count] : g::element_z_order_count_vec | std::views::enumerate)
		{
			if (z_count > 0)
			{
				render_data_z_range_vec.emplace_back(util::range{ .offset = offset, .count = z_count });
				c_auto temp	 = offset;
				offset		+= z_count;
				z_count		 = temp;
			}
		}

		g::element_pos_parent_idx_stack.clear();
		g::element_pos_parent_idx_stack.emplace_back(0);	// root

		for (auto current_idx : std::views::iota(1u, g::element_layout_pos_data_vec.size<uint32>()))
		{
			while (g::element_layout_pos_data_vec[g::element_pos_parent_idx_stack.back()].child_count == 0)
			{
				g::element_pos_parent_idx_stack.pop_back();
			}

			c_auto parent_idx = g::element_pos_parent_idx_stack.back();
			auto&  parent	  = g::element_layout_pos_data_vec[parent_idx];
			auto&  child	  = g::element_layout_pos_data_vec[current_idx];

			if (parent.layout == e::widget_layout::horizontal)
			{
				child.offset.x += parent.offset.x;
				if (child.align == e::widget_align::begin)
				{
					child.offset.y += parent.offset.y + parent.padding_top;
				}
				else if (child.align == e::widget_align::end)
				{
					child.offset.y += parent.offset.y + parent.height - parent.padding_bottom - child.height;
				}
				else if (child.align == e::widget_align::center)
				{
					child.offset.y += parent.offset.y + (parent.padding_top + parent.height - parent.padding_bottom - child.height) / 2;
				}
				else
				{
					AGE_UNREACHABLE();
				}

				parent.offset.x += child.width + parent.child_gap;
			}
			else if (parent.layout == e::widget_layout::vertical)
			{
				child.offset.y += parent.offset.y;
				if (child.align == e::widget_align::begin)
				{
					child.offset.x += parent.offset.x + parent.padding_left;
				}
				else if (child.align == e::widget_align::end)
				{
					child.offset.x += parent.offset.x + parent.width - parent.padding_right - child.width;
				}
				else if (child.align == e::widget_align::center)
				{
					child.offset.x += parent.offset.x + (parent.padding_left + parent.width - parent.padding_right - child.width) / 2;
				}
				else
				{
					AGE_UNREACHABLE();
				}

				parent.offset.y += child.height + parent.child_gap;
			}

			c_auto parent_content_rect = parent.clip_rect + float4(parent.padding_left, parent.padding_top, -parent.padding_right, -parent.padding_bottom);
			c_auto child_rect		   = float4(child.offset, child.offset + float2(child.width, child.height));
			child.clip_rect			   = math::intersect_2d(parent_content_rect, child_rect);

			AGE_ASSERT(parent.child_count > 0);
			--parent.child_count;
			g::element_pos_parent_idx_stack.emplace_back(current_idx);

			if (auto draw = child.render_data_count > 0)
			{
				auto& render_data_current	  = g::element_render_data_vec[child.render_data_idx];
				render_data_current.size	  = float2(child.width, child.height);
				render_data_current.pivot_pos = child.offset + render_data_current.pivot_uv * render_data_current.size;
				render_data_current.clip_rect = child.clip_rect;

				c_auto final_idx = g::element_z_order_count_vec[child.z_offset]++;

				// this is because we skip root element
				render_data_vec[final_idx] = render_data_current;
			}

			if (child.layout == e::widget_layout::horizontal)
			{
				child.offset.x += child.padding_left;
			}
			else if (child.layout == e::widget_layout::vertical)
			{
				child.offset.y += child.padding_top;
			}
			else
			{
				AGE_UNREACHABLE();
			}
		}

		g::id_stack.clear();
		g::element_layout_data_h_stack.clear();
		g::element_layout_data_v_stack.clear();
		g::element_layout_pos_data_vec.clear();
		g::element_render_data_vec.clear();
	}

	void
	deinit() noexcept
	{
		g::element_state_map.clear();

		for (auto&& [hash, h_asset_font] : g::font_vec)
		{
			asset::unload(h_asset_font);
		}

		g::font_vec.clear();
	}
}	 // namespace age::ui
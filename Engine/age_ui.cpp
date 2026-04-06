#include "age_pch.hpp"
#include "age.hpp"

namespace age::ui
{
	void
	init() noexcept
	{
		AGE_ASSERT(g::id_stack.is_empty());
		g::current_font_idx = 0;

		std::ranges::copy(g::theme_opacity_default, g::theme_opacity);
		std::ranges::copy(g::theme_color_default, g::theme_color);
		std::ranges::copy(g::theme_font_size_defaults, g::theme_font_size_base);

		g::theme_font_scale = g::theme_font_scale_default;
		font::set_scale(g::theme_font_scale_default);

		g::theme_slider_track_height = g::theme_slider_track_height_default;
		std::ranges::copy(g::theme_slider_thumb_size_default, g::theme_slider_thumb_size);
		std::ranges::copy(g::theme_slider_thumb_border_thickness_default, g::theme_slider_thumb_border_thickness);
	}

	void
	begin_frame(platform::window_handle h_window) noexcept
	{
		AGE_ASSERT(g::id_stack.is_empty());
		AGE_ASSERT(g::layout_size_data_stack.is_empty());
		AGE_ASSERT(g::element_layout_pos_data_vec.is_empty());
		AGE_ASSERT(g::element_render_data_vec.is_empty());
		AGE_ASSERT(g::text_data_vec.is_empty());
		AGE_ASSERT(g::word_data_vec.is_empty());
		AGE_ASSERT(g::char_data_vec.is_empty());
		AGE_ASSERT(g::char_pos_data_vec.is_empty());

		g::id_stack.emplace_back(id_scope{ .hash_id = g::fnv1a_offset_basis, .counter = 0 });

		g::element_z_order_count_vec.clear();

		g::window_width	 = static_cast<float>(platform::get_client_width(h_window));
		g::window_height = static_cast<float>(platform::get_client_height(h_window));

		std::ranges::fill(g::element_z_order_count_vec, 0u);

		detail::widget_begin<true>(
			set_draw(false)
			| set_layout(e::widget_layout::vertical)
			| set_size(size_mode::fixed(platform::get_client_width(h_window)), size_mode::fixed(platform::get_client_height(h_window)))
			| set_z_offset(0)
			| set_offset(float2(0.f, 0.f))
			| set_padding(0.f, 0.f, 0.f, 0.f)
			/*			widget_desc{
							.draw			  = false,
							.layout			  = e::widget_layout::vertical,
							.size_mode_width  = size_mode::fixed(platform::get_client_width(h_window)),
							.size_mode_height = size_mode::fixed(platform::get_client_height(h_window)),
							.z_offset		  = 0,
							.offset			  = { 0.f, 0.f },
							.padding_left	  = 0.f,
							.padding_right	  = 0.f,
							.padding_top	  = 0.f,
							.padding_bottom	  = 0.f }*/
		);

		g::element_layout_pos_data_vec[0].clip_rect = float4{ 0, 0, platform::get_client_width(h_window), platform::get_client_height(h_window) };

		// handle input
		g::p_input_ctx = std::addressof(*h_window->h_input);
	}

	void
	end_frame(age::vector<render_data>& render_data_vec, age::vector<util::range>& render_data_z_range_vec) noexcept
	{
		detail::widget_end();

		AGE_ASSERT(g::id_stack.size() == 2);
		AGE_ASSERT(g::layout_size_data_stack.size() == 1);

		{
			auto z_count_total = 0u;
			for (auto&& [i, z_count] : g::element_z_order_count_vec | std::views::enumerate)
			{
				if (z_count > 0)
				{
					render_data_z_range_vec.emplace_back(util::range{ .offset = z_count_total, .count = z_count });
					c_auto temp	   = z_count_total;
					z_count_total += z_count;
					z_count		   = temp;
				}
			}

			render_data_vec.resize(z_count_total);
		}

		g::element_pos_parent_idx_stack.clear();
		g::element_pos_parent_idx_stack.emplace_back(0);	// root

		auto current_hover_z_offset = 0u;
		g::hover_id					= get_invalid_id<t_hash>();

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
				auto& render_data_current = g::element_render_data_vec[child.render_data_idx];

				if (render_data_current.shape_kind == e::shape_kind::text)
				{
					render_data_current.clip_rect				 = child.clip_rect;
					render_data_current.shape_data.text.atlas_id = child.text.atlas_id;

					for (c_auto& char_pos_data : std::span(g::char_pos_data_vec.data() + child.text.idx, child.render_data_count))
					{
						render_data_current.size						 = char_pos_data.size;
						render_data_current.pivot_pos					 = child.offset + char_pos_data.offset + render_data_current.pivot_uv * char_pos_data.size;
						render_data_current.shape_data.text.atlas_uv_min = char_pos_data.atlas_uv_min;
						render_data_current.shape_data.text.atlas_uv_max = char_pos_data.atlas_uv_max;

						c_auto final_idx = g::element_z_order_count_vec[child.z_offset]++;

						render_data_vec[final_idx] = render_data_current;
					}
				}
				else
				{
					render_data_current.size	  = float2(child.width, child.height);
					render_data_current.pivot_pos = child.offset + render_data_current.pivot_uv * render_data_current.size;
					render_data_current.clip_rect = child.clip_rect;

					c_auto final_idx = g::element_z_order_count_vec[child.z_offset]++;

					render_data_vec[final_idx] = render_data_current;
				}
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

			if (child.save_state)
			{
				auto& state	 = g::widget_state_map[child.id];
				state.pos	 = child.offset;
				state.width	 = child.width;
				state.height = child.height;
			}

			// handle interaction
			if (child.interact and child.z_offset >= current_hover_z_offset and math::contains_2d(child.clip_rect, g::p_input_ctx->mouse_pos))
			{
				current_hover_z_offset = child.z_offset;
				g::hover_id			   = child.id;
			}
		}

		// finalize interaction
		{
			if (g::p_input_ctx->is_pressed(input::e::key_kind::mouse_left))
			{
				g::mouse_l_pressed_id = g::hover_id;
			}
			else if (g::p_input_ctx->is_released(input::e::key_kind::mouse_left))
			{
				// AGE_ASSERT(g::mouse_l_clicked_id == get_invalid_id<t_hash>());
				if (g::mouse_l_pressed_id == g::hover_id)
				{
					g::mouse_l_clicked_id = g::mouse_l_pressed_id;
				}

				g::mouse_l_pressed_id = age::get_invalid_id<t_hash>();
			}
			else
			{
				g::mouse_l_clicked_id = age::get_invalid_id<t_hash>();
			}

			if (g::p_input_ctx->is_released(input::e::key_kind::mouse_right))
			{
				g::mouse_r_clicked_id = g::hover_id;
			}
			else
			{
				g::mouse_r_clicked_id = age::get_invalid_id<t_hash>();
			}
		}

		g::id_stack.clear();
		g::layout_size_data_stack.clear();
		g::element_layout_pos_data_vec.clear();
		g::element_render_data_vec.clear();

		g::text_data_vec.clear();
		g::word_data_vec.clear();
		g::char_data_vec.clear();
		g::char_pos_data_vec.clear();
	}
}	 // namespace age::ui
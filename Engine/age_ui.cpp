#include "age_pch.hpp"
#include "age.hpp"

namespace age::ui
{
	void
	init() noexcept
	{
		AGE_ASSERT(g::id_stack.is_empty());
		g::current_font_idx = 0;

		g::cam_world_pos_prev = {};
		g::mouse_ray_dir_prev = {};
		g::cam_world_pos	  = {};
		g::mouse_ray_dir	  = {};
	}

	namespace detail
	{
		t_hash
		root_begin_impl(const root_desc& desc) noexcept
		{
			c_auto id = new_id();

			auto& root_vec		= g::root_data_vec_arr[to_idx(desc.space_mode)];
			auto& root_vec_size = g::root_data_vec_size_arr[to_idx(desc.space_mode)];
			root_vec.resize(std::max<uint64>(root_vec.capacity(), ++root_vec_size));

			auto& root = root_vec[root_vec_size - 1];

			g::root_data_idx_stack.emplace_back((to_idx<uint32>(desc.space_mode) << 24u) | (static_cast<uint32>(root_vec_size - 1) & 0x00ff'ffff));

			root.width		  = desc.width;
			root.height		  = desc.height;
			root.world_pos	  = desc.world_pos;
			root.quaternion	  = desc.quaternion;
			root.world_width  = desc.world_width;
			root.world_height = desc.world_height;
			if (desc.space_mode == e::space_mode_kind::screen)
			{
				root.mouse_uv		= g::p_input_ctx->mouse_pos;
				root.mouse_delta_uv = g::p_input_ctx->mouse_delta;
			}
			else if (desc.space_mode != e::space_mode_kind::screen)
			{
				c_auto normal = math::rotate(root.quaternion, float3{ 0, 0, -1.f });
				c_auto denorm = math::dot(g::mouse_ray_dir, normal);

				if (std::abs(denorm) < math::g::epsilon_1e6)
				{
					root.mouse_uv = float2{ -1.f, -1.f };
				}
				else
				{
					c_auto t = math::dot(root.world_pos - g::cam_world_pos, normal) / denorm;

					if (t <= 0.f)
					{
						root.mouse_uv = float2{ -1.f, -1.f };
					}
					else
					{
						c_auto hit_world = g::cam_world_pos + t * g::mouse_ray_dir;

						c_auto hit_world_delta = hit_world - root.world_pos;

						root.mouse_uv = float2{ math::dot(math::rotate(root.quaternion, float3{ 1, 0, 0 }), hit_world_delta), math::dot(math::rotate(root.quaternion, float3{ 0, -1, 0 }), hit_world_delta) }
									  * float2{ root.width / root.world_width, root.height / root.world_height };


						c_auto hit_world_prev = g::cam_world_pos_prev + t * g::mouse_ray_dir_prev;

						// todo: root transform may change between frames; possible drag delta inaccuracy on animated panels.
						c_auto hit_world_delta_prev = hit_world_prev - root.world_pos;

						root.mouse_delta_uv = float2{ math::dot(math::rotate(root.quaternion, float3{ 1, 0, 0 }), hit_world_delta_prev), math::dot(math::rotate(root.quaternion, float3{ 0, -1, 0 }), hit_world_delta_prev) }
											* float2{ root.width / root.world_width, root.height / root.world_height };

						root.mouse_delta_uv = root.mouse_uv - root.mouse_delta_uv;
					}
				}
			}


			g::layout_size_data_stack.emplace_back(layout_size_data{
				.layout				= desc.layout,
				.width_mode			= e::size_mode_kind::fixed,
				.height_mode		= e::size_mode_kind::fixed,
				.child_subtree_size = 0,
				.parent_idx			= g::layout_size_data_current_idx,
				.pos_data_idx		= 0u,
				.child_gap			= 0.f,
				.width_min			= desc.width,
				.width_max			= desc.width,
				.width_final		= 0.f,
				.height_min			= desc.height,
				.height_max			= desc.height,
				.height_final		= 0.f,
			});

			AGE_ASSERT(root.layout_pos_data_vec.is_empty());

			root.layout_pos_data_vec.emplace_back(layout_pos_data{
				.id				   = id,
				.render_data_idx   = g::render_data_vec.size<uint32>(),
				.render_data_count = 0,
				.layout			   = desc.layout,
				.align			   = {},
				.z_offset		   = 0,
				.offset			   = float2::zero(),
				.child_gap		   = 0,
				.padding_left	   = 0,
				.padding_right	   = 0,
				.padding_top	   = 0,
				.padding_bottom	   = 0,
				.clip_rect		   = float4{ 0, 0, desc.width, desc.height },
				.interact		   = false,
				.save_state		   = false,
				.direct_draw	   = false,
				.text			   = {},
			});

			g::layout_size_data_current_idx = g::layout_size_data_stack.size<uint32>() - 1;

			return id;
		}
	}	 // namespace detail

	root_ctx
	root_begin(const root_desc& desc) noexcept
	{
		return root_ctx{ detail::root_begin_impl(desc) };
	}

	void
	begin_frame(platform::window_handle h_window, const float3& cam_world_pos, const float4x4& cam_view_proj_inv) noexcept
	{
		AGE_ASSERT(g::id_stack.is_empty());
		AGE_ASSERT(g::layout_size_data_stack.is_empty());
		AGE_ASSERT(g::render_data_vec.is_empty());
		AGE_ASSERT(g::root_data_idx_stack.is_empty());
		AGE_ASSERT(g::text_data_vec.is_empty());
		AGE_ASSERT(g::word_data_vec.is_empty());
		AGE_ASSERT(g::char_data_vec.is_empty());
		AGE_ASSERT(g::char_pos_data_vec.is_empty());

		g::id_stack.emplace_back(id_scope{ .hash_id = g::fnv1a_offset_basis, .counter = 0 });

		g::window_width	 = static_cast<float>(platform::get_client_width(h_window));
		g::window_height = static_cast<float>(platform::get_client_height(h_window));

		// handle input
		g::p_input_ctx = std::addressof(*h_window->h_input);

		g::utf8_buf_len = util::encode_utf8(g::utf8_buf, g::p_input_ctx->char_buf, g::p_input_ctx->char_count);

		g::cam_world_pos_prev = g::cam_world_pos;
		g::mouse_ray_dir_prev = g::mouse_ray_dir;

		g::cam_world_pos = cam_world_pos;

		c_auto target_world = math::ndc_to_world(cam_view_proj_inv, float3{ math::screen_to_ndc(float2{ g::window_width, g::window_height }, g::p_input_ctx->mouse_pos), 0.f });

		g::mouse_ray_dir = math::normalize(target_world - cam_world_pos);

		detail::root_begin_impl(root_desc{
			.width	= g::window_width,
			.height = g::window_height });
	}

	void
	end_frame(std::tuple<age::vector<ui::render_data>&,
						 age::vector<util::range>&,
						 age::vector<util::range>&,
						 age::array<age::vector<ui::root_graphics_data>, ui::e::space_mode_kind_size>&>
				  tpl) noexcept
	{
		detail::widget_end();
		g::id_stack.pop_back();

		AGE_ASSERT(g::id_stack.size() == 1);
		AGE_ASSERT(g::root_data_idx_stack.is_empty());
		AGE_ASSERT(g::layout_size_data_stack.is_empty());

		auto&& [gpu_render_data_vec,
				gpu_render_data_z_range_vec,
				gpu_render_data_z_range_of_range_vec,
				gpu_root_data_vec_arr] = tpl;

		auto render_data_offset = 0u;

		for (auto&& [space_mode, root_data_vec] : g::root_data_vec_arr | std::views::enumerate)
		{
			auto& gpu_root_data_vec = gpu_root_data_vec_arr[space_mode];

			gpu_root_data_vec.reserve(root_data_vec.size());
			gpu_render_data_z_range_of_range_vec.reserve(gpu_render_data_z_range_of_range_vec.size() + root_data_vec.size());

			for (auto&& [i, root_data] : root_data_vec | std::views::enumerate)
			{
				gpu_root_data_vec.emplace_back(root_graphics_data{

					.world_pos	= root_data.world_pos,
					.quaternion = root_data.quaternion,

					.width	= root_data.width,
					.height = root_data.height,

					.world_width  = root_data.world_width,
					.world_height = root_data.world_height,
				});

				auto& z_range_of_range	= gpu_render_data_z_range_of_range_vec.emplace_back();
				z_range_of_range.offset = gpu_render_data_z_range_vec.size<uint32>();

				for (auto& z_order_count : root_data.z_order_count_vec)
				{
					if (z_order_count > 0)
					{
						gpu_render_data_z_range_vec.emplace_back(util::range{ .offset = render_data_offset, .count = z_order_count });
						c_auto temp			= render_data_offset;
						render_data_offset += z_order_count;
						z_order_count		= temp;
					}
				}

				z_range_of_range.count = gpu_render_data_z_range_vec.size<uint32>() - z_range_of_range.offset;
			}
		}

		gpu_render_data_vec.resize(render_data_offset);

		g::hover_id = get_invalid_id<t_hash>();
		g::hover_id_stack.clear();

		for (auto&& [space_mode, root_vec] : g::root_data_vec_arr | std::views::enumerate)
		{
			for (auto&& [root_idx, root] : root_vec | std::views::enumerate)
			{
				g::element_pos_parent_idx_stack.clear();
				g::element_pos_parent_idx_stack.emplace_back(0);	// root

				auto current_hover_z_offset = 0u;

				for (auto current_idx = 1u; current_idx < root.layout_pos_data_vec.size<uint32>(); ++current_idx)
				// for (auto current_idx : std::views::iota(1u, root.layout_pos_data_vec.size<uint32>()))
				{
					while (root.layout_pos_data_vec[g::element_pos_parent_idx_stack.back()].child_count == 0)
					{
						g::element_pos_parent_idx_stack.pop_back();
					}

					c_auto parent_idx = g::element_pos_parent_idx_stack.back();
					auto&  parent	  = root.layout_pos_data_vec[parent_idx];
					auto&  child	  = root.layout_pos_data_vec[current_idx];

					auto parnet_offset_backup = parent.offset;

					if (parent.layout == e::widget_layout::horizontal or parent.layout == e::widget_layout::horizontal_inv)
					{
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

						if (parent.layout == e::widget_layout::horizontal)
						{
							child.offset.x	+= parent.offset.x;
							parent.offset.x += child.width + parent.child_gap;
						}
						else
						{
							parent.offset.x -= child.width;
							child.offset.x	+= parent.offset.x;
							parent.offset.x -= parent.child_gap;
						}
					}
					else if (parent.layout == e::widget_layout::vertical or parent.layout == e::widget_layout::vertical_inv)
					{
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

						if (parent.layout == e::widget_layout::vertical)
						{
							child.offset.y	+= parent.offset.y;
							parent.offset.y += child.height + parent.child_gap;
						}
						else
						{
							parent.offset.y -= child.height;
							child.offset.y	+= parent.offset.y;
							parent.offset.y -= parent.child_gap;
						}
					}

					if (child.direct_draw)
					{
						parent.offset = parnet_offset_backup;
					}

					c_auto parent_content_rect = parent.clip_rect + float4(parent.padding_left, parent.padding_top, -parent.padding_right, -parent.padding_bottom);
					c_auto child_rect		   = float4(child.offset, child.offset + float2(child.width, child.height));
					child.clip_rect			   = math::intersect_2d(parent_content_rect, child_rect);

					AGE_ASSERT(parent.child_count > 0);
					--parent.child_count;
					g::element_pos_parent_idx_stack.emplace_back(current_idx);

					if (auto draw = child.render_data_count > 0)
					{
						auto& render_data_current = g::render_data_vec[child.render_data_idx];

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

								c_auto final_idx = root.z_order_count_vec[child.z_offset]++;

								gpu_render_data_vec[final_idx] = render_data_current;
							}
						}
						else
						{
							render_data_current.size	  = float2(child.width, child.height);
							render_data_current.pivot_pos = child.offset + render_data_current.pivot_uv * render_data_current.size;
							render_data_current.clip_rect = child.clip_rect;

							c_auto final_idx = root.z_order_count_vec[child.z_offset]++;

							gpu_render_data_vec[final_idx] = render_data_current;
						}

						// debug
						{
							// std::println("pos : {} size : {}, clip_rect : {}", child.offset, float2{ child.width, child.height }, child.clip_rect);
						}
					}

					if (child.save_state)
					{
						auto& state	 = g::widget_state_map[child.id];
						state.pos	 = child.offset;
						state.width	 = child.width;
						state.height = child.height;

						state.clip_width  = child.clip_rect.z - child.clip_rect.x;
						state.clip_height = child.clip_rect.w - child.clip_rect.y;
					}

					// handle interaction
					if (child.interact and child.z_offset >= current_hover_z_offset and math::contains_2d(child.clip_rect, root.mouse_uv))
					{
						current_hover_z_offset = child.z_offset;
						// todo replace hover_id -> g::hover_id_stack.back();
						g::hover_id = child.id;
						g::hover_id_stack.emplace_back(child.id);
					}


					if (child.layout == e::widget_layout::horizontal)
					{
						child.offset.x += child.padding_left;
					}
					else if (child.layout == e::widget_layout::horizontal_inv)
					{
						child.offset.x += child.width - child.padding_right;
					}
					else if (child.layout == e::widget_layout::vertical)
					{
						child.offset.y += child.padding_top;
					}
					else if (child.layout == e::widget_layout::vertical_inv)
					{
						child.offset.y += child.height - child.padding_bottom;
					}
					else
					{
						AGE_UNREACHABLE();
					}
				}
			}
		}


		// finalize interaction
		{
			if (g::p_input_ctx->is_pressed(input::e::key_kind::mouse_left))
			{
				g::mouse_l_pressed_id = g::hover_id;
				g::focus_id			  = g::hover_id;
				g::focus_id_stack	  = g::hover_id_stack;
			}
			else if (g::p_input_ctx->is_released(input::e::key_kind::mouse_left))
			{
				if (g::mouse_l_pressed_id == g::hover_id)
				{
					g::mouse_l_clicked_id = g::mouse_l_pressed_id;

					// click count
					c_auto elapsed = runtime::i_time.get_now_s() - g::mouse_l_clicked_time;
					if (elapsed < 0.5f)
					{
						g::mouse_l_clicked_count = std::min<uint8>(g::mouse_l_clicked_count + 1, uint8{ 3 });
					}
					else
					{
						g::mouse_l_clicked_count = 1;
					}

					g::mouse_l_clicked_time = runtime::i_time.get_now_s();
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

		clear();
	}

	void
	clear() noexcept
	{
		g::id_stack.clear();
		g::layout_size_data_stack.clear();
		g::render_data_vec.clear();
		g::root_data_idx_stack.clear();

		std::ranges::fill(g::root_data_vec_size_arr, 0u);

		for (auto& root_vec : g::root_data_vec_arr)
		{
			for (auto& root : root_vec)
			{
				root.layout_pos_data_vec.clear();
				std::ranges::fill(root.z_order_count_vec, 0u);
			}
		}

		g::text_data_vec.clear();
		g::word_data_vec.clear();
		g::char_data_vec.clear();
		g::char_pos_data_vec.clear();
	}

	bool
	is_any_hovered() noexcept
	{
		return ui::g::hover_id_stack.is_empty() is_false;
	}

	bool
	is_any_focused() noexcept
	{
		return ui::g::focus_id_stack.is_empty() is_false;
	}
}	 // namespace age::ui
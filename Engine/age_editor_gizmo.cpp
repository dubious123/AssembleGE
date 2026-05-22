#include "age_pch.hpp"
#include "age.hpp"

namespace age::editor::gizmo
{
	float3
	translation(const float cam_fov_y, const float3& cam_pos, const float3& cam_forward, const float3& world_pos, const float4& quat, const float screen_size) noexcept
	{
		using namespace ui;
		using namespace ui::widget;
		using enum input::e::key_kind;

		c_auto view_z			= std::max(math::dot(world_pos - cam_pos, cam_forward), 0.5f);
		c_auto world_size_scale = (screen_size / ui::g::window_height) * 2.0f * std::tanf(cam_fov_y * 0.5f);
		c_auto world_size		= world_size_scale * view_z;

		auto res_translation = float3::zero();

		c_auto drag_color		 = theme::color_amber();
		c_auto disabled_color	 = theme::palette_cool_gray();
		c_auto disable_threshold = 0.15f;

		// xy, normal = (0,0,-1)
		{
			auto h_root_front = root_begin(root_desc{
				.space_mode	  = ui::e::space_mode_kind::world_always_on_top,
				.layout		  = ui::e::widget_layout::vertical,
				.width		  = screen_size,
				.height		  = screen_size,
				.world_pos	  = world_pos + math::rotate(quat, float3(0, world_size, 0)),
				.quaternion	  = quat,
				.world_width  = world_size,
				.world_height = world_size,
			});

			auto h_plane_front = widget::vertical_inv(set_width_grow() | set_height_grow() | set_child_gap(0));

			// +x translation
			if (auto h_translation_x = widget::horizontal(set_child_gap(0) | set_width_fixed(screen_size) | set_height_fit()))
			{
				auto& state = h_translation_x.get_state();

				c_auto is_drag_prev	   = static_cast<bool>(state.storage[0]);
				c_auto is_hovered_prev = static_cast<bool>(state.storage[1]);

				c_auto is_disabled = is_drag_prev is_false and (std::abs(math::dot(cam_forward, ui::detail::get_current_root().world_basis_u)) > (1.f - disable_threshold));

				c_auto color = is_drag_prev
								 ? drag_color
							 : is_disabled
								 ? disabled_color
							 : is_hovered_prev
								 ? theme::palette_red_bright()
								 : theme::color_red();

				if (auto h_center = widget::begin(set_width_fixed(0)
												  | set_height_fixed(screen_size * 0.05f)
												  | set_draw()
												  | set_interact_mesh()
												  | set_align_end()
												  | set_border_thickness(0)
												  | set_pivot_uv(0.f, 1.f)
												  //| set_border_brush_data(theme::color_black())
												  | set_fit_mode_cover()
												  | set_shape_mesh(g::h_mesh_cube)
												  | set_body_brush_data(theme::color_white())))
				{
				}

				auto is_drag  = false;
				auto is_hover = false;
				if (auto h_line = widget::begin(set_width_grow()
												| set_height_fixed(screen_size * 0.025f)
												| set_draw()
												| set_interact_mesh(is_disabled is_false)
												| set_align_end()
												| set_border_thickness(0)
												| set_pivot_uv(0.5f, 1.f)
												| set_fit_mode_fill()
												| set_shape_mesh(g::h_mesh_cube)
												| set_body_brush_data(color)))
				{
					is_drag	 |= h_line.pressed<mouse_left>();
					is_hover |= h_line.hovered();
				}

				if (auto h_cone = widget::begin(set_width_fixed(screen_size * 0.1f)
												| set_height_fixed(screen_size * 0.1f)
												| set_draw()
												| set_interact_mesh(is_disabled is_false)
												| set_align_center()
												| set_border_thickness(0)
												| set_pivot_uv(0.f, 1.f)
												| set_body_brush_data(color)
												| set_rotation(age::cvt_to_radian(90.f))
												| set_shape_mesh(g::h_mesh_cone)))
				{
					is_drag	 |= h_cone.pressed<mouse_left>();
					is_hover |= h_cone.hovered();
				}

				if (is_drag)
				{
					res_translation.x += ui::detail::get_current_root().mouse_delta_uv.x * world_size / screen_size;
				}

				auto& state_	  = h_translation_x.get_state();
				state_.storage[0] = is_drag ? 1 : 0;
				state_.storage[1] = is_hover ? 1 : 0;
			}

			auto h = widget::horizontal(set_width_grow() | set_height_grow());

			if (auto _ = widget::vertical(set_width_fit() | set_height_grow()))
			{
				// +y translation
				if (auto h_translation_y = widget::vertical_inv(set_child_gap(0) | set_height_fixed(screen_size) | set_width_fit()))
				{
					auto& state = h_translation_y.get_state();

					c_auto is_drag_prev	   = static_cast<bool>(state.storage[0]);
					c_auto is_hovered_prev = static_cast<bool>(state.storage[1]);

					c_auto is_disabled = is_drag_prev is_false and (std::abs(math::dot(cam_forward, ui::detail::get_current_root().world_basis_v)) > (1.f - disable_threshold));

					c_auto color = is_drag_prev
									 ? drag_color
								 : is_disabled
									 ? disabled_color
								 : is_hovered_prev
									 ? theme::palette_green_bright()
									 : theme::color_green();

					auto is_drag  = false;
					auto is_hover = false;
					if (auto h_line = widget::begin(set_width_fixed(screen_size * 0.025f)
													| set_height_grow()
													| set_draw()
													| set_interact_mesh(is_disabled is_false)
													| set_align_begin()
													| set_padding(0)
													| set_pivot_uv(0.f, 0.5f)
													| set_body_brush_data(color)
													| set_fit_mode_fill()
													| set_shape_mesh(g::h_mesh_cube)))
					{
						is_drag	 |= h_line.pressed<mouse_left>();
						is_hover |= h_line.hovered();
					}

					if (auto h_cone = widget::begin(set_width_fixed(screen_size * 0.1f)
													| set_height_fixed(screen_size * 0.1f)
													| set_draw()
													| set_interact_mesh(is_disabled is_false)
													| set_align_center()
													| set_padding(0)
													| set_pivot_uv(0.f, 1.f)
													| set_body_brush_data(color)
													| set_shape_mesh(g::h_mesh_cone)))
					{
						is_drag	 |= h_cone.pressed<mouse_left>();
						is_hover |= h_cone.hovered();
					}

					if (is_drag)
					{
						res_translation.y -= ui::detail::get_current_root().mouse_delta_uv.y * world_size / screen_size;
					}

					auto& state_	  = h_translation_y.get_state();
					state_.storage[0] = is_drag ? 1 : 0;
					state_.storage[1] = is_hover ? 1 : 0;
				}
			}

			auto _ = widget::horizontal(set_offset(0 /*- screen_size * 0.08f*/, 0 /*+ screen_size * 0.08f*/) | set_fit() | set_align_end() | set_clip(false));

			if (auto h_translation_xy = widget::begin(set_vertical() | set_fit()))
			{
				auto& state = h_translation_xy.get_state();

				c_auto is_drag_prev	   = static_cast<bool>(state.storage[0]);
				c_auto is_hovered_prev = static_cast<bool>(state.storage[1]);

				c_auto is_disabled = is_drag_prev is_false and (std::abs(math::dot(cam_forward, ui::detail::get_current_root().world_normal)) < disable_threshold);
				c_auto color	   = is_drag_prev
									   ? drag_color
								   : is_disabled
									   ? disabled_color
								   : is_hovered_prev
									   ? theme::palette_blue_bright()
									   : theme::palette_blue();
				c_auto opacity	   = is_drag_prev or is_hovered_prev or is_disabled ? 1.f : theme::opacity_heavy();


				auto h_plane_y = widget::begin(set_align(ui::e::widget_align::center)
											   | set_draw()
											   | set_interact_rect(is_disabled is_false)
											   | set_fixed(screen_size * 0.2f)
											   | set_z_offset(1)
											   | set_border_thickness(theme::thickness_thin())
											   | set_border_brush_data(color, 1.f)
											   | set_body_brush_data(color, opacity));

				c_auto is_drag	= h_plane_y.pressed<mouse_left>();
				c_auto is_hover = h_plane_y.hovered();

				if (is_drag)
				{
					c_auto delta_uv	   = ui::detail::get_current_root().mouse_delta_uv;
					res_translation.x += delta_uv.x * world_size / screen_size;
					res_translation.z -= delta_uv.y * world_size / screen_size;
				}


				auto& state_	  = h_translation_xy.get_state();
				state_.storage[0] = is_drag ? 1 : 0;
				state_.storage[1] = is_hover ? 1 : 0;
			}
		}

		// yz, normal = (1,0,0)
		{
			auto h_root_left = root_begin(root_desc{
				.space_mode	  = ui::e::space_mode_kind::world_always_on_top,
				.layout		  = ui::e::widget_layout::vertical,
				.width		  = screen_size,
				.height		  = screen_size,
				.world_pos	  = world_pos + math::rotate(quat, float3(0, world_size, 0)),
				.quaternion	  = math::quat_mul(quat, math::euler_deg_to_quat(float3{ 0, -90, 0 })),
				.world_width  = world_size,
				.world_height = world_size,
			});

			auto h_plane_left = widget::vertical_inv(set_width_grow() | set_height_grow() | set_child_gap(0));

			// +z translation
			if (auto h_translation_z = widget::horizontal(set_child_gap(0) | set_width_fixed(screen_size) | set_height_fit()))
			{
				auto& state = h_translation_z.get_state();

				c_auto is_drag_prev	   = static_cast<bool>(state.storage[0]);
				c_auto is_hovered_prev = static_cast<bool>(state.storage[1]);

				c_auto is_disabled = is_drag_prev is_false and (std::abs(math::dot(cam_forward, ui::detail::get_current_root().world_basis_u)) > (1.f - disable_threshold));

				c_auto color = is_drag_prev
								 ? drag_color
							 : is_disabled
								 ? disabled_color
							 : is_hovered_prev
								 ? theme::palette_blue_bright()
								 : theme::palette_blue();

				auto is_drag  = false;
				auto is_hover = false;
				if (auto h_line = widget::begin(set_width_grow()
												| set_height_fixed(screen_size * 0.025f)
												| set_draw()
												| set_interact_mesh(is_disabled is_false)
												| set_align_end()
												| set_border_thickness(0)
												| set_pivot_uv(0.5f, 1.f)
												| set_fit_mode_fill()
												| set_shape_mesh(g::h_mesh_cube)
												| set_body_brush_data(color)))
				{
					is_drag	 |= h_line.pressed<mouse_left>();
					is_hover |= h_line.hovered();
				}

				if (auto h_cone = widget::begin(set_width_fixed(screen_size * 0.1f)
												| set_height_fixed(screen_size * 0.1f)
												| set_draw()
												| set_interact_mesh(is_disabled is_false)
												| set_align_center()
												| set_border_thickness(0)
												| set_pivot_uv(0.f, 1.f)
												| set_body_brush_data(color)
												| set_rotation(age::cvt_to_radian(90.f))
												| set_shape_mesh(g::h_mesh_cone)))
				{
					is_drag	 |= h_cone.pressed<mouse_left>();
					is_hover |= h_cone.hovered();
				}

				if (is_drag)
				{
					res_translation.z += ui::detail::get_current_root().mouse_delta_uv.x * world_size / screen_size;
				}

				auto& state_	  = h_translation_z.get_state();
				state_.storage[0] = is_drag ? 1 : 0;
				state_.storage[1] = is_hover ? 1 : 0;
			}

			auto _ = widget::horizontal(set_offset(screen_size * 0.1f /*- screen_size * 0.08f*/, 0 /*+ screen_size * 0.08f*/) | set_fit() | set_align_begin() | set_clip(false));
			if (auto h_translation_yz = widget::begin(style::vertical() | set_fit()))
			{
				auto& state = h_translation_yz.get_state();

				c_auto is_drag_prev	   = static_cast<bool>(state.storage[0]);
				c_auto is_hovered_prev = static_cast<bool>(state.storage[1]);

				c_auto is_disabled = is_drag_prev is_false and (std::abs(math::dot(cam_forward, ui::detail::get_current_root().world_normal)) < disable_threshold);
				c_auto color	   = is_drag_prev
									   ? drag_color
								   : is_disabled
									   ? disabled_color
								   : is_hovered_prev
									   ? theme::palette_red_bright()
									   : theme::color_red();
				c_auto opacity	   = is_drag_prev or is_hovered_prev or is_disabled ? 1.f : theme::opacity_heavy();


				auto h_plane_x = widget::begin(set_align(ui::e::widget_align::center)
											   | set_draw()
											   | set_interact_rect(is_disabled is_false)
											   | set_fixed(screen_size * 0.2f)
											   | set_z_offset(1)
											   | set_border_thickness(theme::thickness_thin())
											   | set_border_brush_data(color, 1.f)
											   | set_body_brush_data(color, opacity));

				c_auto is_drag	= h_plane_x.pressed<mouse_left>();
				c_auto is_hover = h_plane_x.hovered();

				if (is_drag)
				{
					c_auto delta_uv	   = ui::detail::get_current_root().mouse_delta_uv;
					res_translation.z += delta_uv.x * world_size / screen_size;
					res_translation.y -= delta_uv.y * world_size / screen_size;
				}


				auto& state_	  = h_translation_yz.get_state();
				state_.storage[0] = is_drag ? 1 : 0;
				state_.storage[1] = is_hover ? 1 : 0;
			}
		}

		// xz, normal = (0,1,0)
		{
			auto h_root_down = root_begin(root_desc{
				.space_mode	  = ui::e::space_mode_kind::world_always_on_top,
				.layout		  = ui::e::widget_layout::vertical,
				.width		  = screen_size,
				.height		  = screen_size,
				.world_pos	  = world_pos + math::rotate(quat, float3(0, 0, world_size)),
				.quaternion	  = math::quat_mul(quat, math::euler_deg_to_quat(float3{ 90, 0, 0 })),
				.world_width  = world_size,
				.world_height = world_size,
			});

			auto _0 = widget::vertical_inv(set_grow());
			auto _1 = widget::horizontal(set_offset(screen_size * 0.1f /*- screen_size * 0.08f*/, -screen_size * 0.1f /*+ screen_size * 0.08f*/) | set_fit() | set_align_begin());
			if (auto h_translation_xz = widget::begin(style::vertical() | set_fit()))
			{
				auto& state = h_translation_xz.get_state();

				c_auto is_drag_prev	   = static_cast<bool>(state.storage[0]);
				c_auto is_hovered_prev = static_cast<bool>(state.storage[1]);

				c_auto is_disabled = is_drag_prev is_false and (std::abs(math::dot(cam_forward, ui::detail::get_current_root().world_normal)) < disable_threshold);
				c_auto color	   = is_drag_prev
									   ? drag_color
								   : is_disabled
									   ? disabled_color
								   : is_hovered_prev
									   ? theme::palette_green_bright()
									   : theme::color_green();
				c_auto opacity	   = is_drag_prev or is_hovered_prev or is_disabled ? 1.f : theme::opacity_heavy();


				auto h_plane_y = widget::begin(set_align(ui::e::widget_align::center)
											   | set_draw()
											   | set_interact_rect(is_disabled is_false)
											   | set_fixed(screen_size * 0.2f)
											   | set_z_offset(1)
											   | set_border_thickness(theme::thickness_thin())
											   | set_border_brush_data(color, 1.f)
											   | set_body_brush_data(color, opacity));

				c_auto is_drag	= h_plane_y.pressed<mouse_left>();
				c_auto is_hover = h_plane_y.hovered();

				if (is_drag)
				{
					c_auto delta_uv	   = ui::detail::get_current_root().mouse_delta_uv;
					res_translation.x += delta_uv.x * world_size / screen_size;
					res_translation.z -= delta_uv.y * world_size / screen_size;
				}


				auto& state_	  = h_translation_xz.get_state();
				state_.storage[0] = is_drag ? 1 : 0;
				state_.storage[1] = is_hover ? 1 : 0;
			}
		}

		return math::rotate(quat, res_translation);
	}

	std::tuple<float3, bool, bool>
	scale(const float cam_fov_y, const float3& cam_pos, const float3& cam_forward, const float3& world_pos, const float4& quat, const float screen_size) noexcept
	{
		using namespace ui;
		using namespace ui::widget;
		using enum input::e::key_kind;

		static auto res_scale_ratio		= float3::one();
		static auto is_any_pressed_prev = false;
		static auto anchor_pos_prev		= float3::zero();

		auto is_any_pressed = false;

		c_auto drag_color		 = theme::color_amber();
		c_auto disabled_color	 = theme::palette_cool_gray();
		c_auto disable_threshold = 0.15f;

		c_auto anchor_pos = is_any_pressed_prev ? anchor_pos_prev : world_pos;

		c_auto view_z			= std::max(math::dot(anchor_pos - cam_pos, cam_forward), 0.5f);
		c_auto world_size_scale = (screen_size / ui::g::window_height) * 2.0f * std::tanf(cam_fov_y * 0.5f);
		c_auto world_size		= world_size_scale * view_z;

		auto center_dragging = false;
		// xy, normal = (0,0,-1)
		{
			auto h_root_front = root_begin(root_desc{
				.space_mode	  = ui::e::space_mode_kind::world_always_on_top,
				.layout		  = ui::e::widget_layout::vertical,
				.width		  = screen_size,
				.height		  = screen_size,
				.world_pos	  = anchor_pos + math::rotate(quat, float3(0, world_size, 0)),
				.quaternion	  = quat,
				.world_width  = world_size,
				.world_height = world_size,
			});

			auto h_plane_front = widget::vertical_inv(set_width_grow() | set_height_grow() | set_child_gap(0));

			// +x scale
			if (auto h_scale_x = widget::horizontal(set_child_gap(0) | set_width_fixed(screen_size) | set_height_fit()))
			{
				if (auto h_center_state = widget::horizontal(set_fixed(0) | set_align_end()))
				{
					c_auto& state			= h_center_state.get_state();
					c_auto	is_drag_prev	= static_cast<bool>(state.storage[0]);
					c_auto	is_hovered_prev = static_cast<bool>(state.storage[1]);

					c_auto color = is_drag_prev
									 ? drag_color
								 : is_hovered_prev
									 ? theme::color_white() * 2
									 : theme::color_white_subtle() * 2;

					if (auto h_center = widget::begin(set_width_fixed(0)
													  | set_height_fixed(screen_size * 0.1f)
													  | set_clip(false)
													  | set_draw()
													  | set_interact_mesh()
													  | set_align_end()
													  | set_border_thickness(0)
													  | set_pivot_uv(0.f, 1.f)
													  | set_fit_mode_cover()
													  | set_shape_mesh(g::h_mesh_cube)
													  | set_body_brush_data(color)))
					{
						c_auto is_drag	= h_center.pressed<mouse_left>();
						c_auto is_hover = h_center.hovered();

						c_auto is_drag_start = is_drag_prev is_false and is_drag;

						auto& state_safe = h_center_state.get_state();

						if (is_drag)
						{
							c_auto normal = cam_forward;
							c_auto denorm = math::dot(ui::g::mouse_ray_dir, normal);

							if (std::abs(denorm) > math::g::epsilon_1e6)
							{
								c_auto t = math::dot(anchor_pos - ui::g::cam_world_pos, normal) / denorm;
								if (t > 0.f)
								{
									c_auto hit_world	   = ui::g::cam_world_pos + t * ui::g::mouse_ray_dir;
									c_auto hit_world_delta = hit_world - anchor_pos;

									c_auto cam_right = math::normalize(math::cross(math::g::up, cam_forward));
									c_auto cam_up	 = math::cross(cam_forward, cam_right);

									c_auto mouse_world_delta  = float2{ math::dot(cam_up, hit_world_delta), math::dot(cam_right, hit_world_delta) };
									c_auto mouse_screen_delta = mouse_world_delta * screen_size / world_size;

									res_scale_ratio = (screen_size * 0.5f * 2 + mouse_screen_delta.x + mouse_screen_delta.y) / (screen_size * 0.5f * 2);
									AGE_LOG(res_scale_ratio, mouse_screen_delta.x + mouse_screen_delta.y, screen_size);
								}
							}
						}

						state_safe.storage[0] = is_drag ? 1 : 0;
						state_safe.storage[1] = is_hover ? 1 : 0;

						is_any_pressed |= is_drag;

						center_dragging = is_drag;
					}
				}

				auto& state = h_scale_x.get_state();

				c_auto is_drag_prev	   = static_cast<bool>(state.storage[0]);
				c_auto is_hovered_prev = static_cast<bool>(state.storage[1]);

				c_auto is_disabled = is_drag_prev is_false and (std::abs(math::dot(cam_forward, ui::detail::get_current_root().world_basis_u)) > (1.f - disable_threshold));

				c_auto color = is_drag_prev or center_dragging
								 ? drag_color
							 : is_disabled
								 ? disabled_color
							 : is_hovered_prev
								 ? theme::palette_red_bright()
								 : theme::color_red();

				auto is_drag  = false;
				auto is_hover = false;
				if (auto h_line = widget::begin(set_width_fixed(screen_size * 0.9f * res_scale_ratio.x)
												| set_height_fixed(screen_size * 0.025f)
												| set_draw()
												| set_interact_mesh(is_disabled is_false)
												| set_clip(false)
												| set_align_end()
												| set_border_thickness(0)
												| set_pivot_uv(0.5f, 1.f)
												| set_fit_mode_fill()
												| set_shape_mesh(g::h_mesh_cube)
												| set_body_brush_data(color)))
				{
					is_drag	 |= h_line.pressed<mouse_left>();
					is_hover |= h_line.hovered();
				}

				if (auto h_cone = widget::begin(set_width_fixed(screen_size * 0.1f)
												| set_height_fixed(screen_size * 0.1f)
												| set_draw()
												| set_interact_mesh(is_disabled is_false)
												| set_clip(false)
												| set_align_center()
												| set_border_thickness(0)
												| set_pivot_uv(0.f, 1.f)
												| set_body_brush_data(color)
												| set_rotation(age::cvt_to_radian(90.f))
												| set_shape_mesh(g::h_mesh_cube)))
				{
					is_drag	 |= h_cone.pressed<mouse_left>();
					is_hover |= h_cone.hovered();
				}

				auto& state_ = h_scale_x.get_state();

				if (is_drag)
				{
					state_.drag_x	  += ui::detail::get_current_root().mouse_delta_uv.x;
					res_scale_ratio.x  = (screen_size + state_.drag_x) / screen_size;
				}
				else
				{
					state.drag_x = 0.f;
				}

				state_.storage[0] = is_drag ? 1 : 0;
				state_.storage[1] = is_hover ? 1 : 0;

				is_any_pressed |= is_drag;
			}

			auto h = widget::horizontal(set_width_grow() | set_height_grow());

			if (auto _ = widget::vertical(set_width_fit() | set_height_grow()))
			{
				// +y scale
				if (auto h_scale_y = widget::vertical_inv(set_child_gap(0) | set_height_fixed(screen_size) | set_width_fit()))
				{
					auto& state = h_scale_y.get_state();

					c_auto is_drag_prev	   = static_cast<bool>(state.storage[0]);
					c_auto is_hovered_prev = static_cast<bool>(state.storage[1]);

					c_auto is_disabled = is_drag_prev is_false and (std::abs(math::dot(cam_forward, ui::detail::get_current_root().world_basis_v)) > (1.f - disable_threshold));

					c_auto color = is_drag_prev or center_dragging
									 ? drag_color
								 : is_disabled
									 ? disabled_color
								 : is_hovered_prev
									 ? theme::palette_green_bright()
									 : theme::color_green();

					auto is_drag  = false;
					auto is_hover = false;
					if (auto h_line = widget::begin(set_width_fixed(screen_size * 0.025f)
													| set_height_fixed(screen_size * 0.9f * res_scale_ratio.y)
													| set_clip(false)
													| set_draw()
													| set_interact_mesh(is_disabled is_false)
													| set_align_begin()
													| set_padding(0)
													| set_pivot_uv(0.f, 0.5f)
													| set_body_brush_data(color)
													| set_fit_mode_fill()
													| set_shape_mesh(g::h_mesh_cube)))
					{
						is_drag	 |= h_line.pressed<mouse_left>();
						is_hover |= h_line.hovered();
					}

					if (auto h_cone = widget::begin(set_width_fixed(screen_size * 0.1f)
													| set_height_fixed(screen_size * 0.1f)
													| set_clip(false)
													| set_draw()
													| set_interact_mesh(is_disabled is_false)
													| set_align_center()
													| set_padding(0)
													| set_pivot_uv(0.f, 1.f)
													| set_body_brush_data(color)
													| set_shape_mesh(g::h_mesh_cube)))
					{
						is_drag	 |= h_cone.pressed<mouse_left>();
						is_hover |= h_cone.hovered();
					}

					auto& state_ = h_scale_y.get_state();

					if (is_drag)
					{
						state_.drag_y	  -= ui::detail::get_current_root().mouse_delta_uv.y;
						res_scale_ratio.y  = (screen_size + state_.drag_y) / screen_size;
					}
					else
					{
						state.drag_y = 0.f;
					}

					state_.storage[0] = is_drag ? 1 : 0;
					state_.storage[1] = is_hover ? 1 : 0;

					is_any_pressed |= is_drag;
				}
			}
		}

		// yz, normal = (1,0,0)
		{
			auto h_root_left = root_begin(root_desc{
				.space_mode	  = ui::e::space_mode_kind::world_always_on_top,
				.layout		  = ui::e::widget_layout::vertical,
				.width		  = screen_size,
				.height		  = screen_size,
				.world_pos	  = anchor_pos + math::rotate(quat, float3(0, world_size, 0)),
				.quaternion	  = math::quat_mul(quat, math::euler_deg_to_quat(float3{ 0, -90, 0 })),
				.world_width  = world_size,
				.world_height = world_size,
			});

			auto h_plane_left = widget::vertical_inv(set_width_grow() | set_height_grow() | set_child_gap(0));

			// +z scale
			if (auto h_scale_z = widget::horizontal(set_child_gap(0) | set_width_fixed(screen_size) | set_height_fit()))
			{
				auto& state = h_scale_z.get_state();

				c_auto is_drag_prev	   = static_cast<bool>(state.storage[0]);
				c_auto is_hovered_prev = static_cast<bool>(state.storage[1]);

				c_auto is_disabled = is_drag_prev is_false and (std::abs(math::dot(cam_forward, ui::detail::get_current_root().world_basis_u)) > (1.f - disable_threshold));

				c_auto color = is_drag_prev or center_dragging
								 ? drag_color
							 : is_disabled
								 ? disabled_color
							 : is_hovered_prev
								 ? theme::palette_azure_bright()
								 : theme::palette_blue();

				auto is_drag  = false;
				auto is_hover = false;
				if (auto h_line = widget::begin(set_width_fixed(screen_size * 0.9f * res_scale_ratio.z)
												| set_clip(false)
												| set_height_fixed(screen_size * 0.025f)
												| set_draw()
												| set_interact_mesh(is_disabled is_false)
												| set_align_end()
												| set_border_thickness(0)
												| set_pivot_uv(0.5f, 1.f)
												//| set_border_brush_data(theme::color_black())
												| set_fit_mode_fill()
												| set_shape_mesh(g::h_mesh_cube)
												| set_body_brush_data(color)))
				{
					is_drag	 |= h_line.pressed<mouse_left>();
					is_hover |= h_line.hovered();
				}

				if (auto h_cone = widget::begin(set_width_fixed(screen_size * 0.1f)
												| set_height_fixed(screen_size * 0.1f)
												| set_clip(false)
												| set_draw()
												| set_interact_mesh(is_disabled is_false)
												| set_align_center()
												| set_border_thickness(0)
												| set_pivot_uv(0.f, 1.f)
												//| set_border_brush_data(theme::color_black())
												| set_body_brush_data(color)
												| set_rotation(age::cvt_to_radian(90.f))
												| set_shape_mesh(g::h_mesh_cube)))
				{
					is_drag	 |= h_cone.pressed<mouse_left>();
					is_hover |= h_cone.hovered();
				}

				auto& state_ = h_scale_z.get_state();

				if (is_drag)
				{
					state_.drag_z	  += ui::detail::get_current_root().mouse_delta_uv.x;
					res_scale_ratio.z  = (screen_size + state_.drag_z) / screen_size;
				}
				else
				{
					state.drag_z = 0.f;
				}

				state_.storage[0] = is_drag ? 1 : 0;
				state_.storage[1] = is_hover ? 1 : 0;

				is_any_pressed |= is_drag;
			}
		}

		// xz, normal = (0,1,0)
		{
			auto h_root_down = root_begin(root_desc{
				.space_mode	  = ui::e::space_mode_kind::world_always_on_top,
				.layout		  = ui::e::widget_layout::vertical,
				.width		  = screen_size,
				.height		  = screen_size,
				.world_pos	  = anchor_pos + math::rotate(quat, float3(0, 0, world_size)),
				.quaternion	  = math::quat_mul(quat, math::euler_deg_to_quat(float3{ 90, 0, 0 })),
				.world_width  = world_size,
				.world_height = world_size,
			});
		}

		if (is_any_pressed is_false)
		{
			res_scale_ratio = float3::one();
		}

		c_auto is_drag_start = is_any_pressed_prev is_false and is_any_pressed is_true;
		is_any_pressed_prev	 = is_any_pressed;

		if (is_drag_start)
		{
			anchor_pos_prev = world_pos;
		}

		return std::tuple{ res_scale_ratio, is_drag_start, is_any_pressed };
	}

	std::tuple<float4, float3, bool, bool>
	rotation(const float cam_fov_y, const float3& cam_pos, const float3& cam_forward, const float3& world_pos, const float4& quat, const float screen_size) noexcept
	{
		enum class mode_kind : uint8
		{
			none,
			outer_circle,
			axis_x,
			axis_y,
			axis_z,
			trackball
		};

		using namespace ui;
		using namespace ui::widget;
		using enum input::e::key_kind;

		static auto prev_hover = mode_kind::none;
		static auto prev_drag  = mode_kind::none;

		static auto prev_mouse_sc		= float2{ 1, 0 };
		static auto drag_start_mouse_sc = float2{ 1, 0 };

		static auto drag_angle = 0.f;

		static auto gizmo_world_pos_on_drag_start = float3{};

		static auto trackball_quat = math::g::quaternion_identity;

		static auto quat_on_drag_start = math::g::quaternion_identity;

		auto res_quat = math::g::quaternion_identity;

		auto current_hover = mode_kind::none;
		auto current_drag  = mode_kind::none;


		c_auto outer_circle_hover = prev_hover == mode_kind::outer_circle;
		c_auto axis_x_hover		  = prev_hover == mode_kind::axis_x;
		c_auto axis_y_hover		  = prev_hover == mode_kind::axis_y;
		c_auto axis_z_hover		  = prev_hover == mode_kind::axis_z;

		c_auto outer_circle_drag = prev_drag == mode_kind::outer_circle;
		c_auto axis_x_drag		 = prev_drag == mode_kind::axis_x;
		c_auto axis_y_drag		 = prev_drag == mode_kind::axis_y;
		c_auto axis_z_drag		 = prev_drag == mode_kind::axis_z;

		c_auto outer_circle_draw = outer_circle_drag or prev_drag == mode_kind::none;
		c_auto axis_x_draw		 = axis_x_drag or prev_drag == mode_kind::none;
		c_auto axis_y_draw		 = axis_y_drag or prev_drag == mode_kind::none;
		c_auto axis_z_draw		 = axis_z_drag or prev_drag == mode_kind::none;

		c_auto outer_circle_interact = outer_circle_draw;
		c_auto axis_x_interact		 = axis_x_draw;
		c_auto axis_y_interact		 = axis_y_draw;
		c_auto axis_z_interact		 = axis_z_draw;

		c_auto outer_circle_bright = outer_circle_hover or outer_circle_drag;
		c_auto axis_x_bright	   = axis_x_hover or axis_x_drag;
		c_auto axis_y_bright	   = axis_y_hover or axis_y_drag;
		c_auto axis_z_bright	   = axis_z_hover or axis_z_drag;

		c_auto quaternion = prev_drag == mode_kind::none ? quat : quat_on_drag_start;

		c_auto obj_x = rotate(quaternion, math::g::right);
		c_auto obj_y = rotate(quaternion, math::g::up);
		c_auto obj_z = rotate(quaternion, math::g::forward);

		c_auto is_drag_prev	   = prev_drag != mode_kind::none;
		c_auto gizmo_world_pos = is_drag_prev ? gizmo_world_pos_on_drag_start : world_pos;

		c_auto view_z			= std::max(math::dot(gizmo_world_pos - cam_pos, cam_forward), 0.5f);
		c_auto world_size_scale = (screen_size / ui::g::window_height) * 2.0f * std::tanf(cam_fov_y * 0.5f);
		c_auto world_size		= world_size_scale * view_z;

		c_auto root_quat = math::quat_look_to(cam_forward);


		c_auto h_root = root_begin(root_desc{
			.space_mode	  = ui::e::space_mode_kind::world_always_on_top,
			.layout		  = ui::e::widget_layout::vertical,
			.width		  = screen_size,
			.height		  = screen_size,
			.world_pos	  = gizmo_world_pos + math::rotate(root_quat, float3(-world_size * 0.5f, world_size * 0.5f, 0)),
			.quaternion	  = root_quat,
			.world_width  = world_size,
			.world_height = world_size,
		});

		c_auto& root = ui::detail::get_current_root();

		c_auto mouse_center_offset = root.mouse_uv - float2{ screen_size } * 0.5f;
		c_auto mouse_sc			   = normalize(float2{ mouse_center_offset.x, -mouse_center_offset.y });

		c_auto h_outer_circle = widget::horizontal(set_grow()
												   | set_child_gap(0)
												   | set_draw(outer_circle_draw)
												   | set_padding(theme::padding_large())
												   | set_interact(outer_circle_interact ? ui::e::interact_mode_kind::sdf : ui::e::interact_mode_kind::none)
												   | set_shape_circle()
												   | set_shape_arc(theme::thickness_medium() * 2, cvt_to_radian(360))
												   | set_body_brush_color(float4::zero())
												   //| set_border_thickness(theme::thickness_thick())
												   | set_body_brush_color(outer_circle_bright ? theme::color_white() * 3.f : theme::palette_white_mild() * 3.f));
		if (h_outer_circle.hovered())
		{
			current_hover = mode_kind::outer_circle;
		}
		if (h_outer_circle.pressed<mouse_left>())
		{
			current_drag = mode_kind::outer_circle;
		}

		{
			if (c_auto h_outer_circle_pie_circle = widget::vertical(set_fixed(0) | set_align_begin()))
			{
				c_auto circle_count = static_cast<uint32>(abs(std::trunc(drag_angle / math::g::pi_2)));

				// after = opacity_mild() + before * ( 1 - opacity_mild() );
				auto opacity = 0.f;
				for (auto _ : views::loop(circle_count))
				{
					opacity = theme::opacity_mild() + opacity * (1.f - theme::opacity_mild());
				}

				if (outer_circle_drag)
				{
					widget::begin(set_fixed(screen_size)
								  | set_offset(-theme::padding_large(), -theme::padding_large())
								  | set_clip(false)
								  | set_align_begin()
								  | set_shape_circle()
								  | set_body_brush_color(theme::color_white() * 3.f, opacity));
				}
			}

			if (c_auto h_outer_circle_pie = widget::vertical(set_fixed(0) | set_align_begin()))
			{
				c_auto width  = screen_size - theme::padding_large() * 2;
				c_auto height = max(theme::thickness_medium() * 4, (screen_size - theme::padding_large() * 2) * abs(dot(obj_y, cam_forward)));
				if (outer_circle_drag)
				{
					widget::begin(set_fixed(screen_size)
								  | set_offset(-theme::padding_large(), -theme::padding_large())
								  | set_clip(false)
								  | set_align_begin()
								  | set_shape_pie_range(drag_start_mouse_sc, mouse_sc, std::fmod(drag_angle, 2.f * math::g::pi))
								  | set_body_brush_color(theme::color_white() * 3.f, theme::opacity_mild()));
				}
			}
		}

		{
			c_auto width  = screen_size - theme::padding_large() * 2;
			c_auto height = max(theme::thickness_medium() * 4, (screen_size - theme::padding_large() * 2) * abs(dot(obj_y, cam_forward)));

			c_auto axis_dir_uv = normalize(float2{ dot(obj_y, root.world_basis_u), dot(obj_y, -root.world_basis_v) });
			c_auto perp_dir_uv = float2{ axis_dir_uv.y, -axis_dir_uv.x };

			c_auto rot_cos = axis_dir_uv.y;
			c_auto rot_sin = axis_dir_uv.x;
			c_auto rot	   = std::atan2(rot_sin, rot_cos);

			if (c_auto h_xz_plane = widget::vertical(set_fixed(0) | set_align_begin()))
			{
				if (c_auto h_axis_y = widget::begin(set_height_fixed(height)
													| set_width_fixed(width)
													| set_draw(axis_y_draw)
													| set_interact(axis_y_interact ? ui::e::interact_mode_kind::sdf : ui::e::interact_mode_kind::none)
													| set_offset(0, width * 0.5f - height * 0.5f)
													| set_fit_mode_fill()
													| set_clip(false)
													| set_z_offset(1)
													| set_border_thickness(0)
													| set_align_begin()
													| set_rotation(rot + (dot(cam_forward, obj_y) > 0.f ? 0.f : cvt_to_radian(180)))
													| set_shape_arc(theme::thickness_medium() * 2, axis_y_drag ? cvt_to_radian(360.f) : cvt_to_radian(160.f))
													| set_body_brush_color(axis_y_bright ? theme::palette_green_bright() : theme::palette_green())))
				{
					if (h_axis_y.hovered())
					{
						current_hover = mode_kind::axis_y;
					}
					if (h_axis_y.pressed<mouse_left>())
					{
						current_drag = mode_kind::axis_y;
					}
				}
			}


			if (c_auto h_xz_pie_circle = widget::vertical(set_fixed(0) | set_align_begin()))
			{
				c_auto circle_count = static_cast<uint32>(abs(std::trunc(drag_angle / math::g::pi_2)));

				auto opacity = 0.f;
				for (auto _ : views::loop(circle_count))
				{
					opacity = theme::opacity_mild() + opacity * (1.f - theme::opacity_mild());
				}

				if (axis_y_drag)
				{
					widget::begin(set_height_fixed(height)
								  | set_width_fixed(width)
								  | set_offset(0, width * 0.5f - height * 0.5f)
								  | set_fit_mode_fill()
								  | set_clip(false)
								  | set_align_begin()
								  | set_shape_circle()
								  | set_rotation(rot)
								  | set_border_brush_color(theme::palette_green_bright())
								  | set_border_thickness(theme::thickness_medium())
								  | set_body_brush_color(theme::palette_green_bright(), opacity));
				}
			}

			if (c_auto h_xz_pie = widget::vertical(set_fixed(0) | set_align_begin()))
			{
				if (axis_y_drag)
				{
					c_auto rotate_sc = [&](float2 sc) {
						return float2{ sc.x * rot_cos - sc.y * rot_sin, sc.x * rot_sin + sc.y * rot_cos };
					};

					widget::begin(set_height_fixed(height)
								  | set_width_fixed(width)
								  | set_offset(0, width * 0.5f - height * 0.5f)
								  | set_fit_mode_fill()
								  | set_clip(false)
								  | set_align_begin()
								  | set_rotation(rot)
								  | set_shape_pie_range(rotate_sc(drag_start_mouse_sc), rotate_sc(mouse_sc), std::fmod(drag_angle, 2.f * math::g::pi))
								  | set_body_brush_color(theme::palette_green_bright(), theme::opacity_mild()));
				}
			}

			if (c_auto h_xz_normal_line = widget::vertical(set_fixed(0) | set_align_center()))
			{
				c_auto width  = theme::thickness_medium() * 2;
				c_auto height = 10000.f;
				widget::begin(set_height_fixed(height)
							  | set_width_fixed(width)
							  | set_draw(axis_y_drag)
							  | set_offset((screen_size - theme::padding_large() * 2) * 0.5f, -height * 0.5f)
							  | set_clip(false)
							  | set_align_begin()
							  | set_rotation(rot)
							  | set_body_brush_color(theme::palette_green_bright(), theme::opacity_heavy()));
			}
		}

		{
			c_auto width  = screen_size - theme::padding_large() * 2;
			c_auto height = max(theme::thickness_medium() * 4, (screen_size - theme::padding_large() * 2) * abs(dot(obj_z, cam_forward)));

			c_auto axis_dir_uv = normalize(float2{ dot(obj_z, root.world_basis_u), dot(obj_z, -root.world_basis_v) });
			c_auto perp_dir_uv = float2{ axis_dir_uv.y, -axis_dir_uv.x };

			c_auto rot_cos = axis_dir_uv.y;
			c_auto rot_sin = axis_dir_uv.x;
			c_auto rot	   = std::atan2(rot_sin, rot_cos);

			if (c_auto h_xy_plane = widget::vertical(set_fixed(0) | set_align_begin()))
			{
				if (c_auto h_axis_z = widget::begin(set_height_fixed(height)
													| set_width_fixed(width)
													| set_draw(axis_z_draw)
													| set_interact(axis_z_interact ? ui::e::interact_mode_kind::sdf : ui::e::interact_mode_kind::none)
													| set_offset(0, width * 0.5f - height * 0.5f)
													| set_fit_mode_fill()
													| set_clip(false)
													| set_z_offset(1)
													| set_border_thickness(0)
													| set_align_begin()
													| set_rotation(rot + (dot(cam_forward, obj_z) > 0.f ? 0.f : cvt_to_radian(180)))
													| set_shape_arc(theme::thickness_medium() * 2, axis_z_drag ? cvt_to_radian(360.f) : cvt_to_radian(160.f))
													| set_body_brush_color(axis_z_bright ? theme::palette_blue_bright() : theme::palette_blue())))
				{
					if (h_axis_z.hovered())
					{
						current_hover = mode_kind::axis_z;
					}
					if (h_axis_z.pressed<mouse_left>())
					{
						current_drag = mode_kind::axis_z;
					}
				}
			}


			if (c_auto h_xy_pie_circle = widget::vertical(set_fixed(0) | set_align_begin()))
			{
				c_auto circle_count = static_cast<uint32>(abs(std::trunc(drag_angle / math::g::pi_2)));

				auto opacity = 0.f;
				for (auto _ : views::loop(circle_count))
				{
					opacity = theme::opacity_mild() + opacity * (1.f - theme::opacity_mild());
				}

				if (axis_z_drag)
				{
					widget::begin(set_height_fixed(height)
								  | set_width_fixed(width)
								  | set_offset(0, width * 0.5f - height * 0.5f)
								  | set_fit_mode_fill()
								  | set_clip(false)
								  | set_align_begin()
								  | set_shape_circle()
								  | set_rotation(rot)
								  | set_border_brush_color(theme::palette_blue_bright())
								  | set_border_thickness(theme::thickness_medium())
								  | set_body_brush_color(theme::palette_blue_bright(), opacity));
				}
			}

			if (c_auto h_xy_pie = widget::vertical(set_fixed(0) | set_align_begin()))
			{
				if (axis_z_drag)
				{
					c_auto rotate_sc = [&](float2 sc) {
						return float2{ sc.x * rot_cos - sc.y * rot_sin, sc.x * rot_sin + sc.y * rot_cos };
					};

					widget::begin(set_height_fixed(height)
								  | set_width_fixed(width)
								  | set_offset(0, width * 0.5f - height * 0.5f)
								  | set_fit_mode_fill()
								  | set_clip(false)
								  | set_align_begin()
								  | set_rotation(rot)
								  | set_shape_pie_range(rotate_sc(drag_start_mouse_sc), rotate_sc(mouse_sc), std::fmod(drag_angle, 2.f * math::g::pi))
								  | set_body_brush_color(theme::palette_blue_bright(), theme::opacity_mild()));
				}
			}

			if (c_auto h_xy_normal_line = widget::vertical(set_fixed(0) | set_align_center()))
			{
				c_auto width  = theme::thickness_medium() * 2;
				c_auto height = 10000.f;
				widget::begin(set_height_fixed(height)
							  | set_width_fixed(width)
							  | set_draw(axis_z_drag)
							  | set_offset((screen_size - theme::padding_large() * 2) * 0.5f, -height * 0.5f)
							  | set_clip(false)
							  | set_align_begin()
							  | set_rotation(rot)
							  | set_body_brush_color(theme::palette_blue_bright(), theme::opacity_heavy()));
			}
		}

		{
			c_auto width  = screen_size - theme::padding_large() * 2;
			c_auto height = max(theme::thickness_medium() * 4, (screen_size - theme::padding_large() * 2) * abs(dot(obj_x, cam_forward)));

			c_auto axis_dir_uv = normalize(float2{ dot(obj_x, root.world_basis_u), dot(obj_x, -root.world_basis_v) });
			c_auto perp_dir_uv = float2{ axis_dir_uv.y, -axis_dir_uv.x };

			c_auto rot_cos = axis_dir_uv.y;
			c_auto rot_sin = axis_dir_uv.x;
			c_auto rot	   = std::atan2(rot_sin, rot_cos);

			if (c_auto h_yz_plane = widget::vertical(set_fixed(0) | set_align_begin()))
			{
				if (c_auto h_axis_x = widget::begin(set_height_fixed(height)
													| set_width_fixed(width)
													| set_draw(axis_x_draw)
													| set_interact(axis_x_interact ? ui::e::interact_mode_kind::sdf : ui::e::interact_mode_kind::none)
													| set_offset(0, width * 0.5f - height * 0.5f)
													| set_fit_mode_fill()
													| set_clip(false)
													| set_z_offset(1)
													| set_border_thickness(0)
													| set_align_begin()
													| set_rotation(rot + (dot(cam_forward, obj_x) > 0.f ? 0.f : cvt_to_radian(180)))
													| set_shape_arc(theme::thickness_medium() * 2, axis_x_drag ? cvt_to_radian(360.f) : cvt_to_radian(160.f))
													| set_body_brush_color(axis_x_bright ? theme::palette_red_bright() : theme::palette_red())))
				{
					if (h_axis_x.hovered())
					{
						current_hover = mode_kind::axis_x;
					}
					if (h_axis_x.pressed<mouse_left>())
					{
						current_drag = mode_kind::axis_x;
					}
				}
			}


			if (c_auto h_yz_pie_circle = widget::vertical(set_fixed(0) | set_align_begin()))
			{
				c_auto circle_count = static_cast<uint32>(abs(std::trunc(drag_angle / math::g::pi_2)));

				auto opacity = 0.f;
				for (auto _ : views::loop(circle_count))
				{
					opacity = theme::opacity_mild() + opacity * (1.f - theme::opacity_mild());
				}

				if (axis_x_drag)
				{
					widget::begin(set_height_fixed(height)
								  | set_width_fixed(width)
								  | set_offset(0, width * 0.5f - height * 0.5f)
								  | set_fit_mode_fill()
								  | set_clip(false)
								  | set_align_begin()
								  | set_shape_circle()
								  | set_rotation(rot)
								  | set_border_brush_color(theme::palette_red_bright())
								  | set_border_thickness(theme::thickness_medium())
								  | set_body_brush_color(theme::palette_red_bright(), opacity));
				}
			}

			if (c_auto h_yz_pie = widget::vertical(set_fixed(0) | set_align_begin()))
			{
				if (axis_x_drag)
				{
					c_auto rotate_sc = [&](float2 sc) {
						return float2{ sc.x * rot_cos - sc.y * rot_sin, sc.x * rot_sin + sc.y * rot_cos };
					};

					widget::begin(set_height_fixed(height)
								  | set_width_fixed(width)
								  | set_offset(0, width * 0.5f - height * 0.5f)
								  | set_fit_mode_fill()
								  | set_clip(false)
								  | set_align_begin()
								  | set_rotation(rot)
								  | set_shape_pie_range(rotate_sc(drag_start_mouse_sc), rotate_sc(mouse_sc), std::fmod(drag_angle, 2.f * math::g::pi))
								  | set_body_brush_color(theme::palette_red_bright(), theme::opacity_mild()));
				}
			}

			if (c_auto h_yz_normal_line = widget::vertical(set_fixed(0) | set_align_center()))
			{
				c_auto width  = theme::thickness_medium() * 2;
				c_auto height = 10000.f;
				widget::begin(set_height_fixed(height)
							  | set_width_fixed(width)
							  | set_draw(axis_x_drag)
							  | set_offset((screen_size - theme::padding_large() * 2) * 0.5f, -height * 0.5f)
							  | set_clip(false)
							  | set_align_begin()
							  | set_rotation(rot)
							  | set_body_brush_color(theme::palette_red_bright(), theme::opacity_heavy()));
			}
		}

		{
			if (c_auto h_trackball_panel = widget::vertical(set_fixed(0) | set_align_begin()))
			{
				c_auto radius = screen_size - theme::padding_large() * 2;

				c_auto h_trackball = widget::begin(set_height_fixed(radius)
												   | set_width_fixed(radius)
												   | set_draw(prev_hover == mode_kind::trackball or prev_drag == mode_kind::trackball)
												   | set_z_offset(0)
												   | set_save_state()
												   | set_offset(0, radius * 0.5f - radius * 0.5f)
												   | set_interact_sdf()
												   | set_clip(false)
												   | set_align_begin()
												   | set_shape_circle()
												   | set_body_brush_color(theme::color_white(), theme::opacity_mild()));

				if (h_trackball.contains_mouse() and (current_hover == mode_kind::none and current_drag == mode_kind::none))
				{
					current_hover = mode_kind::trackball;
				}
				if (h_trackball.pressed<mouse_left>())
				{
					current_drag = mode_kind::trackball;
				}
			}
		}

		c_auto is_drag = current_drag != mode_kind::none;

		if (is_drag is_false)
		{
			drag_angle	   = 0.f;
			trackball_quat = math::g::quaternion_identity;
		}
		else
		{
			auto axis_world = float3{};

			if (prev_drag != mode_kind::trackball)
			{
				c_auto delta_sin = mouse_sc.x * prev_mouse_sc.y - mouse_sc.y * prev_mouse_sc.x;
				c_auto delta_cos = mouse_sc.y * prev_mouse_sc.y + mouse_sc.x * prev_mouse_sc.x;

				drag_angle += std::atan2(delta_sin, delta_cos);

				if (prev_drag == mode_kind::outer_circle)
				{
					axis_world = -cam_forward;
				}
				else if (prev_drag == mode_kind::axis_x)
				{
					axis_world = obj_x;
				}
				else if (prev_drag == mode_kind::axis_y)
				{
					axis_world = obj_y;
				}
				else if (prev_drag == mode_kind::axis_z)
				{
					axis_world = obj_z;
				}

				res_quat = math::quat_rotation_normal(axis_world, drag_angle);
			}
			else
			{
				c_auto world_delta = root.world_basis_u * root.mouse_delta_uv.x + root.world_basis_v * root.mouse_delta_uv.y;
				axis_world		   = normalize(-cross(world_delta, root.world_normal));

				c_auto angle = length(world_delta) / world_size * g::gizmo_rotation_trackball_sensitivity;

				if (angle > math::g::epsilon_1e4)
				{
					c_auto delta_quat = math::quat_rotation_normal(axis_world, angle);

					trackball_quat = quat_mul(delta_quat, trackball_quat);
				}

				res_quat = trackball_quat;
			}
		}

		c_auto is_drag_start = is_drag_prev is_false and is_drag is_true;

		if (is_drag_start)
		{
			drag_start_mouse_sc			  = mouse_sc;
			gizmo_world_pos_on_drag_start = world_pos;
			quat_on_drag_start			  = quat;
		}

		prev_mouse_sc = mouse_sc;


		prev_hover = current_hover;
		prev_drag  = current_drag;


		return std::tuple{ res_quat, gizmo_world_pos_on_drag_start, is_drag_start, is_drag };
	}
}	 // namespace age::editor::gizmo
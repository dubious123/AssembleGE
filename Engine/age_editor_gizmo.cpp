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
								 ? theme::palette_light_red()
								 : theme::color_red();

				if (auto h_center = widget::begin(set_width_fixed(0)
												  | set_height_fixed(screen_size * 0.05f)
												  | set_draw()
												  | set_interact()
												  | set_align_end()
												  | set_border_thickness(0)
												  | set_pivot_uv(0.f, 1.f)
												  //| set_border_brush_data(theme::color_black())
												  | set_shape_mesh(g::h_mesh_cube, ui::e::fit_mode_kind::cover)
												  | set_body_brush_data(theme::color_white())))
				{
				}

				auto is_drag  = false;
				auto is_hover = false;
				if (auto h_line = widget::begin(set_width_grow()
												| set_height_fixed(screen_size * 0.025f)
												| set_draw()
												| set_interact(is_disabled is_false)
												| set_align_end()
												| set_border_thickness(0)
												| set_pivot_uv(0.5f, 1.f)
												| set_shape_mesh(g::h_mesh_cube, ui::e::fit_mode_kind::fill)
												| set_body_brush_data(color)))
				{
					is_drag	 |= h_line.pressed<mouse_left>();
					is_hover |= h_line.hovered();
				}

				if (auto h_cone = widget::begin(set_width_fixed(screen_size * 0.1f)
												| set_height_fixed(screen_size * 0.1f)
												| set_draw()
												| set_interact(is_disabled is_false)
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
									 ? theme::palette_light_green()
									 : theme::color_green();

					auto is_drag  = false;
					auto is_hover = false;
					if (auto h_line = widget::begin(set_width_fixed(screen_size * 0.025f)
													| set_height_grow()
													| set_draw()
													| set_interact(is_disabled is_false)
													| set_align_begin()
													| set_padding(0)
													| set_pivot_uv(0.f, 0.5f)
													| set_body_brush_data(color)
													| set_shape_mesh(g::h_mesh_cube, ui::e::fit_mode_kind::fill)))
					{
						is_drag	 |= h_line.pressed<mouse_left>();
						is_hover |= h_line.hovered();
					}

					if (auto h_cone = widget::begin(set_width_fixed(screen_size * 0.1f)
													| set_height_fixed(screen_size * 0.1f)
													| set_draw()
													| set_interact(is_disabled is_false)
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
									   ? theme::palette_light_blue()
									   : theme::color_blue();
				c_auto opacity	   = is_drag_prev or is_hovered_prev or is_disabled ? 1.f : theme::opacity_heavy();


				auto h_plane_y = widget::begin(set_align(ui::e::widget_align::center)
											   | set_draw()
											   | set_interact(is_disabled is_false)
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
								 ? theme::palette_light_blue()
								 : theme::color_blue();

				auto is_drag  = false;
				auto is_hover = false;
				if (auto h_line = widget::begin(set_width_grow()
												| set_height_fixed(screen_size * 0.025f)
												| set_draw()
												| set_interact(is_disabled is_false)
												| set_align_end()
												| set_border_thickness(0)
												| set_pivot_uv(0.5f, 1.f)
												//| set_border_brush_data(theme::color_black())
												| set_shape_mesh(g::h_mesh_cube, ui::e::fit_mode_kind::fill)
												| set_body_brush_data(color)))
				{
					is_drag	 |= h_line.pressed<mouse_left>();
					is_hover |= h_line.hovered();
				}

				if (auto h_cone = widget::begin(set_width_fixed(screen_size * 0.1f)
												| set_height_fixed(screen_size * 0.1f)
												//| set_offset(-screen_size * 0.05f, 0)
												| set_draw()
												| set_interact(is_disabled is_false)
												| set_align_center()
												| set_border_thickness(0)
												| set_pivot_uv(0.f, 1.f)
												//| set_border_brush_data(theme::color_black())
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
									   ? theme::palette_light_red()
									   : theme::color_red();
				c_auto opacity	   = is_drag_prev or is_hovered_prev or is_disabled ? 1.f : theme::opacity_heavy();


				auto h_plane_x = widget::begin(set_align(ui::e::widget_align::center)
											   | set_draw()
											   | set_interact(is_disabled is_false)
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
									   ? theme::palette_light_green()
									   : theme::color_green();
				c_auto opacity	   = is_drag_prev or is_hovered_prev or is_disabled ? 1.f : theme::opacity_heavy();


				auto h_plane_y = widget::begin(set_align(ui::e::widget_align::center)
											   | set_draw()
											   | set_interact(is_disabled is_false)
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
									 ? theme::palette_light_red()
									 : theme::color_white();

					if (auto h_center = widget::begin(set_width_fixed(0)
													  | set_height_fixed(screen_size * 0.1f)
													  | set_clip(false)
													  | set_draw()
													  | set_interact()
													  | set_align_end()
													  | set_border_thickness(0)
													  | set_pivot_uv(0.f, 1.f)
													  //| set_border_brush_data(theme::color_black())
													  | set_shape_mesh(g::h_mesh_cube, ui::e::fit_mode_kind::cover)
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
								 ? theme::palette_light_red()
								 : theme::color_red();

				auto is_drag  = false;
				auto is_hover = false;
				if (auto h_line = widget::begin(set_width_fixed(screen_size * 0.9f * res_scale_ratio.x)
												| set_height_fixed(screen_size * 0.025f)
												| set_draw()
												| set_interact(is_disabled is_false)
												| set_clip(false)
												| set_align_end()
												| set_border_thickness(0)
												| set_pivot_uv(0.5f, 1.f)
												| set_shape_mesh(g::h_mesh_cube, ui::e::fit_mode_kind::fill)
												| set_body_brush_data(color)))
				{
					is_drag	 |= h_line.pressed<mouse_left>();
					is_hover |= h_line.hovered();
				}

				if (auto h_cone = widget::begin(set_width_fixed(screen_size * 0.1f)
												| set_height_fixed(screen_size * 0.1f)
												| set_draw()
												| set_interact(is_disabled is_false)
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
									 ? theme::palette_light_green()
									 : theme::color_green();

					auto is_drag  = false;
					auto is_hover = false;
					if (auto h_line = widget::begin(set_width_fixed(screen_size * 0.025f)
													| set_height_fixed(screen_size * 0.9f * res_scale_ratio.y)
													| set_clip(false)
													| set_draw()
													| set_interact(is_disabled is_false)
													| set_align_begin()
													| set_padding(0)
													| set_pivot_uv(0.f, 0.5f)
													| set_body_brush_data(color)
													| set_shape_mesh(g::h_mesh_cube, ui::e::fit_mode_kind::fill)))
					{
						is_drag	 |= h_line.pressed<mouse_left>();
						is_hover |= h_line.hovered();
					}

					if (auto h_cone = widget::begin(set_width_fixed(screen_size * 0.1f)
													| set_height_fixed(screen_size * 0.1f)
													| set_clip(false)
													| set_draw()
													| set_interact(is_disabled is_false)
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
								 ? theme::palette_light_blue()
								 : theme::color_blue();

				auto is_drag  = false;
				auto is_hover = false;
				if (auto h_line = widget::begin(set_width_fixed(screen_size * 0.9f * res_scale_ratio.z)
												| set_clip(false)
												| set_height_fixed(screen_size * 0.025f)
												| set_draw()
												| set_interact(is_disabled is_false)
												| set_align_end()
												| set_border_thickness(0)
												| set_pivot_uv(0.5f, 1.f)
												//| set_border_brush_data(theme::color_black())
												| set_shape_mesh(g::h_mesh_cube, ui::e::fit_mode_kind::fill)
												| set_body_brush_data(color)))
				{
					is_drag	 |= h_line.pressed<mouse_left>();
					is_hover |= h_line.hovered();
				}

				if (auto h_cone = widget::begin(set_width_fixed(screen_size * 0.1f)
												| set_height_fixed(screen_size * 0.1f)
												| set_clip(false)
												| set_draw()
												| set_interact(is_disabled is_false)
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
}	 // namespace age::editor::gizmo
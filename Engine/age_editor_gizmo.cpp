#include "age_pch.hpp"
#include "age.hpp"

namespace age::editor::gizmo
{
	namespace detail
	{
		std::optional<float3>
		closest_point_on_axis(float3 axis_origin, float3 axis_dir, float3 ray_origin, float3 ray_dir) noexcept
		{
			// v = (axis_origin + t * axis_dir) - (ray_origin + s * ray_dir)
			// v dot axis_dir == 0 => dot(origin_delta, axis_dir) + t - s * dot(axis_dir, ray_dir) == 0
			// v dot ray_dir == 0  => dot(origin_delta, ray_dir) + t * dot(axis_dir, ray_dir) - s == 0
			// s == dot(origin_delta, ray_dir) + t * dot(axis_dir, ray_dir)
			// delta_on_axis + t - (delta_on_ray + t * cos_theta) * cos_theta == 0;
			// delta_on_axis - delta_on_ray * cos_thata == - sin_sq_theta * t;
			// t == (cos_thata * delta_on_ray - delta_on_axis) / sin_sq_theta;

			c_auto origin_delta	 = axis_origin - ray_origin;
			c_auto cos_theta	 = math::dot(axis_dir, ray_dir);
			c_auto delta_on_axis = math::dot(axis_dir, origin_delta);
			c_auto delta_on_ray	 = math::dot(ray_dir, origin_delta);
			c_auto sin_sq_theta	 = 1.f - cos_theta * cos_theta;

			if (sin_sq_theta < 0.01f)
			{
				return std::nullopt;
			}

			c_auto t = (cos_theta * delta_on_ray - delta_on_axis) / sin_sq_theta;
			c_auto s = delta_on_ray + t * cos_theta;

			if (s <= 0.f)
			{
				return std::nullopt;
			}

			return axis_origin + axis_dir * t;
		}

		std::optional<float3>
		ray_plane_intersection(
			const float3& ray_origin,
			const float3& ray_dir,
			const float3& plane_point,
			const float3& plane_normal) noexcept
		{
			c_auto denom = math::dot(ray_dir, plane_normal);

			if (std::abs(denom) < math::g::epsilon_1e6)
			{
				return std::nullopt;
			}

			c_auto t = math::dot(plane_point - ray_origin, plane_normal) / denom;

			if (t <= 0.f)
			{
				return std::nullopt;
			}

			return ray_origin + ray_dir * t;
		}

		// translation axis drag hit
		std::optional<float3>
		axis_drag_hit(const float3& axis_origin, const float3& axis_dir, const float3& cam_pos, const float3& cam_forward, const float3& mouse_ray_dir) noexcept
		{
			c_auto hit_on_cam_plane = ray_plane_intersection(cam_pos, mouse_ray_dir, axis_origin, cam_forward);
			if (hit_on_cam_plane.has_value() is_false)
			{
				return std::nullopt;
			}

			c_auto axis_plane_normal	 = math::cross(axis_dir, axis_origin - cam_pos);
			c_auto axis_plane_normal_len = math::length(axis_plane_normal);
			if (axis_plane_normal_len < math::g::epsilon_1e4)
			{
				return std::nullopt;
			}

			c_auto axis_line_dir = math::cross(axis_plane_normal / axis_plane_normal_len, cam_forward);
			c_auto axis_line_len = math::length(axis_line_dir);
			if (axis_line_len < 0.1f)
			{
				return std::nullopt;
			}

			c_auto mouse_delta		  = *hit_on_cam_plane - axis_origin;
			c_auto mouse_on_axis_line = axis_origin + (axis_line_dir / axis_line_len) * math::dot(mouse_delta, axis_line_dir / axis_line_len);

			c_auto ray_dir = math::normalize(mouse_on_axis_line - cam_pos);
			return closest_point_on_axis(axis_origin, axis_dir, cam_pos, ray_dir);
		}
	}	 // namespace detail

	// translation, drag_started, dragging
	std::tuple<float3, bool, bool>
	translation(const float cam_fov_y, const float3& cam_pos, const float3& cam_forward, const float3& world_pos, const float4& quat, const float screen_size) noexcept
	{
		using namespace ui;
		using namespace ui::widget;
		using enum input::e::key_kind;

		static auto is_any_pressed_prev			   = false;
		static auto object_world_pos_on_drag_start = float3::zero();
		static auto hit_world_pos_on_drag_start	   = float3::zero();
		static auto hit_world_pos_prev			   = float3::zero();

		auto is_any_pressed = false;
		auto hit_world_pos	= float3::zero();

		c_auto anchor_pos = is_any_pressed_prev ? object_world_pos_on_drag_start : world_pos;

		c_auto view_z			= std::max(math::dot(world_pos - cam_pos, cam_forward), 0.5f);
		c_auto world_size_scale = (screen_size / ui::g::window_height) * 2.0f * std::tanf(cam_fov_y * 0.5f);
		c_auto world_size		= world_size_scale * view_z;

		c_auto drag_color			   = theme::color_amber();
		c_auto disabled_color		   = theme::palette_cool_gray();
		c_auto axis_disable_threshold  = 0.15;
		c_auto plane_disable_threshold = 0.3f;

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

				c_auto is_disabled = is_drag_prev is_false and (std::abs(math::dot(cam_forward, ui::detail::get_current_root().world_basis_u)) > (1.f - axis_disable_threshold));

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
					c_auto& current_root = ui::detail::get_current_root();
					if (c_auto res = detail::axis_drag_hit(anchor_pos, current_root.world_basis_u, ui::g::cam_world_pos, cam_forward, ui::g::mouse_ray_dir);
						res.has_value())
					{
						hit_world_pos = *res;
					}
					else
					{
						hit_world_pos = hit_world_pos_prev;
					}
				}

				auto& state_	  = h_translation_x.get_state();
				state_.storage[0] = is_drag ? 1 : 0;
				state_.storage[1] = is_hover ? 1 : 0;

				is_any_pressed |= is_drag;
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

					c_auto is_disabled = is_drag_prev is_false and (std::abs(math::dot(cam_forward, ui::detail::get_current_root().world_basis_v)) > (1.f - axis_disable_threshold));

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
						c_auto& current_root = ui::detail::get_current_root();
						if (c_auto res = detail::axis_drag_hit(anchor_pos, current_root.world_basis_v, ui::g::cam_world_pos, cam_forward, ui::g::mouse_ray_dir);
							res.has_value())
						{
							hit_world_pos = *res;
						}
						else
						{
							hit_world_pos = hit_world_pos_prev;
						}
					}

					auto& state_	  = h_translation_y.get_state();
					state_.storage[0] = is_drag ? 1 : 0;
					state_.storage[1] = is_hover ? 1 : 0;

					is_any_pressed |= is_drag;
				}
			}

			auto _ = widget::horizontal(set_offset(0 /*- screen_size * 0.08f*/, 0 /*+ screen_size * 0.08f*/) | set_fit() | set_align_end() | set_clip(false));

			if (auto h_translation_xy = widget::begin(set_vertical() | set_fit()))
			{
				auto& state = h_translation_xy.get_state();

				c_auto is_drag_prev	   = static_cast<bool>(state.storage[0]);
				c_auto is_hovered_prev = static_cast<bool>(state.storage[1]);

				c_auto is_disabled = is_drag_prev is_false and (std::abs(math::dot(cam_forward, ui::detail::get_current_root().world_normal)) < plane_disable_threshold);
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
					hit_world_pos = ui::detail::get_current_root().mouse_world_pos();
				}

				auto& state_	  = h_translation_xy.get_state();
				state_.storage[0] = is_drag ? 1 : 0;
				state_.storage[1] = is_hover ? 1 : 0;

				is_any_pressed |= is_drag;
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

				c_auto is_disabled = is_drag_prev is_false and (std::abs(math::dot(cam_forward, ui::detail::get_current_root().world_basis_u)) > (1.f - axis_disable_threshold));

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
					c_auto& current_root = ui::detail::get_current_root();
					if (c_auto res = detail::axis_drag_hit(anchor_pos, current_root.world_basis_u, ui::g::cam_world_pos, cam_forward, ui::g::mouse_ray_dir);
						res.has_value())
					{
						hit_world_pos = *res;
					}
					else
					{
						hit_world_pos = hit_world_pos_prev;
					}
				}

				auto& state_	  = h_translation_z.get_state();
				state_.storage[0] = is_drag ? 1 : 0;
				state_.storage[1] = is_hover ? 1 : 0;

				is_any_pressed |= is_drag;
			}

			auto _ = widget::horizontal(set_offset(screen_size * 0.1f /*- screen_size * 0.08f*/, 0 /*+ screen_size * 0.08f*/) | set_fit() | set_align_begin() | set_clip(false));
			if (auto h_translation_yz = widget::begin(style::vertical() | set_fit()))
			{
				auto& state = h_translation_yz.get_state();

				c_auto is_drag_prev	   = static_cast<bool>(state.storage[0]);
				c_auto is_hovered_prev = static_cast<bool>(state.storage[1]);

				c_auto is_disabled = is_drag_prev is_false and (std::abs(math::dot(cam_forward, ui::detail::get_current_root().world_normal)) < plane_disable_threshold);
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
					hit_world_pos = ui::detail::get_current_root().mouse_world_pos();
				}

				auto& state_	  = h_translation_yz.get_state();
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

				c_auto is_disabled = is_drag_prev is_false and (std::abs(math::dot(cam_forward, ui::detail::get_current_root().world_normal)) < plane_disable_threshold);
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
					hit_world_pos = ui::detail::get_current_root().mouse_world_pos();
				}


				auto& state_	  = h_translation_xz.get_state();
				state_.storage[0] = is_drag ? 1 : 0;
				state_.storage[1] = is_hover ? 1 : 0;

				is_any_pressed |= is_drag;
			}
		}

		if (is_any_pressed is_false)
		{
			hit_world_pos_on_drag_start = hit_world_pos;
		}

		c_auto is_drag_start = is_any_pressed_prev is_false and is_any_pressed is_true;
		is_any_pressed_prev	 = is_any_pressed;

		if (is_drag_start)
		{
			object_world_pos_on_drag_start = world_pos;
			hit_world_pos_on_drag_start	   = hit_world_pos;
		}

		hit_world_pos_prev = hit_world_pos;

		return { hit_world_pos - hit_world_pos_on_drag_start, is_drag_start, is_any_pressed };
	}

	std::tuple<float3, bool, bool>
	scale(const float cam_fov_y, const float3& cam_pos, const float3& cam_forward, const float3& world_pos, const float4& quat, const float screen_size) noexcept
	{
		using namespace ui;
		using namespace ui::widget;
		using enum input::e::key_kind;

		static auto res_scale_ratio				= float3::one();
		static auto is_any_pressed_prev			= false;
		static auto anchor_pos_prev				= float3::zero();
		static auto hit_world_pos_on_drag_start = float3::zero();

		auto hit_world_pos = float3::zero();
		auto hit_axis_dir  = float3::zero();
		// 0 : x, 1 : y, 2 : z
		auto dragged_axis_idx = -1;

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
							if (c_auto hit = detail::ray_plane_intersection(ui::g::cam_world_pos, ui::g::mouse_ray_dir, anchor_pos, cam_forward))
							{
								c_auto hit_world_delta = *hit - anchor_pos;
								c_auto cam_right	   = math::normalize(math::cross(math::g::up, cam_forward));
								c_auto cam_up		   = math::cross(cam_forward, cam_right);

								c_auto mouse_world_delta  = float2{ math::dot(cam_right, hit_world_delta), math::dot(cam_up, hit_world_delta) };
								c_auto mouse_screen_delta = mouse_world_delta * screen_size / world_size;

								res_scale_ratio = (screen_size * 0.5f * 2 + mouse_screen_delta.x + mouse_screen_delta.y) / (screen_size * 0.5f * 2);
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
					c_auto& current_root = ui::detail::get_current_root();
					if (c_auto hit = detail::axis_drag_hit(anchor_pos, current_root.world_basis_u, ui::g::cam_world_pos, cam_forward, ui::g::mouse_ray_dir))
					{
						hit_world_pos	 = *hit;
						hit_axis_dir	 = current_root.world_basis_u;
						dragged_axis_idx = 0;
					}
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
						c_auto& current_root = ui::detail::get_current_root();
						if (c_auto hit = detail::axis_drag_hit(anchor_pos, current_root.world_basis_v, ui::g::cam_world_pos, cam_forward, ui::g::mouse_ray_dir))
						{
							hit_world_pos	 = *hit;
							hit_axis_dir	 = current_root.world_basis_v;
							dragged_axis_idx = 1;
						}
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
					c_auto& current_root = ui::detail::get_current_root();
					if (c_auto hit = detail::axis_drag_hit(anchor_pos, current_root.world_basis_u, ui::g::cam_world_pos, cam_forward, ui::g::mouse_ray_dir))
					{
						hit_world_pos	 = *hit;
						hit_axis_dir	 = current_root.world_basis_u;
						dragged_axis_idx = 2;
					}
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
			anchor_pos_prev				= world_pos;
			hit_world_pos_on_drag_start = hit_world_pos;
		}

		if (dragged_axis_idx >= 0)
		{
			c_auto drag_start_dist = math::dot(hit_world_pos_on_drag_start - anchor_pos, hit_axis_dir);
			c_auto current_dist	   = math::dot(hit_world_pos - anchor_pos, hit_axis_dir);

			if (std::abs(drag_start_dist) > math::g::epsilon_1e4)
			{
				res_scale_ratio[dragged_axis_idx] = current_dist / drag_start_dist;
			}
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

		static auto prev_mouse_sin_cos		 = float2{ 1, 0 };
		static auto drag_start_mouse_sin_cos = float2{ 1, 0 };

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
		c_auto mouse_sin_cos	   = normalize(float2{ mouse_center_offset.x, -mouse_center_offset.y });

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
								  | set_shape_pie_range(drag_start_mouse_sin_cos, mouse_sin_cos, std::fmod(drag_angle, 2.f * math::g::pi))
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
								  | set_shape_pie_range(rotate_sc(drag_start_mouse_sin_cos), rotate_sc(mouse_sin_cos), std::fmod(drag_angle, 2.f * math::g::pi))
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
								  | set_shape_pie_range(rotate_sc(drag_start_mouse_sin_cos), rotate_sc(mouse_sin_cos), std::fmod(drag_angle, 2.f * math::g::pi))
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
								  | set_shape_pie_range(rotate_sc(drag_start_mouse_sin_cos), rotate_sc(mouse_sin_cos), std::fmod(drag_angle, 2.f * math::g::pi))
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
				c_auto delta_sin = mouse_sin_cos.x * prev_mouse_sin_cos.y - mouse_sin_cos.y * prev_mouse_sin_cos.x;
				c_auto delta_cos = mouse_sin_cos.y * prev_mouse_sin_cos.y + mouse_sin_cos.x * prev_mouse_sin_cos.x;

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

				if (math::dot(axis_world, cam_forward) > 0.f)
				{
					axis_world = -axis_world;
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
			drag_start_mouse_sin_cos	  = mouse_sin_cos;
			gizmo_world_pos_on_drag_start = world_pos;
			quat_on_drag_start			  = quat;
		}

		prev_mouse_sin_cos = mouse_sin_cos;


		prev_hover = current_hover;
		prev_drag  = current_drag;


		return std::tuple{ res_quat, gizmo_world_pos_on_drag_start, is_drag_start, is_drag };
	}
}	 // namespace age::editor::gizmo
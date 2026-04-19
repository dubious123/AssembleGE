#pragma once
#include "age.hpp"

namespace age::editor
{
	void
	update_camera(auto& renderer, bool focused, platform::window_handle h_window) noexcept
	{
		g::cam.aspect_ratio = age::platform::get_client_width(h_window)
							/ static_cast<float>(age::platform::get_client_height(h_window));

		if (focused is_false)
		{
			auto cam_desc					  = renderer.get_camera_desc(0);
			cam_desc.perspective.aspect_ratio = g::cam.aspect_ratio;
			renderer.update_camera(0, cam_desc);
			renderer.set_main_camera(0);
			return;
		}

		using enum age::input::e::key_kind;
		c_auto& editor_input_ctx = *ui::g::p_input_ctx;

		g::cam.move = float2{
			editor_input_ctx.is_down(key_d) - editor_input_ctx.is_down(key_a),
			editor_input_ctx.is_down(key_w) - editor_input_ctx.is_down(key_s),
		};

		g::cam.look	  = editor_input_ctx.mouse_delta;
		g::cam.zoom	  = editor_input_ctx.wheel_delta;
		g::cam.sprint = editor_input_ctx.is_down(key_shift);

		c_auto dt_s = std::max(runtime::i_time.get_delta_time_s(), 1.f / 160);

		c_auto speed = g::cam.sprint ? g::cam.move_speed * g::cam.sprint_mult : g::cam.move_speed;

		c_auto move_smoothing_factor = 1.f - std::exp(-g::cam.move_smoothing * dt_s);
		c_auto look_smoothing_factor = 1.f - std::exp(-g::cam.look_smoothing * dt_s);
		c_auto zoom_smoothing_factor = 1.f - std::exp(-g::cam.zoom_smoothing * dt_s);

		g::cam.smoothed_move = math::lerp(g::cam.smoothed_move, g::cam.move, move_smoothing_factor);
		g::cam.smoothed_zoom = math::lerp(g::cam.smoothed_zoom, g::cam.zoom, zoom_smoothing_factor);

		auto look_target	 = editor_input_ctx.is_down(mouse_right) ? g::cam.look : float2::zero();
		g::cam.smoothed_look = math::lerp(g::cam.smoothed_look, look_target, look_smoothing_factor);

		auto pan_target		= editor_input_ctx.is_down(mouse_middle) ? g::cam.look : float2::zero();
		g::cam.smoothed_pan = math::lerp(g::cam.smoothed_pan, pan_target, look_smoothing_factor);

		g::cam.euler_deg.y += g::cam.smoothed_look.x * g::cam.sensitivity;
		g::cam.euler_deg.x += g::cam.smoothed_look.y * g::cam.sensitivity;
		g::cam.euler_deg.x	= std::clamp(g::cam.euler_deg.x, -89.f, 89.f);

		c_auto xm_look_quat = g::cam.euler_deg * age::g::degree_to_radian
							| simd::load()
							| simd::euler_to_quat();

		c_auto forward = simd::g::xm_forward_f4 | simd::rotate3(xm_look_quat) | simd::to<float3>();
		c_auto right   = simd::g::xm_right_f4 | simd::rotate3(xm_look_quat) | simd::to<float3>();
		c_auto up	   = simd::g::xm_up_f4 | simd::rotate3(xm_look_quat) | simd::to<float3>();

		g::cam.pos -= right * g::cam.smoothed_pan.x * g::cam.pan_speed * dt_s;
		g::cam.pos += up * g::cam.smoothed_pan.y * g::cam.pan_speed * dt_s;
		g::cam.pos += forward * g::cam.smoothed_zoom * g::cam.zoom_speed;
		g::cam.pos += (right * g::cam.smoothed_move.x + forward * g::cam.smoothed_move.y) * speed * dt_s;

		auto cam_desc					  = renderer.get_camera_desc(0);
		cam_desc.pos					  = g::cam.pos;
		cam_desc.quaternion				  = age::euler_deg_to_quat(g::cam.euler_deg);
		cam_desc.perspective.aspect_ratio = g::cam.aspect_ratio;
		renderer.update_camera(0, cam_desc);
		renderer.set_main_camera(0);
	}
}	 // namespace age::editor

namespace age::editor::detail
{
	void
	update_storage(auto& ecs_storage, auto& renderer) noexcept
	{
		using namespace ecs;
		// todo
		for (auto&& [light] : ecs_storage | each_entity<directional_light>())
		{
			renderer.update_directional_light(light.render_id,
											  {
												  .direction = age::math::normalize(light.direction),
												  .intensity = light.intensity,
												  .color	 = light.color,
											  },
											  light.cast_shadow);
		}

		for (auto&& [light, pos] : ecs_storage | each_entity<point_light, position>())
		{
			renderer.update_point_light(
				light.render_id,
				{
					.position  = pos,
					.range	   = light.range,
					.color	   = light.color,
					.intensity = light.intensity,
				},
				light.cast_shadow);
		}

		for (auto&& [light, pos] : ecs_storage | each_entity<spot_light, position>())
		{
			renderer.update_spot_light(
				light.render_id,
				{
					.position  = pos,
					.range	   = light.range,
					.direction = age::math::normalize(light.direction),
					.intensity = light.intensity,
					.color	   = light.color,
					.cos_inner = light.cos_inner,
					.cos_outer = light.cos_outer,
				},
				light.cast_shadow);
		}

		for (auto&& [pos, rot, scale, obj, mesh, mat] : ecs_storage
															| each_entity<const position, const rotation, const scale, const render_object, const mesh, const material>())
		{
			renderer.update_object(obj.render_id, pos, rot, scale);

			if (mat.is_opaque)
			{
				renderer.render_mesh(0, obj.render_id, mesh.render_id);
			}
			else
			{
				renderer.render_transparent_mesh(0, obj.render_id, mesh.render_id);
			}
		}
	}
}	 // namespace age::editor::detail

namespace age::editor
{
	void
	update_game(auto& ecs_game, auto& renderer) noexcept
	{
		c_auto& active_scene = g::current_game.scene_data_vec[g::current_game.current_active_scene_idx];

		for (c_auto& editor_storage : active_scene.storage_data_vec)
		{
			ecs_game.visit_storage_at(active_scene.code_idx, editor_storage.code_idx, AGE_FUNC(detail::update_storage), renderer);
		}
	}
}	 // namespace age::editor

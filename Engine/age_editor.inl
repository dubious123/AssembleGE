#pragma once
#include "age.hpp"

namespace age::editor
{
	void
	update_camera(auto& renderer, bool focused, platform::window_handle h_window) noexcept
	{
		auto& current_scene = g::current_game.get_current_scene();
		auto& cam			= current_scene.cam;
		cam.aspect_ratio	= age::platform::get_client_width(h_window)
							/ static_cast<float>(age::platform::get_client_height(h_window));

		if (focused is_false)
		{
			auto cam_desc					  = renderer.get_camera_desc(0);
			cam_desc.perspective.aspect_ratio = cam.aspect_ratio;
			renderer.update_camera(0, cam_desc);
			renderer.set_main_camera(0);
			return;
		}

		using enum age::input::e::key_kind;
		c_auto& editor_input_ctx = *ui::g::p_input_ctx;

		cam.move = float2{
			editor_input_ctx.is_down(key_d) - editor_input_ctx.is_down(key_a),
			editor_input_ctx.is_down(key_w) - editor_input_ctx.is_down(key_s),
		};

		cam.look   = editor_input_ctx.mouse_delta;
		cam.zoom   = editor_input_ctx.wheel_delta;
		cam.sprint = editor_input_ctx.is_down(key_shift);

		c_auto dt_s = std::max(runtime::i_time.get_delta_time_s(), 1.f / 160);

		c_auto speed = cam.sprint ? cam.move_speed * cam.sprint_mult : cam.move_speed;

		c_auto move_smoothing_factor = 1.f - std::exp(-cam.move_smoothing * dt_s);
		c_auto look_smoothing_factor = 1.f - std::exp(-cam.look_smoothing * dt_s);
		c_auto zoom_smoothing_factor = 1.f - std::exp(-cam.zoom_smoothing * dt_s);

		cam.smoothed_move = math::lerp(cam.smoothed_move, cam.move, move_smoothing_factor);
		cam.smoothed_zoom = math::lerp(cam.smoothed_zoom, cam.zoom, zoom_smoothing_factor);

		auto look_target  = editor_input_ctx.is_down(mouse_right) ? cam.look : float2::zero();
		cam.smoothed_look = math::lerp(cam.smoothed_look, look_target, look_smoothing_factor);

		auto pan_target	 = editor_input_ctx.is_down(mouse_middle) ? cam.look : float2::zero();
		cam.smoothed_pan = math::lerp(cam.smoothed_pan, pan_target, look_smoothing_factor);

		cam.euler_deg.y += cam.smoothed_look.x * cam.sensitivity;
		cam.euler_deg.x += cam.smoothed_look.y * cam.sensitivity;
		cam.euler_deg.x	 = std::clamp(cam.euler_deg.x, -89.f, 89.f);

		c_auto xm_look_quat = cam.euler_deg * age::g::degree_to_radian
							| simd::load()
							| simd::euler_to_quat();

		c_auto forward = simd::g::xm_forward_f4 | simd::rotate3(xm_look_quat) | simd::to<float3>();
		c_auto right   = simd::g::xm_right_f4 | simd::rotate3(xm_look_quat) | simd::to<float3>();
		c_auto up	   = simd::g::xm_up_f4 | simd::rotate3(xm_look_quat) | simd::to<float3>();

		cam.pos -= right * cam.smoothed_pan.x * cam.pan_speed * dt_s;
		cam.pos += up * cam.smoothed_pan.y * cam.pan_speed * dt_s;
		cam.pos += forward * cam.smoothed_zoom * cam.zoom_speed;
		cam.pos += (right * cam.smoothed_move.x + forward * cam.smoothed_move.y) * speed * dt_s;

		auto cam_desc					  = renderer.get_camera_desc(0);
		cam_desc.pos					  = cam.pos;
		cam_desc.quaternion				  = age::euler_deg_to_quat(cam.euler_deg);
		cam_desc.perspective.aspect_ratio = cam.aspect_ratio;
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
		for (auto&& [mesh] : ecs_storage | each_entity<mesh>())
		{
			if (runtime::is_handle_invalid(mesh.h_mesh)) { continue; }

			if (asset::registry::is_registered<asset::e::kind::mesh_baked>(mesh.h_mesh) is_false)
			{
				mesh.update_h_mesh(asset::handle{});
			}
		}

		for (auto&& [mat] : ecs_storage | each_entity<material>())
		{
			if (runtime::is_handle_invalid(mat.h_mat)) { continue; }

			if (asset::registry::is_registered<asset::e::kind::material>(mat.h_mat) is_false)
			{
				mat.update_h_mat(asset::handle{});
			}
		}

		for (auto&& [light] : ecs_storage | each_entity<directional_light>())
		{
			renderer.update_directional_light(light.render_id,
											  { .direction	 = age::math::normalize(light.direction),
												.intensity	 = light.intensity,
												.color		 = light.color,
												.cast_shadow = light.cast_shadow });
		}

		for (auto&& [light, pos] : ecs_storage | each_entity<point_light, position>())
		{
			renderer.update_point_light(
				light.render_id,
				{ .position	   = pos,
				  .range	   = light.range,
				  .color	   = light.color,
				  .intensity   = light.intensity,
				  .cast_shadow = light.cast_shadow });
		}

		for (auto&& [light, pos] : ecs_storage | each_entity<spot_light, position>())
		{
			renderer.update_spot_light(
				light.render_id,
				{ .position	   = pos,
				  .range	   = light.range,
				  .direction   = age::math::normalize(light.direction),
				  .intensity   = light.intensity,
				  .color	   = light.color,
				  .cos_inner   = light.cos_inner,
				  .cos_outer   = light.cos_outer,
				  .cast_shadow = light.cast_shadow });
		}
	}
}	 // namespace age::editor::detail

namespace age::editor
{
	void
	update_game(auto& ecs_game, auto& renderer) noexcept
	{
		c_auto& active_scene = g::current_game.scene_data_vec[g::current_game.current_active_scene_idx];

		using enum age::asset::e::kind;
		{
			auto& pool = asset::pool_of<mesh_baked>();
			for (auto it = pool.begin(); it != pool.end(); ++it)
			{
				auto  h_mesh = asset::handle::make<mesh_baked>(it.idx<uint32>());
				auto& entry	 = h_mesh.get_entry<mesh_baked>();

				if (entry.ref_counter == 0)
				{
					asset::mesh_baked::full_unload(h_mesh, renderer);

					if (asset::registry::is_registered<mesh_baked>(h_mesh) is_false)
					{
						asset::destroy_entry<mesh_baked>(h_mesh);
					}
				}

				if (entry.ref_counter > 0)
				{
					asset::mesh_baked::gpu_load(h_mesh, renderer);
				}
			}
		}

		{
			auto& pool = asset::pool_of<material>();
			for (auto it = pool.begin(); it != pool.end(); ++it)
			{
				auto  h		= asset::handle::make<material>(it.idx<uint32>());
				auto& entry = h.get_entry<material>();

				if (entry.ref_counter == 0)
				{
					asset::material::full_unload(h, renderer);

					if (asset::registry::is_registered<material>(h) is_false)
					{
						asset::destroy_entry<material>(h);
					}
				}

				if (entry.ref_counter > 0)
				{
					asset::material::load(h, renderer);
				}
			}
		}

		{
			auto& pool = asset::pool_of<texture>();
			for (auto it = pool.begin(); it != pool.end(); ++it)
			{
				auto  h		= asset::handle::make<texture>(it.idx<uint32>());
				auto& entry = h.get_entry<texture>();

				if (entry.ref_counter == 0)
				{
					asset::texture::full_unload(h, renderer);

					if (asset::registry::is_registered<texture>(h) is_false)
					{
						asset::destroy_entry<texture>(h);
					}
				}

				if (entry.ref_counter > 0)
				{
					asset::texture::gpu_load(h, renderer);
				}
			}
		}


		for (c_auto& editor_storage : active_scene.storage_data_vec)
		{
			ecs_game.visit_storage_at(active_scene.code_idx, editor_storage.code_idx, AGE_FUNC(detail::update_storage), renderer);
		}
	}
}	 // namespace age::editor

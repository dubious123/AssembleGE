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

namespace age::editor
{
	namespace detail
	{
		inline uint32
		find_arch_idx(storage_editor_data& editor_storage, uint64 archetype) noexcept
		{
			for (auto&& [arch_idx, arch_data] : editor_storage.archetype_data_vec | std::views::enumerate)
			{
				if (arch_data.archetype == archetype)
				{
					return static_cast<uint32>(arch_idx);
				}
			}

			return get_invalid_idx<uint32>();
		}
	}	 // namespace detail

	void
	add_components(auto& storage, auto& renderer, storage_editor_data& editor_storage, auto ent_id, auto archetype) noexcept
	{
		using t_storage			 = BARE_OF(storage);
		using t_ent_id			 = typename t_storage::t_ent_id;
		using t_archetype		 = typename t_storage::t_archetype;
		using t_archetype_traits = typename t_storage::t_archetype_traits;

		static_assert(std::is_same_v<t_ent_id, BARE_OF(ent_id)>);
		static_assert(std::is_same_v<t_archetype, BARE_OF(archetype)>);


		for (auto storage_cmp_idx : age::views::each_set_bit_idx(archetype))
		{
			t_archetype_traits::visit_component(storage_cmp_idx, AGE_LAMBDA(<typename t_cmp>(auto& entities, auto ent_id, auto& renderer), { entities.add_component<t_cmp>(ent_id, get_ecs_context(renderer)); }), storage, ent_id, renderer);
		}

		c_auto new_archetype = storage.get_archetype(ent_id);

		detail::re_register_entity(editor_storage, ent_id, new_archetype);
	}

	void
	new_entity(auto& storage, auto& renderer, storage_editor_data& editor_storage, uint32 arch_editor_idx, auto archetype) noexcept
	{
		using t_storage	  = BARE_OF(storage);
		using t_archetype = typename t_storage::t_archetype;

		auto new_ent_id = storage.new_entity(static_cast<t_archetype>(archetype), get_ecs_context(renderer));

		auto& arch_data = editor_storage.archetype_data_vec[arch_editor_idx];

		editor_storage.id_to_editor_location_map[new_ent_id] = std::pair{ arch_editor_idx, arch_data.entity_data_vec.size() };

		arch_data.entity_data_vec.emplace_back(entity_editor_data{
			.id	  = new_ent_id,
			.name = util::to_fixed_str<config::max_entity_name_len>(std::format("new_entity_{}", editor_storage.entity_count++)) });
	}

	void
	new_entity(auto& storage, auto& renderer, storage_editor_data& editor_storage, auto archetype) noexcept
	{
		if (auto arch_idx = detail::find_arch_idx(editor_storage, archetype);
			arch_idx != get_invalid_idx<uint32>())
		{
			return new_entity(storage, renderer, editor_storage, arch_idx, archetype);
		}

		auto& arch_data		= editor_storage.archetype_data_vec.emplace_back();
		arch_data.archetype = archetype;
		util::integral_to_str<16>(arch_data.name, archetype);

		new_entity(storage, renderer, editor_storage, editor_storage.archetype_data_vec.size<uint32>() - 1, archetype);
	}

	void
	copy_entity(auto& storage, auto& renderer, storage_editor_data& editor_storage, auto ecs_ent_id, uint32 arch_editor_idx) noexcept
	{
		using t_storage	  = BARE_OF(storage);
		using t_archetype = typename t_storage::t_archetype;

		auto new_ent_id = storage.copy_entity(ecs_ent_id, get_ecs_context(renderer));

		auto& arch_data = editor_storage.archetype_data_vec[arch_editor_idx];

		editor_storage.id_to_editor_location_map[new_ent_id] = std::pair{ arch_editor_idx, arch_data.entity_data_vec.size() };


		arch_data.entity_data_vec.emplace_back(entity_editor_data{
			.id	  = new_ent_id,
			.name = util::to_fixed_str<config::max_entity_name_len>(
				std::format("{}_clone", arch_data.entity_data_vec[editor_storage.id_to_editor_location_map[ecs_ent_id].second].name.data())) });

		// return new_ent_id;
	}

	void
	copy_entity(auto& storage, auto& renderer, storage_editor_data& editor_storage, auto ecs_ent_id) noexcept
	{
		using t_storage			 = BARE_OF(storage);
		using t_ent_id			 = typename t_storage::t_ent_id;
		using t_archetype		 = typename t_storage::t_archetype;
		using t_archetype_traits = typename t_storage::t_archetype_traits;

		auto archetype = storage.get_archetype(static_cast<t_ent_id>(ecs_ent_id));
		if (auto arch_idx = detail::find_arch_idx(editor_storage, archetype);
			arch_idx != get_invalid_idx<uint32>())
		{
			return copy_entity(storage, renderer, editor_storage, static_cast<t_ent_id>(ecs_ent_id), arch_idx);
		}

		AGE_UNREACHABLE();
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

		for (auto&& [env_light] : ecs_storage | each_entity_soft<env_light>())
		{
			if (runtime::is_handle_invalid(env_light.h_env_light)) { continue; }

			if (asset::registry::is_registered<asset::e::kind::env_light>(env_light.h_env_light) is_false)
			{
				env_light.update_h_env_light(asset::handle{});
			}
		}
	}
}	 // namespace age::editor::detail

namespace age::editor
{
	void
	update_game(auto& ecs_game, auto& renderer) noexcept
	{
		using enum age::asset::e::kind;
		using enum age::input::e::key_kind;

		auto& active_scene = g::current_game.scene_data_vec[g::current_game.current_active_scene_idx];

		if (ui::g::p_input_ctx->is_ctrl_down() and ui::g::p_input_ctx->is_pressed(key_d))
		{
			for (auto&& [storage_code_idx, vec] : g::select_vec | std::views::enumerate /*editor::all_selected()*/)
			{
				for (auto id : vec)
				{
					ecs_game.visit_storage_at(active_scene.code_idx, static_cast<uint32>(storage_code_idx), AGE_FUNC(copy_entity), renderer, active_scene.find_storage_data(static_cast<uint32>(storage_code_idx)), id);
				}
				// editor::command::copy(g::current_select_kind, ecs_game, renderer);
			}
		}

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

				auto need_update = false;

				for (auto& h_tex : entry.all_textures() | views::deref)
				{
					if (runtime::is_handle_invalid(h_tex)) { continue; }

					if (asset::registry::is_registered<texture>(h_tex) is_false)
					{
						asset::texture::full_unload(h_tex, renderer);
						h_tex = {};

						need_update = true;
					}
				}

				if (entry.ref_counter > 0 and need_update)
				{
					renderer.update_material(h);
				}
			}
		}

		{
			auto& pool = asset::pool_of<env_light>();
			for (auto it = pool.begin(); it != pool.end(); ++it)
			{
				auto  h		= asset::handle::make<env_light>(it.idx<uint32>());
				auto& entry = h.get_entry<env_light>();

				if (entry.ref_counter == 0)
				{
					asset::env_light::full_unload(h, renderer);

					if (asset::registry::is_registered<env_light>(h) is_false)
					{
						asset::destroy_entry<env_light>(h);
					}
				}

				if (entry.ref_counter > 0)
				{
					asset::env_light::gpu_load(h, renderer);
				}
			}
		}

		for (c_auto& editor_storage : active_scene.storage_data_vec)
		{
			ecs_game.visit_storage_at(active_scene.code_idx, editor_storage.code_idx, AGE_FUNC(detail::update_storage), renderer);
		}

		if (ui::g::p_input_ctx->is_pressed(input::e::key_kind::key_ctrl) and ui::g::p_input_ctx->is_pressed(input::e::key_kind::key_c))
		{
			editor::save_game(ecs_game, renderer);
		}
	}
}	 // namespace age::editor

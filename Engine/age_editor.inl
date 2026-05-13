#pragma once
#include "age.hpp"

namespace age::editor
{
	void
	update_camera(auto& renderer, bool update, platform::window_handle h_window) noexcept
	{
		auto& current_scene = g::current_game.get_current_scene();
		auto& cam			= current_scene.cam;
		cam.aspect_ratio	= age::platform::get_client_width(h_window)
							/ static_cast<float>(age::platform::get_client_height(h_window));

		if (update is_false)
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
		cam_desc.perspective.fov_y		  = cam.fov_y;
		renderer.update_camera(0, cam_desc);
		renderer.set_main_camera(0);
	}

	void
	focus_camera(auto& renderer, const float3& aabb_min, const float3& aabb_max) noexcept
	{
		c_auto origin = (aabb_min + aabb_max) * 0.5f;

		c_auto radius = (aabb_max - origin) | simd::load() | simd::length_3() | simd::to<float>();

		auto& cam = g::current_game.get_current_scene().cam;

		c_auto xm_look_quat = cam.euler_deg * age::g::degree_to_radian
							| simd::load()
							| simd::euler_to_quat();

		c_auto forward = simd::g::xm_forward_f4 | simd::rotate3(xm_look_quat) | simd::to<float3>();

		auto distance = float{};

		if (radius < age::g::epsilon_1e6)
		{
			distance = 1.f / std::sin(cam.fov_y * 0.5f);
		}
		else
		{
			c_auto fov_x_half = std::atan(1.f * std::tan(cam.fov_y * 0.5f) * cam.aspect_ratio);

			distance = radius / std::sin(std::min(cam.fov_y * 0.5f, fov_x_half)) * 1.15f;
		}

		cam.pos = origin - forward * distance;

		auto cam_desc = renderer.get_camera_desc(0);
		cam_desc.pos  = cam.pos;
		renderer.update_camera(0, cam_desc);
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
	copy_entity(auto& storage, auto& renderer, storage_editor_data& editor_storage, auto ecs_ent_id) noexcept
	{
		using t_storage = BARE_OF(storage);
		using t_ent_id	= typename t_storage::t_ent_id;

		auto&& [src_arch_idx, src_ent_idx] = editor_storage.id_to_editor_location_map[ecs_ent_id];

		auto new_ent_id = storage.copy_entity(static_cast<t_ent_id>(ecs_ent_id), get_ecs_context(renderer));

		auto& arch_data = editor_storage.archetype_data_vec[src_arch_idx];

		editor_storage.id_to_editor_location_map[new_ent_id] = std::pair{ src_arch_idx, arch_data.entity_data_vec.size() };

		arch_data.entity_data_vec.emplace_back(entity_editor_data{
			.id	  = new_ent_id,
			.name = util::to_fixed_str<config::max_entity_name_len>(
				std::format("{}_clone", arch_data.entity_data_vec[src_ent_idx].name.data())) });

		// return new_ent_id;
	}

	// return pair { aabb_min, aabb_max }
	decltype(auto)
	handle_entity_focus(auto& storage, auto& renderer, storage_editor_data& editor_storage, auto ecs_ent_id) noexcept
	{
		using t_storage = BARE_OF(storage);
		using t_ent_id	= typename t_storage::t_ent_id;

		auto&& [src_arch_idx, src_ent_idx] = editor_storage.id_to_editor_location_map[ecs_ent_id];

		auto& arch_data = editor_storage.archetype_data_vec[src_arch_idx];

		if (storage.has_component<ecs::render_object, ecs::mesh>(static_cast<t_ent_id>(ecs_ent_id)))
		{
			auto&& [obj, mesh] = storage.get_component<const ecs::render_object, const ecs::mesh>(static_cast<t_ent_id>(ecs_ent_id));

			if (runtime::is_handle_invalid(mesh.h_mesh) is_false)
			{
				c_auto& entry = mesh.h_mesh.get_entry<asset::e::kind::mesh_baked>();

				auto&& [xm_aabb_min, xm_aabb_max, xm_trans] = simd::load(entry.aabb_min, entry.aabb_max, renderer.get_object_transform_matrix(obj.render_id));
				return std::pair{ simd::transform3(xm_trans, xm_aabb_min) | simd::to<float3>(),
								  simd::transform3(xm_trans, xm_aabb_max) | simd::to<float3>() };
			}
			else
			{
				c_auto pos = simd::transform3(simd::load(renderer.get_object_transform_matrix(obj.render_id)), simd::load(float3::zero()))
						   | simd::to<float3>();
				return std::pair{ pos, pos };
			}
		}

		if (storage.has_component<ecs::position>(static_cast<t_ent_id>(ecs_ent_id)))
		{
			auto&& [pos] = storage.get_component<const ecs::position>(static_cast<t_ent_id>(ecs_ent_id));
			return std::pair{ static_cast<float3>(pos), static_cast<float3>(pos) };
		}

		return std::pair{ float3{ std::numeric_limits<float>::max() }, float3{ std::numeric_limits<float>::lowest() } };
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

		static auto raycast_req_vec = std::array<uint32, graphics::g::frame_buffer_count>{};


		c_auto raycast_res		 = renderer.get_raycast_result(raycast_req_vec[graphics::g::frame_buffer_idx]);
		auto   need_object_click = AGE_IS_INVALID_IDX(raycast_res.object_id) is_false and ui::g::p_input_ctx->is_released(mouse_left);

		{
			c_auto uv = ui::g::p_input_ctx->mouse_pos / float2{ ui::g::window_width, ui::g::window_height };

			c_auto ndc = float2{ uv.x * 2.0f - 1.0f, 1.0f - uv.y * 2.0f };

			c_auto& data = renderer.get_camera_data(0);

			auto world_pos	= simd::transform4(data.view_proj_inv | simd::load(), float4(ndc.x, ndc.y, 0.f, 1.f) | simd::load()) | simd::to<float4>();
			world_pos.xyz  /= world_pos.w;

			raycast_req_vec[graphics::g::frame_buffer_idx] = renderer.request_raycast(active_scene.cam.pos, math::normalize(world_pos.xyz - active_scene.cam.pos), std::numeric_limits<float>::max());

			ecs_game.visit_scene_at(
				active_scene.code_idx,
				[&](auto& scene) {
					for (auto i = 0u; i < scene.storage_count(); ++i)
					{
						scene.visit_storage_at(i, [&](auto& entities) {
							if (need_object_click is_false) { return; }

							if constexpr (entities.has_component<ecs::render_object>())
							{
								for (auto&& [obj, ent_id] : entities | ecs::each_entity<ecs::render_object, ecs::sv_entity_id>())
								{
									if (obj.render_id != raycast_res.object_id) { continue; }

									if (ui::g::p_input_ctx->is_shift_down())
									{
										add_select(e::select_kind::entity, i, ent_id);
									}
									else
									{
										clear_select();
										add_select(e::select_kind::entity, i, ent_id);
									}

									need_object_click = false;
									return;
								}
							}
						});
					}
				});
		}


		c_auto need_copy = ui::g::p_input_ctx->is_ctrl_down() and ui::g::p_input_ctx->is_pressed(key_d);

		if (g::current_select_kind == e::select_kind::entity)
		{
			auto   aabb_min	  = float3::max();
			auto   aabb_max	  = float3::lowest();
			c_auto need_focus = g::set_focus or ui::g::p_input_ctx->is_pressed(key_f);

			for (auto&& [storage_code_idx, vec] : g::select_vec | std::views::enumerate /*editor::all_selected()*/)
			{
				if (storage_code_idx >= ecs_game.scene_count()) { continue; }

				ecs_game.visit_storage_at(
					active_scene.code_idx, static_cast<uint32>(storage_code_idx),
					[&](auto& entities) {
						for (auto ecs_ent_id : vec)
						{
							if (need_copy)
							{
								copy_entity(entities, renderer, active_scene.find_storage_data(static_cast<uint32>(storage_code_idx)), ecs_ent_id);
							}
							if (need_focus)
							{
								auto&& [min, max] = handle_entity_focus(entities, renderer, active_scene.find_storage_data(static_cast<uint32>(storage_code_idx)), ecs_ent_id);
								aabb_min		  = float3::min(aabb_min, min);
								aabb_max		  = float3::max(aabb_max, max);
							}
						}
					});

				// editor::command::copy(g::current_select_kind, ecs_game, renderer);
			}

			if (aabb_min <= aabb_max)
			{
				focus_camera(renderer, aabb_min, aabb_max);
			}
		}

		g::set_focus = false;


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

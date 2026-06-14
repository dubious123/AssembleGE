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

		cam.move = editor_input_ctx.is_down(mouse_right)
					 ? float2{
						   editor_input_ctx.is_down(key_d) - editor_input_ctx.is_down(key_a),
						   editor_input_ctx.is_down(key_w) - editor_input_ctx.is_down(key_s),
					   }
					 : float2::zero();

		cam.look   = editor_input_ctx.mouse_delta;
		cam.zoom   = editor_input_ctx.wheel_delta;
		cam.sprint = editor_input_ctx.is_down(key_shift);

		c_auto dt_s = std::min(runtime::i_time.get_delta_time_s(), 1.f / 160);

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

		c_auto forward = simd::rotate3(xm_look_quat, simd::g::xm_forward_f4) | simd::to<float3>();
		c_auto right   = simd::rotate3(xm_look_quat, simd::g::xm_right_f4) | simd::to<float3>();
		c_auto up	   = simd::rotate3(xm_look_quat, simd::g::xm_up_f4) | simd::to<float3>();

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
	calc_entity_aabb(auto& storage, auto& renderer, storage_editor_data& editor_storage, auto ecs_ent_id) noexcept
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

				c_auto aabb_min = simd::transform3(xm_trans, xm_aabb_min) | simd::to<float3>();
				c_auto aabb_max = simd::transform3(xm_trans, xm_aabb_max) | simd::to<float3>();
				return std::pair{ age::min(aabb_min, aabb_max), age::max(aabb_min, aabb_max) };
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
	update_storage(auto& ecs_storage, auto& renderer, auto& update_storage_ctx) noexcept
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

		for (auto&& [cmp] : ecs_storage | each_entity_soft<bloom>())
		{
			if (AGE_IS_INVALID_ID(cmp.render_id)) { continue; }

			renderer.update_bloom(cmp.render_id, { .threshold = cmp.threshold,
												   .knee	  = cmp.knee,
												   .intensity = cmp.intensity,
												   .radius	  = cmp.radius,
												   .tint	  = cmp.tint });

			renderer.set_bloom_active(cmp.render_id, cmp.active);
		}


		if (update_storage_ctx.gi_active_found is_false)
		{
			for (auto&& [cmp] : ecs_storage | each_entity_soft<gi_config>())
			{
				if (cmp.enable_ddgi)
				{
					update_storage_ctx.gi_active_found = true;
					if (renderer.gibs_enabled())
					{
						renderer.disable_gibs();
					}
					if (renderer.ddgi_enabled() is_false)
					{
						renderer.enable_ddgi({
							.probe_per_level_axis = cmp.ddgi_probe_per_level_axis,
							.base_probe_spacing	  = cmp.ddgi_base_probe_spacing,
							.level_count		  = cmp.ddgi_level_count,
						});
					}

					break;
				}
				else if (cmp.enable_gibs)
				{
					update_storage_ctx.gi_active_found = true;
					if (renderer.ddgi_enabled())
					{
						renderer.disable_ddgi();
					}
					if (renderer.gibs_enabled() is_false)
					{
						renderer.enable_gibs({
							.max_surfel_count		= cmp.max_surfel_count,
							.debug_flags			= cmp.gibs_debug_flags,
							.lock_origin			= cmp.gibs_lock_origin,
							.cell_count				= cmp.gibs_cell_count,
							.outer_layer_count		= cmp.gibs_outer_layer_count,
							.cell_size				= cmp.gibs_cell_size,
							.outer_cell_size_factor = cmp.outer_cell_size_factor,
						});
					}

					break;
				}
			}
		}


		for (auto&& [cmp] : ecs_storage | each_entity_soft<editor_cam_setting>())
		{
			auto& cam = g::current_game.get_current_scene().cam;

			cam.move_speed	   = cmp.move_speed;
			cam.sprint_mult	   = cmp.sprint_mult;
			cam.sensitivity	   = cmp.sensitivity;
			cam.zoom_speed	   = cmp.zoom_speed;
			cam.zoom_distance  = cmp.zoom_distance;
			cam.pan_speed	   = cmp.pan_speed;
			cam.move_smoothing = cmp.move_smoothing;
			cam.look_smoothing = cmp.look_smoothing;
			cam.zoom_smoothing = cmp.zoom_smoothing;
		}
	}
}	 // namespace age::editor::detail

// gizmo
namespace age::editor
{
	void
	widget_transform(auto& ecs_game, auto& renderer, const float3& world_pos, const float4& quat) noexcept
	{
		using namespace ui;
		using namespace ui::widget;
		using enum input::e::key_kind;

		constexpr c_auto world_size_base = 1.f;
		constexpr c_auto screen_size	 = 180.f;

		c_auto& active_scene = g::current_game.scene_data_vec[g::current_game.current_active_scene_idx];
		c_auto& cam			 = active_scene.cam;

		c_auto xm_look_quat = cam.euler_deg * age::g::degree_to_radian
							| simd::load()
							| simd::euler_to_quat();

		c_auto cam_forward = simd::rotate3(xm_look_quat, simd::g::xm_forward_f4) | simd::to<float3>();


		if (ui::g::p_input_ctx->is_down(mouse_right) is_false)
		{
			if (ui::g::p_input_ctx->is_down(key_q))
			{
				g::gizmo_transform_mode = e::transform_mode_kind::select;
			}
			else if (ui::g::p_input_ctx->is_down(key_w))
			{
				g::gizmo_transform_mode = e::transform_mode_kind::translation;
			}
			else if (ui::g::p_input_ctx->is_down(key_e))
			{
				g::gizmo_transform_mode = e::transform_mode_kind::rotation;
			}
			else if (ui::g::p_input_ctx->is_down(key_r))
			{
				g::gizmo_transform_mode = e::transform_mode_kind::scale;
			}
		}

		c_auto mode		   = g::gizmo_transform_mode;
		c_auto translation = mode == e::transform_mode_kind::translation
							   ? gizmo::translation(cam.fov_y, cam.pos, cam_forward, world_pos, quat, screen_size)
							   : float3::zero();

		c_auto && [ rotation_res, pivot_pos, rotation_drag_start, rotation_dragging ] = mode == e::transform_mode_kind::rotation
																						  ? gizmo::rotation(cam.fov_y, cam.pos, cam_forward, world_pos, quat, screen_size)
																						  : std::tuple{ math::g::quaternion_identity, float3::zero(), false, false };

		c_auto && [ scale_res, scale_drag_start, scale_dragging ] = mode == e::transform_mode_kind::scale
																	  ? gizmo::scale(cam.fov_y, cam.pos, cam_forward, world_pos, quat, screen_size)
																	  : std::tuple{ float3::one(), false, false };

		if (rotation_dragging is_false)
		{
			for (auto& map : g::rotation_snapshot_vec)
			{
				map.clear();
			}
		}

		if (scale_dragging is_false)
		{
			for (auto& map : g::scale_snapshot_vec)
			{
				map.clear();
			}
		}

		g::scale_snapshot_vec.resize(g::select_vec.size());
		g::rotation_snapshot_vec.resize(g::select_vec.size());

		for (auto&& [storage_code_idx, vec] : g::select_vec | std::views::enumerate /*editor::all_selected()*/)
		{
			if (storage_code_idx >= ecs_game.scene_count()) { continue; }

			ecs_game.visit_storage_at(
				active_scene.code_idx, static_cast<uint32>(storage_code_idx),
				[&](auto& entities) {
					using t_storage = BARE_OF(entities);
					using t_ent_id	= typename t_storage::t_ent_id;
					for (auto ecs_ent_id : vec)
					{
						c_auto id = static_cast<t_ent_id>(ecs_ent_id);
						if (mode == e::transform_mode_kind::translation)
						{
							if (entities.has_component<ecs::position>(id))
							{
								auto&& [pos]  = entities.get_component<ecs::position>(id);
								pos			 += translation;
							}
						}
						else if (mode == e::transform_mode_kind::rotation)
						{
							if (entities.has_component<ecs::rotation, ecs::position>(id))
							{
								AGE_ASSERT(rotation_dragging is_false or g::rotation_snapshot_vec[active_scene.code_idx].contains(ecs_ent_id));

								auto& snap			   = g::rotation_snapshot_vec[active_scene.code_idx][ecs_ent_id];
								auto&& [pos, rotation] = entities.get_component<ecs::position, ecs::rotation>(id);
								if (rotation_drag_start)
								{
									snap.position = pos;
									snap.rotation = rotation;
								}
								else if (rotation_dragging)
								{
									pos		 = math::rotate_around(rotation_res, snap.position, pivot_pos);
									rotation = math::quat_mul(rotation_res, snap.rotation);
								}
							}
							else if (entities.has_component<ecs::rotation>(id))
							{
								auto& snap		  = g::rotation_snapshot_vec[active_scene.code_idx][ecs_ent_id];
								auto&& [rotation] = entities.get_component<ecs::rotation>(id);
								if (rotation_drag_start)
								{
									snap.rotation = rotation;
								}
								else if (rotation_dragging)
								{
									rotation = math::quat_mul(rotation_res, snap.rotation);
								}
							}
						}
						else if (mode == e::transform_mode_kind::scale)
						{
							if (entities.has_component<ecs::scale>(id))
							{
								auto&& [scale] = entities.get_component<ecs::scale>(id);
								if (scale_drag_start)
								{
									g::scale_snapshot_vec[active_scene.code_idx][ecs_ent_id] = scale;
								}
								else if (scale_dragging)
								{
									AGE_ASSERT(g::scale_snapshot_vec[active_scene.code_idx].contains(ecs_ent_id));
									scale = g::scale_snapshot_vec[active_scene.code_idx][ecs_ent_id] * scale_res;
								}
							}
						}
					}
				});


			// editor::command::copy(g::current_select_kind, ecs_game, renderer);
		}
	}
}	 // namespace age::editor

namespace age::editor
{
	void
	update_game(auto& ecs_game, auto& renderer) noexcept
	{
		using enum age::asset::e::kind;

		using enum age::input::e::key_kind;

		auto& active_scene = g::current_game.scene_data_vec[g::current_game.current_active_scene_idx];

		static auto raycast_req_vec = std::array<uint32, global::frame_buffer_count>{};

		c_auto raycast_res = renderer.get_raycast_result(raycast_req_vec[global::i_graphics.get_frame_buffer_idx]);

		auto need_object_click = AGE_IS_INVALID_IDX(raycast_res.object_id) is_false and ui::g::p_input_ctx->is_released(mouse_left) and (ui::is_any_focused() is_false);

		{
			c_auto target_world = math::ndc_to_world(renderer.get_camera_data(0).view_proj_inv, float3{ math::screen_to_ndc(float2{ ui::g::window_width, ui::g::window_height }, ui::g::p_input_ctx->mouse_pos), 0.f });

			raycast_req_vec[global::i_graphics.get_frame_buffer_idx] = renderer.request_raycast(active_scene.cam.pos, math::normalize(target_world - active_scene.cam.pos), std::numeric_limits<float>::max());

			if (ui::is_any_hovered() is_false)
			{
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
		}


		c_auto need_copy = ui::g::p_input_ctx->is_ctrl_down() and ui::g::p_input_ctx->is_pressed(key_d);

		if (g::current_select_kind == e::select_kind::entity)
		{
			auto   aabb_min	  = float3::max();
			auto   aabb_max	  = float3::lowest();
			auto   quat_sum	  = float4::zero();
			c_auto need_focus = g::set_focus or ui::g::p_input_ctx->is_pressed(key_f);

			for (auto&& [storage_code_idx, vec] : g::select_vec | std::views::enumerate /*editor::all_selected()*/)
			{
				if (storage_code_idx >= ecs_game.scene_count()) { continue; }

				ecs_game.visit_storage_at(
					active_scene.code_idx, static_cast<uint32>(storage_code_idx),
					[&](auto& entities) {
						using t_storage = BARE_OF(entities);
						using t_ent_id	= typename t_storage::t_ent_id;
						for (auto ecs_ent_id : vec)
						{
							c_auto id = static_cast<t_ent_id>(ecs_ent_id);
							if (need_copy)
							{
								copy_entity(entities, renderer, active_scene.find_storage_data(static_cast<uint32>(storage_code_idx)), id);
							}

							auto&& [min, max] = calc_entity_aabb(entities, renderer, active_scene.find_storage_data(static_cast<uint32>(storage_code_idx)), id);
							aabb_min		  = age::min(aabb_min, min);
							aabb_max		  = age::max(aabb_max, max);

							if (entities.has_component<ecs::render_object, ecs::mesh>(id))
							{
								auto&& [obj, mesh] = entities.get_component<const ecs::render_object, const ecs::mesh>(id);

								if (AGE_IS_INVALID_ID(obj.render_id) or runtime::is_handle_invalid(mesh.h_mesh)) { continue; }

								if (c_auto& entry = mesh.h_mesh.get_entry<asset::e::kind::mesh_baked>();
									entry.is_gpu_loaded())
								{
									renderer.render_selection_outline(obj.render_id, mesh.h_mesh, math::srgb_to_linear(float4{ 1, 0, 0, 1 }), 2.f, 0.f);
								}
							}

							if (g::gizmo_space == e::transform_space_kind::local)
							{
								if (entities.has_component<ecs::rotation>(id))
								{
									auto&& [quat] = entities.get_component<const ecs::rotation>(id);
									if (math::dot(quat_sum, quat) >= 0.f)
									{
										quat_sum += quat;
									}
									else
									{
										quat_sum += quat;
									}
								}
							}
						}
					});


				// editor::command::copy(g::current_select_kind, ecs_game, renderer);
			}

			if (aabb_min <= aabb_max)
			{
				if (need_focus)
				{
					focus_camera(renderer, aabb_min, aabb_max);
				}

				auto orientation = math::g::quaternion_identity;
				if (g::gizmo_space == e::transform_space_kind::local)
				{
					AGE_ASSERT(quat_sum.x != 0.f or quat_sum.y != 0.f or quat_sum.z != 0.f or quat_sum.w != 0.f);

					orientation = math::normalize(quat_sum);
				}

				widget_transform(ecs_game, renderer, (aabb_min + aabb_max) * 0.5f, orientation);
			}
		}

		g::set_focus = false;

		asset::for_each_kind([&renderer]<asset::e::kind e_kind> noexcept {
			for (auto h : asset::each_handle_of<e_kind>())
			{
				if constexpr (e_kind == asset::e::kind::material)
				{
					auto& entry = h.get_entry<material>();

					if (entry.ref_counter == 0)
					{
						asset::material::full_unload(h, renderer);
					}

					if (entry.ref_counter > 0)
					{
						asset::material::load(h, renderer);

						auto need_update = false;

						for (auto& h_tex : entry.all_textures() | views::deref)
						{
							if (runtime::is_handle_invalid(h_tex)) { continue; }

							// handle texture delete
							if (std::ranges::contains(g::asset_to_delete[to_idx(texture)], h_tex))
							{
								if (h_tex.get_entry<texture>().ref_counter == 1)
								{
									asset::texture::full_unload(h_tex, renderer);
								}

								asset::material::update_texture(h_tex, asset::handle{});

								need_update = true;
							}
						}

						if (need_update)
						{
							renderer.update_material(h);
						}
					}
				}
				else if constexpr (e_kind == asset::e::kind::font)
				{
					// skip;
					AGE_ASSERT(asset::registry::is_registered(h) is_false);
				}
				else
				{
					auto& entry = h.get_entry<e_kind>();

					if (entry.ref_counter == 0)
					{
						if constexpr (e_kind == texture)
						{
							if (entry.is_gpu_loaded())
							{
								std::println("unload : {}", entry.get_path());
							}
						}
						asset::full_unload<e_kind>(h, renderer);
					}

					if (entry.ref_counter > 0)
					{
						asset::gpu_load<e_kind>(h, renderer);
					}
				}
			}

			for (auto h : g::asset_to_delete[to_idx(e_kind)])
			{
				AGE_ASSERT(asset::registry::is_registered(h));

				asset::registry::unregister_asset(h);
			}

			g::asset_to_delete[to_idx(e_kind)].clear();
		});

		{
			struct
			{
				bool gi_active_found;
			} update_storage_ctx{ false };

			for (c_auto& editor_storage : active_scene.storage_data_vec)
			{
				ecs_game.visit_storage_at(active_scene.code_idx, editor_storage.code_idx, AGE_FUNC(detail::update_storage), renderer, update_storage_ctx);
			}

			if (renderer.ddgi_enabled() is_true and update_storage_ctx.gi_active_found is_false)
			{
				renderer.disable_ddgi();
			}

			if (renderer.gibs_enabled() is_true and update_storage_ctx.gi_active_found is_false)
			{
				renderer.disable_gibs();
			}
		}


		if (ui::g::p_input_ctx->is_pressed(input::e::key_kind::key_ctrl) and ui::g::p_input_ctx->is_pressed(input::e::key_kind::key_c))
		{
			editor::save_game(ecs_game, renderer);
		}
	}
}	 // namespace age::editor

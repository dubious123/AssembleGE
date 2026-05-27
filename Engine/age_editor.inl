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


		if (update_storage_ctx.ddgi_active_found is_false)
		{
			for (auto&& [cmp] : ecs_storage | each_entity_soft<ddgi_config>())
			{
				if (cmp.enabled)
				{
					update_storage_ctx.ddgi_active_found = true;
					if (renderer.ddgi_enabled() is_false)
					{
						renderer.enable_ddgi({
							.probe_per_level_axis = cmp.probe_per_level_axis,
							.base_probe_spacing	  = cmp.base_probe_spacing,
							.level_count		  = cmp.level_count,
						});
					}

					break;
				}
			}
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
	struct cage_out
	{
		uint32 probe_id[16];
		float  weight[16];	  // trilinear weight
		float  t;
	};

	struct ddgi_config
	{
		uint32_3 probe_count;
		float3	 base_probe_spacing;
		uint32	 level_count;

		uint32_3 probe_count_log_2 = { (uint32)std::countr_zero(probe_count.x),
									   (uint32)std::countr_zero(probe_count.y),
									   (uint32)std::countr_zero(probe_count.z) };

		uint32 b_ppl = probe_count_log_2[0] + probe_count_log_2[1] + probe_count_log_2[2];

		float3
		level_spacing(uint32 lv) const
		{
			return base_probe_spacing * (1u << lv);
		}

		float3
		level_extent(uint32 lv) const
		{
			return level_spacing(lv) * float3{ probe_count };
		}

		float3
		level_probe_radius(uint32 lv) const
		{
			return level_spacing(lv) * (float3(probe_count) * 0.5f - 0.5f);
		}

		uint32
		probes_per_level() const
		{
			return probe_count.x * probe_count.y * probe_count.z;
		}

		uint32
		total_probes() const
		{
			return probes_per_level() * level_count;
		}

		float3
		max_coverage() const
		{
			return level_extent(level_count - 1);
		}

		float3
		calc_probe_pos(uint32 probe_id, float3 cam_pos) const
		{
			uint32	 level = probe_id >> b_ppl;
			uint32	 li	   = probe_id & ((1u << b_ppl) - 1);
			uint32_3 local;
			local.x = li & (probe_count.x - 1);
			local.y = (li >> probe_count_log_2.x) & (probe_count.y - 1);
			local.z = li >> (probe_count_log_2.x + probe_count_log_2.y);

			float3 spacing = base_probe_spacing * (1u << level);

			int32_3 cam_coord = int32_3{ floor(cam_pos / spacing) };
			int32_3 half	  = int32_3(probe_count.x >> 1, probe_count.y >> 1, probe_count.z >> 1);
			int32_3 mask	  = int32_3{ probe_count } - 1;

			int32_3 world_coord;
			world_coord.x = cam_coord.x + ((((int)local.x - cam_coord.x + half.x) & mask.x) - half.x);
			world_coord.y = cam_coord.y + ((((int)local.y - cam_coord.y + half.y) & mask.y) - half.y);
			world_coord.z = cam_coord.z + ((((int)local.z - cam_coord.z + half.z) & mask.z) - half.z);

			return (float3{ world_coord } + 0.5f) * spacing;
		}

		uint32
		calc_axis_level(float cam_axis, float pixel_axis, uint32 x_y_z) const
		{
			uint32 dist = (uint32)floor(abs(pixel_axis - cam_axis) / base_probe_spacing[x_y_z]);
			uint32 half = 1u << (probe_count_log_2[x_y_z] - 1);
			if (dist < half) { return 0; }
			uint32 ratio = dist >> (probe_count_log_2[x_y_z] - 1);

			return std::bit_width(ratio);
		}

		// level_curr * (1-t) + level_next * t
		float
		calc_blend_t(float3 cam_pos, float3 pixel_world_pos, uint32 level) const
		{
			float3 spacing = base_probe_spacing * (1u << level);
			float3 d	   = abs(pixel_world_pos - cam_pos) / spacing;

			float3 half		  = float3{ probe_count } * 0.5f - 0.5;
			float  transition = 1.0f;

			float3 t3 = saturate((d - (half - transition)) / transition);
			return max(t3.x, t3.y, t3.z);
		}

		uint32
		local_to_probe_id(uint32 lx, uint32 ly, uint32 lz, uint32 level) const
		{
			const uint32 li = lx
							| (ly << probe_count_log_2[0])
							| (lz << (probe_count_log_2[0] + probe_count_log_2[1]));
			return (level << b_ppl) | li;
		}

		cage_out
		query_cage(float3 cam_pos, float3 pixel_world_pos) const
		{
			cage_out	 res;
			const uint32 level = max(
				calc_axis_level(cam_pos.x, pixel_world_pos.x, 0),
				calc_axis_level(cam_pos.y, pixel_world_pos.y, 1),
				calc_axis_level(cam_pos.z, pixel_world_pos.z, 2));

			float3	spacing	  = base_probe_spacing * (1u << level);
			int32_3 cam_coord = int32_3{ floor(cam_pos / spacing) };

			float3	grid_float = pixel_world_pos / spacing - 0.5f;
			int32_3 grid_base  = int32_3{ floor(grid_float) };
			float3	grid_frac  = grid_float - float3{ grid_base };


			c_auto t = calc_blend_t(cam_pos, pixel_world_pos, level);

			// AGE_LOG(t);

			const uint32_3 mask = probe_count - 1;
			if (t < 1.f - math::g::epsilon_1e4)
			{
				for (uint32 c = 0; c < 8; ++c)
				{
					int32_3 grid_offset = int32_3((c >> 0) & 1, (c >> 1) & 1, (c >> 2) & 1);
					int32_3 world_coord = grid_base + grid_offset;

					uint32_3 local;
					local.x = (uint32)(world_coord.x & mask.x);
					local.y = (uint32)(world_coord.y & mask.y);
					local.z = (uint32)(world_coord.z & mask.z);

					res.probe_id[c] = local_to_probe_id(local.x, local.y, local.z, level);


					float weight_x = grid_offset.x ? grid_frac.x : 1.0f - grid_frac.x;
					float weight_y = grid_offset.y ? grid_frac.y : 1.0f - grid_frac.y;
					float weight_z = grid_offset.z ? grid_frac.z : 1.0f - grid_frac.z;
					res.weight[c]  = weight_x * weight_y * weight_z;

					// AGE_LOG(c, world_coord, local, res.probe_id[c], res.weight[c], calc_probe_pos(res.probe_id[c], cam_pos));
				}
			}


			if (t > math::g::epsilon_1e4)
			{
				uint32	level_next	 = level + 1;
				float3	spacing_next = base_probe_spacing * (1u << level_next);
				float3	grid_f_next	 = pixel_world_pos / spacing_next - 0.5f;
				int32_3 base_next	 = int32_3{ floor(grid_f_next) };
				float3	frac_next	 = grid_f_next - float3{ base_next };

				for (uint32 c = 0; c < 8; ++c)
				{
					res.weight[c] *= (1 - t);

					int32_3	 grid_offset = int32_3((c >> 0) & 1, (c >> 1) & 1, (c >> 2) & 1);
					int32_3	 world_coord = base_next + grid_offset;
					uint32_3 local		 = {
						(uint32)(world_coord.x & mask.x),
						(uint32)(world_coord.y & mask.y),
						(uint32)(world_coord.z & mask.z)
					};

					res.probe_id[c + 8] = local_to_probe_id(local.x, local.y, local.z, level_next);

					float weight_x	  = grid_offset.x ? frac_next.x : 1.0f - frac_next.x;
					float weight_y	  = grid_offset.y ? frac_next.y : 1.0f - frac_next.y;
					float weight_z	  = grid_offset.z ? frac_next.z : 1.0f - frac_next.z;
					res.weight[c + 8] = (weight_x * weight_y * weight_z) * t;

					// AGE_LOG(c + 8, world_coord, local, res.probe_id[c + 8], res.weight[c + 8], calc_probe_pos(res.probe_id[c + 8], cam_pos));
				}
			}

			res.t = t;

			return res;
		}

		bool
		is_in_hole(uint32 probe_id, float3 cam_pos) const
		{
			uint32 level = probe_id >> b_ppl;
			if (level == 0) return false;
			float3 pos = calc_probe_pos(probe_id, cam_pos);
			float3 d   = abs(pos - cam_pos);

			float3 spacing		= level_spacing(level);
			float3 prev_spacing = level_spacing(level - 1);
			float3 prev_radius	= level_probe_radius(level - 1);
			// float3 hole_extent	= prev_radius - spacing * 0.5f;
			float3 hole_extent = prev_radius /*- prev_spacing*/ - spacing;

			return d < hole_extent.x and d < hole_extent.y and d < hole_extent.z;
		}
	};

	void
	update_game(auto& ecs_game, auto& renderer) noexcept
	{
		using enum age::asset::e::kind;

		using enum age::input::e::key_kind;

		auto& active_scene = g::current_game.scene_data_vec[g::current_game.current_active_scene_idx];


		{
			static auto raycast_req_vec2 = std::array<uint32, global::frame_buffer_count>{};
			c_auto		raycast_res		 = renderer.get_raycast_result(raycast_req_vec2[global::i_graphics.get_frame_buffer_idx]);

			c_auto target_world										  = math::ndc_to_world(renderer.get_camera_data(0).view_proj_inv, float3{ math::screen_to_ndc(float2{ ui::g::window_width, ui::g::window_height }, ui::g::p_input_ctx->mouse_pos), 0.f });
			raycast_req_vec2[global::i_graphics.get_frame_buffer_idx] = renderer.request_raycast(active_scene.cam.pos, math::normalize(target_world - active_scene.cam.pos), std::numeric_limits<float>::max(),
																								 graphics::e::rt_mask_kind::debug);

			c_auto color	 = std::array<float3, 8>{ ui::theme::palette_red(), ui::theme::palette_blue(), ui::theme::palette_green(), ui::theme::palette_yellow(),
													  ui::theme::palette_mint(), ui::theme::palette_cyan(), ui::theme::palette_sky(), ui::theme::palette_cerulean() };
			c_auto gi_config = ddgi_config{ uint32_3{ 32, 16, 32 }, float3{ 1, 2, 1 }, 8 };

			c_auto& obj_data	= renderer.get_object_data(22);
			c_auto& gi_cam_data = renderer.get_object_data(23);

			c_auto cage = gi_config.query_cage(gi_cam_data.pos, obj_data.pos);

			// AGE_LOG(gi_config.level_probe_radius(0), gi_config.level_probe_radius(1));

			auto redundant_count = 0u;
			auto count			 = 0u;
			for (auto&& [x, y, z, level] : std::views::cartesian_product(views::loop(gi_config.probe_count.x), views::loop(gi_config.probe_count.y), views::loop(gi_config.probe_count.z), views::loop(gi_config.level_count)))
			{
				++count;

				uint32 probe_id = gi_config.local_to_probe_id(x, y, z, level);
				float3 pos		= gi_config.calc_probe_pos(probe_id, gi_cam_data.pos);

				if (gi_config.is_in_hole(probe_id, gi_cam_data.pos))
				{
					++redundant_count;
					continue;
				}

				float  radius	 = 0.1f * (1u << level);
				c_auto probe_pos = gi_config.calc_probe_pos(probe_id, gi_cam_data.pos);

				if (x == 0 and y == 0 and z == 0 and level == 0)
				{
				}

				if (abs(probe_pos.z - gi_cam_data.pos.z) < 1 and abs(probe_pos.y - gi_cam_data.pos.y) < 2 and level == 0)
				{
					// AGE_LOG(probe_pos.x - gi_cam_data.pos.x);
				}

				bool found = false;
				if (cage.t < 1.f - math::g::epsilon_1e4)
				{
					for (auto i : views::loop(8))
					{
						if (cage.probe_id[i] == probe_id)
						{
							// renderer.render_debug_mesh(probe_pos, math::g::quaternion_identity, float3{ radius }, g::h_mesh_cube, ui::theme::color_white() * cage.weight[i] * 100, true, true);
							found = true;
						}
					}
				}


				if (cage.t > math::g::epsilon_1e4 and found is_false)
				{
					for (auto i : views::loop(8))
					{
						if (cage.probe_id[i + 8] == probe_id)
						{
							// renderer.render_debug_mesh(probe_pos, math::g::quaternion_identity, float3{ radius }, g::h_mesh_cube, ui::theme::color_white() * cage.weight[i + 8] * 100, true, true);
							found = true;
						}
					}
				}


				if (found is_false)
				{
					// renderer.render_debug_mesh(probe_pos, math::g::quaternion_identity, float3{ radius }, g::h_mesh_cube, color[level], true, true);
				}
			}

			// AGE_LOG(float(redundant_count) / count);
		}


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
				bool ddgi_active_found;
			} update_storage_ctx{ false };

			for (c_auto& editor_storage : active_scene.storage_data_vec)
			{
				ecs_game.visit_storage_at(active_scene.code_idx, editor_storage.code_idx, AGE_FUNC(detail::update_storage), renderer, update_storage_ctx);
			}

			if (renderer.ddgi_enabled() is_true and update_storage_ctx.ddgi_active_found is_false)
			{
				renderer.disable_ddgi();
			}
		}


		if (ui::g::p_input_ctx->is_pressed(input::e::key_kind::key_ctrl) and ui::g::p_input_ctx->is_pressed(input::e::key_kind::key_c))
		{
			editor::save_game(ecs_game, renderer);
		}
	}
}	 // namespace age::editor

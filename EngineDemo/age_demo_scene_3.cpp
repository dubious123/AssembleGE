#include "age_demo_pch.hpp"
#include "age_demo.hpp"

namespace age_demo::scene_3
{
	void
	init() noexcept
	{
		using namespace age::ecs::system;

		age::editor::init();

		i_init.get_editor_game->init();

		age::editor::load_game(i_init.get_editor_game(), "./resources/demo_game/", i_init.get_render_pipeline());

		on_ctx{
			AGE_LAMBDA(
				(),
				{
					i_init.set_smoothed_move(float2{ 0.f, 0.f });
					i_init.set_smoothed_look(float2{ 0.f, 0.f });
					i_init.set_smoothed_zoom(0.f);
					i_init.set_smoothed_pan(float2{ 0.f, 0.f });
				}),

			// [0] cube
			identity{ age::asset::primitive_desc{
				.size	   = { 0.5, 0.5, 0.5 },
				.seg_u	   = 30,
				.seg_v	   = 30,
				.mesh_kind = age::asset::e::primitive_mesh_kind::cube } }
				| age::asset::create_primitive_mesh
				| age::asset::bake_mesh<age::asset::vertex_pnt_uv1>
				| AGE_FUNC(i_init.get_render_pipeline().upload_mesh)
				| AGE_FUNC(i_init.get_mesh_id_vec().emplace_back),

			// [1] plane
			identity{ age::asset::primitive_desc{
				.size	   = { 0.5, 0.5, 0.5 },
				.seg_u	   = 30,
				.seg_v	   = 30,
				.mesh_kind = age::asset::e::primitive_mesh_kind::plane } }
				| age::asset::create_primitive_mesh
				| age::asset::bake_mesh<age::asset::vertex_pnt_uv1>
				| AGE_FUNC(i_init.get_render_pipeline().upload_mesh)
				| AGE_FUNC(i_init.get_mesh_id_vec().emplace_back),

			// [2] sphere
			identity{ age::asset::primitive_desc{
				.size	   = { 0.5, 0.5, 0.5 },
				.seg_u	   = 30,
				.seg_v	   = 30,
				.mesh_kind = age::asset::e::primitive_mesh_kind::cube_sphere } }
				| age::asset::create_primitive_mesh
				| age::asset::bake_mesh<age::asset::vertex_pnt_uv1>
				| AGE_FUNC(i_init.get_render_pipeline().upload_mesh)
				| AGE_FUNC(i_init.get_mesh_id_vec().emplace_back),

			exec_inline{}
		}();
	}

	FORCE_INLINE decltype(auto)
	update() noexcept
	{
		using namespace age::ui;
		using namespace age::ecs;

		using enum age::input::e::key_kind;

		if (i_update.get_render_pipeline->begin_render(i_update.get_h_render_surface) is_false)
		{
			return;
		}

		age::ui::begin_frame(i_update.get_h_window);

		if (auto _ = widget::horizontal(set_size(size_mode::grow(), size_mode::grow()), set_child_gap(0)))
		{
			if (auto _ = widget::panel_resizable_h(300, 1000))
			{
				if (auto _ = widget::scroll_area_v())
				{
					age::editor::ui_entity_hierarchy(i_update.get_editor_game(), i_update.get_render_pipeline());
				}
			}

			if (auto _ = widget::panel_resizable_h(300, 1000))
			{
				if (auto _ = widget::scroll_area_v())
				{
					age::editor::ui_inspector(i_update.get_editor_game(), i_update.get_render_pipeline());
				}
			}

			if (auto _ = widget::begin(style::vertical() | set_width_grow() | set_height_grow()))
			{
				age::editor::ui_scene_view(i_update.get_render_pipeline(), i_init.get_h_window());
			}

			if (age::editor::is_edit_mode())
			{
				age::editor::update_game(i_update.get_editor_game(), i_update.get_render_pipeline());
			}
			else if (age::editor::is_play_mode())
			{
				// play mode
			}
		}

		age::ui::end_frame(i_update.get_render_pipeline->get_ui_render_data_vec(),
						   i_update.get_render_pipeline->get_ui_render_data_z_range_vec());


		auto& entities = i_update.get_editor_game->editor_scene_0.ent_storage_main;

		for (auto&& [light] : entities | each_entity<directional_light>())
		{
			i_update.get_render_pipeline->update_directional_light(light.render_id,
																   { .direction	  = age::math::normalize(light.direction),
																	 .intensity	  = light.intensity,
																	 .color		  = light.color,
																	 .cast_shadow = light.cast_shadow });
		}

		for (auto&& [light, pos] : entities | each_entity<point_light, position>())
		{
			i_update.get_render_pipeline->update_point_light(
				light.render_id,
				{ .position	   = pos,
				  .range	   = light.range,
				  .color	   = light.color,
				  .intensity   = light.intensity,
				  .cast_shadow = light.cast_shadow });
		}

		for (auto&& [light, pos] : entities | each_entity<spot_light, position>())
		{
			i_update.get_render_pipeline->update_spot_light(
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

		for (auto&& [pos, rot, scale, obj, mesh, mat] : entities
															| each_entity<const position, const rotation, const scale, const render_object, const mesh, const material>())
		{
			i_update.get_render_pipeline->update_object(obj.render_id, pos, rot, scale);

			if (mat.is_opaque)
			{
				i_update.get_render_pipeline->render_mesh(0, obj.render_id, mesh.render_id);
			}
			else
			{
				i_update.get_render_pipeline->render_transparent_mesh(0, obj.render_id, mesh.render_id);
			}
		}

		// for (auto&& [pos, cam] : entities | age::ecs::each_entity<age::ecs::position, age::ecs::camera>())
		//{
		//	if (focused)
		//	{
		//		c_auto dt_s = std::max(age::runtime::i_time.get_delta_time_s(), 1.f / 160);

		//		c_auto speed = i_update.get_sprint() ? input::g::move_speed * input::g::sprint_mult : input::g::move_speed;

		//		c_auto move_smoothing_factor = 1.f - std::exp(-input::g::move_smoothing * dt_s);
		//		c_auto look_smoothing_factor = 1.f - std::exp(-input::g::look_smoothing * dt_s);
		//		c_auto zoom_smoothing_factor = 1.f - std::exp(-input::g::zoom_smoothing * dt_s);

		//		i_update.set_smoothed_move = age::math::lerp(i_update.get_smoothed_move(), i_update.get_move(), move_smoothing_factor);
		//		i_update.set_smoothed_zoom = age::math::lerp(i_update.get_smoothed_zoom(), i_update.get_zoom(), zoom_smoothing_factor);

		//		auto look_target		   = i_update.get_right_mouse_down() ? i_update.get_look() : float2{ 0.f, 0.f };
		//		i_update.set_smoothed_look = age::math::lerp(i_update.get_smoothed_look(), look_target, look_smoothing_factor);

		//		auto pan_target			  = i_update.get_middle_mouse_down() ? i_update.get_look() : float2{ 0.f, 0.f };
		//		i_update.set_smoothed_pan = age::math::lerp(i_update.get_smoothed_pan(), pan_target, look_smoothing_factor);

		//		cam.euler_deg.y += i_update.get_smoothed_look->x * input::g::sensitivity;
		//		cam.euler_deg.x += i_update.get_smoothed_look->y * input::g::sensitivity;
		//		cam.euler_deg.x	 = std::clamp(cam.euler_deg.x, -89.f, 89.f);

		//		c_auto xm_look_quat = cam.euler_deg * age::g::degree_to_radian
		//							| age::simd::load()
		//							| age::simd::euler_to_quat();

		//		c_auto forward = age::simd::g::xm_forward_f4 | age::simd::rotate3(xm_look_quat) | age::simd::to<float3>();
		//		c_auto right   = age::simd::g::xm_right_f4 | age::simd::rotate3(xm_look_quat) | age::simd::to<float3>();
		//		c_auto up	   = age::simd::g::xm_up_f4 | age::simd::rotate3(xm_look_quat) | age::simd::to<float3>();

		//		pos -= right * i_update.get_smoothed_pan->x * input::g::pan_speed * dt_s;
		//		pos += up * i_update.get_smoothed_pan->y * input::g::pan_speed * dt_s;
		//		pos += forward * i_update.get_smoothed_zoom() * input::g::zoom_speed;
		//		pos += (right * i_update.get_smoothed_move->x + forward * i_update.get_smoothed_move->y) * speed * dt_s;
		//	}


		//	auto cam_desc					  = i_update.get_render_pipeline->get_camera_desc(cam.render_id);
		//	cam_desc.pos					  = pos;
		//	cam_desc.quaternion				  = age::euler_deg_to_quat(cam.euler_deg);
		//	cam_desc.perspective.aspect_ratio = cam.aspect_ratio;
		//	i_update.get_render_pipeline->update_camera(cam.render_id, cam_desc);
		//}

		i_update.get_render_pipeline->end_render(i_update.get_h_render_surface());
	}

	void
	deinit_storage(auto& entities) noexcept
	{
		for (auto&& [cam] : entities | age::ecs::each_entity<age::ecs::camera>())
		{
			i_deinit.get_render_pipeline->remove_camera(cam.render_id);
		}

		for (auto&& [l] : entities | age::ecs::each_entity<age::ecs::directional_light>())
		{
			i_deinit.get_render_pipeline->remove_directional_light(l.render_id);
		}

		for (auto&& [l] : entities | age::ecs::each_entity<age::ecs::point_light>())
		{
			i_deinit.get_render_pipeline->remove_point_light(l.render_id);
		}

		for (auto&& [l] : entities | age::ecs::each_entity<age::ecs::spot_light>())
		{
			i_deinit.get_render_pipeline->remove_spot_light(l.render_id);
		}

		for (auto&& [obj] : entities | age::ecs::each_entity<age::ecs::render_object>())
		{
			i_deinit.get_render_pipeline->remove_object(obj.render_id);
		}
	}

	void
	deinit() noexcept
	{
		age::editor::save_game(i_deinit.get_editor_game(), i_deinit.get_render_pipeline());
		age::editor::deinit();

		i_deinit.get_editor_game->visit_all_storages(AGE_FUNC(deinit_storage));

		for (auto m_id : i_deinit.get_mesh_id_vec() | std::views::reverse)
		{
			i_deinit.get_render_pipeline().release_mesh(m_id);
		}

		i_deinit.get_mesh_id_vec->clear();


		i_deinit.get_editor_game->deinit();
	}
}	 // namespace age_demo::scene_3
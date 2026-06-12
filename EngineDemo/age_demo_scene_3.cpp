#include "age_demo_pch.hpp"
#include "age_demo.hpp"

namespace age_demo::scene_3
{
	void
	init() noexcept
	{
		using namespace age::ecs::system;
		age::editor::init(AGE_LAMBDA((auto&& _1, auto&&... arg), { return age::asset::mesh_baked::gpu_load(FWD(_1), i_init.get_render_pipeline(), FWD(arg)...); }));

		i_init.get_editor_game->init();

		age::editor::load_game(i_init.get_editor_game(), "./resources/demo_game/", i_init.get_render_pipeline());

		i_init.set_smoothed_move = float2{ 0.f, 0.f };
		i_init.set_smoothed_look = float2{ 0.f, 0.f };
		i_init.set_smoothed_zoom = 0.f;
		i_init.set_smoothed_pan	 = float2{ 0.f, 0.f };
	}

	FORCE_INLINE decltype(auto)
	update() noexcept
	{
		using namespace age::ui;
		using namespace age::ecs;

		using enum age::input::e::key_kind;

		i_update.get_render_pipeline->begin_frame();

		{
			c_auto& ui_main_cam = i_update.get_render_pipeline->get_camera_data(0);
			age::ui::begin_frame(i_update.get_h_window, ui_main_cam.pos, ui_main_cam.view_proj_inv);
		}


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
				if (auto _ = widget::panel_resizable_v(500.f, (float)age::platform::get_client_height(i_update.get_h_window)))
				{
					if (auto _ = widget::scroll_area_v())
					{
						age::editor::ui_inspector(i_update.get_editor_game(), i_update.get_render_pipeline());
					}
				}

				if (auto _ = widget::begin(style::panel() | set_width_grow() | set_height_grow()))
				{
					auto h_scroll = widget::scroll_area_v();
					age::editor::ui_asset();
				}
			}

			if (auto _ = widget::begin(style::vertical() | set_width_grow() | set_height_grow()))
			{
				if (age::editor::g::show_modal)
				{
					age::editor::ui_modal();
				}
				else
				{
					age::editor::ui_scene_view(i_update.get_render_pipeline(), i_init.get_h_window());
				}
			}

			if (age::editor::is_edit_mode())
			{
				age::editor::update_game(i_update.get_editor_game(), i_update.get_render_pipeline());
			}
			else if (age::editor::is_play_mode())
			{
				for (auto h_mesh : age::asset::registry::all(age::asset::e::kind::mesh_baked))
				{
					c_auto& entry = h_mesh.get_entry<age::asset::e::kind::mesh_baked>();
					if (entry.ref_counter == 0)
					{
						age::asset::mesh_baked::full_unload(h_mesh, i_update.get_render_pipeline());
					}

					if (entry.ref_counter > 0)
					{
						age::asset::mesh_baked::gpu_load(h_mesh, i_update.get_render_pipeline());
					}
				}
				// play mode
			}
		}

		if (i_update.get_render_pipeline->begin_render(i_update.get_h_render_surface) is_false)
		{
			age::ui::clear();
			return;
		}

		age::ui::end_frame(i_update.get_render_pipeline());


		i_update.get_editor_game->visit_all_storages(
			AGE_LAMBDA(
				(auto& entities),
				{
					for (auto&& [pos, rot, scale, obj, mesh, mat] : entities
																		| each_entity<const position, const rotation, const scale, const render_object, const mesh, const material>())
					{
						i_update.get_render_pipeline->update_object(obj.render_id, pos, rot, scale);

						if (age::runtime::is_handle_invalid(mesh.h_mesh) or age::runtime::is_handle_invalid(mat.h_mat))
						{
							continue;
						}

						if (auto& entry = mesh.h_mesh.get_entry<age::asset::e::kind::mesh_baked>();
							entry.is_gpu_loaded() is_false)
						{
							continue;
						}

						// if (auto& entry = mat.h_mat.get_entry<age::asset::e::kind::material>();
						//	entry.is_gpu_loaded() is_false)
						//{
						//	continue;
						// }

						i_update.get_render_pipeline->render_mesh(0, obj.render_id, mesh.h_mesh, mat.h_mat);
					}
				}));
		// auto& entities = i_update.get_editor_game->editor_scene_0.ent_storage_main;
		// for (auto&& [light] : entities | each_entity<directional_light>())
		//{
		//	i_update.get_render_pipeline->update_directional_light(light.render_id,
		//														   { .direction	  = age::math::normalize(light.direction),
		//															 .intensity	  = light.intensity,
		//															 .color		  = light.color,
		//															 .cast_shadow = light.cast_shadow });
		//}

		// for (auto&& [light, pos] : entities | each_entity<point_light, position>())
		//{
		//	i_update.get_render_pipeline->update_point_light(
		//		light.render_id,
		//		{ .position	   = pos,
		//		  .range	   = light.range,
		//		  .color	   = light.color,
		//		  .intensity   = light.intensity,
		//		  .cast_shadow = light.cast_shadow });
		// }

		// for (auto&& [light, pos] : entities | each_entity<spot_light, position>())
		//{
		//	i_update.get_render_pipeline->update_spot_light(
		//		light.render_id,
		//		{ .position	   = pos,
		//		  .range	   = light.range,
		//		  .direction   = age::math::normalize(light.direction),
		//		  .intensity   = light.intensity,
		//		  .color	   = light.color,
		//		  .cos_inner   = light.cos_inner,
		//		  .cos_outer   = light.cos_outer,
		//		  .cast_shadow = light.cast_shadow });
		// }

		// for (auto&& [pos, rot, scale, obj, mesh, mat] : entities
		//													| each_entity<const position, const rotation, const scale, const render_object, const mesh, const material>())
		//{
		//	i_update.get_render_pipeline->update_object(obj.render_id, pos, rot, scale);

		//	if (age::runtime::is_handle_invalid(mesh.h_mesh))
		//	{
		//		continue;
		//	}

		//	if (mat.is_opaque)
		//	{
		//		i_update.get_render_pipeline->render_mesh(0, obj.render_id, mesh.h_mesh);
		//	}
		//	else
		//	{
		//		i_update.get_render_pipeline->render_transparent_mesh(0, obj.render_id, mesh.h_mesh);
		//	}
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

		for (auto&& [b] : entities | age::ecs::each_entity_soft<age::ecs::bloom>())
		{
			i_deinit.get_render_pipeline->remove_bloom(b.render_id);
		}
	}

	void
	deinit() noexcept
	{
		age::graphics::command::signal();
		age::graphics::command::cpu_wait();
		age::editor::save_game(i_deinit.get_editor_game(), i_deinit.get_render_pipeline());
		age::editor::deinit(AGE_LAMBDA((age::asset::handle h_mesh), { return age::asset::mesh_baked::full_unload(h_mesh, i_init.get_render_pipeline()); }));

		i_deinit.get_editor_game->visit_all_storages(AGE_FUNC(deinit_storage));

		for (auto h_mesh : age::asset::registry::all(age::asset::e::kind::mesh_baked))
		{
			if (age::runtime::is_handle_invalid(h_mesh))
			{
				continue;
			}
			age::asset::mesh_baked::full_unload(h_mesh, i_deinit.get_render_pipeline());
		}

		for (auto h_mat : age::asset::registry::all(age::asset::e::kind::material))
		{
			if (age::runtime::is_handle_invalid(h_mat))
			{
				continue;
			}
			age::asset::material::full_unload(h_mat, i_deinit.get_render_pipeline());
		}

		for (auto h : age::asset::registry::all(age::asset::e::kind::texture))
		{
			if (age::runtime::is_handle_invalid(h))
			{
				continue;
			}
			age::asset::texture::full_unload(h, i_deinit.get_render_pipeline());
		}

		for (auto h : age::asset::registry::all(age::asset::e::kind::env_light))
		{
			if (age::runtime::is_handle_invalid(h))
			{
				continue;
			}
			age::asset::env_light::full_unload(h, i_deinit.get_render_pipeline());
		}

		if (i_deinit.get_render_pipeline->ddgi_enabled())
		{
			i_deinit.get_render_pipeline->disable_ddgi();
		}
		else if (i_deinit.get_render_pipeline->gibs_enabled())
		{
			i_deinit.get_render_pipeline->disable_gibs();
		}

		i_deinit.get_editor_game->deinit();
		age::asset::registry::clear();
	}
}	 // namespace age_demo::scene_3
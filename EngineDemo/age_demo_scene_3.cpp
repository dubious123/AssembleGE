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


		age::asset::registry::register_asset(
			age::asset::mesh_baked::gpu_load("./resources/demo_game/assets/mesh/primitive_cube",
											 i_init.get_render_pipeline(),
											 age::asset::primitive_desc{
												 .size		= { 0.5, 0.5, 0.5 },
												 .seg_u		= 30,
												 .seg_v		= 30,
												 .mesh_kind = age::asset::e::primitive_mesh_kind::cube },
											 age::asset::e::vertex_kind::pnt_uv1));

		age::asset::registry::register_asset(
			age::asset::mesh_baked::gpu_load("./resources/demo_game/assets/mesh/primitive_plane",
											 i_init.get_render_pipeline(),
											 age::asset::primitive_desc{
												 .size		= { 0.5, 0.5, 0.5 },
												 .seg_u		= 30,
												 .seg_v		= 30,
												 .mesh_kind = age::asset::e::primitive_mesh_kind::plane },
											 age::asset::e::vertex_kind::pnt_uv1));
		age::asset::registry::register_asset(
			age::asset::mesh_baked::gpu_load("./resources/demo_game/assets/mesh/primitive_cube_sphere",
											 i_init.get_render_pipeline(),
											 age::asset::primitive_desc{
												 .size		= { 0.5, 0.5, 0.5 },
												 .seg_u		= 30,
												 .seg_v		= 30,
												 .mesh_kind = age::asset::e::primitive_mesh_kind::cube_sphere },
											 age::asset::e::vertex_kind::pnt_uv1));

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
				if (auto _ = widget::panel_resizable_v(150.f, (float)age::platform::get_client_height(i_update.get_h_window)))
				{
					if (auto _ = widget::scroll_area_v())
					{
						age::editor::ui_inspector(i_update.get_editor_game(), i_update.get_render_pipeline());
					}
				}

				if (auto _ = widget::begin(style::panel() | set_width_grow() | set_height_grow()))
				{
					age::editor::ui_asset();
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

		auto h_cube		   = age::asset::registry::find(age::asset::e::kind::mesh_baked, age::asset::get_asset_full_path<age::asset::e::kind::mesh_baked>("./resources/demo_game/assets/mesh/primitive_cube").data());
		auto h_cube_sphere = age::asset::registry::find(age::asset::e::kind::mesh_baked, age::asset::get_asset_full_path<age::asset::e::kind::mesh_baked>("./resources/demo_game/assets/mesh/primitive_plane").data());
		auto h_plane	   = age::asset::registry::find(age::asset::e::kind::mesh_baked, age::asset::get_asset_full_path<age::asset::e::kind::mesh_baked>("./resources/demo_game/assets/mesh/primitive_cube_sphere").data());

		age::asset::mesh_baked::full_unload(h_cube, i_deinit.get_render_pipeline());
		age::asset::mesh_baked::full_unload(h_cube_sphere, i_deinit.get_render_pipeline());
		age::asset::mesh_baked::full_unload(h_plane, i_deinit.get_render_pipeline());

		i_deinit.get_editor_game->deinit();
	}
}	 // namespace age_demo::scene_3
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

		age::editor::load_game(i_init.get_editor_game(), "resources/demo_game/", i_init.get_render_pipeline());

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

		static bool first = false;

		// sample editor script
		if (first)
		{
			auto ent_vec = age::vector<uint32>{};
			auto desc	 = age::asset::model_desc{};
			desc.h_materials.resize(1);

			auto& renderer	= i_update.get_render_pipeline();
			auto  model_idx = 0u;

			auto model_name_buf = age::array<char, age::config::max_asset_path_len>{};


			for (c_auto scene_idx : age::views::loop(i_update.get_editor_game->scene_count()))
			{
				i_update.get_editor_game->visit_scene_at(scene_idx, [&](auto& scene) {
					scene.visit_all_storages([&](c_auto storage_idx, auto& entities) {
						if constexpr (entities.has_component<render_object, mesh, material, model>())
						{
							auto& editor_storage_data = age::editor::detail::find_storage_editor_data(scene_idx, storage_idx);
							for (const auto&& [ent_id, obj, mesh, mat] :
								 entities | each_entity<sv_entity_id, const render_object, const mesh, const material>())
							{
								ent_vec.emplace_back(ent_id);
							}

							for (auto ent_id : ent_vec)
							{
								auto&& [obj, msh, mat] = entities.get_component<const render_object, mesh, material>(ent_id);
								desc.h_materials[0]	   = mat.h_mat;
								desc.h_mesh			   = msh.h_mesh;

								auto [out, len]	  = std::format_to_n(model_name_buf.data(), model_name_buf.size() - 1, "new_model_{}", model_idx++);
								c_auto model_path = age::editor::get_asset_full_path(age::asset::e::kind::model, std::string_view{ model_name_buf.data(), static_cast<uint32>(len) });
								age::asset::model::build({ model_path.data() }, desc);
								c_auto h_model = age::asset::model::load_common_from_path(model_path, renderer);
								age::asset::registry::register_asset(h_model);

								age::editor::remove_components<mesh, material>(entities, renderer, editor_storage_data, ent_id);
								age::editor::add_components<model>(entities, renderer, editor_storage_data, ent_id);

								auto&& [mdl] = entities.get_component<model>(ent_id);
								mdl.update_h_model(h_model);
							}

							ent_vec.clear();
						}
					});
				});
			}
			first = false;
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
					age::editor::ui_asset_list_panel();
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
					age::editor::ui_scene_view(i_update.get_render_pipeline());
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

		if (age::editor::is_edit_mode())
		{
			age::editor::render_current_scene(i_update.get_editor_game(), i_update.get_render_pipeline(), i_init.get_h_window());
		}
		else
		{
			// user render loop begin

			i_update.get_editor_game->visit_all_storages(
				AGE_LAMBDA(
					(auto& entities),
					{
						if constexpr (entities.has_component<position, rotation, scale, render_object, model, model_render_option>())
						{
							for (auto&& [ent_id, pos, rot, scale, obj, model] :
								 entities | each_entity<sv_entity_id, const position, const rotation, const scale, const render_object, const model>())
							{
								i_update.get_render_pipeline->update_object(obj.render_id, pos, rot, scale);

								if (age::runtime::is_handle_invalid(model.h_model)) { continue; }

								if (c_auto& entry = model.h_model.get_entry<age::asset::e::kind::model>();
									entry.is_loaded() is_false)
								{
									continue;
								}

								if (entities.has_component<model_render_option>(ent_id))
								{
									auto&& [option] = entities.get_component<const model_render_option>(ent_id);
									i_update.get_render_pipeline->render_model(
										0, obj.render_id, model.h_model,
										{
											.raster_override_kind		 = option.raster_override_kind,
											.rt_alpha_test_override_kind = option.rt_alpha_test_override_kind,
											.option_flags				 = option.option_flags,
											.fade_unorm8				 = option.fade_unorm8,
										});
								}
								else
								{
									i_update.get_render_pipeline->render_model(0, obj.render_id, model.h_model);
								}
							}
						}
					}));
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

		for (auto h : age::asset::registry::all(age::asset::e::kind::model))
		{
			if (age::runtime::is_handle_invalid(h))
			{
				continue;
			}

			age::asset::model::full_unload(h, i_deinit.get_render_pipeline());
		}

		if (i_deinit.get_render_pipeline->ddgi_enabled())
		{
			i_deinit.get_render_pipeline->disable_ddgi();
		}
		else if (i_deinit.get_render_pipeline->gibs_enabled())
		{
			i_deinit.get_render_pipeline->disable_gibs();
		}

		if (i_deinit.get_render_pipeline->ao_enabled())
		{
			i_deinit.get_render_pipeline->disable_ao();
		}

		if (i_deinit.get_render_pipeline->aa_enabled())
		{
			i_deinit.get_render_pipeline->disable_aa();
		}

		i_deinit.get_editor_game->deinit();
		age::asset::registry::clear();
	}
}	 // namespace age_demo::scene_3
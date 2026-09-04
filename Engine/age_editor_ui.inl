#include "age.hpp"

namespace age::editor
{
	ui::widget_ctx
	ui_entity_tree_node(storage_editor_data&, uint64 ecs_ent_id, uint64 archetype, bool selected) noexcept;
}	 // namespace age::editor

namespace age::editor
{
	float3
	get_component_color(uint32 cmp_idx) noexcept;
}	 // namespace age::editor

namespace age::editor
{
	ui::widget_ctx
	ui_component_header(const char* p_name, bool& close_out) noexcept;

	void
	ui_component(ecs::position& pos) noexcept;

	void
	ui_component(ecs::render_object& obj) noexcept;

	void
	ui_component(ecs::rotation& rot) noexcept;

	void
	ui_component(ecs::scale& scale) noexcept;

	void
	ui_component(ecs::mesh& mesh) noexcept;

	void
	ui_component(asset::handle h_mat, asset::entry<asset::e::kind::material>& mat_entry) noexcept;

	void
	ui_component(ecs::material& mat) noexcept;

	void
	ui_component(ecs::material& mat, auto& renderer) noexcept
	{
		ui_component(mat);

		if (runtime::is_handle_invalid(mat.h_mat) is_false)
		{
			auto& entry = mat.h_mat.get_entry<asset::e::kind::material>();
			if (entry.is_loaded())
			{
				renderer.update_material(mat.h_mat);
			}
		}
	}

	void
	ui_component(ecs::model_render_option&) noexcept;

	void
	ui_component(ecs::model&) noexcept;

	void
	ui_component(ecs::directional_light& light) noexcept;

	void
	ui_component(ecs::point_light& light) noexcept;

	void
	ui_component(ecs::spot_light& light) noexcept;

	void
	ui_component(ecs::env_light& env_light) noexcept;

	void
	ui_component(ecs::env_light& env_light, auto& renderer) noexcept
	{
		ui_component(env_light);

		if (runtime::is_handle_invalid(env_light.h_env_light) is_false)
		{
			auto& entry = env_light.h_env_light.get_entry<asset::e::kind::env_light>();

			if (entry.is_cpu_loaded())
			{
				auto btn = ui::widget::button("save");

				if (btn.clicked())
				{
					asset::env_light::save(env_light.h_env_light);
				}

				if (entry.is_gpu_loaded())
				{
					renderer.update_env_light_runtime(env_light.h_env_light);
				}
			}
		}
	}

	void
	ui_component(ecs::camera& cam) noexcept;

	void
	ui_component(ecs::bloom& cmp) noexcept;

	// return : need update
	std::tuple<bool, bool>
	ui_component(ecs::gi_config& _, uint32 gibs_max_surfel_count, uint32 gist_max_cell_surfel_count) noexcept;

	void
	ui_component(ecs::gi_config& cmp, auto& renderer) noexcept
	{
		auto&& [update, update_debug_flags] = ui_component(cmp, renderer.gibs_max_surfel_count(), renderer.gist_max_cell_surfel_count());
		if (update)
		{
			if (cmp.enable_ddgi)
			{
				renderer.update_ddgi({
					.probe_per_level_axis = cmp.ddgi_probe_per_level_axis,
					.base_probe_spacing	  = cmp.ddgi_base_probe_spacing,
					.level_count		  = cmp.ddgi_level_count,
					.debug_flags		  = cmp.ddgi_debug_flags,
					.lock_origin		  = cmp.ddgi_lock_origin,
				});
			}
			else if (cmp.enable_gibs)
			{
				renderer.update_gibs({
					.max_surfel_count		= cmp.max_surfel_count,
					.debug_flags			= cmp.gibs_debug_flags,
					.lock_origin			= cmp.gibs_lock_origin,
					.cell_count				= cmp.gibs_cell_count,
					.outer_layer_count		= cmp.gibs_outer_layer_count,
					.cell_size				= cmp.gibs_cell_size,
					.outer_cell_size_factor = cmp.outer_cell_size_factor,
				});
			}
			else if (cmp.enable_gist)
			{
				renderer.update_gist({
					.diffuse_ray_period			   = cmp.gist_diffuse_ray_period,
					.specular_ray_period		   = cmp.gist_specular_ray_period,
					.cell_surfel_ray_count_min	   = cmp.gist_cell_surfel_ray_count_min,
					.cell_surfel_ray_count_max	   = cmp.gist_cell_surfel_ray_count_max,
					.max_cell_surfel_count		   = cmp.gist_max_cell_surfel_count,
					.cell_surfel_ray_budget_factor = cmp.gist_cell_surfel_ray_budget_factor,
					.debug_flags				   = cmp.gist_debug_flags,
					.lock_origin				   = cmp.gist_lock_origin,
					.cell_count_per_axis		   = cmp.gist_cell_count_per_axis,
					.outer_layer_count			   = cmp.gist_outer_layer_count,
					.cell_size					   = cmp.gist_cell_size,
					.outer_cell_size_factor		   = cmp.gist_outer_cell_size_factor,
				});
			}
		}
		else if (update_debug_flags)
		{
			if (cmp.enable_ddgi)
			{
				renderer.update_ddgi_debug_flags(cmp.ddgi_debug_flags);
			}
			else if (cmp.enable_gibs)
			{
				renderer.update_gibs_debug_flags(cmp.gibs_debug_flags);
			}
			else if (cmp.enable_gist)
			{
				renderer.update_gist_debug_flags(cmp.gist_debug_flags);
			}
		}
	}

	void
	ui_component(age::ecs::editor_cam_setting& cmp) noexcept;

	bool
	ui_component(age::ecs::ao_config& cmp) noexcept;

	void
	ui_component(age::ecs::ao_config& cmp, auto& renderer) noexcept
	{
		const bool need_update = ui_component(cmp);

		if (cmp.enabled and need_update)
		{
			renderer.update_ao({
				.slice_count   = cmp.slice_count,
				.offset_count  = cmp.offset_count,
				.radius		   = cmp.radius,
				.max_px_radius = cmp.max_px_radius,
				.intensity	   = cmp.intensity,
				.power		   = cmp.power,
				.thickness	   = cmp.thickness,
				.fade_distance = cmp.fade_distance,
				.fade_range	   = cmp.fade_range,
				.debug_flags   = cmp.debug_flags,
			});
		}
	}

	bool
	ui_component(age::ecs::aa_config& cmp) noexcept;

	void
	ui_component(age::ecs::aa_config& cmp, auto& renderer) noexcept
	{
		const bool need_update = ui_component(cmp);

		if (cmp.enabled and need_update)
		{
			renderer.update_aa({
				.fxaa_on_offscreen			  = cmp.fxaa_on_offscreen,
				.opaque_aa_ray_per_px		  = cmp.opaque_aa_ray_per_px,
				.transparent_aa_ray_per_px	  = cmp.transparent_aa_ray_per_px,
				.aa_px_cap					  = cmp.aa_px_cap,
				.aa_px_headroom				  = cmp.aa_px_headroom,
				.edge_plane_dist_tolerance_px = cmp.edge_plane_dist_tolerance_px,
				.edge_normal_threshold		  = cmp.edge_normal_threshold,
			});
		}
	}

	bool
	ui_component(age::ecs::debug_view_config& cmp, bool aa_enabled, bool ao_enabled, bool ddgi_enabled, bool gibs_enabled, bool gist_enabled) noexcept;

	void
	ui_component(age::ecs::debug_view_config& cmp, auto& renderer) noexcept
	{
		c_auto need_update = ui_component(cmp,
										  renderer.aa_enabled(),
										  renderer.ao_enabled(),
										  renderer.ddgi_enabled(),
										  renderer.gibs_enabled(),
										  renderer.gist_enabled());

		// todo
		if (cmp.enabled and renderer.debug_view_enabled())
		{
			renderer.update_debug_view(cmp_to_desc(cmp));
		}
	}

	void
	ui_component(auto&& cmp) noexcept
	{
		ui::widget::text_heading("ui for component ?? not implemented yet");
	}

	void
	ui_transform(auto&& cmp) noexcept
	{
		if constexpr (requires { cmp.pos; })
		{
			ui::widget::numeric_field(cmp.pos, "possition");
		}
		if constexpr (requires { cmp.rot; })
		{
			ui::widget::rotation_field(cmp.rot, "rotation");
		}
		if constexpr (requires { cmp.scale; })
		{
			ui::widget::numeric_field(cmp.scale, "scale");
		}
	}

	void
	ui_component_section(ecs::cx_entity_storage auto&& storage, auto&& ent_id, auto&& cmp, uint32 cmp_idx, auto& renderer) noexcept
	{
		using namespace age::ui;

		using t_cmp = BARE_OF(cmp);

		if (auto _ = widget::begin(style::section() | set_horizontal() | set_height_fit() | set_width_grow()))
		{
			widget::separator_h(set_body_brush_data(get_component_color(cmp_idx), theme::opacity_medium()), set_width_fixed(theme::thickness_thick()));

			auto remove_cmp = false;
			if (auto _ = ui_component_header(ecs::get_component_name_at<t_cmp, 0>().data(), remove_cmp))
			{
				c_auto disclosure_size = font::get_line_height(theme::text_heading_font_size());
				c_auto gap			   = theme::header_bar_child_gap();
				c_auto padding_l	   = theme::header_bar_padding().x;

				if (auto _ = widget::vertical(set_padding_left(disclosure_size + padding_l + gap)))
				{
					if constexpr (requires { ui_component(FWD(cmp), renderer); })
					{
						ui_component(FWD(cmp), renderer);
					}
					else
					{
						ui_component(FWD(cmp));
					}
				}
			}

			if (remove_cmp)
			{
				storage.remove_component<t_cmp>(ent_id, get_ecs_context(renderer));
			}
		}
	}


}	 // namespace age::editor

namespace age::editor
{
	bool
	ui_asset_header(asset::e::kind e_kind, asset::handle h_asset) noexcept;

	// template <asset::e::kind e_kind>
	// bool /*is_dirty*/
	// ui_asset(asset::handle) noexcept
	//{
	//	AGE_UNREACHABLE(std::format("no inspector for {}", to_string(e_kind)).data());
	//	return false;
	// }

	void
	ui_asset(asset::e::kind e_kind, asset::handle h_asset, auto& renderer) noexcept
	{
		auto is_dirty = asset::visit(e_kind, [&]<asset::e::kind k> { return ui_asset<k>(h_asset); });

		if (is_dirty is_false) { return; }
		// todo
	}
}	 // namespace age::editor

namespace age::editor::detail
{
	void
	ui_inspector_entity(auto& entities, auto& renderer, storage_editor_data& editor_storage) noexcept
	{
		using namespace age::ui;
		using enum input::e::key_kind;

		if (g::select_vec[editor_storage.code_idx].is_empty()) { return; }

		using t_storage			 = BARE_OF(entities);
		using t_ent_id			 = typename t_storage::t_ent_id;
		using t_archetype		 = typename t_storage::t_archetype;
		using t_archetype_traits = typename t_storage::t_archetype_traits;

		// todo : implement multiselection
		if (g::select_vec[editor_storage.code_idx].size() > 1) { return; }

		c_auto ent_id = static_cast<t_ent_id>(g::select_vec[editor_storage.code_idx][0]);

		for (c_auto archetype = entities.get_archetype(ent_id);
			 auto	storage_cmp_idx : age::views::each_set_bit_idx(archetype))
		{
			t_archetype_traits::visit_component(entities, ent_id, storage_cmp_idx, AGE_FUNC(ui_component_section), renderer);
		}

		c_auto archetype = entities.get_archetype(ent_id);
		detail::re_register_entity(editor_storage, ent_id, archetype);

		widget::separator_v();

		[&] {
			auto drop_down_panel = widget::begin(style::panel() | set_height_fit() | set_save_state());
			if (drop_down_panel is_false) { return; }
			auto drop_down_panel_state = drop_down_panel.get_state();

			if (auto add_cmp_btn = widget::button("+ add component", set_align_center(), set_width_grow());
				add_cmp_btn and add_cmp_btn.clicked())
			{
				drop_down_panel_state.toggled = !drop_down_panel_state.toggled;
			}

			if (drop_down_panel_state.toggled is_false)
			{
				drop_down_panel.get_state() = drop_down_panel_state;
				return;
			}

			widget::separator_v();

			auto drop_down_selected = drop_down_panel_state.drop_down_data.selected;

			for (auto storage_cmp_idx : views::loop(t_archetype_traits::cmp_count()))
			{
				c_auto already_has = archetype & (1ull << storage_cmp_idx);

				auto interact = widget::begin(style::horizontal() | set_interact(already_has is_false) | set_save_state(already_has is_false) | set_width_grow() | set_height_fit());

				if (interact is_false) { continue; }

				c_auto style_state = interact.pressed<mouse_left>() ? ui::e::style_state::active
								   : interact.contains_mouse()		? ui::e::style_state::hover
																	: ui::e::style_state::idle;

				[&] {
					if (interact.clicked() is_false) { return; }

					if (ui::g::p_input_ctx->is_ctrl_down())
					{
						drop_down_selected.flip(storage_cmp_idx);
						return;
					}

					if (c_auto bit_range = drop_down_selected.calc_set_range();
						ui::g::p_input_ctx->is_shift_down())
					{
						if (bit_range is_false)
						{
							drop_down_selected.set(storage_cmp_idx);
							return;
						}

						if (storage_cmp_idx < bit_range.min)
						{
							drop_down_selected.set_range(storage_cmp_idx, bit_range.min);
						}
						else if (storage_cmp_idx < bit_range.max)
						{
							drop_down_selected.set_range(bit_range.min, storage_cmp_idx);
						}
						else
						{
							drop_down_selected.set_range(bit_range.max, storage_cmp_idx);
						}
					}
					else
					{
						drop_down_selected.reset();
						drop_down_selected.set(storage_cmp_idx);
					}
				}();

				auto selected = drop_down_selected.test(storage_cmp_idx);

				if (auto _ = widget::begin(style::item(selected, style_state) | set_border_thickness(0.f) | set_width_grow() | set_height_fit() | set_padding_left(0)))
				{
					widget::separator_h(set_draw(selected), set_body_brush_data(theme::color_blue(), theme::opacity_medium()), set_width_fixed(theme::thickness_thick()));

					widget::indicator(ui::e::shape_kind::circle, font::get_line_height(theme::text_font_size()), float4{ get_component_color(storage_cmp_idx), already_has ? theme::opacity_medium() : 1.0f });

					widget::text(t_archetype_traits::get_component_name(storage_cmp_idx).data(), ui::e::style_state::idle, already_has is_false);
				}
			}

			drop_down_panel_state.drop_down_data.selected = drop_down_selected;

			widget::separator_v();
			if (auto add_cancel_panel = widget::horizontal_inv(set_height_fit()))
			{
				if (auto btn_add = widget::button("add");
					btn_add and btn_add.clicked())
				{
					add_components(entities, renderer, editor_storage, ent_id, drop_down_panel_state.drop_down_data.selected.extract<t_archetype>());

					drop_down_panel_state.drop_down_data.selected.reset();
					drop_down_panel_state.toggled = false;
				}


				if (auto btn_cancel = widget::button("cancel");
					btn_cancel and btn_cancel.clicked())
				{
					drop_down_panel_state.drop_down_data.selected.reset();
					drop_down_panel_state.toggled = false;
				}
			}

			drop_down_panel.get_state() = drop_down_panel_state;
		}();
	}

	void
	ui_inspector_asset(asset::e::kind e_kind, auto& ecs_game, auto& renderer)
	{
		using namespace ui;
		using enum asset::e::asset_path_error_kind;

		c_auto group_idx = to_idx(e_kind);
		if (group_idx >= g::select_vec.size()) { return; }

		auto& h_asset_vec = g::select_vec[group_idx];

		if (h_asset_vec.is_empty()) { return; }
		// todo : implement multiselection
		if (h_asset_vec.size() > 1) { return; }

		// static auto h_asset_prev	 = asset::handle{};
		// static auto display_name_buf = age::array<char, config::max_asset_display_name_len>{};
		// static auto show_rename_btn	 = false;
		// static auto last_error		 = none;

		c_auto h_asset = asset::handle{ cast_to<uint32>(h_asset_vec[0]) };

		AGE_ASSERT(h_asset.get_kind() == e_kind);
		AGE_ASSERT(asset::registry::is_registered(h_asset));


		if (ui_asset_header(e_kind, h_asset))
		{
			// todo, implement partial saving
			editor::save_game(ecs_game, renderer);
		}

		ui_asset(e_kind, h_asset, renderer);
	}

	void
	ui_entity_hierarchy_impl(auto& entities, auto& renderer, storage_editor_data& editor_storage) noexcept
	{
		using namespace age::ui;
		using enum age::ui::e::style_state;
		using enum input::e::key_kind;

		using t_ent_id = typename BARE_OF(entities)::t_ent_id;

		if (auto _ = widget::vertical(set_child_gap(0)))
		{
			auto is_open = false;

			if (auto header = widget::begin(style::header_bar() | set_interact() | set_save_state()))
			{
				if (header.clicked<mouse_left>())
				{
					header.toggle();
				}

				// is_open = header.is_toggled() != editor_storage.default_open;
				is_open = header.is_toggled() is_false;

				widget::disclosure_indicator(is_open);

				if (auto _ = widget::begin(set_width_grow() | set_height_fit()))
				{
					widget::text_input2(editor_storage.names[0]);
				}

				// if (auto _ = widget::begin(set_padding(theme::frame_padding())))
				//{
				//	widget::text_heading(editor_storage.names[0].data());
				// }

				if (header.contains_mouse())
				{
					auto _			 = widget::horizontal_inv();
					auto new_ent_btn = widget::begin(style::vertical() | set_width_fit() | set_height_fit() | set_interact() | set_align_center());
					widget::text_button("+");
					if (new_ent_btn.clicked())
					{
						new_entity(entities, renderer, editor_storage, 0);
					}
				}
			}

			if (is_open is_false) { return; }

			auto h_panel = widget::panel(set_vertical() | set_height_fit() | set_padding_left(theme::frame_padding().x));

			for (const auto&& [arch_idx, arch] : editor_storage.archetype_data_vec | std::views::enumerate)
			{
				if (arch.entity_data_vec.empty()) { continue; }

				auto arch_open = false;

				if (auto header = widget::begin(style::header_bar() | set_interact() | set_save_state()))
				{
					if (header.clicked<mouse_left>())
					{
						header.toggle();
					}

					// is_open = header.is_toggled() != arch.default_open;
					arch_open = header.is_toggled() is_false;

					widget::disclosure_indicator(arch_open);

					if (auto _ = widget::begin(set_width_grow() | set_height_fit()))
					{
						widget::text_input2(arch.name);
					}

					if (auto _ = widget::horizontal_inv())
					{
						auto new_ent_btn = widget::begin(style::vertical() | set_width_fit() | set_height_fit() | set_interact() | set_align_center());
						widget::begin(style::text_button("+") | set_draw(header.contains_mouse()));
						if (new_ent_btn.clicked())
						{
							new_entity(entities, renderer, editor_storage, static_cast<uint32>(arch_idx), arch.archetype);
						}
					}
				}

				if (arch_open is_false) { continue; }

				auto h_inner_panel = widget::panel(set_vertical() | set_height_fit() | set_padding_left(theme::frame_padding().x));

				auto remove_vec = age::vector<uint64>{};
				auto add_vec	= age::vector<uint64>{};
				for (const auto&& [ent_idx, ent] : arch.entity_data_vec | std::views::enumerate)
				{
					c_auto selected = is_selected(e::select_kind::entity, editor_storage.code_idx, ent.id);
					ui_entity_tree_node(editor_storage, ent.id, arch.archetype, selected);

					if (selected is_false) { continue; }

					if (ui::g::p_input_ctx->is_pressed(input::e::key_kind::key_delete))
					{
						entities.remove_entity(static_cast<t_ent_id>(ent.id), get_ecs_context(renderer));

						remove_select(e::select_kind::entity, editor_storage.code_idx, ent.id);

						remove_vec.emplace_back(ent_idx);
					}
				}

				for (auto ent_idx : remove_vec)
				{
					auto& ent = arch.entity_data_vec[ent_idx];
					detail::unregister_entity(editor_storage, static_cast<uint32>(arch_idx), ent_idx, ent.id);
				}
			}
		}
	}	 // namespace detail
}	 // namespace age::editor::detail

namespace age::editor
{
	void
	ui_inspector(auto& ecs_game, auto& renderer) noexcept
	{
		switch (g::current_select_kind)
		{
		case e::select_kind::entity:
		{
			auto& current_scene = g::current_game.scene_data_vec[g::current_game.current_active_scene_idx];
			for (auto& storage_data : current_scene.storage_data_vec)
			{
				ecs_game.visit_storage_at(current_scene.code_idx, storage_data.code_idx, AGE_FUNC(detail::ui_inspector_entity), renderer, storage_data);
			}

			break;
		}
		case e::select_kind::asset:
		{
			asset::for_each_kind(AGE_LAMBDA(<asset::e::kind e_kind>(auto& ecs_game, auto& renderer), { detail::ui_inspector_asset(e_kind, ecs_game, renderer); }), ecs_game, renderer);
			break;
		}
		default:
		{
			break;
		}
		}
	}

	void
	ui_entity_hierarchy(auto& ecs_game, auto& renderer) noexcept
	{
		using namespace age::ui;

		static auto scene_dropdown_option = age::array<widget::dropdown_option<uint32>, ecs_game.scene_count()>{};

		for (auto&& [scene_idx, editor_scene_data] : g::current_game.scene_data_vec | views::enumerate<uint32>)
		{
			scene_dropdown_option[scene_idx].value = scene_idx;
			scene_dropdown_option[scene_idx].label = std::string_view{ editor_scene_data.names[0].data() };
		}

		if (auto _ = widget::horizontal(set_width_grow(), set_height_fit(), set_padding(theme::frame_padding())))
		{
			if (auto _ = widget::vertical(set_width_grow(), set_height_fit(), set_align_center()))
			{
				widget::begin(style::text_title("hierarchy") | set_align_begin());
			}

			if (widget::dropdown(g::current_game.current_active_scene_idx, std::span<const widget::dropdown_option<uint32>>{ scene_dropdown_option.data(), scene_dropdown_option.size() }))
			{
				// deinit scene?
			}
		}

		// widget::separator_v();

		auto& current_scene = g::current_game.scene_data_vec[g::current_game.current_active_scene_idx];


		for (auto& storage_data : current_scene.storage_data_vec)
		{
			ecs_game.visit_storage_at(current_scene.code_idx, storage_data.code_idx, AGE_FUNC(detail::ui_entity_hierarchy_impl), renderer, storage_data);
		}
	}

	void
	ui_scene_view(auto& renderer) noexcept
	{
		using namespace ui;
		if (auto h_game_scene = widget::begin(style::vertical() | set_width_grow() | set_height_grow() | set_padding_top(theme::padding_large())))
		{
			// g::scene_view_focused = h_game_scene.hovered_all();

			auto h_top_panel = widget::begin(style::horizontal() | set_align_center() | set_width_grow() | set_height_fit());
			widget::begin(set_height_fixed(0) | set_width_fixed(200));
			if (auto h_top_center = widget::begin(style::vertical() | set_width_grow() | set_height_fit()))
			{
				auto h_play_pause_stop = widget::begin(style::horizontal() | set_align_center() | set_width_fit() | set_height_fit());
				if (auto _ = widget::toggle_button(ui::e::shape_kind::triangle, 30, theme::color_text_green(), theme::palette_light_green(), theme::palette_green(), set_rotation(age::cvt_to_radian(30.f))))
				{
				}
				if (auto _ = widget::toggle_button(ui::e::shape_kind::circle, 30, theme::color_text_amber(), theme::palette_light_gold(), theme::palette_amber()))
				{
				}
				if (auto _ = widget::toggle_button(ui::e::shape_kind::rounded_rect, 30, theme::color_text_red(), theme::palette_light_red(), theme::palette_red(), set_shape_data(theme::roundness_small())))
				{
				}
			}

			if (auto h_top_right_panel = widget::begin(set_width_fixed(200) | set_height_fit()))
			{
				auto is_world = g::gizmo_space == e::transform_space_kind::world;

				widget::checkbox("world", is_world);

				g::gizmo_space = is_world ? e::transform_space_kind::world : e::transform_space_kind::local;
			}
		}
	}
}	 // namespace age::editor

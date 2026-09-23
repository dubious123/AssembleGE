#include "age_pch.hpp"
#include "age.hpp"

namespace age::editor::detail
{
	void
	ui_inspector_entity(uint32 editor_scene_idx, uint32 editor_storage_idx, std::span<const uint64> ecs_entity_id_select_span) noexcept
	{
		using namespace age::ui;
		using enum input::e::key_kind;

		auto& editor_scene	 = g::current_game.scene_data_vec[editor_scene_idx];
		auto& editor_storage = editor_scene.storage_data_vec[editor_storage_idx];

		c_auto ecs_entity_id = ecs_entity_id_select_span[0];

		g::host_ops.p_ui_component_section(editor_scene_idx, editor_storage_idx, ecs_entity_id);

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

			for (c_auto archetype = get_archetype(editor_scene_idx, editor_storage_idx, ecs_entity_id);
				 c_auto ecs_component_id : views::loop(get_component_count(editor_scene_idx, editor_storage_idx)))
			{
				c_auto already_has = archetype & (1ull << ecs_component_id);

				auto interact = widget::begin(style::horizontal() | set_interact(already_has is_false) | set_save_state(already_has is_false) | set_width_grow() | set_height_fit());

				if (interact is_false) { continue; }

				c_auto style_state = interact.pressed<mouse_left>() ? ui::e::style_state::active
								   : interact.contains_mouse()		? ui::e::style_state::hover
																	: ui::e::style_state::idle;

				[&] {
					if (interact.clicked() is_false) { return; }

					if (ui::g::p_input_ctx->is_ctrl_down())
					{
						drop_down_selected.flip(ecs_component_id);
						return;
					}

					if (c_auto bit_range = drop_down_selected.calc_set_range();
						ui::g::p_input_ctx->is_shift_down())
					{
						if (bit_range is_false)
						{
							drop_down_selected.set(ecs_component_id);
							return;
						}

						if (ecs_component_id < bit_range.min)
						{
							drop_down_selected.set_range(ecs_component_id, bit_range.min);
						}
						else if (ecs_component_id < bit_range.max)
						{
							drop_down_selected.set_range(bit_range.min, ecs_component_id);
						}
						else
						{
							drop_down_selected.set_range(bit_range.max, ecs_component_id);
						}
					}
					else
					{
						drop_down_selected.reset();
						drop_down_selected.set(ecs_component_id);
					}
				}();

				auto selected = drop_down_selected.test(ecs_component_id);

				if (auto _ = widget::begin(style::item(selected, style_state) | set_border_thickness(0.f) | set_width_grow() | set_height_fit() | set_padding_left(0)))
				{
					widget::separator_h(set_draw(selected), set_body_brush_data(theme::color_blue(), theme::opacity_medium()), set_width_fixed(theme::thickness_thick()));

					widget::indicator(ui::e::shape_kind::circle, font::get_line_height(theme::text_font_size()), float4{ get_component_color(ecs_component_id), already_has ? theme::opacity_medium() : 1.0f });

					widget::text(get_component_name(editor_scene_idx, editor_storage_idx, ecs_component_id).data(), ui::e::style_state::idle, already_has is_false);
				}
			}

			drop_down_panel_state.drop_down_data.selected = drop_down_selected;

			widget::separator_v();
			if (auto add_cancel_panel = widget::horizontal_inv(set_height_fit()))
			{
				if (auto btn_add = widget::button("add");
					btn_add and btn_add.clicked())
				{
					add_components(editor_scene_idx, editor_storage_idx, ecs_entity_id, drop_down_panel_state.drop_down_data.selected.extract<uint64>());

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
}	 // namespace age::editor::detail

namespace age::editor::detail
{
	bool
	ui_asset_header(asset::e::kind e_kind, asset::handle h_asset) noexcept
	{
		using enum asset::e::asset_path_error_kind;
		using namespace ui;

		static auto h_asset_prev	 = asset::handle{};
		static auto display_name_buf = age::array<char, config::max_asset_display_name_len>{};
		static auto show_rename_btn	 = false;
		static auto last_error		 = none;

		auto need_save = false;

		AGE_ASSERT(h_asset.get_kind() == e_kind);
		AGE_ASSERT(asset::registry::is_registered(h_asset));

		if (h_asset != h_asset_prev)
		{
			h_asset_prev	 = h_asset;
			display_name_buf = h_asset.get_display_name();
			show_rename_btn	 = false;
			last_error		 = none;
		}

		auto input_detected = false;
		// auto asset_header	= widget::begin(style::layout(ui::e::widget_layout::vertical) | set_width_grow() | set_height_fit());
		auto asset_header = widget::begin(style::header_bar() | set_vertical() | set_width_grow() | set_height_fit());

		if (auto _ = widget::begin(style::header_bar()))
		{
			widget::text_heading(to_string(e_kind).data());

			input_detected = widget::text_input3(display_name_buf);
		}

		widget::separator_v();

		if (asset_header is_false) { return need_save; }

		if (input_detected) { show_rename_btn = true; }

		if (auto _ = ui::id_begin();
			show_rename_btn)
		{
			auto btn_header = widget::begin(style::header_bar());

			if (auto btn = widget::button("rename");
				btn and (btn.clicked() /*or ui::g::p_input_ctx->is_pressed(input::e::key_kind::key_enter)*/))
			{
				show_rename_btn = false;

				c_auto path_arr = editor::get_asset_full_path(e_kind, std::string_view{ display_name_buf.data() });
				last_error		= asset::validate_asset_path(e_kind, h_asset, path_arr);

				if (last_error == none)
				{
					if (asset::visit(e_kind, [&]<asset::e::kind k> { return asset::update_asset_path<k>(h_asset, path_arr); }))
					{
						need_save		 = true;
						display_name_buf = h_asset.get_display_name();
					}
					else
					{
						last_error = io_failed;
					}
				}
			}

			if (auto btn = widget::button("cancel");
				btn and (btn.clicked() or ui::g::p_input_ctx->is_pressed(input::e::key_kind::key_escape)))
			{
				show_rename_btn	 = false;
				last_error		 = none;
				display_name_buf = h_asset.get_display_name();
			}

			widget::separator_v();
		}

		auto _ = ui::id_begin();

		if (to_idx(last_error) > to_idx(fixable_by_normalize_begin))
		{
			auto btn_header = widget::begin(style::header_bar());

			if (auto btn = widget::button("normalize");
				btn and btn.clicked())
			{
				auto path_arr = editor::get_asset_full_path(e_kind, std::string_view{ display_name_buf.data() });
				asset::normalize_asset_path(e_kind, path_arr);

				display_name_buf = asset::visit(e_kind, [&]<asset::e::kind k> { return asset::get_display_name<k>(path_arr); });
				last_error		 = none;
				show_rename_btn	 = true;	// the name changed - let the user commit it
			}

			widget::separator_v();
		}

		if (last_error != none)
		{
			widget::text(asset::get_path_error_msg(last_error).data());
			need_save = false;
		}

		return need_save;
	}

	void
	ui_inspector_asset(asset::e::kind asset_kind) noexcept
	{
		using namespace ui;
		using enum asset::e::asset_path_error_kind;

		c_auto group_idx = to_idx(asset_kind);
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

		AGE_ASSERT(h_asset.get_kind() == asset_kind);
		AGE_ASSERT(asset::registry::is_registered(h_asset));

		if (c_auto need_save = ui_asset_header(asset_kind, h_asset))
		{
			// todo, implement partial saving
			editor::save_game();
		}

		c_auto is_dirty = asset::visit(asset_kind, [&]<asset::e::kind k> { return ui_asset<k>(h_asset); });

		if (is_dirty is_false) { return; }
		// todo
	}
}	 // namespace age::editor::detail

namespace age::editor
{
	void
	ui_inspector() noexcept
	{
		switch (g::current_select_kind)
		{
		case e::select_kind::entity:
		{
			for (const auto&& [editor_storage_idx, ecs_entity_id_select_vec] : g::select_vec | views::enumerate<uint32>)
			{
				if (has_selection(e::select_kind::entity, editor_storage_idx) is_false) { continue; }
				// todo : implement multi-selection
				if (g::select_vec[editor_storage_idx].size() > 1) { continue; }

				detail::ui_inspector_entity(g::current_game.current_active_scene_idx, editor_storage_idx, ecs_entity_id_select_vec);
			}

			break;
		}
		case e::select_kind::asset:
		{
			asset::for_each_kind(AGE_LAMBDA(<asset::e::kind e_kind>, { detail::ui_inspector_asset(e_kind); }));
			break;
		}
		default:
		{
			break;
		}
		}
	}
}	 // namespace age::editor
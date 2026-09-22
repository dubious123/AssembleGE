#include "age_pch.hpp"
#include "age.hpp"

namespace age::editor::detail
{
	ui::widget_ctx
	ui_entity_tree_node(storage_editor_data& editor_storage, uint64 ecs_ent_id, uint64 archetype, bool selected) noexcept
	{
		using namespace age::ui;
		using enum input::e::key_kind;

		c_auto child_padidng_left = theme::thickness_thick() + theme::item_child_gap() + theme::thickness_thick() + theme::item_child_gap();
		auto   is_opened		  = false;

		auto&& [arch_idx, ent_idx] = editor_storage.ecs_ent_id_to_editor_location_map[ecs_ent_id];

		if (auto interact = widget::begin(style::vertical() | set_interact(true) | set_save_state(true)))
		{
			auto style_state = ui::e::style_state::idle;
			if (interact.pressed<mouse_left>())
			{
				style_state = ui::e::style_state::active;
			}
			else if (interact.contains_mouse())
			{
				style_state = ui::e::style_state::hover;
			}

			if (interact.double_clicked())
			{
				g::set_focus = true;
			}

			if (interact.clicked())
			{
				if (ui::g::p_input_ctx->is_ctrl_down())
				{
					if (selected)
					{
						editor::remove_select(e::select_kind::entity, editor_storage.code_idx, ecs_ent_id);
					}
					else
					{
						editor::add_select(e::select_kind::entity, editor_storage.code_idx, ecs_ent_id);
					}
				}
				else if (ui::g::p_input_ctx->is_shift_down())
				{
					if (c_auto p_ecs_ent_id_last = editor::last_selected(e::select_kind::entity, editor_storage.code_idx))
					{
						auto&& [last_arch_idx, last_ent_idx] = editor_storage.ecs_ent_id_to_editor_location_map[*p_ecs_ent_id_last];
						c_auto& arch_data					 = editor_storage.archetype_data_vec[last_arch_idx];

						if (archetype == arch_data.archetype)
						{
							for (auto i = std::min(ent_idx, last_ent_idx); i <= std::max(ent_idx, last_ent_idx); ++i)
							{
								editor::add_select(e::select_kind::entity, editor_storage.code_idx, arch_data.entity_data_vec[i].id);
							}
						}
					}
					else
					{
						editor::add_select(e::select_kind::entity, editor_storage.code_idx, ecs_ent_id);
					}
				}
				else
				{
					editor::clear_select();

					editor::add_select(e::select_kind::entity, editor_storage.code_idx, ecs_ent_id);
				}
			}


			if (auto _ = widget::begin(style::item(selected, style_state) | set_border_thickness(0) | set_padding_left(0)))
			{
				widget::separator_h(set_draw(selected), set_body_brush_data(theme::color_blue(), theme::opacity_medium()), set_width_fixed(theme::thickness_thick()));

				c_auto disclosure_indicator_size = font::get_line_height(theme::text_font_size());

				if (auto btn = widget::begin(style::horizontal() | set_interact(true) | set_width_fit() | set_height_fit() | set_align_center()))
				{
					auto& btn_state = btn.get_state();
					if (btn.clicked())
					{
						btn_state.toggled = btn_state.toggled is_false;
					}

					is_opened = btn_state.toggled;

					widget::disclosure_indicator(btn_state.toggled, disclosure_indicator_size);
				}


				widget::text_input(editor_storage.archetype_data_vec[arch_idx].entity_data_vec[ent_idx].name.data(), config::max_entity_name_len);

				// widget::text(p_name);

				if (auto _ = widget::begin(set_horizontal_inv() | set_width_grow() | set_height_fit() | set_child_gap(theme::gap_large())))
				{
					char arch_buf[24];
					util::to_str<16, 8>(arch_buf, archetype, "0x");
					widget::text_hint(arch_buf);

					widget::separator_h(set_width_fixed(theme::thickness_thick()), set_body_brush_data(theme::color_gray_light()));

					char id_buf[24];
					util::to_str(id_buf, ecs_ent_id, "#");
					widget::text_hint(id_buf);
				}
			}
		}

		if (is_opened)
		{
			return widget::vertical(set_padding_left(child_padidng_left));
		}
		else
		{
			return {};
		}
	}

	void
	ui_entity_hierarchy_impl(const uint32 editor_scene_idx, const uint32 editor_storage_idx, storage_editor_data& editor_storage) noexcept
	{
		using namespace age::ui;
		using enum age::ui::e::style_state;
		using enum input::e::key_kind;

		static auto ecs_remove_entity_id_vec = age::vector<uint64>{};

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

				if (header.contains_mouse())
				{
					auto _			 = widget::horizontal_inv();
					auto new_ent_btn = widget::begin(style::vertical() | set_width_fit() | set_height_fit() | set_interact() | set_align_center());
					widget::text_button("+");
					if (new_ent_btn.clicked())
					{
						c_auto ent_name_buf = util::to_fixed_str<config::max_entity_name_len>(std::format("new_entity_{}", editor_storage.entity_count + 1));
						add_entity(editor_scene_idx, editor_storage_idx, ent_name_buf.data());
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
							c_auto ent_name_buf = util::to_fixed_str<config::max_entity_name_len>(std::format("new_entity_{}", editor_storage.entity_count + 1));
							add_entity(editor_scene_idx, editor_storage_idx, ent_name_buf.data(), arch.archetype);
						}
					}
				}

				if (arch_open is_false) { continue; }

				auto h_inner_panel = widget::panel(set_vertical() | set_height_fit() | set_padding_left(theme::frame_padding().x));

				ecs_remove_entity_id_vec.clear();
				for (const auto&& [ent_idx, ent] : arch.entity_data_vec | std::views::enumerate)
				{
					c_auto selected = is_selected(e::select_kind::entity, editor_storage.code_idx, ent.id);
					detail::ui_entity_tree_node(editor_storage, ent.id, arch.archetype, selected);

					if (selected is_false) { continue; }

					if (ui::g::p_input_ctx->is_pressed(input::e::key_kind::key_delete))
					{
						ecs_remove_entity_id_vec.emplace_back(ent.id);
					}
				}

				for (c_auto ecs_entity_id : ecs_remove_entity_id_vec | std::views::reverse)
				{
					remove_select(e::select_kind::entity, editor_storage.code_idx, ecs_entity_id);
					remove_entity(editor_scene_idx, editor_storage_idx, ecs_entity_id);
				}
			}
		}
	}
}	 // namespace age::editor::detail

namespace age::editor
{
	void
	ui_entity_hierarchy() noexcept
	{
		using namespace age::ui;

		static auto scene_dropdown_option = age::vector<widget::dropdown_option<uint32>>{};
		scene_dropdown_option.clear();
		scene_dropdown_option.reserve(g::current_game.scene_data_vec.size());
		for (auto&& [scene_idx, editor_scene_data] : g::current_game.scene_data_vec | views::enumerate<uint32>)
		{
			scene_dropdown_option.emplace_back(widget::dropdown_option<uint32>{
				.value = scene_idx,
				.label = editor_scene_data.names[0].data(),
			});
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

		for (auto& current_scene = g::current_game.get_current_scene();
			 auto&& [editor_storage_id, storage_data] : current_scene.storage_data_vec | views::enumerate<uint32>)
		{
			detail::ui_entity_hierarchy_impl(g::current_game.current_active_scene_idx, editor_storage_id, storage_data);
		}
	}
}	 // namespace age::editor
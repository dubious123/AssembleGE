#include "age.hpp"

namespace age::editor
{
	void
	add_components(auto& storage, auto& renderer, auto& editor_storage, auto ent_id, auto archetype) noexcept
	{
		using t_storage			 = BARE_OF(storage);
		using t_ent_id			 = typename t_storage::t_ent_id;
		using t_archetype		 = typename t_storage::t_archetype;
		using t_archetype_traits = typename t_storage::t_archetype_traits;

		static_assert(std::is_same_v<t_ent_id, BARE_OF(ent_id)>);
		static_assert(std::is_same_v<t_archetype, BARE_OF(archetype)>);

		for (auto storage_cmp_idx : age::views::each_set_bit_idx(archetype))
		{
			switch (storage_cmp_idx)
			{
#define X(N)                                                                                                               \
	case N:                                                                                                                \
	{                                                                                                                      \
		if constexpr (N < t_archetype_traits::cmp_count())                                                                 \
		{                                                                                                                  \
			g::command_buf.add_component<typename t_archetype_traits::template t_component<N>>(storage, renderer, ent_id); \
			break;                                                                                                         \
		}                                                                                                                  \
		else                                                                                                               \
		{                                                                                                                  \
			AGE_UNREACHABLE();                                                                                             \
		}                                                                                                                  \
	}
				__X_REPEAT_LIST_512
#undef X
			default:
			{
				AGE_UNREACHABLE();
			}
			}
		}
	}
}	 // namespace age::editor

namespace age::editor
{
	ui::widget_ctx
	ui_entity_tree_node(storage_editor_data&, uint64 ent_id, uint64 archetype, bool selected) noexcept;
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
	ui_component(ecs::material& mat) noexcept;

	void
	ui_component(ecs::directional_light& light) noexcept;

	void
	ui_component(ecs::point_light& light) noexcept;

	void
	ui_component(ecs::spot_light& light) noexcept;

	void
	ui_component(age::ecs::camera& cam) noexcept;

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
					ui_component(FWD(cmp));
				}
			}

			if (remove_cmp)
			{
				g::command_buf.remove_component<t_cmp>(storage, renderer, ent_id);
			}
		}
	}

	namespace detail
	{
		void
		ui_inspector_impl(auto& entities, auto& renderer, storage_editor_data& editor_storage) noexcept
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

			c_auto archetype = entities.get_archetype(ent_id);

			for (auto storage_cmp_idx : age::views::each_set_bit_idx(archetype))
			{
				t_archetype_traits::visit_component(entities, ent_id, storage_cmp_idx, AGE_FUNC(ui_component_section), renderer);
			}

			widget::separator_v();

			if (auto drop_down = widget::begin(style::panel() | set_height_fit() | set_save_state(true)))
			{
				auto drop_down_state = drop_down.get_state();

				if (auto add_cmp_btn = widget::button("+ add component", set_align_center(), set_width_grow()))
				{
					auto& state = add_cmp_btn.get_state();
					if (add_cmp_btn.clicked())
					{
						drop_down_state.toggled = !drop_down_state.toggled;
					}
				}

				if (drop_down_state.toggled)
				{
					widget::separator_v();

					auto drop_down_selected = drop_down_state.drop_down_data.selected;

					for (auto storage_cmp_idx : views::loop(t_archetype_traits::cmp_count()))
					{
						c_auto already_has = archetype & (1ull << storage_cmp_idx);

						if (auto interact = widget::begin(style::horizontal() | set_interact(already_has is_false) | set_save_state(already_has is_false) | set_width_grow() | set_height_fit()))
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

							if (ui::g::p_input_ctx->is_shift_down())
							{
								if (interact.clicked())
								{
									drop_down_selected.flip(storage_cmp_idx);
								}
							}
							else
							{
								if (interact.clicked())
								{
									drop_down_selected.reset();
									drop_down_selected.set(storage_cmp_idx);
								}
							}

							auto selected = drop_down_selected.test(storage_cmp_idx);

							if (auto _ = widget::begin(style::item(selected, style_state) | set_border_thickness(0.f) | set_width_grow() | set_height_fit() | set_padding_left(0)))
							{
								widget::separator_h(set_draw(selected), set_body_brush_data(theme::color_blue(), theme::opacity_medium()), set_width_fixed(theme::thickness_thick()));

								widget::indicator(ui::e::shape_kind::circle, font::get_line_height(theme::text_font_size()), float4{ get_component_color(storage_cmp_idx), already_has ? theme::opacity_medium() : 1.0f });

								widget::text(t_archetype_traits::get_component_name(storage_cmp_idx).data(), ui::e::style_state::idle, already_has is_false);
							}
						}
					}

					drop_down_state.drop_down_data.selected = drop_down_selected;

					widget::separator_v();
					if (auto _ = widget::horizontal_inv(set_height_fit()))
					{
						if (auto btn_add = widget::button("add"))
						{
							if (btn_add.clicked())
							{
								add_components(entities, renderer, editor_storage, ent_id, drop_down_state.drop_down_data.selected.extract<t_archetype>());

								drop_down_state.drop_down_data.selected.reset();
								drop_down_state.toggled = false;
							}
						}


						if (auto btn_cancel = widget::button("cancel"))
						{
							if (btn_cancel.clicked())
							{
								drop_down_state.drop_down_data.selected.reset();
								drop_down_state.toggled = false;
							}
						}
					}
				}


				drop_down.get_state() = drop_down_state;
			}


			g::command_buf.flush(entities, renderer, g::ui_new_entity_buffer);

			c_auto new_archetype = entities.get_archetype(ent_id);

			detail::re_register_entity(editor_storage, ent_id, new_archetype);
		}
	}	 // namespace detail

	void
	ui_inspector(auto& editor_game, auto& renderer) noexcept
	{
		auto& current_scene = g::current_game.scene_data_vec[g::current_game.current_active_scene_idx];
		for (auto& storage_data : current_scene.storage_data_vec)
		{
			editor_game.visit_storage_at(current_scene.code_idx, storage_data.code_idx, AGE_FUNC(detail::ui_inspector_impl), renderer, storage_data);
		}
	}
}	 // namespace age::editor

namespace age::editor
{
	namespace detail
	{
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

				if (auto header = widget::begin(style::header_bar() | set_interact(true) | set_save_state(true)))
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
						if (auto _ = widget::horizontal_inv())
						{
							if (auto new_ent_btn = widget::begin(style::vertical() | set_width_fit() | set_height_fit() | set_interact(true) | set_align_center()))
							{
								widget::text_button("+");
								if (new_ent_btn.clicked())
								{
									// g::ui_new_entity_with_archetype_buffer.emplace_back(0);
									g::command_buf.new_entity(entities, renderer);
								}
							}
						}
					}
				}

				if (is_open)
				{
					if (auto _ = widget::panel(set_vertical() | set_height_fit() | set_padding_left(theme::frame_padding().x)))
					{
						// todo, we don't need cmd buffer for this.
						for (const auto&& [arch_idx, arch] : editor_storage.archetype_data_vec | std::views::enumerate)
						{
							auto arch_open = false;

							if (auto header = widget::begin(style::header_bar() | set_interact(true) | set_save_state(true)))
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
									if (auto new_ent_btn = widget::begin(style::vertical() | set_width_fit() | set_height_fit() | set_interact(true) | set_align_center()))
									{
										// widget::text_button("+");
										widget::begin(style::text_button("+") | set_draw(header.contains_mouse()));
										if (new_ent_btn.clicked())
										{
											// g::command_buf.new_entity(entities, renderer);
											g::ui_new_entity_with_archetype_buffer.emplace_back(arch.archetype);
										}
									}
								}
							}

							if (arch_open is_false) { continue; }

							if (auto _ = widget::panel(set_vertical() | set_height_fit() | set_padding_left(theme::frame_padding().x)))
							{
								for (const auto&& [ent_idx, ent] : arch.entity_data_vec | std::views::enumerate)
								{
									c_auto selected = is_selected(e::select_kind::entity, editor_storage.code_idx, ent.id);
									ui_entity_tree_node(editor_storage, ent.id, arch.archetype, selected);

									if (selected and ui::g::p_input_ctx->is_pressed(input::e::key_kind::key_delete))
									{
										g::command_buf.remove_entity(entities, renderer, static_cast<t_ent_id>(ent.id));
										remove_select(e::select_kind::entity, editor_storage.code_idx, ent.id);

										detail::unregister_entity(editor_storage, static_cast<uint32>(arch_idx), ent.id, ent_idx);
									}
								}
							}
						}
					}
				}
			}

			AGE_ASSERT(g::ui_new_entity_buffer.is_empty());

			for (auto& arch : g::ui_new_entity_with_archetype_buffer)
			{
				c_auto ent_id = entities.new_entity();
				g::ui_new_entity_buffer.emplace_back(ent_id);
				add_components(entities, renderer, editor_storage, ent_id, static_cast<typename BARE_OF(entities)::t_archetype>(arch));
			}

			g::command_buf.flush(entities, renderer, g::ui_new_entity_buffer);

			g::ui_new_entity_with_archetype_buffer.clear();

			for (auto new_ent_id : g::ui_new_entity_buffer)
			{
				auto archetype = entities.get_archetype(static_cast<typename BARE_OF(entities)::t_ent_id>(new_ent_id));
				auto found	   = false;

				auto new_ent_data = entity_editor_data{
					.id	  = new_ent_id,
					.name = util::to_fixed_str<config::max_entity_name_len>(std::format("new_entity_{}", editor_storage.entity_count++))
				};

				for (auto&& [arch_idx, arch_data] : editor_storage.archetype_data_vec | std::views::enumerate)
				{
					if (arch_data.archetype == archetype)
					{
						editor_storage.id_to_editor_location_map[new_ent_id] = std::pair{ static_cast<uint32>(arch_idx), arch_data.entity_data_vec.size<uint64>() };
						arch_data.entity_data_vec.emplace_back(std::move(new_ent_data));
						found = true;
						break;
					}
				}

				if (found is_false)
				{
					editor_storage.id_to_editor_location_map[new_ent_id] = std::pair{ editor_storage.archetype_data_vec.size<uint32>(), 0 };
					auto& arch_data										 = editor_storage.archetype_data_vec.emplace_back();
					arch_data.archetype									 = archetype;
					arch_data.entity_data_vec.emplace_back(std::move(new_ent_data));
				}
			}

			g::ui_new_entity_buffer.clear();
		}
	}	 // namespace detail

	void
	ui_entity_hierarchy(auto& editor_game, auto& renderer) noexcept
	{
		using namespace age::ui;
		auto& current_scene = g::current_game.scene_data_vec[g::current_game.current_active_scene_idx];

		g::select_vec.resize(current_scene.storage_data_vec.size());

		if (auto _ = widget::horizontal(set_width_grow(), set_height_fit(), set_padding(theme::frame_padding())))
		{
			if (auto _ = widget::vertical(set_width_grow(), set_height_fit(), set_align_center()))
			{
				widget::begin(style::text_title("hierarchy") | set_align_begin());
			}
		}

		// widget::separator_v();

		for (auto& storage_data : current_scene.storage_data_vec)
		{
			editor_game.visit_storage_at(current_scene.code_idx, storage_data.code_idx, AGE_FUNC(detail::ui_entity_hierarchy_impl), renderer, storage_data);
		}
	}

	void
	ui_scene_view(auto& renderer, platform::window_handle h_window) noexcept
	{
		using namespace ui;
		if (auto h_game_scene = widget::begin(style::vertical() | set_width_grow() | set_height_grow() | set_padding_top(theme::padding_large()) | set_interact(true)))
		{
			age::editor::update_camera(renderer, h_game_scene.focused(), h_window);

			if (auto h_play_pause_stop = widget::begin(style::horizontal() | set_align_center() | set_width_fit() | set_height_fit()))
			{
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
		}
	}

	inline void
	ui_asset() noexcept
	{
	}
}	 // namespace age::editor

#include "age.hpp"

namespace age::editor
{
	void
	add_components(auto& storage, auto& renderer, auto ent_id, auto archetype) noexcept
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
	ui_entity_tree_node(const char* p_name, uint64 ent_id, uint64 archetype, bool selected) noexcept;
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
			if (auto _ = ui_component_header(ecs::get_component_name<t_cmp>(), remove_cmp))
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

	void
	ui_inspector(auto& entities, auto& renderer) noexcept
	{
		using namespace ui;
		using enum input::e::key_kind;

		if (g::select_vec.is_empty()) { return; }

		using t_storage			 = BARE_OF(entities);
		using t_ent_id			 = typename t_storage::t_ent_id;
		using t_archetype		 = typename t_storage::t_archetype;
		using t_archetype_traits = typename t_storage::t_archetype_traits;

		// todo : implement multiselection
		if (g::select_vec.size() > 1) { return; }

		c_auto ent_id = static_cast<t_ent_id>(g::select_vec[0]);

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
						auto style_state = e::style_state::idle;
						if (interact.pressed<mouse_left>())
						{
							style_state = e::style_state::active;
						}
						else if (interact.contains_mouse())
						{
							style_state = e::style_state::hover;
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

							widget::text(t_archetype_traits::get_component_name(storage_cmp_idx), e::style_state::idle, already_has is_false);
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
							add_components(entities, renderer, ent_id, drop_down_state.drop_down_data.selected.extract<t_archetype>());

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


		g::command_buf.flush(entities, renderer);
	}
}	 // namespace age::editor

namespace age::editor
{
	void
	ui_entity_hierarchy(auto& entities, auto& renderer) noexcept
	{
		using namespace ui;
		if (auto _ = widget::vertical())
		{
			if (auto _ = widget::horizontal(set_width_grow(), set_height_fit(), set_padding(theme::frame_padding())))
			{
				if (auto _ = widget::vertical(set_width_grow(), set_height_fit(), set_align_center()))
				{
					widget::begin(style::text_heading("hierarchy") | set_align_begin());
				}

				if (auto new_ent_btn = widget::button("+ entity"))
				{
					if (new_ent_btn.clicked())
					{
						g::command_buf.new_entity(entities, renderer);
					}
				}
			}

			widget::separator_v();

			for (auto&& [ent_id, ent_arch] : entities | each_entity(ecs::query<ecs::sv_entity_id, ecs::sv_archetype>()))
			{
				c_auto selected = is_selected(ent_id);
				ui_entity_tree_node("entity", ent_id, ent_arch, selected);

				if (selected and ui::g::p_input_ctx->is_pressed(input::e::key_kind::key_delete))
				{
					g::command_buf.remove_entity(entities, renderer, ent_id);
					remove_select(ent_id);
				}
			}
		}

		g::command_buf.flush(entities, renderer);
	}
}	 // namespace age::editor
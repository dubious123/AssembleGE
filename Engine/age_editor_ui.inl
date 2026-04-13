#include "age.hpp"

namespace age::editor
{
	void
	ui_select(uint64 ent_id) noexcept;

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
	ui_component_section(auto&& cmp, uint32 cmp_idx) noexcept
	{
		using namespace age::ui;
		if (auto _ = widget::begin(style::section() | set_horizontal() | set_height_fit() | set_width_grow()))
		{
			widget::separator_h(set_body_brush_data(theme::color_red()), set_width_fixed(theme::thickness_thick()));

			if (auto _ = widget::collapsible_header2("component_name"))
			{
				c_auto disclosure_size = font::get_line_height(theme::text_heading_font_size());
				c_auto gap			   = theme::header_bar_child_gap();
				c_auto padding_l	   = theme::header_bar_padding().x;

				if (auto _ = widget::vertical(set_padding_left(disclosure_size + padding_l + gap)))
				{
					ui_component(FWD(cmp));
				}
			}
		}
	}

	void
	ui_inspector(auto& entities) noexcept
	{
		if (g::select_vec.is_empty()) { return; }

		using t_storage			 = BARE_OF(entities);
		using t_ent_id			 = typename t_storage::t_ent_id;
		using t_archetype_traits = typename t_storage::t_archetype_traits;

		// todo : implement multiselection
		if (g::select_vec.size() > 1) { return; }

		c_auto ent_id = static_cast<t_ent_id>(g::select_vec[0]);

		c_auto archetype = entities.get_archetype(ent_id);

		for (auto storage_cmp_idx : age::views::each_set_bit_idx(archetype))
		{
			t_archetype_traits::visit_component(entities, ent_id, storage_cmp_idx, AGE_FUNC(ui_component_section));
		}
	}
}	 // namespace age::editor
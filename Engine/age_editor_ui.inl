#include "age.hpp"

namespace age::editor
{
	void
	ui_component(auto&& cmp) noexcept
	{
		ui::widget::text_heading("ui for component ?? not implemented yet");
	}
}	 // namespace age::editor

namespace age::editor
{
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

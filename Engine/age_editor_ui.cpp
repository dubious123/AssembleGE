#include "age_pch.hpp"
#include "age.hpp"

namespace age::editor
{
	ui::widget_ctx
	ui_entity_tree_node(storage_editor_data& editor_storage, uint64 ent_id, uint64 archetype, bool selected) noexcept
	{
		using namespace age::ui;
		using enum input::e::key_kind;

		c_auto child_padidng_left = theme::thickness_thick() + theme::item_child_gap() + theme::thickness_thick() + theme::item_child_gap();
		auto   is_opened		  = false;


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

			if (ui::g::p_input_ctx->is_shift_down())
			{
				if (interact.clicked())
				{
					if (selected)
					{
						age::editor::remove_select(e::select_kind::entity, editor_storage.code_idx, ent_id);
					}
					else
					{
						age::editor::add_select(e::select_kind::entity, editor_storage.code_idx, ent_id);
					}
				}
			}
			else
			{
				if (interact.clicked())
				{
					age::editor::clear_select();

					if (selected is_false)
					{
						age::editor::add_select(e::select_kind::entity, editor_storage.code_idx, ent_id);
					}
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
						btn_state.toggled = !btn_state.toggled;
					}

					is_opened = btn_state.toggled;

					widget::disclosure_indicator(btn_state.toggled, disclosure_indicator_size);
				}

				auto&& [arch_idx, ent_idx] = editor_storage.id_to_editor_location_map[ent_id];

				widget::text_input(editor_storage.archetype_data_vec[arch_idx].entity_data_vec[ent_idx].name.data(), config::max_entity_name_len);

				// widget::text(p_name);

				if (auto _ = widget::begin(set_horizontal_inv() | set_width_grow() | set_height_fit() | set_child_gap(theme::gap_large())))
				{
					char arch_buf[24];
					util::to_str<16, 8>(arch_buf, archetype, "0x");
					widget::text_hint(arch_buf);

					widget::separator_h(set_width_fixed(theme::thickness_thick()), set_body_brush_data(theme::color_gray_light()));

					char id_buf[24];
					util::to_str(id_buf, ent_id, "#");
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
}	 // namespace age::editor

namespace age::editor
{
	float3
	get_component_color(uint32 cmp_idx) noexcept
	{
		switch (cmp_idx % 64)
		{
		case 0:
			return ui::theme::palette_red();
		case 1:
			return ui::theme::palette_light_yellow();
		case 2:
			return ui::theme::palette_harlequin();
		case 3:
			return ui::theme::palette_light_cyan();
		case 4:
			return ui::theme::palette_blue();
		case 5:
			return ui::theme::palette_light_amethyst();
		case 6:
			return ui::theme::palette_rose();
		case 7:
			return ui::theme::palette_tangerine();
		case 8:
			return ui::theme::palette_light_lime();
		case 9:
			return ui::theme::palette_spring();
		case 10:
			return ui::theme::palette_light_azure();
		case 11:
			return ui::theme::palette_indigo();
		case 12:
			return ui::theme::palette_light_cerise();
		case 13:
			return ui::theme::palette_light_vermilion();
		case 14:
			return ui::theme::palette_yellow();
		case 15:
			return ui::theme::palette_light_emerald();
		case 16:
			return ui::theme::palette_cyan();
		case 17:
			return ui::theme::palette_light_ultramarine();
		case 18:
			return ui::theme::palette_amethyst();
		case 19:
			return ui::theme::palette_light_coral();
		case 20:
			return ui::theme::palette_light_gold();
		case 21:
			return ui::theme::palette_lime();
		case 22:
			return ui::theme::palette_light_mint();
		case 23:
			return ui::theme::palette_azure();
		case 24:
			return ui::theme::palette_light_purple();
		case 25:
			return ui::theme::palette_cerise();
		case 26:
			return ui::theme::palette_orange();
		case 27:
			return ui::theme::palette_light_chartreuse();
		case 28:
			return ui::theme::palette_emerald();
		case 29:
			return ui::theme::palette_light_cerulean();
		case 30:
			return ui::theme::palette_ultramarine();
		case 31:
			return ui::theme::palette_light_fuchsia();
		case 32:
			return ui::theme::palette_light_red();
		case 33:
			return ui::theme::palette_gold();
		case 34:
			return ui::theme::palette_light_green();
		case 35:
			return ui::theme::palette_mint();
		case 36:
			return ui::theme::palette_light_cobalt();
		case 37:
			return ui::theme::palette_purple();
		case 38:
			return ui::theme::palette_light_crimson();
		case 39:
			return ui::theme::palette_light_tangerine();
		case 40:
			return ui::theme::palette_chartreuse();
		case 41:
			return ui::theme::palette_light_jade();
		case 42:
			return ui::theme::palette_cerulean();
		case 43:
			return ui::theme::palette_light_violet();
		case 44:
			return ui::theme::palette_fuchsia();
		case 45:
			return ui::theme::palette_vermilion();
		case 46:
			return ui::theme::palette_light_pear();
		case 47:
			return ui::theme::palette_green();
		case 48:
			return ui::theme::palette_light_sky();
		case 49:
			return ui::theme::palette_cobalt();
		case 50:
			return ui::theme::palette_light_magenta();
		case 51:
			return ui::theme::palette_crimson();
		case 52:
			return ui::theme::palette_amber();
		case 53:
			return ui::theme::palette_light_harlequin();
		case 54:
			return ui::theme::palette_jade();
		case 55:
			return ui::theme::palette_light_blue();
		case 56:
			return ui::theme::palette_violet();
		case 57:
			return ui::theme::palette_light_rose();
		case 58:
			return ui::theme::palette_light_orange();
		case 59:
			return ui::theme::palette_pear();
		case 60:
			return ui::theme::palette_light_spring();
		case 61:
			return ui::theme::palette_sky();
		case 62:
			return ui::theme::palette_light_indigo();
		case 63:
			return ui::theme::palette_magenta();
		default:
			return ui::theme::palette_red();
		}
	}
}	 // namespace age::editor

namespace age::editor
{
	ui::widget_ctx
	ui_component_header(const char* p_name, bool& close_out) noexcept
	{
		using enum input::e::key_kind;
		using namespace ui;

		if (auto res = widget::begin(style::layout(ui::e::widget_layout::vertical)
									 | set_size(size_mode::grow(), size_mode::fit())))
		{
			auto is_open = false;

			if (auto header = widget::begin(style::header_bar() | set_save_state(true) | set_interact(true)))
			{
				if (header.clicked<mouse_left>())
				{
					header.toggle();
				}

				is_open = header.is_toggled() is_false;

				c_auto disclosure_indicator_size = font::get_line_height(theme::text_heading_font_size());
				widget::disclosure_indicator(is_open, disclosure_indicator_size);

				widget::text_heading(p_name);

				if (auto _ = widget::vertical())
				{
					if (auto close_btn = widget::begin(style::vertical()
													   | set_interact(true)
													   | set_size(size_mode::fixed(disclosure_indicator_size), size_mode::fixed(disclosure_indicator_size))
													   | set_padding(theme::padding_small() + 1.f)
													   | set_align_end()))
					{
						close_out = close_btn.clicked();

						widget::begin(set_align(ui::e::widget_align::center)
									  | set_draw(header.contains_mouse())
									  | set_size(size_mode::grow(), size_mode::grow())
									  | set_border_thickness(0.f)
									  | set_shape_kind(ui::e::shape_kind::cross)
									  | set_body_brush_data(theme::color_text_gray_dark()));
					}
				}
			}

			if (is_open)
			{
				widget::separator_v();
				return res;
			}
		}

		return {};
	}

	void
	ui_component(ecs::position& pos) noexcept
	{
		ui::widget::numeric_field(pos, "possition");
	}

	void
	ui_component(ecs::render_object& obj) noexcept
	{
		char c_buf[12];
		age::util::to_str(c_buf, obj.render_id);
		ui::widget::text_heading(c_buf);
	}

	void
	ui_component(ecs::rotation& rot) noexcept
	{
		ui::widget::rotation_field(rot, "rotation");
	}

	void
	ui_component(ecs::scale& scale) noexcept
	{
		ui::widget::numeric_field(scale, "scale");
	}

	void
	ui_component(ecs::mesh& mesh) noexcept
	{
		using namespace ui;
		using enum input::e::key_kind;
		using enum ui::e::style_state;

		auto display_name = std::array<char, config::max_asset_display_name_len>{ "invalid mesh" };
		if (runtime::is_handle_invalid(mesh.h_mesh) is_false)
		{
			display_name = mesh.h_mesh.get_display_name<asset::e::kind::mesh_baked>();
		}

		auto open	= false;
		auto btn_id = age::get_invalid_id<ui::t_hash>();

		if (auto btn = widget::begin(set_width_fit() | set_height_fit() | set_vertical() | set_interact(true) | set_save_state(true)))
		{
			btn_id			 = btn.hash_id;
			auto style_state = idle;
			if (btn.pressed<mouse_left>())
			{
				style_state = active;
			}
			else if (btn.hovered())
			{
				style_state = hover;
			}

			if (auto _ = widget::frame(style_state, set_width_fit(), set_height_fit()))
			{
				widget::text_button(display_name.data());
			}

			auto& state = btn.get_state();

			if (btn.clicked())
			{
				state.toggled = !state.toggled;
			}

			open = state.toggled;
		}

		if (open)
		{
			for (auto h : asset::registry::all(asset::e::kind::mesh_baked))
			{
				auto id = id_begin();

				if (auto sel = widget::button(h.get_display_name<asset::e::kind::mesh_baked>().data()))
				{
					if (sel.clicked())
					{
						auto& state	  = ui::g::widget_state_map[btn_id];
						state.toggled = false;	  // close this context
						mesh.update_h_mesh(h);
						break;
					}
				}
			}
		}
	}

	void
	ui_component(ecs::material& mat) noexcept
	{
		ui::widget::checkbox("is_opaque", mat.is_opaque);
	}

	void
	ui_component(ecs::directional_light& light) noexcept
	{
		char c_buf[12];
		age::util::to_str(c_buf, light.render_id);
		ui::widget::text_heading(c_buf);

		ui::widget::checkbox("cast shadow", light.cast_shadow);
		ui::widget::numeric_field(light.direction, "direction");
		ui::widget::numeric_field(light.intensity, "intensity");
		ui::widget::numeric_field(light.color, "color");
	}

	void
	ui_component(ecs::point_light& light) noexcept
	{
		char c_buf[12];
		age::util::to_str(c_buf, light.render_id);
		ui::widget::text_heading(c_buf);

		ui::widget::checkbox("cast shadow", light.cast_shadow);
		ui::widget::numeric_field(light.range, "range");
		ui::widget::numeric_field(light.color, "color");
		ui::widget::numeric_field(light.intensity, "intensity");
	}

	void
	ui_component(ecs::spot_light& light) noexcept
	{
		char c_buf[12];
		age::util::to_str(c_buf, light.render_id);
		ui::widget::text_heading(c_buf);

		ui::widget::checkbox("cast shadow", light.cast_shadow);
		ui::widget::numeric_field(light.range, "range");
		ui::widget::numeric_field(light.direction, "direction");
		ui::widget::numeric_field(light.intensity, "intensity");
		ui::widget::numeric_field(light.color, "color");
		ui::widget::numeric_field(light.cos_inner, "cos_inner", 0.f, 1.f);
		ui::widget::numeric_field(light.cos_outer, "cos_outer", light.cos_inner, 1.f);
	}

	void
	ui_component(age::ecs::camera& cam) noexcept
	{
		ui::widget::text_heading(age::graphics::e::to_string(cam.kind).data());

		ui::widget::numeric_field(cam.euler_deg, "rotation", float3{ -90.f, -180.f, -180.f }, float3{ 90.f, 180.f, 180.f });

		ui::widget::numeric_field(cam.near_z, "near_z", cam.far_z);
		ui::widget::numeric_field(cam.far_z, "far_z", 0.f);

		if (cam.kind == age::graphics::e::camera_kind::perspective)
		{
			ui::widget::numeric_field(cam.fov_y, "fov_y");
			ui::widget::numeric_field(cam.aspect_ratio, "aspect_ratio");
		}
		else
		{
			ui::widget::numeric_field(cam.view_width, "view_width");
			ui::widget::numeric_field(cam.view_height, "view_height");
		}
	}
}	 // namespace age::editor

namespace age::editor
{
	namespace detail
	{
		auto&
		ui_modal_asset_name() noexcept
		{
			static auto name = std::array<char, config::max_asset_path_len>{ "sample name" };

			return name;
		}
	}	 // namespace detail

	void
	ui_modal_new_asset_font() noexcept
	{
		using namespace age::ui;
		widget::text_heading("ui_modal_new_asset_font");
	}

	void
	ui_modal_new_asset_mesh_baked() noexcept
	{
		using namespace age::ui;
		static auto size		= float3::one();
		static auto seg_uv		= vec2<uint32>{ 30, 30 };
		static auto mesh_kind	= age::asset::e::primitive_mesh_kind::cube;
		static auto vertex_kind = age::asset::e::vertex_kind::pnt_uv1;

		if (auto _ = widget::begin(set_vertical() | set_width_grow() | set_height_grow()))
		{
			if (auto _ = widget::begin(set_horizontal() | set_width_grow() | set_height_fit()))
			{
				if (auto _ = widget::begin(set_width_fixed(100) | set_height_fit()))
				{
					widget::text("size");
				}


				if (auto _ = widget::begin(set_width_grow() | set_height_fit() | set_padding_left(100)))
				{
					widget::numeric_field(size);
				}
			}

			if (auto _ = widget::begin(set_horizontal() | set_width_grow() | set_height_fit()))
			{
				if (auto _ = widget::begin(set_width_fixed(100) | set_height_fit()))
				{
					widget::text("seg_uv");
				}

				if (auto _ = widget::begin(set_width_grow() | set_height_fit() | set_padding_left(100)))
				{
					widget::numeric_field(seg_uv);
				}
			}

			if (auto _ = widget::begin(set_horizontal() | set_width_grow() | set_height_fit()))
			{
				if (auto _ = widget::begin(set_width_fixed(100) | set_height_fit()))
				{
					widget::text("mesh_kind");
				}

				if (auto _ = widget::begin(set_vertical() | set_width_grow() | set_height_fit() | set_padding_left(100)))
				{
					static auto is_open = false;
					if (auto btn = widget::button(asset::e::to_string(mesh_kind).data()))
					{
						if (btn.clicked())
						{
							is_open = !is_open;
						}
					}
					if (is_open)
					{
						using enum age::asset::e::primitive_mesh_kind;
						for (auto e_mesh_kind : std::array{ cube, plane, cube_sphere })
						{
							if (auto select = widget::button(asset::e::to_string(e_mesh_kind).data()))
							{
								if (select.clicked())
								{
									mesh_kind = e_mesh_kind;
									is_open	  = false;
								}
							}
						}
					}
				}
			}

			if (auto _ = widget::begin(set_horizontal() | set_width_grow() | set_height_fit()))
			{
				if (auto _ = widget::begin(set_width_fixed(100) | set_height_fit()))
				{
					widget::text("vertex layout");
				}

				if (auto _ = widget::begin(set_vertical() | set_width_grow() | set_height_fit() | set_padding_left(100)))
				{
					static auto is_open = false;
					if (auto btn = widget::button(asset::e::to_string(vertex_kind).data()))
					{
						if (btn.clicked())
						{
							is_open = !is_open;
						}
					}
					if (is_open)
					{
						using enum age::asset::e::vertex_kind;
						for (auto e_vertex_kind : std::array{ p_uv1, pn_uv1, pnt_uv1 })
						{
							if (auto select = widget::button(asset::e::to_string(e_vertex_kind).data()))
							{
								if (select.clicked())
								{
									vertex_kind = e_vertex_kind;
									is_open		= false;
								}
							}
						}
					}
				}
			}

			if (auto _ = widget::begin(set_horizontal() | set_align_center()))
			{
				widget::text_input2(detail::ui_modal_asset_name());

				if (auto h_cancel = widget::button("cancel"))
				{
					if (h_cancel.clicked())
					{
						g::show_modal = false;
					}
				}

				if (auto h_create = widget::button("create"))
				{
					if (h_create.clicked())
					{
						g::show_modal = false;

						c_auto mesh_name = g::current_game.dir_path / "asset" / "mesh" / detail::ui_modal_asset_name().data();

						auto h_mesh = asset::mesh_baked::cpu_load(mesh_name.string(),
																  age::asset::primitive_desc{
																	  .size		 = size,
																	  .seg_u	 = seg_uv.x,
																	  .seg_v	 = seg_uv.y,
																	  .mesh_kind = mesh_kind },
																  age::asset::e::vertex_kind::pnt_uv1);
						asset::mesh_baked::cpu_unload(h_mesh);
						asset::registry::register_asset(h_mesh);
					}
				}
			}
		}
	}

	void
	ui_modal_new_asset() noexcept
	{
		using namespace age::ui;

		static auto selected = age::asset::e::kind::mesh_baked;
		if (auto _ = widget::begin(style::panel() | set_horizontal() | set_width_grow() | set_height_grow()))
		{
			if (auto _ = widget::begin(style::section() | set_vertical() | set_width_fit() | set_height_grow()))
			{
				for (auto e_asset_kind : std::array{ age::asset::e::kind::font, age::asset::e::kind::mesh_baked })
				{
					if (auto _ = id_begin())
					{
						if (auto asset_btn = widget::button(asset::e::to_string(e_asset_kind).data()))
						{
							if (asset_btn.clicked())
							{
								selected = e_asset_kind;
							}
						}
					}
				}
			}

			if (auto _ = widget::begin(style::section() | set_vertical() | set_width_grow() | set_height_grow()))
			{
				switch (selected)
				{
				case age::asset::e::kind::font:
				{
					ui_modal_new_asset_font();
					break;
				}
				case age::asset::e::kind::mesh_baked:
				{
					ui_modal_new_asset_mesh_baked();
					break;
				}
				default:
				{
					AGE_UNREACHABLE();
				}
				}
			}
		}
	}

	void
	ui_modal() noexcept
	{
		switch (g::modal_kind)
		{
		case e::modal_kind::new_asset:
		{
			ui_modal_new_asset();
			break;
		}
		default:
		{
			AGE_UNREACHABLE();
		}
		}
	}
}	 // namespace age::editor

namespace age::editor
{
	void
	ui_asset() noexcept
	{
		using namespace ui;
		if (auto _ = widget::panel())
		{
			if (auto _ = widget::begin(style::horizontal() | set_width_grow() | set_height_fit()))
			{
				if (auto _ = widget::begin(style::panel() | set_vertical() | set_width_grow() | set_height_fit()))
				{
					widget::begin(style::text_title("assets") | set_align_begin());
				}

				if (auto h_btn = widget::button("+ new", set_align_center()))
				{
					if (h_btn.clicked())
					{
						g::modal_kind = e::modal_kind::new_asset;
						g::show_modal = !g::show_modal;
					}
				}
			}

			widget::separator_v();

			asset::for_each_kind(
				AGE_LAMBDA(
					<asset::e::kind e_kind>(),
					{
						auto id_0 = id_begin();
						if (asset::registry::all(e_kind).size() > 0)
						{
							widget::text_heading(asset::e::to_string(e_kind).data());
						}

						auto h_asset_remove = asset::handle{};

						for (c_auto h : asset::registry::all(e_kind))
						{
							auto id_0 = id_begin();
							// c_auto& display_name = h.get_display_name();
							auto _0 = widget::begin(style::section() | set_horizontal() | set_width_grow() | set_height_fit());

							c_auto display_name = h.get_display_name();
							widget::begin(style::text(display_name.data()));

							auto _1 = widget::begin(style::section() | set_vertical() | set_width_grow() | set_height_fit());
							auto _2 = widget::begin(set_align_end() | set_width_fit() | set_height_fit());

							auto btn_remove_asset = widget::button("X");
							if (btn_remove_asset.clicked())
							{
								h_asset_remove = h;
							}
						}

						if (runtime::is_handle_invalid(h_asset_remove) is_false)
						{
							asset::registry::unregister_asset(h_asset_remove);
						}
					}));
		}
	}
}	 // namespace age::editor
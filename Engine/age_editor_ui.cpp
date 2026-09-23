#include "age_pch.hpp"
#include "age.hpp"

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

namespace age::editor::detail
{
	template <asset::e::kind e_kind>
	bool
	ui_component_asset_dropdown(asset::handle& h_asset, bool rebuild_options = true)
	{
		static auto option_vec = age::vector<ui::widget::dropdown_option<asset::handle>>{};
		static auto label_vec  = age::vector<age::array<char, config::max_asset_display_name_len>>{};
		if (rebuild_options)
		{
			option_vec.reserve(asset::registry::all(e_kind).size());
			label_vec.reserve(asset::registry::all(e_kind).size());
			option_vec.clear();
			label_vec.clear();
			for (auto h : asset::registry::all(e_kind))
			{
				label_vec.emplace_back(h.get_display_name<e_kind>());
				option_vec.emplace_back(
					ui::widget::dropdown_option<asset::handle>{ .value = h, .label = label_vec.back().data() });
			}
		}

		return ui::widget::dropdown<asset::handle>(h_asset, option_vec);
	}

	bool
	ui_component_index_dropdown(std::integral auto& n, uint32 count)
	{
		static auto option_vec = age::vector<ui::widget::dropdown_option<BARE_OF(n)>>{};
		static auto label_vec  = age::vector<age::array<char, 22>>{};
		if (option_vec.size() < count)
		{
			option_vec.reserve(count);
			label_vec.reserve(count);
			for (auto i : views::loop(label_vec.size()))
			{
				option_vec[i].label = label_vec[i].data();
			}

			for (auto i : std::views::iota(label_vec.size()) | std::views::take(count - option_vec.size()))
			{
				auto buf = age::array<char, 22>{};
				util::integral_to_str(buf, i);
				label_vec.emplace_back(std::move(buf));
				option_vec.emplace_back(
					ui::widget::dropdown_option<BARE_OF(n)>{ .value = cast_to<BARE_OF(n)>(i), .label = label_vec.back().data() });
			}
		}

		return ui::widget::dropdown<BARE_OF(n)>(n, std::span{ option_vec.data(), count });
	}

	template <asset::e::kind e_kind>
	std::tuple<bool, bool, asset::handle>
	ui_component_asset_common_header(asset::handle h_asset)
	{
		using namespace ui;
		auto handle_changed = false;
		auto save			= false;

		if (auto header = widget::begin(style::header_bar() | set_vertical() | set_width_grow() | set_height_fit()))
		{
			handle_changed = detail::ui_component_asset_dropdown<e_kind>(h_asset);

			// todo, handle asset dirty
			if (/*asset_mgr::is_dirty(h_model)*/ true)
			{
				save = widget::button("save").clicked();
			}

			ui::widget::separator_v();
		}

		return { handle_changed, save, h_asset };
	}
}	 // namespace age::editor::detail

namespace age::editor
{
	template <>
	bool
	ui_asset<asset::e::kind::font>(asset::handle h_font) noexcept
	{
		AGE_UNREACHABLE("not implemented");
		return false;
	}

	template <>
	bool
	ui_asset<asset::e::kind::mesh_baked>(asset::handle h_mesh) noexcept
	{
		return false;
	}

	template <>
	bool
	ui_asset<asset::e::kind::material>(asset::handle h_mat) noexcept
	{
		using namespace age::ui;
		using namespace age::ui::style;

		auto is_dirty = false;

		if (runtime::is_handle_invalid(h_mat)) { return is_dirty; }

		auto& mat_entry = h_mat.get_entry<asset::e::kind::material>();


		// todo. cleanup asset load system
		// we don't need full load here.
		if (mat_entry.is_loaded() is_false)
		{
			ui::widget::text("asset not loded");
			return is_dirty;
		}

		is_dirty = true;

		c_auto tex_dropdown = [](asset::handle& h_tex, bool is_first = false) {
			auto h_tex_after = h_tex;
			if (detail::ui_component_asset_dropdown<age::asset::e::kind::texture>(h_tex_after, is_first))
			{
				asset::material::update_texture(h_tex, h_tex_after);
			}
		};

		if (auto _ = ui::widget::begin(set_horizontal() | set_width_grow() | set_height_fit()))
		{
			if (auto _ = ui::widget::begin(set_width_fixed(100) | set_height_fit() | set_align_center()))
			{
				ui::widget::text("Base color");
			}

			if (auto _ = ui::widget::begin(set_vertical() | set_width_grow() | set_height_fit()))
			{
				ui::widget::color_field(mat_entry.base_color_factor);
				tex_dropdown(mat_entry.h_tex_base_color, true);
				ui::widget::dropdown(mat_entry.base_color_sampler_kind);
			}
		}

		ui::widget::separator_v();

		if (auto _ = ui::widget::begin(set_horizontal() | set_width_grow() | set_height_fit()))
		{
			if (auto _ = ui::widget::begin(set_width_fixed(100) | set_height_fit() | set_align_center()))
			{
				ui::widget::text("Matallic");
			}

			if (auto _ = ui::widget::begin(set_vertical() | set_width_grow() | set_height_fit()))
			{
				ui::widget::numeric_field(mat_entry.metallic_factor, nullptr, 0.f, 1.f);
				ui::widget::slider(mat_entry.metallic_factor, 0.f, 1.f);
			}
		}

		if (auto _ = ui::widget::begin(set_horizontal() | set_width_grow() | set_height_fit()))
		{
			if (auto _ = ui::widget::begin(set_width_fixed(100) | set_height_fit() | set_align_center()))
			{
				ui::widget::text("Roughness");
			}

			if (auto _ = ui::widget::begin(set_vertical() | set_width_grow() | set_height_fit()))
			{
				ui::widget::numeric_field(mat_entry.roughness_factor, nullptr, 0.f, 1.f);
				ui::widget::slider(mat_entry.roughness_factor, 0.f, 1.f);
			}
		}

		if (auto _ = ui::widget::begin(set_horizontal() | set_width_grow() | set_height_fit()))
		{
			if (auto _ = ui::widget::begin(set_width_fixed(100) | set_height_fit() | set_align_center()))
			{
				ui::widget::text("MR Texture");
			}

			if (auto _ = ui::widget::begin(set_vertical() | set_width_grow() | set_height_fit()))
			{
				tex_dropdown(mat_entry.h_tex_metallic_roughness);
				ui::widget::dropdown(mat_entry.metallic_roughness_sampler_kind);
			}
		}

		ui::widget::separator_v();

		if (auto _ = ui::widget::begin(set_horizontal() | set_width_grow() | set_height_fit()))
		{
			if (auto _ = ui::widget::begin(set_width_fixed(100) | set_height_fit() | set_align_center()))
			{
				ui::widget::text("Normal");
			}

			if (auto _ = ui::widget::begin(set_vertical() | set_width_grow() | set_height_fit()))
			{
				ui::widget::numeric_field(mat_entry.normal_scale, nullptr, 0.f, 2.f);
				ui::widget::slider(mat_entry.normal_scale, 0.f, 2.f);
				tex_dropdown(mat_entry.h_tex_normal);
				ui::widget::dropdown(mat_entry.normal_sampler_kind);
			}
		}

		ui::widget::separator_v();

		if (auto _ = ui::widget::begin(set_horizontal() | set_width_grow() | set_height_fit()))
		{
			if (auto _ = ui::widget::begin(set_width_fixed(100) | set_height_fit() | set_align_center()))
			{
				ui::widget::text("Occlusion");
			}

			if (auto _ = ui::widget::begin(set_vertical() | set_width_grow() | set_height_fit()))
			{
				ui::widget::numeric_field(mat_entry.occlusion_strength, nullptr, 0.f, 1.f);
				ui::widget::slider(mat_entry.occlusion_strength, 0.f, 1.f);
				tex_dropdown(mat_entry.h_tex_occlusion);
				ui::widget::dropdown(mat_entry.occlusion_sampler_kind);
			}
		}

		ui::widget::separator_v();

		if (auto _ = ui::widget::begin(set_horizontal() | set_width_grow() | set_height_fit()))
		{
			if (auto _ = ui::widget::begin(set_width_fixed(100) | set_height_fit() | set_align_center()))
			{
				ui::widget::text("Alpha Cutoff");
			}

			if (auto _ = ui::widget::begin(set_vertical() | set_width_grow() | set_height_fit()))
			{
				ui::widget::numeric_field(mat_entry.alpha_cutoff, nullptr, 0.f, 1.f);
				ui::widget::slider(mat_entry.alpha_cutoff, 0.f, 1.f);
			}
		}

		ui::widget::separator_v();

		if (auto _ = ui::widget::begin(set_horizontal() | set_width_grow() | set_height_fit()))
		{
			if (auto _ = ui::widget::begin(set_width_fixed(100) | set_height_fit() | set_align_center()))
			{
				ui::widget::text("Emissive");
			}

			if (auto _ = ui::widget::begin(set_vertical() | set_width_grow() | set_height_fit()))
			{
				ui::widget::color_field(mat_entry.emissive_factor, 0.f, std::numeric_limits<float>::max());
				tex_dropdown(mat_entry.h_tex_emissive);
				ui::widget::dropdown(mat_entry.emissive_sampler_kind);
			}
		}

		ui::widget::separator_v();

		if (auto _ = ui::widget::begin(set_horizontal() | set_width_grow() | set_height_fit()))
		{
			if (auto _ = ui::widget::begin(set_width_fixed(100) | set_height_fit() | set_align_center()))
			{
				ui::widget::text("Shading Model");
			}

			if (auto _ = ui::widget::begin(set_vertical() | set_width_grow() | set_height_fit()))
			{
				ui::widget::dropdown(mat_entry.shading_model);
			}
		}

		ui::widget::separator_v();

		if (auto _ = ui::widget::begin(set_horizontal() | set_width_grow() | set_height_fit()))
		{
			if (auto _ = ui::widget::begin(set_width_fixed(100) | set_height_fit() | set_align_center()))
			{
				ui::widget::text("Double Sided");
			}

			if (auto _ = ui::widget::begin(set_vertical() | set_width_grow() | set_height_fit()))
			{
				ui::widget::checkbox(nullptr, mat_entry.double_sided);
			}
		}

		ui::widget::separator_v();

		if (mat_entry.is_loaded())
		{
			auto btn = ui::widget::button("save");

			if (btn.clicked())
			{
				asset::material::save(h_mat);
			}
		}

		return is_dirty;
	}

	template <>
	bool
	ui_asset<asset::e::kind::texture>(asset::handle h_tex) noexcept
	{
		return false;
	}

	template <>
	bool
	ui_asset<asset::e::kind::env_light>(asset::handle h_env_light) noexcept
	{
		return false;
	}

	template <>
	bool
	ui_asset<asset::e::kind::model>(asset::handle h_model) noexcept
	{
		if (runtime::is_handle_invalid(h_model)) { return false; }

		auto& entry = h_model.get_entry<asset::e::kind::model>();

		// todo. cleanup asset load system
		// we don't need full load here.
		if (entry.is_loaded() is_false)
		{
			ui::widget::text("asset not loded");
			return false;
		}

		if (auto h_mesh = entry.h_mesh;
			detail::ui_component_asset_dropdown<asset::e::kind::mesh_baked>(h_mesh))
		{
			asset::model::update_mesh(h_model, h_mesh);
		}

		ui::widget::separator_v();

		for (auto [i, h_mat] : entry.h_material_vec | views::enumerate_copy<uint32>)
		{
			if (detail::ui_component_asset_dropdown<asset::e::kind::material>(h_mat, i == 0))
			{
				asset::model::update_material(h_model, i, h_mat);
			}

			ui_asset<asset::e::kind::material>(h_mat);
		}

		return false;
	}
}	 // namespace age::editor

// ui component
namespace age::editor
{
	ui::widget_ctx
	ui_component_header(const char* p_name, AGE_OUT bool& close_out) noexcept
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
	ui_component(ecs::mesh& cmp_mesh) noexcept
	{
		auto h_mesh = cmp_mesh.h_mesh;
		if (detail::ui_component_asset_dropdown<asset::e::kind::mesh_baked>(h_mesh))
		{
			cmp_mesh.update_h_mesh(h_mesh);
		}
	}

	void
	ui_component(ecs::material& mat) noexcept
	{
		using namespace age::ui;
		using namespace age::ui::style;
		{
			auto h_mat = mat.h_mat;

			if (detail::ui_component_asset_dropdown<asset::e::kind::material>(h_mat))
			{
				mat.update_h_mat(h_mat);
			}

			if (runtime::is_handle_invalid(mat.h_mat)) { return; }
		}

		ui::widget::separator_v();

		ui_asset<asset::e::kind::material>(mat.h_mat);

		auto& entry = mat.h_mat.get_entry<asset::e::kind::material>();
		if (entry.is_loaded())
		{
			g::host_ops.p_renderer_update_material(mat.h_mat);
		}
	}

	void
	ui_component(ecs::model_render_option& option) noexcept
	{
		ui::widget::text("mesh_raster_override_kind");
		ui::widget::dropdown<age::graphics::e::mesh_raster_override_kind>(option.raster_override_kind);
		ui::widget::text("mesh_rt_alpha_test_override_kind");
		ui::widget::dropdown<age::graphics::e::mesh_rt_alpha_test_override_kind>(option.rt_alpha_test_override_kind);
		ui::widget::checkbox_flags("model_render_option_flags", option.option_flags);

		if (auto _ = ui::id_begin();
			has_any(option.option_flags, age::graphics::e::model_render_option_flags::fade))
		{
			auto fade_float = cvt_to<float>(option.fade_unorm8, cvt_unorm_tag{});
			ui::widget::text("fade");
			ui::widget::slider(fade_float, 0.f, 1.f);
			if (abs(fade_float - 1.f) < age::g::epsilon_1e4)
			{
				option.fade_unorm8 = 0xff;
			}
			else
			{
				option.fade_unorm8 = cvt_to<uint8>(fade_float, cvt_unorm_tag{});
			}
		}
	}

	void
	ui_component(ecs::model& cmp_model) noexcept
	{
		c_auto[handle_changed, save, new_handle] = detail::ui_component_asset_common_header<asset::e::kind::model>(cmp_model.h_model);

		if (handle_changed)
		{
			cmp_model.update_h_model(new_handle);
		}

		ui_asset<asset::e::kind::model>(cmp_model.h_model);

		if (save)
		{
			asset::model::save(cmp_model.h_model);
		}

		if (runtime::is_handle_invalid(cmp_model.h_model)) { return; }

		for (c_auto& entry = cmp_model.h_model.get_entry<asset::e::kind::model>();
			 c_auto& h_mat : entry.h_material_vec)
		{
			if (runtime::is_handle_invalid(h_mat)) { continue; }

			if (h_mat.get_entry<asset::e::kind::material>().is_loaded())
			{
				g::host_ops.p_renderer_update_material(h_mat);
			}
		}
	}

	void
	ui_component(ecs::directional_light& light) noexcept
	{
		char c_buf[12];
		age::util::to_str(c_buf, light.render_id);
		ui::widget::text_heading(c_buf);

		ui::widget::checkbox("cast shadow", light.cast_shadow);
		ui::widget::numeric_field(light.direction, "direction");
		ui::widget::text("color");
		ui::widget::color_field(light.color, light.intensity, 0.f, std::numeric_limits<float>::max());
	}

	void
	ui_component(ecs::point_light& light) noexcept
	{
		char c_buf[12];
		age::util::to_str(c_buf, light.render_id);
		ui::widget::text_heading(c_buf);

		ui::widget::checkbox("cast shadow", light.cast_shadow);
		ui::widget::numeric_field(light.range, "range");
		ui::widget::text("color");
		ui::widget::color_field(light.color, light.intensity, 0.f, std::numeric_limits<float>::max());
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
		ui::widget::text("color");
		ui::widget::color_field(light.color, light.intensity, 0.f, std::numeric_limits<float>::max());
		ui::widget::numeric_field(light.cos_inner, "cos_inner", 0.f, 1.f);
		ui::widget::numeric_field(light.cos_outer, "cos_outer", light.cos_inner, 1.f);
	}

	void
	ui_component(ecs::env_light& cmp) noexcept
	{
		using namespace ui;
		using enum input::e::key_kind;
		using enum ui::e::style_state;
		using enum asset::e::kind;

		static auto env_light_vec = age::vector<widget::dropdown_option<asset::handle>>{};
		static auto label_vec	  = age::vector<age::array<char, config::max_asset_display_name_len>>{};
		env_light_vec.reserve(asset::registry::all(env_light).size() + 1);
		label_vec.reserve(asset::registry::all(env_light).size() + 1);
		env_light_vec.clear();
		label_vec.clear();

		label_vec.emplace_back(util::to_fixed_str<config::max_asset_display_name_len>("(none)"));
		env_light_vec.emplace_back(ui::widget::dropdown_option<asset::handle>{ .value = {}, .label = label_vec.back().data() });
		for (auto h_env_light : asset::registry::all(env_light))
		{
			label_vec.emplace_back(h_env_light.get_display_name<env_light>());
			env_light_vec.emplace_back(
				widget::dropdown_option<asset::handle>{ .value = h_env_light, .label = label_vec.back().data() });
		}

		if (auto h_env_light = cmp.h_env_light;
			widget::dropdown<asset::handle>(h_env_light, env_light_vec))
		{
			cmp.update_h_env_light(h_env_light);
		}

		if (runtime::is_handle_invalid(cmp.h_env_light)) { return; }

		auto& entry = cmp.h_env_light.get_entry<env_light>();

		if (entry.is_cpu_loaded() is_false)
		{
			if (widget::button2("cpu load"))
			{
				asset::env_light::cpu_load(cmp.h_env_light);
			}
			return;
		}
		else
		{
			if (widget::button2("cpu unload"))
			{
				asset::env_light::cpu_unload(cmp.h_env_light);
				return;
			}
		}

		auto& runtime_info = entry.get_runtime_info();

		widget::numeric_field(runtime_info.intensity, "intensity", 0.f, 1.f);

		widget::text("tint");
		widget::color_field(runtime_info.tint);

		widget::numeric_field(runtime_info.euler_deg, "rotation", float3{ -180.f, -180.f, -180.f }, float3{ 180.f, 180.f, 180.f });

		if (auto btn = ui::widget::button("save");
			btn.clicked())
		{
			asset::env_light::save(cmp.h_env_light);
		}

		if (entry.is_gpu_loaded())
		{
			g::host_ops.p_renderer_update_env_light_runtime(cmp.h_env_light);
		}
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

	void
	ui_component(ecs::bloom& cmp) noexcept
	{
		ui::widget::checkbox("active", cmp.active);
		ui::widget::numeric_field(cmp.threshold, "threshold", 0.f, 10.f, ui::theme::text_label_color(), 0.001f);
		cmp.knee = clamp(cmp.knee, 0.f, cmp.threshold);
		ui::widget::numeric_field(cmp.knee, "knee", 0.f, cmp.threshold, ui::theme::text_label_color(), 0.001f);
		ui::widget::numeric_field(cmp.intensity, "intensity", 0.f, 1.f, ui::theme::text_label_color(), 0.001f);
		ui::widget::numeric_field(cmp.radius, "radius", 0.f, 10.f);
		ui::widget::text_label("tint");
		ui::widget::color_field(cmp.tint);
	}

	void
	ui_component(age::ecs::gi_config& cmp) noexcept
	{
		auto update				= false;
		auto update_debug_flags = false;
		{
			auto _ = ui::id_begin();
			if (cmp.enable_ddgi or cmp.enable_gibs or cmp.enable_gist)
			{
				update = ui::widget::button2("update");
			}
		}

		ui::widget::checkbox("enable ddgi", cmp.enable_ddgi);
		if (cmp.enable_ddgi)
		{
			cmp.enable_gibs = false;
			cmp.enable_gist = false;
		}

		ui::widget::checkbox("enable gibs", cmp.enable_gibs);
		if (cmp.enable_gibs)
		{
			cmp.enable_ddgi = false;
			cmp.enable_gist = false;
		}

		ui::widget::checkbox("enable gist", cmp.enable_gist);
		if (cmp.enable_gist)
		{
			cmp.enable_ddgi = false;
			cmp.enable_gibs = false;
		}

		if (auto _ = ui::id_begin();
			cmp.enable_ddgi)
		{
			ui::widget::checkbox("lock origin", cmp.ddgi_lock_origin);

			constexpr c_auto probe_count_option_arr = age::array{
				ui::widget::dropdown_option<uint32>{ .value = 4, .label = "4" },
				ui::widget::dropdown_option<uint32>{ .value = 8, .label = "8" },
				ui::widget::dropdown_option<uint32>{ .value = 16, .label = "16" },
				ui::widget::dropdown_option<uint32>{ .value = 32, .label = "32" },
				ui::widget::dropdown_option<uint32>{ .value = 64, .label = "64" },
				ui::widget::dropdown_option<uint32>{ .value = 128, .label = "128" },
				ui::widget::dropdown_option<uint32>{ .value = 256, .label = "256" },
			};

			ui::widget::text_label("base_probe_count_x");
			ui::widget::dropdown<uint32>(cmp.ddgi_probe_per_level_axis.x, probe_count_option_arr);
			ui::widget::text_label("base_probe_count_y");
			ui::widget::dropdown<uint32>(cmp.ddgi_probe_per_level_axis.y, probe_count_option_arr);
			ui::widget::text_label("base_probe_count_z");
			ui::widget::dropdown<uint32>(cmp.ddgi_probe_per_level_axis.z, probe_count_option_arr);

			ui::widget::numeric_field(cmp.ddgi_base_probe_spacing, "base_probe_spacing", float3::one(), float3{ 1000.f });

			ui::widget::text_label("level_count");
			constexpr c_auto level_count_option_arr = age::array{
				ui::widget::dropdown_option<uint32>{ .value = 1, .label = "1" },
				ui::widget::dropdown_option<uint32>{ .value = 2, .label = "2" },
				ui::widget::dropdown_option<uint32>{ .value = 3, .label = "3" },
				ui::widget::dropdown_option<uint32>{ .value = 4, .label = "4" },
				ui::widget::dropdown_option<uint32>{ .value = 5, .label = "5" },
				ui::widget::dropdown_option<uint32>{ .value = 6, .label = "6" },
				ui::widget::dropdown_option<uint32>{ .value = 7, .label = "7" },
				ui::widget::dropdown_option<uint32>{ .value = 8, .label = "8" },
			};
			ui::widget::dropdown<uint32>(cmp.ddgi_level_count, level_count_option_arr);

			c_auto debug_flag_cached = cmp.ddgi_debug_flags;
#define ddgi_flag_checkbox(enum_name)                                                     \
	{                                                                                     \
		bool b = has_any(cmp.ddgi_debug_flags, graphics::e::ddgi_debug_flags::enum_name); \
		ui::widget::checkbox(#enum_name, b);                                              \
		if (b)                                                                            \
		{                                                                                 \
			cmp.ddgi_debug_flags |= graphics::e::ddgi_debug_flags::enum_name;             \
		}                                                                                 \
		else                                                                              \
		{                                                                                 \
			cmp.ddgi_debug_flags &= ~graphics::e::ddgi_debug_flags::enum_name;            \
		}                                                                                 \
	}

			ddgi_flag_checkbox(render_probe_in_hole);
			ddgi_flag_checkbox(render_irradiance);
			ddgi_flag_checkbox(render_visibility);
			ddgi_flag_checkbox(render_front_back);
			ddgi_flag_checkbox(render_level);
			ddgi_flag_checkbox(render_weight_sum);
			ddgi_flag_checkbox(render_ray_count);
			ddgi_flag_checkbox(render_state);
			ddgi_flag_checkbox(render_msme);
			ddgi_flag_checkbox(render_ray_factor);
			ddgi_flag_checkbox(render_probe);

			update_debug_flags = cmp.ddgi_debug_flags != debug_flag_cached;
#undef ddgi_flag_checkbox
		}

		else if (auto _ = ui::id_begin();
				 cmp.enable_gibs)
		{
			ui::widget::checkbox("lock origin", cmp.gibs_lock_origin);

			ui::widget::text_label("max_surfel_count");
			ui::widget::numeric_field(cmp.max_surfel_count, nullptr, 10000u, g::host_ops.p_renderer_gibs_max_surfel_count());

			constexpr c_auto cell_count_option_arr = age::array{
				ui::widget::dropdown_option<uint8>{ .value = 4, .label = "4" },
				ui::widget::dropdown_option<uint8>{ .value = 8, .label = "8" },
				ui::widget::dropdown_option<uint8>{ .value = 16, .label = "16" },
				ui::widget::dropdown_option<uint8>{ .value = 32, .label = "32" },
				ui::widget::dropdown_option<uint8>{ .value = 64, .label = "64" },
				// ui::widget::dropdown_option<uint8>{ .value = 128, .label = "128" },
			};

			ui::widget::text_label("cell_count");
			ui::widget::dropdown<uint8>(cmp.gibs_cell_count, cell_count_option_arr);


			constexpr c_auto layer_count_option_arr = age::array{
				ui::widget::dropdown_option<uint8>{ .value = 2, .label = "2" },
				ui::widget::dropdown_option<uint8>{ .value = 4, .label = "4" },
				ui::widget::dropdown_option<uint8>{ .value = 6, .label = "6" },
				ui::widget::dropdown_option<uint8>{ .value = 8, .label = "8" },
				ui::widget::dropdown_option<uint8>{ .value = 10, .label = "10" },
				ui::widget::dropdown_option<uint8>{ .value = 12, .label = "12" },
				ui::widget::dropdown_option<uint8>{ .value = 14, .label = "14" },
				ui::widget::dropdown_option<uint8>{ .value = 16, .label = "16" },
			};

			ui::widget::text_label("outer_layer_count");
			ui::widget::dropdown<uint8>(cmp.gibs_outer_layer_count, layer_count_option_arr);

			ui::widget::text_label("gibs_cell_size");
			ui::widget::numeric_field(cmp.gibs_cell_size, nullptr, 1.f, 100.f);

			ui::widget::text_label("outer_cell_size_factor");
			ui::widget::numeric_field(cmp.outer_cell_size_factor, nullptr, 1.f + math::g::epsilon_1e4, 2.5f, ui::theme::text_label_color(), 0.01f);


			c_auto debug_flag_cached = cmp.gibs_debug_flags;
#define gibs_flag_checkbox(enum_name)                                                     \
	{                                                                                     \
		bool b = has_any(cmp.gibs_debug_flags, graphics::e::gibs_debug_flags::enum_name); \
		ui::widget::checkbox(#enum_name, b);                                              \
		if (b)                                                                            \
		{                                                                                 \
			cmp.gibs_debug_flags |= graphics::e::gibs_debug_flags::enum_name;             \
		}                                                                                 \
		else                                                                              \
		{                                                                                 \
			cmp.gibs_debug_flags &= ~graphics::e::gibs_debug_flags::enum_name;            \
		}                                                                                 \
	}

			gibs_flag_checkbox(freeze_spawn_kill);
			gibs_flag_checkbox(render_tile);
			gibs_flag_checkbox(render_cell);
			gibs_flag_checkbox(render_tile_surfel_count);
			gibs_flag_checkbox(render_cell_surfel_count);
			gibs_flag_checkbox(render_tile_surfels);
			gibs_flag_checkbox(render_cell_surfels);
			gibs_flag_checkbox(render_id_hash);
			gibs_flag_checkbox(render_radiance);
			gibs_flag_checkbox(render_irradiance);
			gibs_flag_checkbox(render_normal);
			gibs_flag_checkbox(render_visibility);
			gibs_flag_checkbox(render_near_coverage);
			gibs_flag_checkbox(render_far_coverage);
			gibs_flag_checkbox(render_ray_count);
			gibs_flag_checkbox(render_age);

#undef gibs_flag_checkbox

			update_debug_flags = cmp.gibs_debug_flags != debug_flag_cached;
		}
		else if (auto _ = ui::id_begin();
				 cmp.enable_gist)

		{
			ui::widget::checkbox("lock origin", cmp.gist_lock_origin);

			constexpr c_auto ray_period_option_arr = age::array{
				ui::widget::dropdown_option<uint8>{ .value = 1, .label = "1x1" },
				ui::widget::dropdown_option<uint8>{ .value = 4, .label = "2x2" },
				ui::widget::dropdown_option<uint8>{ .value = 9, .label = "3x3" },
				ui::widget::dropdown_option<uint8>{ .value = 16, .label = "4x4" },
				ui::widget::dropdown_option<uint8>{ .value = 25, .label = "5x5" },
				ui::widget::dropdown_option<uint8>{ .value = 36, .label = "6x6" },
				ui::widget::dropdown_option<uint8>{ .value = 49, .label = "7x7" },
				ui::widget::dropdown_option<uint8>{ .value = 64, .label = "8x8" },
			};

			ui::widget::text_label("diffuse_ray_period");
			ui::widget::dropdown<uint8>(cmp.gist_diffuse_ray_period, ray_period_option_arr);

			ui::widget::text_label("specular_ray_period");
			ui::widget::dropdown<uint8>(cmp.gist_specular_ray_period, ray_period_option_arr);


			constexpr c_auto pow_of_2_option_arr = age::array{
				ui::widget::dropdown_option<uint8>{ .value = 1, .label = "1" },
				ui::widget::dropdown_option<uint8>{ .value = 2, .label = "2" },
				ui::widget::dropdown_option<uint8>{ .value = 4, .label = "4" },
				ui::widget::dropdown_option<uint8>{ .value = 8, .label = "8" },
				ui::widget::dropdown_option<uint8>{ .value = 16, .label = "16" },
				ui::widget::dropdown_option<uint8>{ .value = 32, .label = "32" },
				ui::widget::dropdown_option<uint8>{ .value = 64, .label = "64" },
			};

			ui::widget::text_label("cell_surfel_ray_count_min");
			ui::widget::dropdown<uint8>(cmp.gist_cell_surfel_ray_count_min, std::span{ pow_of_2_option_arr.begin(), log2_pow2(cmp.gist_cell_surfel_ray_count_max) + 1 });

			ui::widget::text_label("cell_surfel_ray_count_max");
			ui::widget::dropdown<uint8>(cmp.gist_cell_surfel_ray_count_max, std::span{ pow_of_2_option_arr.begin() + log2_pow2(cmp.gist_cell_surfel_ray_count_min),
																					   pow_of_2_option_arr.end() });

			ui::widget::text_label("cell_count_per_axis");
			ui::widget::dropdown<uint8>(cmp.gist_cell_count_per_axis, pow_of_2_option_arr);

			constexpr c_auto layer_count_option_arr = age::array{
				ui::widget::dropdown_option<uint8>{ .value = 2, .label = "2" },
				ui::widget::dropdown_option<uint8>{ .value = 4, .label = "4" },
				ui::widget::dropdown_option<uint8>{ .value = 6, .label = "6" },
				ui::widget::dropdown_option<uint8>{ .value = 8, .label = "8" },
				ui::widget::dropdown_option<uint8>{ .value = 10, .label = "10" },
				ui::widget::dropdown_option<uint8>{ .value = 12, .label = "12" },
				ui::widget::dropdown_option<uint8>{ .value = 14, .label = "14" },
				ui::widget::dropdown_option<uint8>{ .value = 16, .label = "16" },
			};

			ui::widget::text_label("outer_layer_count");
			ui::widget::dropdown<uint8>(cmp.gist_outer_layer_count, layer_count_option_arr);

			ui::widget::text_label("max_cell_surfel_count");
			ui::widget::numeric_field(cmp.gist_max_cell_surfel_count, nullptr, 10000u, g::host_ops.p_renderer_gist_max_cell_surfel_count());

			c_auto cell_count_total = cmp.gist_cell_count_per_axis * cmp.gist_cell_count_per_axis * cmp.gist_cell_count_per_axis
									+ cmp.gist_cell_count_per_axis * cmp.gist_cell_count_per_axis * cmp.gist_outer_layer_count * 6;
			ui::widget::text_label(std::format("surfel_per_cell : {}", float(cmp.gist_max_cell_surfel_count) / (cell_count_total)).c_str());

			ui::widget::text_label("cell_surfel_ray_budget_factor");
			ui::widget::numeric_field(cmp.gist_cell_surfel_ray_budget_factor, nullptr, 0.1f, 100.f, ui::theme::text_label_color(), 0.01f);

			ui::widget::text_label(std::format("cell_surfel_ray_budget : {}",
											   cmp.gist_max_cell_surfel_count
												   * cmp.gist_cell_surfel_ray_count_min
												   * cmp.gist_cell_surfel_ray_budget_factor)
									   .c_str());

			ui::widget::text_label("cell_size");
			ui::widget::numeric_field(cmp.gist_cell_size, nullptr, 1.f, 100.f);

			ui::widget::text_label("outer_cell_size_factor");
			ui::widget::numeric_field(cmp.gist_outer_cell_size_factor, nullptr, 1.f + math::g::epsilon_1e4, 2.5f, ui::theme::text_label_color(), 0.01f);

			c_auto debug_flag_cached = cmp.gist_debug_flags;

#define gist_flag_checkbox(enum_name)                                                     \
	{                                                                                     \
		bool b = has_any(cmp.gist_debug_flags, graphics::e::gist_debug_flags::enum_name); \
		ui::widget::checkbox(#enum_name, b);                                              \
		if (b)                                                                            \
		{                                                                                 \
			cmp.gist_debug_flags |= graphics::e::gist_debug_flags::enum_name;             \
		}                                                                                 \
		else                                                                              \
		{                                                                                 \
			cmp.gist_debug_flags &= ~graphics::e::gist_debug_flags::enum_name;            \
		}                                                                                 \
	}
			gist_flag_checkbox(freeze_surfel_spawn);
			gist_flag_checkbox(freeze_surfel_kill);
			gist_flag_checkbox(freeze_surfel_radius);
			gist_flag_checkbox(freeze_surfel_ray_trace);
#undef gist_flag_checkbox

			update_debug_flags = cmp.gist_debug_flags != debug_flag_cached;
		}

		if (update)
		{
			g::host_ops.p_renderer_update_gi(cmp, update_debug_flags);
		}
	}

	void
	ui_component(age::ecs::editor_cam_setting& cmp) noexcept
	{
		ui::widget::numeric_field(cmp.move_speed, "move_speed");
		ui::widget::numeric_field(cmp.sprint_mult, "sprint_mult");
		ui::widget::numeric_field(cmp.sensitivity, "sensitivity");
		ui::widget::numeric_field(cmp.zoom_speed, "zoom_speed");
		ui::widget::numeric_field(cmp.zoom_distance, "zoom_distance");
		ui::widget::numeric_field(cmp.pan_speed, "pan_speed");
		ui::widget::numeric_field(cmp.move_smoothing, "move_smoothing");
		ui::widget::numeric_field(cmp.look_smoothing, "look_smoothing");
		ui::widget::numeric_field(cmp.zoom_smoothing, "zoom_smoothing");
	}

	void
	ui_component(age::ecs::ao_config& cmp) noexcept
	{
		const bool update = ui::widget::button2("update");

		ui::widget::checkbox("enable", cmp.enabled);
		ui::widget::numeric_field(cmp.slice_count, "slice_count", uint8{ 1 }, uint8{ 16 });
		ui::widget::numeric_field(cmp.offset_count, "offset_count", uint8{ 1 }, uint8{ 16 });
		ui::widget::numeric_field(cmp.radius, "radius", 0.5f, 2.f);
		ui::widget::numeric_field(cmp.max_px_radius, "max_px_radius", 1.f, 128.f);
		ui::widget::numeric_field(cmp.intensity, "intensity", 0.f, 5.f);
		ui::widget::numeric_field(cmp.power, "power", 0.f, 5.f);
		ui::widget::numeric_field(cmp.thickness, "thickness", 0.f, 5.f);
		ui::widget::numeric_field(cmp.fade_distance, "fade_distance", 0.f, 10000.f);
		ui::widget::numeric_field(cmp.fade_range, "fade_range", 0.f, cmp.fade_distance);

		c_auto debug_flag_cached = cmp.debug_flags;
#define ao_debug_flag_checkbox(enum_name)                                          \
	{                                                                              \
		bool b = has_any(cmp.debug_flags, graphics::e::ao_debug_flags::enum_name); \
		ui::widget::checkbox(#enum_name, b);                                       \
		if (b)                                                                     \
		{                                                                          \
			cmp.debug_flags |= graphics::e::ao_debug_flags::enum_name;             \
		}                                                                          \
		else                                                                       \
		{                                                                          \
			cmp.debug_flags &= ~graphics::e::ao_debug_flags::enum_name;            \
		}                                                                          \
	}

		ao_debug_flag_checkbox(render_ao_buffer);

#undef ao_debug_flag_checkbox

		if (cmp.enabled and update)
		{
			g::host_ops.p_renderer_update_ao(cmp);
		}
	}

	void
	ui_component(age::ecs::aa_config& cmp) noexcept
	{
		constexpr c_auto rpp_option = age::array{
			ui::widget::dropdown_option<uint8>{ .value = 0, .label = "disable" },
			ui::widget::dropdown_option<uint8>{ .value = 2, .label = "2" },
			ui::widget::dropdown_option<uint8>{ .value = 4, .label = "4" },
			ui::widget::dropdown_option<uint8>{ .value = 8, .label = "8" },
			ui::widget::dropdown_option<uint8>{ .value = 16, .label = "16" },
			ui::widget::dropdown_option<uint8>{ .value = 32, .label = "32" },
		};

		const bool update = ui::widget::button2("update");

		ui::widget::checkbox("enable", cmp.enabled);
		ui::widget::checkbox("fxaa_on_offscreen", cmp.fxaa_on_offscreen);

		ui::widget::dropdown<uint8>(cmp.opaque_aa_ray_per_px, rpp_option);
		ui::widget::dropdown<uint8>(cmp.transparent_aa_ray_per_px, rpp_option);

		ui::widget::numeric_field(cmp.aa_px_cap, "aa_px_cap", math::g::epsilon_1e4, 1.f);
		ui::widget::numeric_field(cmp.aa_px_headroom, "aa_px_headroom", 1.f, 4.f);
		ui::widget::numeric_field(cmp.edge_plane_dist_tolerance_px, "edge_plane_dist_tolerance_px", 0.f, 8.f);
		ui::widget::numeric_field(cmp.edge_normal_threshold, "edge_normal_threshold", 0.f, 1.f);

		if (cmp.enabled and update)
		{
			g::host_ops.p_renderer_update_aa(cmp);
		}
	}

	void
	ui_component(age::ecs::debug_view_config& cmp) noexcept
	{
		ui::widget::checkbox("enable", cmp.enabled);
		ui::widget::checkbox("enable_pick [ctrl shift I]", cmp.pick_enabled);

		if (cmp.enabled is_false) { return; }

		auto update = false;
		{
			auto _ = ui::id_begin();
			update = ui::widget::button2("update");
		}

		if (auto _ = ui::widget::begin(ui::style::horizontal() | ui::set_width_grow() | ui::set_height_fit()))
		{
			if (auto _ = ui::widget::begin(ui::set_width_grow()))
			{
				ui::widget::text(std::format("pip_count : {}", cmp.slot_count).c_str());
			}

			if (cmp.slot_count == 1)
			{
				ui::widget::text_button("-", false);
			}
			else
			{
				if (ui::widget::button2("-"))
				{
					--cmp.slot_count;
				}
			}

			if (cmp.slot_count == cmp.slot_config_arr.size())
			{
				ui::widget::text_button("+", false);
			}
			else
			{
				if (ui::widget::button2("+"))
				{
					++cmp.slot_count;
				}
			}
		}

		cmp.slot_count = clamp(cmp.slot_count, 0u, cast_to<uint32>(cmp.slot_config_arr.size()));

		auto slot_func = [&](bool is_fullscreen, age::ecs::debug_view_config::debug_view_slot_config& slot_config, uint32 idx = 0) {
			auto id_ctx = ui::id_begin();
			if (auto _ = ui::widget::collapsible_header2(is_fullscreen ? "fullscreen_config" : std::format("pip_config[{}]", idx).c_str(), is_fullscreen))
			{
				ui::widget::dropdown(slot_config.system_kind);

				switch (slot_config.system_kind)
				{
				case age::graphics::e::hrp_debug_view_system_kind::common:
				{
					ui::widget::text("view_kind");
					ui::widget::dropdown<age::graphics::e::hrp_debug_view_kind_sys_common>(slot_config.system_debug_view_kind);

					ui::widget::text("popup_kind");
					ui::widget::dropdown<age::graphics::e::hrp_debug_view_sys_common_popup_kind>(slot_config.system_popup_view_kind);

					ui::widget::checkbox_flags<age::graphics::e::hrp_debug_view_overlay_flags_sys_common>("overlay_flags", slot_config.system_debug_view_overlay_flags, false);
					break;
				}
				case age::graphics::e::hrp_debug_view_system_kind::gist:
				{
					if (g::host_ops.p_renderer_gist_enabled() is_false)
					{
						ui::widget::text("gist is disabled");
						break;
					}

					ui::widget::text("surfel_select_mode");
					ui::widget::dropdown<age::graphics::e::hrp_debug_view_gist_cell_surfel_select_kind>(slot_config.payload[0].x);

					ui::widget::text("view_kind");
					ui::widget::dropdown<age::graphics::e::hrp_debug_view_kind_gist>(slot_config.system_debug_view_kind);

					ui::widget::text("popup_kind");
					ui::widget::dropdown<age::graphics::e::hrp_debug_view_gist_popup_kind>(slot_config.system_popup_view_kind);

					ui::widget::checkbox_flags<age::graphics::e::hrp_debug_view_overlay_flags_gist>("overlay_flags", slot_config.system_debug_view_overlay_flags, false);
					ui::widget::checkbox_flags<age::graphics::e::hrp_debug_view_cursor_overlay_flags_gist>("overlay_cursor_flags", slot_config.system_debug_view_cursor_overlay_flags, false);

					break;
				}
				default:
					break;
				}

				ui::widget::checkbox_flags<age::graphics::e::hrp_debug_view_slot_option_flags>("slot_option_flags", slot_config.option_flags, false);

				ui::widget::text("color_map");
				ui::widget::dropdown<age::graphics::e::hrp_debug_view_color_map_kind>(slot_config.color_map_kind);
				slot_config.system_popup_view_kind;

				if (is_fullscreen is_false)
				{
					ui::widget::numeric_field(slot_config.size_uv, "size_uv", float2::zero(), float2::one());
					ui::widget::numeric_field(slot_config.offset_uv, "offset_uv", float2{ -1.f }, float2::one());
					ui::widget::numeric_field(slot_config.pos_uv, "pos_uv", float2{ -1.f }, float2::one());
				}

				ui::widget::numeric_field(slot_config.scalar_range_min, "scalar_range_min", float3::zero(), slot_config.scalar_range_max);
				ui::widget::numeric_field(slot_config.scalar_range_max, "scalar_range_max", slot_config.scalar_range_min, float3::one());
				ui::widget::numeric_field(slot_config.alpha, "alpha", 0.f, 1.f);
				ui::widget::numeric_field(slot_config.popup_zoom, "popup_zoom", 0.f, 10.f);

				ui::widget::text("background_color");
				ui::widget::color_field(slot_config.background_color, 0.f, 10.f);

				ui::widget::numeric_field(slot_config.border_thickness, "border_thickness", 0u, 16u);
			}
		};

		slot_func(true, cmp.fullscreen_slot_config);

		for (auto&& [i, slot_config] : cmp.slot_config_arr | std::views::take(cmp.slot_count - 1) | std::views::enumerate)
		{
			ui::widget::separator_v();
			slot_func(false, slot_config, cast_to<uint32>(i));
		}

		ui::widget::separator_v();
		ui::widget::numeric_field(cmp.popup_view_size_uv, "popup_view_size_uv", float2::zero(), float2::one());
		ui::widget::numeric_field(cmp.popup_border_thickness, "popup_border_thickness", 0u, 16u);

		ui::widget::text("nan_color");
		ui::widget::color_field(cmp.nan_color, 0.f, 1000.f);
		ui::widget::text("pos_inf_color");
		ui::widget::color_field(cmp.pos_inf_color, 0.f, 1000.f);
		ui::widget::text("neg_inf_color");
		ui::widget::color_field(cmp.neg_inf_color, 0.f, 1000.f);
		ui::widget::text("zero_color");
		ui::widget::color_field(cmp.zero_color, 0.f, 1000.f);
		ui::widget::text("below_min_color");
		ui::widget::color_field(cmp.below_min_color, 0.f, 1000.f);
		ui::widget::text("above_max_color");
		ui::widget::color_field(cmp.above_max_color, 0.f, 1000.f);

		// todo, use update
		if (cmp.enabled and g::host_ops.p_renderer_debug_view_enabled())
		{
			g::host_ops.p_renderer_update_debug_view(cmp);
		}
	}

}	 // namespace age::editor

namespace age::editor
{
	namespace detail
	{
		auto&
		ui_modal_asset_name_input_scratch_buf() noexcept
		{
			static auto name = age::array<char, config::max_asset_path_len>{ "sample name" };

			return name;
		}
	}	 // namespace detail

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
		case e::modal_kind::import_asset:
		{
			ui_modal_import_asset();
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
	ui_asset_list_panel() noexcept
	{
		using namespace ui;
		if (auto _ = widget::panel(set_height_fit()))
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
						if (g::modal_kind == e::modal_kind::new_asset)
						{
							g::show_modal = !g::show_modal;
						}
						else
						{
							g::show_modal = true;
						}

						g::modal_kind = e::modal_kind::new_asset;
					}
				}

				if (auto h_btn = widget::button("import...", set_align_center()))
				{
					if (h_btn.clicked())
					{
						if (g::modal_kind == e::modal_kind::import_asset)
						{
							g::show_modal = !g::show_modal;
						}
						else
						{
							g::show_modal = true;
						}

						g::modal_kind = e::modal_kind::import_asset;
					}
				}
			}

			widget::separator_v();

			asset::for_each_kind(
				AGE_LAMBDA(
					<asset::e::kind e_kind>(),
					{
						// auto id_0 = id_begin();
						// if (asset::registry::all(e_kind).size() > 0)
						//{
						//	widget::text_heading(asset::e::to_string(e_kind).data());
						// }

						if (widget::collapsible_header2(asset::e::to_string(e_kind).data(), false))
						{
							for (c_auto h : asset::registry::all(e_kind))
							{
								auto id_0 = id_begin();
								// c_auto& display_name = h.get_display_name();
								auto btn = widget::begin(style::section() | set_padding_left(theme::padding_medium()) | set_horizontal() | set_width_grow() | set_height_fit() | set_interact());
								if (btn.clicked())
								{
									clear_select();
									add_select(e::select_kind::asset, to_idx(e_kind), h.id);
								}

								c_auto display_name = h.get_display_name();
								widget::begin(style::text(display_name.data()));

								auto _1 = widget::begin(style::section() | set_vertical() | set_width_grow() | set_height_fit());
								auto _2 = widget::begin(set_align_end() | set_width_fit() | set_height_fit());

								auto btn_remove_asset = widget::button("X");
								if (btn_remove_asset.clicked())
								{
									g::asset_to_delete[to_idx(e_kind)].emplace_back(h);
								}
							}
						}
					}));
		}
	}
}	 // namespace age::editor
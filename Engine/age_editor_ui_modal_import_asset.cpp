#include "age_pch.hpp"
#include "age.hpp"

namespace age::editor::detail
{
	age::ui::widget_ctx_impl<2>
	ui_import_data_header_helper(auto& import_data, bool default_open = false) noexcept
	{
		using enum input::e::key_kind;
		using namespace ui;
		if (auto h_header_section = widget::begin(style::layout(ui::e::widget_layout::vertical)
												  | set_size(size_mode::grow(), size_mode::fit())))
		{
			auto is_open = false;

			if (auto header = widget::begin(style::header_bar() | set_save_state(true) | set_interact(true)))
			{
				if (header.clicked<mouse_left>())
				{
					header.toggle();
				}

				is_open = header.is_toggled() != default_open;

				c_auto disclosure_indicator_size = font::get_line_height(theme::text_heading_font_size());
				widget::disclosure_indicator(is_open, disclosure_indicator_size);

				widget::text_input(import_data.name.data(), cast_to<uint32>(import_data.name.size()));

				widget::checkbox("enabled", import_data.enabled);

				auto _ = ui::id_begin();

				if constexpr (AGE_HAS_MEMBER(import_data, error_flags))
				{
					if (import_data.error_flags != BARE_OF(import_data.error_flags)::none)
					{
						widget::begin(style::text("has_error") | set_body_brush_color(theme::color_text_red()));
					}
				}

				if constexpr (AGE_HAS_MEMBER(import_data, warning_flags))
				{
					if (import_data.warning_flags != BARE_OF(import_data.warning_flags)::none)
					{
						widget::begin(style::text("has_warning") | set_body_brush_color(theme::color_text_amber()));
					}
				}
			}

			if (is_open)
			{
				widget::separator_v();

				c_auto disclosure_size = font::get_line_height(theme::text_heading_font_size());
				c_auto gap			   = theme::header_bar_child_gap();
				c_auto padding_l	   = theme::header_bar_padding().x;

				return widget_ctx_impl{ std::move(h_header_section), widget::vertical(set_padding_left(disclosure_size + padding_l + gap)) };
			}
		}

		return {};
	}

	age::ui::widget_ctx_impl<2>
	ui_import_data_header(auto& import_data, auto&& skip_fn) noexcept
	{
		using namespace age::ui;
		if (skip_fn(import_data)) { return {}; }

		if (auto h_header = ui_import_data_header_helper(import_data))
		{
			if (import_data.enabled is_false) { return {}; }

			auto _ = ui::id_begin();

			if constexpr (AGE_HAS_MEMBER(import_data, error_flags))
			{
				if (to_idx(import_data.error_flags))
				{
					for (auto	panel = widget::panel(set_width_grow() | set_height_fit());
						 c_auto flags : views::each_flags(import_data.error_flags))
					{
						widget::begin(style::text(to_string(flags).data()) | set_body_brush_color(theme::color_text_red()));
					}
				}
			}

			if constexpr (AGE_HAS_MEMBER(import_data, warning_flags))
			{
				if (to_idx(import_data.warning_flags))
				{
					for (auto	panel = widget::panel(set_width_grow() | set_height_fit());
						 c_auto flags : views::each_flags(import_data.warning_flags))
					{
						widget::begin(style::text(to_string(flags).data()) | set_body_brush_color(theme::color_text_amber()));
					}
				}
			}

			return h_header;
		}
		return {};
	}
}	 // namespace age::editor::detail

namespace age::editor
{
	void
	ui_modal_import_asset_texture() noexcept
	{
		ui::widget::text("ui_import_asset_texture not implemented yet");
	}

	void
	ui_modal_import_asset_mesh() noexcept
	{
		ui::widget::text("ui_import_asset_mesh not implemented yet");
	}

	void
	ui_modal_import_asset_gltf() noexcept
	{
		using namespace ui;
		static auto gltf_path_arr	   = age::array<char, config::max_asset_path_len>{};
		static auto gltf_import_data   = asset::importer::import_data{};
		static auto import_data_loaded = false;

		// pick path and load panel

		if (auto _ = widget::begin(style::header_bar()))
		{
			widget::text_title("gltf importer");
		}
		widget::separator_v();

		widget::text_heading("file path");
		widget::separator_v();
		widget::path_picker(gltf_path_arr, 10);
		if (std::filesystem::is_regular_file(gltf_path_arr.data()) is_false)
		{
			widget::begin(style::text("path does not exists") | set_body_brush_data(theme::color_text_red()));
			return;
		}

		widget::separator_v();

		if (gltf_import_data.src_full_path != std::filesystem::path{ gltf_path_arr.data() })
		{
			import_data_loaded = false;
			gltf_import_data   = {};
		}

		if (auto _ = widget::horizontal(set_height_fit()))
		{
			if (auto _ = widget::begin(set_width_fixed(100 + 100) | set_height_fit()))
			{
				widget::text_input(detail::ui_modal_asset_name_input_scratch_buf());
			}

			if (auto h_cancel = widget::button("cancel");
				h_cancel and h_cancel.clicked())
			{
				g::show_modal	   = false;
				import_data_loaded = false;
				gltf_import_data   = {};
			}

			if (auto h_load = widget::button(import_data_loaded ? "re_load" : "load");
				h_load is_true and h_load.clicked())
			{
				gltf_import_data = asset::importer::generate_gltf_import_data(
					asset::importer::parse_gltf(std::filesystem::path{ gltf_path_arr.data() }),
					detail::ui_modal_asset_name_input_scratch_buf().data(),
					asset::to_root_relative(get_asset_root_dir_path()));
				import_data_loaded = true;
			}
		}

		if (import_data_loaded is_false) { return; }

		resolve_import(gltf_import_data);

		widget::separator_v();

		// import panel
		static auto show_normal	 = true;
		static auto show_warning = true;
		static auto show_error	 = true;

		// assume flag::none == 0
		c_auto skip_fn = [](c_auto& parse_data) -> bool {
			bool has_warning = false;
			bool has_error	 = false;
			if constexpr (AGE_HAS_MEMBER(parse_data, warning_flags))
			{
				has_warning = to_idx(parse_data.warning_flags) > 0;
			}
			if constexpr (AGE_HAS_MEMBER(parse_data, error_flags))
			{
				has_error = to_idx(parse_data.error_flags) > 0;
			}

			if (has_warning is_false and has_error is_false)
			{
				return show_normal is_false;
			}
			else if (has_warning and has_error)
			{
				return (show_warning or show_error) is_false;
			}
			else if (has_warning)
			{
				return show_warning is_false;
			}
			else if (has_error)
			{
				return show_error is_false;
			}
			else
			{
				AGE_UNREACHABLE();
			}
		};

		c_auto texture_dropdown_option_vec = [&]() {
			auto res = age::vector<widget::dropdown_option<uint32>>::gen_reserved(gltf_import_data.texture_import_data_vec.size() + 1);
			res.emplace_back(age::get_invalid_idx<uint32>(), "(none)");
			for (const auto&& [i, tex] : gltf_import_data.texture_import_data_vec | views::enumerate<uint32>)
			{
				res.emplace_back(widget::dropdown_option<uint32>{ i, { tex.name.data(), tex.name.size() } });
			}
			return res;
		}();

		c_auto material_dropdown_option_vec = [&]() {
			auto res = age::vector<widget::dropdown_option<uint32>>::gen_reserved(gltf_import_data.material_import_data_vec.size() + 1);
			res.emplace_back(age::get_invalid_idx<uint32>(), "(none)");
			for (const auto&& [i, mat] : gltf_import_data.material_import_data_vec | views::enumerate<uint32>)
			{
				res.emplace_back(widget::dropdown_option<uint32>{ i, { mat.name.data(), mat.name.size() } });
			}
			return res;
		}();

		c_auto mesh_dropdown_option_vec = [&]() {
			auto res = age::vector<widget::dropdown_option<uint32>>::gen_reserved(gltf_import_data.mesh_import_data_vec.size() + 1);
			res.emplace_back(age::get_invalid_idx<uint32>(), "(none)");
			for (const auto&& [i, mesh] : gltf_import_data.mesh_import_data_vec | views::enumerate<uint32>)
			{
				res.emplace_back(widget::dropdown_option<uint32>{ i, { mesh.name.data(), mesh.name.size() } });
			}
			return res;
		}();

		c_auto skeleton_dropdown_option_vec = [&]() {
			auto res = age::vector<widget::dropdown_option<uint32>>::gen_reserved(gltf_import_data.skeleton_import_data_vec.size() + 1);
			res.emplace_back(age::get_invalid_idx<uint32>(), "(none)");
			for (const auto&& [i, skeleton] : gltf_import_data.skeleton_import_data_vec | views::enumerate<uint32>)
			{
				res.emplace_back(widget::dropdown_option<uint32>{ i, { skeleton.name.data(), skeleton.name.size() } });
			}
			return res;
		}();

		c_auto model_dropdown_option_vec = [&]() {
			auto res = age::vector<widget::dropdown_option<uint32>>::gen_reserved(gltf_import_data.model_import_data_vec.size() + 1);
			res.emplace_back(age::get_invalid_idx<uint32>(), "(none)");
			for (const auto&& [i, model] : gltf_import_data.model_import_data_vec | views::enumerate<uint32>)
			{
				res.emplace_back(widget::dropdown_option<uint32>{ i, { model.name.data(), model.name.size() } });
			}
			return res;
		}();

		const auto&& [dropdown_label_string_container, storage_dropdown_option_vec] = [&]() {
			c_auto storage_count = *std::ranges::fold_left_first(g::current_game.scene_data_vec
																	 | std::views::transform([](c_auto& scene) { return scene.storage_data_vec.size<uint32>(); }),
																 std::plus{});

			auto dropdown_option_vec	 = age::vector<widget::dropdown_option<std::pair<uint32, uint32>>>::gen_reserved(storage_count);
			auto editor_storage_name_vec = age::vector<std::string>::gen_reserved(storage_count);
			for (const auto&& [scene_idx, scene] : g::current_game.scene_data_vec | views::enumerate<uint32>)
			{
				for (const auto&& [storage_idx, storage] : scene.storage_data_vec | views::enumerate<uint32>)
				{
					editor_storage_name_vec.emplace_back(std::format("scene : {}, storage : {}", scene.names[0].data(), storage.names[0].data()));
					dropdown_option_vec.emplace_back(widget::dropdown_option<std::pair<uint32, uint32>>{ { scene_idx, storage_idx }, std::string_view{ editor_storage_name_vec.back() } });
				}
			}
			return std::tuple{ std::move(editor_storage_name_vec), std::move(dropdown_option_vec) };
		}();

		if (auto _		= widget::begin(set_padding_zero() | set_width_grow() | set_height_grow());
			auto scroll = widget::scroll_area_v())
		{
			// file
			if (auto file_section = widget::begin(style::section() | set_width_grow() | set_height_fit()))
			{
				widget::text_heading(gltf_import_data.src_full_path.data());
				{
					auto _ = ui::id_begin();
					if (gltf_import_data.error_flags != asset::importer::e::import_error_flags::none)
					{
						if (widget::separator_v();
							auto _ = widget::collapsible_header("import error"))
						{
							for (auto	panel = widget::panel(set_width_grow() | set_height_fit());
								 c_auto flags : views::each_flags(gltf_import_data.error_flags))
							{
								widget::begin(style::text(to_string(flags).data()) | set_body_brush_color(theme::color_text_red()));
							}
						}
					}

					if (gltf_import_data.warning_flags != asset::importer::e::import_warning_flags::none)
					{
						if (widget::separator_v();
							auto _ = widget::collapsible_header("import warning"))
						{
							for (auto	panel = widget::panel(set_width_grow() | set_height_fit());
								 c_auto flags : views::each_flags(gltf_import_data.warning_flags))
							{
								widget::begin(style::text(to_string(flags).data()) | set_body_brush_color(theme::color_text_amber()));
							}
						}
					}
				}

				if (widget::separator_v();
					auto _ = widget::collapsible_header("target directories"))
				{
					for (auto	panel = widget::panel(set_width_grow() | set_height_fit());
						 c_auto flags : views::each_flags(gltf_import_data.error_flags))
					{
						widget::begin(style::text(to_string(flags).data()) | set_body_brush_color(theme::color_text_red()));
					}

					if (auto _ = widget::horizontal(set_width_grow() | set_height_fit()))
					{
						widget::begin(style::text("texture_dir") | set_width_fixed(200));
						widget::text_input(gltf_import_data.texture_dir);
					}
					if (auto _ = widget::horizontal(set_width_grow() | set_height_fit()))
					{
						widget::begin(style::text("material_dir") | set_width_fixed(200));
						widget::text_input(gltf_import_data.material_dir);
					}
					if (auto _ = widget::horizontal(set_width_grow() | set_height_fit()))
					{
						widget::begin(style::text("mesh_dir") | set_width_fixed(200));
						widget::text_input(gltf_import_data.mesh_dir);
					}
					if (auto _ = widget::horizontal(set_width_grow() | set_height_fit()))
					{
						widget::begin(style::text("skeleton_dir") | set_width_fixed(200));
						widget::text_input(gltf_import_data.skeleton_dir);
					}
					if (auto _ = widget::horizontal(set_width_grow() | set_height_fit()))
					{
						widget::begin(style::text("model_dir") | set_width_fixed(200));
						widget::text_input(gltf_import_data.model_dir);
					}
					if (auto _ = widget::horizontal(set_width_grow() | set_height_fit()))
					{
						widget::begin(style::text("scene_dir") | set_width_fixed(200));
						widget::text_input(gltf_import_data.scene_dir);
					}
				}

				if (widget::separator_v();
					auto _ = widget::panel(set_horizontal() | set_width_grow() | set_height_fit()))
				{
					auto hide_normal  = show_normal is_false;
					auto hide_warning = show_warning is_false;
					auto hide_error	  = show_error is_false;
					widget::checkbox("hide items without any error or warning", hide_normal);
					widget::checkbox("hide items with warning", hide_warning);
					widget::checkbox("hide items with error", hide_error);
					show_normal	 = hide_normal	 is_false;
					show_warning = hide_warning is_false;
					show_error	 = hide_error	  is_false;
				}
			}

			// texture
			if (widget::separator_v();
				auto _ = widget::collapsible_header3(std::format("texture [{}]", gltf_import_data.texture_import_data_vec.size()).data()))
			{
				for (auto h_child_section = widget::begin(set_vertical() | set_width_grow() | set_height_fit() | set_align_begin());
					 auto&& [i, texture_import] : gltf_import_data.texture_import_data_vec | views::enumerate<uint32>)
				{
					auto header = detail::ui_import_data_header(texture_import, skip_fn);
					if (header is_false) { continue; }

					if (auto _ = widget::horizontal(set_width_grow() | set_height_fit());
						texture_import.material_reference_vec.is_not_empty())
					{
						widget::begin(style::text("material references") | set_width_fixed(200) | set_align_begin());

						auto _0 = widget::begin(set_width_grow() | set_height_fit() | set_vertical());
						for (auto&& [mat_idx, usage] : texture_import.material_reference_vec)
						{
							auto& mat = gltf_import_data.material_import_data_vec[mat_idx];

							auto _1 = widget::begin(set_width_grow() | set_height_fit() | set_horizontal());
							widget::begin(style::text(std::format("usage : {}", to_string(usage)).data()) | set_width_fixed(200));
							widget::begin(style::text(std::format("mat : {}", mat.name.data()).data()));
						}

						widget::separator_v();
					}

					if (auto _ = widget::horizontal(set_width_grow() | set_height_fit());
						texture_import.model_submesh_reference_vec.is_not_empty())
					{
						widget::begin(style::text("model_submesh references") | set_width_fixed(200) | set_align_begin());

						auto _0 = widget::begin(set_width_grow() | set_height_fit() | set_vertical());
						for (auto&& [model_idx, submesh_idx] : texture_import.model_submesh_reference_vec)
						{
							auto& model	  = gltf_import_data.model_import_data_vec[model_idx];
							auto& mesh	  = gltf_import_data.mesh_import_data_vec[model.mesh_idx];
							auto& submesh = mesh.submesh_vec[submesh_idx];

							auto _1 = widget::begin(set_width_grow() | set_height_fit() | set_horizontal());
							widget::begin(style::text(std::format("model : {}", model.name).data()) | set_width_fixed(200));
							widget::begin(style::text(std::format("mesh : {}", mesh.name).data()) | set_width_fixed(200));
							widget::begin(style::text(std::format("submesh_idx : {}", submesh_idx).data()) | set_width_fixed(200));
							widget::begin(style::text(std::format("need_alpha_chenel : {}", submesh.need_alpha_channel()).data()) | set_width_fixed(200));
						}

						widget::separator_v();
					}


					auto& bake_option = texture_import.bake_option;

					if (auto _ = widget::horizontal(set_width_grow() | set_height_fit()))
					{
						widget::begin(style::text(std::format("file kind {}", to_string(texture_import.file_kind)).data()) | set_width_fixed(200));
						widget::dropdown(bake_option.format);
					}

					if (auto _ = widget::horizontal(set_width_grow() | set_height_fit()))
					{
						widget::begin(style::text("fit power of 2") | set_width_fixed(200));
						widget::checkbox(nullptr, bake_option.fit_pow2);
					}
					if (auto _ = widget::horizontal(set_width_grow() | set_height_fit()))
					{
						widget::begin(style::text("mip count") | set_width_fixed(200));
						widget::numeric_field(bake_option.mip_count);
					}
					if (auto _ = widget::horizontal(set_width_grow() | set_height_fit()))
					{
						widget::begin(style::text("mip filter mode") | set_width_fixed(200));
						widget::dropdown(bake_option.filter);
					}
					if (auto _ = widget::horizontal(set_width_grow() | set_height_fit()))
					{
						widget::begin(style::text("wrap mode") | set_width_fixed(200));
						widget::dropdown(bake_option.wrap);
					}
					if (auto _ = widget::horizontal(set_width_grow() | set_height_fit()))
					{
						widget::begin(style::text("hflip") | set_width_fixed(200));
						widget::checkbox(nullptr, bake_option.hflip);
					}
					if (auto _ = widget::horizontal(set_width_grow() | set_height_fit()))
					{
						widget::begin(style::text("vflip") | set_width_fixed(200));
						widget::checkbox(nullptr, bake_option.vflip);
					}
					if (auto _ = widget::horizontal(set_width_grow() | set_height_fit()))
					{
						widget::begin(style::text("invert_y") | set_width_fixed(200));
						widget::checkbox(nullptr, bake_option.invert_y);
					}
					if (auto _ = widget::horizontal(set_width_grow() | set_height_fit()))
					{
						widget::begin(style::text("alpha_threshold") | set_width_fixed(200));
						widget::text(std::format("{}", bake_option.alpha_threshold).data());
					}
					if (auto _ = widget::horizontal(set_width_grow() | set_height_fit()))
					{
						widget::begin(style::text("keep_coverage") | set_border_thickness(1.f) | set_border_brush_color(theme::color_red()) | set_width_fixed(200));
						widget::text(std::format("{}", bake_option.keep_coverage).data());
					}
				}
			}

			// material
			if (widget::separator_v();
				auto _ = widget::collapsible_header3(std::format("material [{}]", gltf_import_data.material_import_data_vec.size()).data()))
			{
				for (auto h_child_section = widget::begin(set_vertical() | set_width_grow() | set_height_fit() | set_align_begin());
					 auto&& [i, material_import] : gltf_import_data.material_import_data_vec | views::enumerate<uint32>)
				{
					c_auto header = detail::ui_import_data_header(material_import, skip_fn);
					if (header is_false) { continue; }

					if (auto _ = widget::horizontal(set_width_grow() | set_height_fit());
						material_import.model_reference_vec.is_not_empty())
					{
						widget::begin(style::text("model_submesh references") | set_width_fixed(200) | set_align_begin());

						auto _0 = widget::begin(set_width_grow() | set_height_fit() | set_vertical());
						for (auto&& [model_idx, nth_mat] : material_import.model_reference_vec)
						{
							c_auto& model = gltf_import_data.model_import_data_vec[model_idx];
							c_auto& mesh  = gltf_import_data.mesh_import_data_vec[model.mesh_idx];

							auto _1 = widget::begin(set_width_grow() | set_height_fit() | set_horizontal());
							widget::begin(style::text(std::format("model : {}", model.name).data()) | set_width_fixed(200));
							widget::begin(style::text(std::format("mesh : {}", mesh.name).data()) | set_width_fixed(200));
							widget::begin(style::text(std::format("nth_mat : {}", nth_mat).data()) | set_width_fixed(200));
						}

						widget::separator_v();
					}

					if (auto _ = widget::horizontal(set_width_grow() | set_height_fit()))
					{
						widget::begin(style::text("base_color_factor") | set_width_fixed(200));
						ui::widget::color_field(material_import.base_color_factor);
					}

					if (auto _ = widget::horizontal(set_width_grow() | set_height_fit()))
					{
						widget::begin(style::text("metallic_factor") | set_width_fixed(200));
						widget::numeric_field(material_import.metallic_factor, nullptr, 0.f, 1.f);
					}

					if (auto _ = widget::horizontal(set_width_grow() | set_height_fit()))
					{
						widget::begin(style::text("roughness_factor") | set_width_fixed(200));
						widget::numeric_field(material_import.roughness_factor, nullptr, 0.f, 1.f);
					}

					if (auto _ = widget::horizontal(set_width_grow() | set_height_fit()))
					{
						widget::begin(style::text("emissive_factor") | set_width_fixed(200));
						widget::color_field(material_import.emissive_factor, 0.f, std::numeric_limits<float>::max());
					}

					if (auto _ = widget::horizontal(set_width_grow() | set_height_fit()))
					{
						widget::begin(style::text("occlusion_strength") | set_width_fixed(200));
						widget::numeric_field(material_import.occlusion_strength, nullptr, 0.f, 1.f);
					}

					if (auto _ = widget::horizontal(set_width_grow() | set_height_fit()))
					{
						widget::begin(style::text("alpha_cutoff") | set_width_fixed(200));
						widget::numeric_field(material_import.alpha_cutoff, nullptr, 0.f, 1.f);
					}

					if (auto _ = widget::horizontal(set_width_grow() | set_height_fit()))
					{
						widget::begin(style::text("shading_model") | set_width_fixed(200));
						widget::dropdown(material_import.shading_model);
					}

					if (auto _ = widget::horizontal(set_width_grow() | set_height_fit()))
					{
						widget::begin(style::text("double_sided") | set_width_fixed(200));
						widget::checkbox(nullptr, material_import.double_sided);
					}

					if (auto _ = widget::horizontal(set_width_grow() | set_height_fit()))
					{
						widget::begin(style::text("base_color_sampler_kind") | set_width_fixed(200));
						widget::dropdown(material_import.base_color_texture_idx, texture_dropdown_option_vec);
						widget::dropdown(material_import.base_color_sampler_kind);
					}

					if (auto _ = widget::horizontal(set_width_grow() | set_height_fit()))
					{
						widget::begin(style::text("metallic_roughness_sampler_kind") | set_width_fixed(200));
						widget::dropdown(material_import.metallic_roughness_texture_idx, texture_dropdown_option_vec);
						widget::dropdown(material_import.metallic_roughness_sampler_kind);
					}

					if (auto _ = widget::horizontal(set_width_grow() | set_height_fit()))
					{
						widget::begin(style::text("normal_sampler_kind") | set_width_fixed(200));
						widget::dropdown(material_import.normal_texture_idx, texture_dropdown_option_vec);
						widget::dropdown(material_import.normal_sampler_kind);
					}

					if (auto _ = widget::horizontal(set_width_grow() | set_height_fit()))
					{
						widget::begin(style::text("occlusion_sampler_kind") | set_width_fixed(200));
						widget::dropdown(material_import.occlusion_texture_idx, texture_dropdown_option_vec);
						widget::dropdown(material_import.occlusion_sampler_kind);
					}

					if (auto _ = widget::horizontal(set_width_grow() | set_height_fit()))
					{
						widget::begin(style::text("emissive_sampler_kind") | set_width_fixed(200));
						widget::dropdown(material_import.emissive_texture_idx, texture_dropdown_option_vec);
						widget::dropdown(material_import.emissive_sampler_kind);
					}
				}
			}

			// mesh
			if (widget::separator_v();
				auto _ = widget::collapsible_header3(std::format("mesh [{}]", gltf_import_data.mesh_import_data_vec.size()).data()))
			{
				for (auto h_child_section = widget::begin(set_vertical() | set_width_grow() | set_height_fit() | set_align_begin());
					 auto&& [i, mesh_import] : gltf_import_data.mesh_import_data_vec | views::enumerate<uint32>)
				{
					c_auto header = detail::ui_import_data_header(mesh_import, skip_fn);
					if (header is_false) { continue; }

					// model references
					if (auto _ = widget::horizontal(set_width_grow() | set_height_fit());
						mesh_import.model_reference_vec.is_not_empty())
					{
						widget::begin(style::text("model references") | set_width_fixed(200) | set_align_begin());

						auto _0 = widget::begin(set_width_grow() | set_height_fit() | set_vertical());
						for (c_auto& model : mesh_import.model_reference_vec | views::idx_to(gltf_import_data.model_import_data_vec))
						{
							auto _1 = widget::begin(set_width_grow() | set_height_fit() | set_horizontal());
							widget::begin(style::text(std::format("model : {}", model.name).data()) | set_width_fixed(200));
						}

						widget::separator_v();
					}

					// blend shape
					if (auto _ = widget::horizontal(set_width_grow() | set_height_fit());
						mesh_import.blend_shape_name_vec.is_not_empty())
					{
						widget::begin(style::text("blend shapes") | set_width_fixed(200) | set_align_begin());

						auto _0 = widget::begin(set_width_grow() | set_height_fit() | set_vertical());
						for (auto&& [i, blend_shape_name, blend_weight] :
							 std::views::zip(views::loop(mesh_import.blend_shape_name_vec.size<uint32>()), mesh_import.blend_shape_name_vec, mesh_import.blend_shape_weight_vec))
						{
							auto _1 = widget::begin(set_width_grow() | set_height_fit() | set_horizontal());
							widget::begin(style::text(std::format("blend_shape [{}]", i).data()) | set_width_fixed(200));
							widget::begin(style::text(std::format("weight : {}", blend_weight).data()) | set_width_fixed(200));
							widget::text(std::format("name : {}", blend_shape_name.data()).data());
						}

						widget::separator_v();
					}

					// joint names
					if (auto _ = widget::horizontal(set_width_grow() | set_height_fit());
						mesh_import.joint_name_vec.is_not_empty())
					{
						widget::begin(style::text("joint names") | set_width_fixed(200) | set_align_begin());

						auto _0 = widget::begin(set_width_grow() | set_height_fit() | set_vertical());
						for (auto&& [i, joint_name] : mesh_import.joint_name_vec | views::enumerate<uint32>)
						{
							auto _1 = widget::begin(set_width_grow() | set_height_fit() | set_horizontal());
							widget::begin(style::text(std::format("joint [{}]", i).data()) | set_width_fixed(200));
							widget::text(std::format(": {}", joint_name.data()).data());
						}

						widget::separator_v();
					}

					// submeshes
					if (auto _ = widget::horizontal(set_width_grow() | set_height_fit());
						mesh_import.submesh_vec.is_not_empty())
					{
						widget::begin(style::text("submesh") | set_width_fixed(200) | set_align_begin());

						auto _0 = widget::begin(set_width_grow() | set_height_fit() | set_vertical());
						for (auto&& [i, submesh] : mesh_import.submesh_vec | views::enumerate<uint32>)
						{
							if (auto _ = widget::collapsible_header(std::format("submesh [{}]", i).data()))
							{
								if (auto _ = widget::horizontal(set_width_grow() | set_height_fit()))
								{
									widget::begin(style::text("has_normal") | set_width_fixed(200));
									widget::text(submesh.has_normal ? "true" : "false");
								}
								if (auto _ = widget::horizontal(set_width_grow() | set_height_fit()))
								{
									widget::begin(style::text("has_tangent") | set_width_fixed(200));
									widget::text(submesh.has_tangent ? "true" : "false");
								}
								if (auto _ = widget::horizontal(set_width_grow() | set_height_fit()))
								{
									widget::begin(style::text("has_uv") | set_width_fixed(200));
									widget::text(submesh.has_uv ? "true" : "false");
								}

								if (auto _ = widget::horizontal(set_width_grow() | set_height_fit()))
								{
									widget::begin(style::text("index count") | set_width_fixed(200));
									widget::text(std::format("{}", submesh.index_buffer.size()).data());
								}

								if (auto _ = widget::horizontal(set_width_grow() | set_height_fit()))
								{
									widget::begin(style::text("vertex count") | set_width_fixed(200));
									widget::text(std::format("{}", submesh.vertex_buffer.size()).data());
								}

								if (auto _ = widget::horizontal(set_width_grow() | set_height_fit()))
								{
									widget::begin(style::text("raster_mode") | set_width_fixed(200));
									widget::dropdown(submesh.raster_mode);
								}
								if (auto _ = widget::horizontal(set_width_grow() | set_height_fit()))
								{
									widget::begin(style::text("rt_alpha_test_mode") | set_width_fixed(200));
									widget::dropdown(submesh.rt_alpha_test_mode);
								}
								if (auto _ = widget::horizontal(set_width_grow() | set_height_fit()))
								{
									widget::begin(style::text("rt_bake_mode") | set_width_fixed(200));
									widget::dropdown(submesh.rt_bake_mode);
								}
							}
						}

						widget::separator_v();
					}
				}
			}

			// skeleton
			if (widget::separator_v();
				auto _ = widget::collapsible_header3(std::format("skeleton [{}]", gltf_import_data.skeleton_import_data_vec.size()).data()))
			{
				for (auto h_child_section = widget::begin(set_vertical() | set_width_grow() | set_height_fit() | set_align_begin());
					 auto&& [i, skeleton_import] : gltf_import_data.skeleton_import_data_vec | views::enumerate<uint32>)
				{
					c_auto header = detail::ui_import_data_header(skeleton_import, skip_fn);
					if (header is_false) { continue; }

					for (auto _id = ui::id_begin();
						 auto&& [nth_joint, joint_name] : skeleton_import.joint_name_vec | views::enumerate<uint32>)
					{
						widget::begin(style::text(std::format("joint [{}]", nth_joint).data()) | set_width_fixed(200));
						widget::text_input(joint_name);
					}
				}
			}

			// model
			if (widget::separator_v();
				auto _ = widget::collapsible_header3(std::format("model [{}]", gltf_import_data.model_import_data_vec.size()).data()))
			{
				for (auto h_child_section = widget::begin(set_vertical() | set_width_grow() | set_height_fit() | set_align_begin());
					 auto&& [i, model_import] : gltf_import_data.model_import_data_vec | views::enumerate<uint32>)
				{
					c_auto header = detail::ui_import_data_header(model_import, skip_fn);
					if (header is_false) { continue; }

					if (auto _ = widget::horizontal(set_width_grow() | set_height_fit()))
					{
						widget::begin(style::text("mesh") | set_width_fixed(200));
						widget::dropdown(model_import.mesh_idx, mesh_dropdown_option_vec);
					}

					if (auto _ = widget::begin(set_horizontal() | set_width_grow() | set_height_fit()))
					{
						widget::begin(style::text("materials") | set_width_fixed(200));

						if (auto btn = widget::button("+");
							btn.clicked())
						{
							model_import.submesh_material_idx_vec.emplace_back(age::get_invalid_idx<uint32>());
						}

						if (auto btn = widget::button("-");
							btn.clicked())
						{
							model_import.submesh_material_idx_vec.pop_back();
						}
					}

					widget::separator_v();

					for (auto _id = ui::id_begin();
						 auto&& [nth_mat, mat_idx] : model_import.submesh_material_idx_vec | views::enumerate<uint32>)
					{
						if (auto _ = widget::horizontal(set_width_grow() | set_height_fit()))
						{
							widget::begin(style::text(std::format("material [{}]", nth_mat).data()) | set_width_fixed(200));
							widget::dropdown(mat_idx, mesh_dropdown_option_vec);
						}
					}
				}
			}

			// scene, entities, light, camera
			if (widget::separator_v();
				auto _ = widget::collapsible_header3(std::format("scene [{}]", gltf_import_data.scene_import_data_vec.size()).data()))
			{
				for (auto h_child_section = widget::begin(set_vertical() | set_width_grow() | set_height_fit() | set_align_begin());
					 auto&& [i, scene_import] : gltf_import_data.scene_import_data_vec | views::enumerate<uint32>)
				{
					c_auto header = detail::ui_import_data_header(scene_import, skip_fn);
					if (header is_false) { continue; }

					if (auto _ = widget::horizontal(set_width_grow() | set_height_fit()))
					{
						widget::begin(style::text("instantiate") | set_width_fixed(200));
						widget::checkbox(nullptr, scene_import.instantiate);
					}

					if (auto _ = ui::id_begin();
						scene_import.instantiate)
					{
						auto pair = std::pair<uint32, uint32>{ scene_import.instantiate_target_scene_idx, scene_import.instantiate_target_storage_idx };
						widget::dropdown(pair, storage_dropdown_option_vec);
						scene_import.instantiate_target_scene_idx	= pair.first;
						scene_import.instantiate_target_storage_idx = pair.second;

						c_auto name_count = [&]() {
							auto res = 0u;
							for (c_auto& cmp_data : g::current_game.scene_data_vec[pair.first].storage_data_vec[pair.second].component_data_vec)
							{
								res += cmp_data.names.size<uint32>();
							}
							return res;
						}();

						auto editor_cmp_names = age::vector<std::string_view>::gen_reserved(name_count);

						for (c_auto& cmp_data : g::current_game.scene_data_vec[pair.first].storage_data_vec[pair.second].component_data_vec)
						{
							for (c_auto& name : cmp_data.names)
							{
								editor_cmp_names.emplace_back(std::string_view{ name.data() });
							}
						}

						c_auto contains_component = [](c_auto& editor_cmp_names, c_auto& ecs_cmp_names) {
							for (c_auto& editor_cmp_name : editor_cmp_names)
							{
								for (c_auto& ecs_cmp_name : ecs_cmp_names)
								{
									if (editor_cmp_name == std::string_view{ ecs_cmp_name.data() })
									{
										return true;
									}
								}
							}

							return false;
						};

						scene_import.error_flags &= ~(BARE_OF(scene_import.error_flags)::target_storage_missing_component);

						if (scene_import.has_hierarchy and contains_component(editor_cmp_names, ecs::get_component_name<ecs::parent_id>()) is_false)
						{
							scene_import.error_flags |= BARE_OF(scene_import.error_flags)::target_storage_missing_component;
						}
						if (scene_import.has_model and contains_component(editor_cmp_names, ecs::get_component_name<ecs::model>()) is_false)
						{
							scene_import.error_flags |= BARE_OF(scene_import.error_flags)::target_storage_missing_component;
						}
						if (scene_import.has_skinned_model and contains_component(editor_cmp_names, ecs::get_component_name<ecs::skinned_model>()) is_false)
						{
							scene_import.error_flags |= BARE_OF(scene_import.error_flags)::target_storage_missing_component;
						}
						if (scene_import.has_skeleton and contains_component(editor_cmp_names, ecs::get_component_name<ecs::skeleton>()) is_false)
						{
							scene_import.error_flags |= BARE_OF(scene_import.error_flags)::target_storage_missing_component;
						}
						if (scene_import.has_directional_light and contains_component(editor_cmp_names, ecs::get_component_name<ecs::directional_light>()) is_false)
						{
							scene_import.error_flags |= BARE_OF(scene_import.error_flags)::target_storage_missing_component;
						}
						if (scene_import.has_point_light and contains_component(editor_cmp_names, ecs::get_component_name<ecs::point_light>()) is_false)
						{
							scene_import.error_flags |= BARE_OF(scene_import.error_flags)::target_storage_missing_component;
						}
						if (scene_import.has_spot_light and contains_component(editor_cmp_names, ecs::get_component_name<ecs::spot_light>()) is_false)
						{
							scene_import.error_flags |= BARE_OF(scene_import.error_flags)::target_storage_missing_component;
						}
						if (scene_import.camera_data_vec.is_not_empty() and contains_component(editor_cmp_names, ecs::get_component_name<ecs::camera>()) is_false)
						{
							scene_import.error_flags |= BARE_OF(scene_import.error_flags)::target_storage_missing_component;
						}
						if (scene_import.joint_attach_data_vec.is_not_empty() and contains_component(editor_cmp_names, ecs::get_component_name<ecs::joint_attach>()) is_false)
						{
							scene_import.error_flags |= BARE_OF(scene_import.error_flags)::target_storage_missing_component;
						}
						if (scene_import.has_blend_shape_weight_override and contains_component(editor_cmp_names, ecs::get_component_name<ecs::blend_shape_weight_override>()) is_false)
						{
							scene_import.error_flags |= BARE_OF(scene_import.error_flags)::target_storage_missing_component;
						}

						widget::separator_v();
					}

					// entity
					if (auto _ = widget::collapsible_header("entity"))
					{
						for (auto _id = ui::id_begin();
							 auto&& [entity_idx, entity_import] : scene_import.entity_vec | views::enumerate<uint32>)
						{
							c_auto ent_header = detail::ui_import_data_header(entity_import, skip_fn);
							if (ent_header is_false) { continue; }

							if (auto _ = widget::horizontal(set_width_grow() | set_height_fit()))
							{
								widget::begin(style::text("is_skinned") | set_width_fixed(200));
								widget::checkbox(nullptr, entity_import.is_skinned);
							}
							if (auto _ = widget::horizontal(set_width_grow() | set_height_fit()))
							{
								widget::begin(style::text("translation") | set_width_fixed(200));
								widget::numeric_field(entity_import.translation, nullptr);
							}
							if (auto _ = widget::horizontal(set_width_grow() | set_height_fit()))
							{
								widget::begin(style::text("rotation") | set_width_fixed(200));
								widget::rotation_field(entity_import.rotation, nullptr);
							}
							if (auto _ = widget::horizontal(set_width_grow() | set_height_fit()))
							{
								widget::begin(style::text("scale") | set_width_fixed(200));
								widget::numeric_field(entity_import.scale, nullptr, float3::zero());
							}
							if (auto _ = widget::horizontal(set_width_grow() | set_height_fit()))
							{
								widget::begin(style::text("parent : ") | set_width_fixed(200));
								widget::text(runtime::is_invalid_idx(entity_import.parent_idx) ? "(none)" : scene_import.entity_vec[entity_import.parent_idx].name.data());
							}

							widget::separator_v();

							// camera
							[&] {
								if (auto _ = ui::id_begin();
									runtime::is_invalid_idx(entity_import.camera_idx) is_false)
								{
									auto camera_header = widget::collapsible_header(std::format("camera [{}]", entity_import.camera_idx).data());
									if (camera_header is_false) { return; }

									auto& camera = scene_import.camera_data_vec[entity_import.camera_idx];
									if (auto _ = widget::horizontal(set_width_grow() | set_height_fit()))
									{
										widget::begin(style::text("enabled") | set_width_fixed(200));
										widget::checkbox(nullptr, camera.enabled);
									}

									if (camera.enabled is_false) { return; }

									if (auto _ = widget::horizontal(set_width_grow() | set_height_fit()))
									{
										widget::begin(style::text("camera_kind") | set_width_fixed(200));
										widget::dropdown(camera.kind);
									}
									if (auto _ = widget::horizontal(set_width_grow() | set_height_fit()))
									{
										widget::begin(style::text("euler_deg") | set_width_fixed(200));
										widget::numeric_field(camera.euler_deg, nullptr, float3{ -90.f, -180.f, -180.f }, float3{ 90.f, 180.f, 180.f });
									}
									if (auto _ = widget::horizontal(set_width_grow() | set_height_fit()))
									{
										widget::begin(style::text("near_z") | set_width_fixed(200));
										widget::numeric_field(camera.near_z, nullptr, 0.f, camera.far_z);
									}
									if (auto _ = widget::horizontal(set_width_grow() | set_height_fit()))
									{
										widget::begin(style::text("far_z") | set_width_fixed(200));
										widget::numeric_field(camera.far_z, nullptr, camera.near_z);
									}
									if (auto _ = widget::horizontal(set_width_grow() | set_height_fit()))
									{
										widget::begin(style::text("fov_y") | set_width_fixed(200));
										widget::numeric_field(camera.fov_y, nullptr, 0.f, math::g::pi);
									}
									if (auto _ = widget::horizontal(set_width_grow() | set_height_fit()))
									{
										widget::begin(style::text("aspect_ratio") | set_width_fixed(200));
										widget::numeric_field(camera.aspect_ratio, nullptr, 0.f);
									}
									if (auto _ = widget::horizontal(set_width_grow() | set_height_fit()))
									{
										widget::begin(style::text("view_width") | set_width_fixed(200));
										widget::numeric_field(camera.view_width, nullptr, 0.f);
									}
									if (auto _ = widget::horizontal(set_width_grow() | set_height_fit()))
									{
										widget::begin(style::text("view_height") | set_width_fixed(200));
										widget::numeric_field(camera.view_height, nullptr, 0.f);
									}

									widget::separator_v();
								}
							}();

							// light
							[&] {
								if (auto _ = ui::id_begin();
									runtime::is_invalid_idx(entity_import.light_idx) is_false)
								{
									auto light_header = widget::collapsible_header(std::format("light [{}]", entity_import.light_idx).data());
									if (light_header is_false) { return; }
									auto& light = scene_import.light_data_vec[entity_import.light_idx];

									if (auto _ = widget::horizontal(set_width_grow() | set_height_fit()))
									{
										widget::begin(style::text("enabled") | set_width_fixed(200));
										widget::checkbox(nullptr, light.enabled);
									}

									if (light.enabled is_false) { return; }

									if (auto _ = widget::horizontal(set_width_grow() | set_height_fit()))
									{
										widget::begin(style::text("light_kind") | set_width_fixed(200));
										widget::dropdown(light.kind);
									}
									if (auto _ = widget::horizontal(set_width_grow() | set_height_fit()))
									{
										widget::begin(style::text("cast_shadow") | set_width_fixed(200));
										widget::checkbox(nullptr, light.cast_shadow);
									}
									if (auto _ = widget::horizontal(set_width_grow() | set_height_fit()))
									{
										widget::begin(style::text("color") | set_width_fixed(200));
										widget::color_field(light.color, light.intensity, 0.f, 1.f);
									}
									if (auto _ = widget::horizontal(set_width_grow() | set_height_fit()))
									{
										widget::begin(style::text("range") | set_width_fixed(200));
										widget::numeric_field(light.range, nullptr, 0.f);
									}
									if (auto _ = widget::horizontal(set_width_grow() | set_height_fit()))
									{
										widget::begin(style::text("direction") | set_width_fixed(200));
										widget::numeric_field(light.direction, nullptr, float3::zero());
									}
									if (auto _ = widget::horizontal(set_width_grow() | set_height_fit()))
									{
										widget::begin(style::text("cos_inner") | set_width_fixed(200));
										widget::numeric_field(light.cos_inner, nullptr, light.cos_outer, 1.f);
									}
									if (auto _ = widget::horizontal(set_width_grow() | set_height_fit()))
									{
										widget::begin(style::text("cos_outer") | set_width_fixed(200));
										widget::numeric_field(light.cos_outer, nullptr, 0.f, light.cos_inner);
									}

									widget::separator_v();
								}
							}();

							// model
							[&] {
								if (auto _ = ui::id_begin();
									runtime::is_invalid_idx(entity_import.model_idx) is_false)
								{
									auto model_header = widget::collapsible_header(std::format("model [{}]", gltf_import_data.model_import_data_vec[entity_import.model_idx].name).data());
									if (model_header is_false) { return; }

									widget::dropdown(entity_import.model_idx, model_dropdown_option_vec);

									if (runtime::is_invalid_idx(entity_import.model_idx) is_false) { return; }

									c_auto& model = gltf_import_data.model_import_data_vec[entity_import.model_idx];

									if (auto _ = widget::horizontal(set_width_grow() | set_height_fit()))
									{
										widget::begin(style::text("mesh : ") | set_width_fixed(200));
										widget::text(runtime::is_invalid_idx(model.mesh_idx) ? "(none)" : gltf_import_data.mesh_import_data_vec[model.mesh_idx].name.data());
									}

									for (auto&& [j, mat_idx] : model.submesh_material_idx_vec | views::enumerate<uint32>)
									{
										if (auto _ = widget::horizontal(set_width_grow() | set_height_fit()))
										{
											widget::begin(style::text(std::format("material [{}] : ", j).data()) | set_width_fixed(200));
											widget::text(runtime::is_invalid_idx(mat_idx) ? "(none)" : gltf_import_data.material_import_data_vec[mat_idx].name.data());
										}
									}

									widget::separator_v();
								}
							}();

							// skeleton
							[&] {
								if (auto _ = ui::id_begin();
									runtime::is_invalid_idx(entity_import.skeleton_idx) is_false)
								{
									auto skeleton_header = widget::collapsible_header(std::format("skeleton [{}]", gltf_import_data.skeleton_import_data_vec[entity_import.skeleton_idx].name).data());
									if (skeleton_header is_false) { return; }

									widget::dropdown(entity_import.skeleton_idx, skeleton_dropdown_option_vec);

									if (runtime::is_invalid_idx(entity_import.skeleton_idx) is_false) { return; }

									c_auto& skeleton = gltf_import_data.skeleton_import_data_vec[entity_import.skeleton_idx];

									for (auto&& [nth_joint, joint_name] : skeleton.joint_name_vec | views::enumerate<uint32>)
									{
										widget::begin(style::text(std::format("joint [{}] :", nth_joint).data()) | set_width_fixed(200));
										widget::text(joint_name.data());
									}

									widget::separator_v();
								}
							}();

							// joint_attach
							[&] {
								if (auto _ = ui::id_begin();
									runtime::is_invalid_idx(entity_import.joint_attach_idx) is_false)
								{
									auto joint_attach_header = widget::collapsible_header(std::format("joint_attach [{}]", entity_import.joint_attach_idx).data());
									if (joint_attach_header is_false) { return; }
									auto& joint_attach = scene_import.joint_attach_data_vec[entity_import.joint_attach_idx];

									if (auto _ = widget::horizontal(set_width_grow() | set_height_fit()))
									{
										widget::begin(style::text("enabled") | set_width_fixed(200));
										widget::checkbox(nullptr, joint_attach.enabled);
									}

									if (joint_attach.enabled is_false) { return; }

									if (auto _ = widget::horizontal(set_width_grow() | set_height_fit()))
									{
										widget::begin(style::text("joint name") | set_width_fixed(200));
										widget::text_input(joint_attach.joint_name);
									}
									widget::separator_v();
								}
							}();
						}
					}
				}
			}

			if (gltf_import_data.error_flags == asset::importer::e::import_error_flags::none)
			{
				widget::separator_v();

				if (auto _ = widget::begin(set_horizontal() | set_width_grow() | set_height_fit());

					gltf_import_data.has_error is_false)
				{
					if (auto commit = widget::button("commit import");
						commit and commit.clicked())
					{
						c_auto commit_res = asset::importer::commit_import(gltf_import_data);

						// instantiate scene and entity

						// editor::command::add_instantiate_import_data_cmd(std::move(gltf_import_data));
					}
				}
			}
		}
	}
}	 // namespace age::editor

namespace age::editor
{
	void
	ui_modal_import_asset() noexcept
	{
		using namespace age::ui;
		using enum age::asset::e::import_kind;

		static auto selected = age::asset::e::import_kind::texture;
		if (auto _ = widget::begin(style::panel() | set_horizontal() | set_width_grow() | set_height_grow()))
		{
			if (auto _ = widget::begin(style::section() | set_vertical() | set_width_fit() | set_height_grow()))
			{
				asset::e::visit_all<asset::e::import_kind>([]<asset::e::import_kind e_kind> {
					auto asset_btn = widget::button(to_string(e_kind).data());
					if (asset_btn.clicked())
					{
						selected = e_kind;
					} });
			}

			if (auto _ = widget::begin(style::section() | set_vertical() | set_width_grow() | set_height_grow()))
			{
				switch (selected)
				{
				case texture:
				{
					ui_modal_import_asset_texture();
					break;
				}
				case mesh:
				{
					ui_modal_import_asset_mesh();
					break;
				}
				case gltf:
				{
					ui_modal_import_asset_gltf();
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
}	 // namespace age::editor
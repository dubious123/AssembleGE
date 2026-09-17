#include "age_pch.hpp"
#include "age.hpp"

namespace age::editor
{
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
		static auto desc_vec			= age::vector<asset::primitive_desc>{ asset::primitive_desc{} };
		static auto vertex_format		= age::asset::e::vertex_kind::pnt_uv1;
		static auto current_submesh_idx = 0u;

		AGE_ASSERT(desc_vec.empty() is_false);

		if (auto _ = widget::begin(set_vertical() | set_width_grow() | set_height_grow()))
		{
			if (auto _ = widget::begin(set_horizontal() | set_width_grow() | set_height_fit()))
			{
				if (auto _ = widget::begin(set_width_fixed(100) | set_height_fit() | set_align_center()))
				{
					widget::text("vertex layout");
				}

				if (auto _ = widget::begin(set_vertical() | set_width_grow() | set_height_fit() | set_padding_left(100)))
				{
					using enum age::asset::e::vertex_kind;
					widget::dropdown<asset::e::vertex_kind>(vertex_format, widget::make_dropdown_option<pnt_uv0, p_uv1, pn_uv1, pnt_uv1>());
				}
			}

			widget::separator_v();

			if (auto _ = widget::begin(set_horizontal_inv() | set_width_grow() | set_height_fit()))
			{
				if (auto _ = widget::begin(set_horizontal() | set_width_fixed(200) | set_height_fit()))
				{
					if (auto btn = widget::button("+"))
					{
						if (btn.clicked())
						{
							desc_vec.emplace_back(asset::primitive_desc{});
						}
					}
					if (auto btn = widget::button("-"))
					{
						if (btn.clicked() and desc_vec.size<uint32>() > 1)
						{
							desc_vec.pop_back();
						}
					}
				}

				if (auto _ = widget::begin(set_width_grow() | set_height_fit()))
				{
					detail::ui_component_index_dropdown(current_submesh_idx, desc_vec.size<uint32>());
				}

				widget::begin(set_width_fixed(200) | set_height_fit());
			}

			widget::separator_v();

			current_submesh_idx = min(desc_vec.size<uint32>() - 1u, current_submesh_idx);
			{
				auto& desc = desc_vec[current_submesh_idx];

				if (auto _ = widget::begin(set_horizontal() | set_width_grow() | set_height_fit() | set_align_center()))
				{
					widget::text_heading(std::format("submesh {}", current_submesh_idx).data());
					if (auto btn = widget::button("duplicate"))
					{
						if (btn.clicked())
						{
							// self reference: safe, emplace_back constructs before relocation
							desc_vec.emplace_back(desc);
						}
					}
					if (auto btn = widget::button("    erase    "))
					{
						if (btn.clicked() and desc_vec.size<uint32>() > 1u)
						{
							ranges::erase_at(desc_vec, current_submesh_idx);
						}
					}
				}
			}

			current_submesh_idx = min(desc_vec.size<uint32>() - 1u, current_submesh_idx);
			auto& desc			= desc_vec[current_submesh_idx];

			widget::separator_v();
			if (auto _ = widget::begin(set_horizontal() | set_width_grow() | set_height_fit()))
			{
				if (auto _ = widget::begin(set_width_fixed(100) | set_height_fit() | set_align_center()))
				{
					widget::text("pos");
				}

				if (auto _ = widget::begin(set_width_grow() | set_height_fit() | set_padding_left(100)))
				{
					widget::numeric_field(desc.pos);
				}
			}

			if (auto _ = widget::begin(set_horizontal() | set_width_grow() | set_height_fit()))
			{
				if (auto _ = widget::begin(set_width_fixed(100) | set_height_fit() | set_align_center()))
				{
					widget::text("size");
				}

				if (auto _ = widget::begin(set_width_grow() | set_height_fit() | set_padding_left(100)))
				{
					widget::numeric_field(desc.size);
				}
			}

			if (auto _ = widget::begin(set_horizontal() | set_width_grow() | set_height_fit()))
			{
				if (auto _ = widget::begin(set_width_fixed(100) | set_height_fit() | set_align_center()))
				{
					widget::text("seg_uv");
				}

				if (auto _ = widget::begin(set_width_grow() | set_height_fit() | set_padding_left(100)))
				{
					auto seg_uv = vec2<uint32>{ desc.seg_u, desc.seg_v };
					widget::numeric_field(seg_uv);
					desc.seg_u = seg_uv.x;
					desc.seg_v = seg_uv.y;
				}
			}

			if (auto _ = widget::begin(set_horizontal() | set_width_grow() | set_height_fit()))
			{
				if (auto _ = widget::begin(set_width_fixed(100) | set_height_fit() | set_align_center()))
				{
					widget::text("mesh_kind");
				}

				if (auto _ = widget::begin(set_vertical() | set_width_grow() | set_height_fit() | set_padding_left(100)))
				{
					using enum age::asset::e::primitive_mesh_kind;
					widget::dropdown<asset::e::primitive_mesh_kind>(desc.mesh_kind, widget::make_dropdown_option<cube, plane, cube_sphere, disk, cone>());
				}
			}

			if (auto _ = widget::begin(set_horizontal() | set_width_grow() | set_height_fit()))
			{
				if (auto _ = widget::begin(set_width_fixed(100) | set_height_fit() | set_align_center()))
				{
					widget::text("raster mode");
				}

				if (auto _ = widget::begin(set_vertical() | set_width_grow() | set_height_fit() | set_padding_left(100)))
				{
					widget::dropdown(desc.raster_mode);
				}
			}

			if (auto _ = widget::begin(set_horizontal() | set_width_grow() | set_height_fit()))
			{
				if (auto _ = widget::begin(set_width_fixed(100) | set_height_fit() | set_align_center()))
				{
					widget::text("rt bake mode");
				}

				if (auto _ = widget::begin(set_vertical() | set_width_grow() | set_height_fit() | set_padding_left(100)))
				{
					widget::dropdown(desc.rt_bake_mode);
				}
			}

			if (auto _ = widget::begin(set_horizontal() | set_width_grow() | set_height_fit()))
			{
				if (auto _ = widget::begin(set_width_fixed(100) | set_height_fit() | set_align_center()))
				{
					widget::text("rt alpha test");
				}

				if (auto _ = widget::begin(set_vertical() | set_width_grow() | set_height_fit() | set_padding_left(100)))
				{
					widget::dropdown(desc.rt_alpha_test_mode);
				}
			}

			widget::separator_v();

			if (auto _ = widget::begin(set_horizontal() | set_align_center()))
			{
				if (auto _ = widget::begin(set_width_fixed(100 + 100) | set_height_fit()))
				{
					widget::text_input(detail::ui_modal_asset_name_input_scratch_buf());
				}

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

						c_auto mesh_name = fs::join("asset/mesh", detail::ui_modal_asset_name_input_scratch_buf().data());

						auto h_mesh = asset::mesh_baked::cpu_load(mesh_name, desc_vec, vertex_format);
						asset::mesh_baked::cpu_unload(h_mesh);
						asset::registry::register_asset(h_mesh);
					}
				}
			}
		}
	}

	void
	ui_modal_new_asset_texture() noexcept
	{
		using namespace age::ui;
		using enum age::asset::e::kind;

		static auto tex_desc = asset::texture_bake_option{};
		static auto src_vec	 = age::vector<age::array<char, config::max_asset_display_name_len>>{};

		auto		is_valid	 = true;
		static auto bake_success = true;

		if (auto _ = widget::begin(set_vertical() | set_width_grow() | set_height_grow()))
		{
			auto scoll = widget::scroll_area_v();
			if (auto _ = widget::begin(set_horizontal() | set_width_grow() | set_height_fit()))
			{
				if (auto _ = widget::begin(set_width_fixed(200) | set_height_fit() | set_align_center()))
				{
					widget::text("format");
				}

				if (auto _ = widget::begin(set_vertical() | set_width_grow() | set_height_fit()))
				{
					widget::dropdown<graphics::e::texture_format>(tex_desc.format, widget::make_dropdown_option_all<graphics::e::texture_format>());
				}
			}

			widget::separator_v();

			if (auto _ = widget::begin(set_horizontal() | set_width_grow() | set_height_fit()))
			{
				if (auto _ = widget::begin(set_width_fixed(200) | set_height_fit() | set_align_center()))
				{
					widget::text("is_cube");
				}

				if (auto _ = widget::begin(set_vertical() | set_width_grow() | set_height_fit()))
				{
					widget::checkbox(nullptr, tex_desc.is_cube);
					if (tex_desc.is_cube) { tex_desc.is_3d = false; }
				}
			}

			if (auto _ = widget::begin(set_horizontal() | set_width_grow() | set_height_fit()))
			{
				if (auto _ = widget::begin(set_width_fixed(200) | set_height_fit() | set_align_center()))
				{
					widget::text("is_3d");
				}

				if (auto _ = widget::begin(set_vertical() | set_width_grow() | set_height_fit()))
				{
					widget::checkbox(nullptr, tex_desc.is_3d);
					if (tex_desc.is_3d) { tex_desc.is_cube = false; }
				}
			}

			widget::separator_v();

			if (auto _ = widget::begin(set_horizontal() | set_width_grow() | set_height_fit()))
			{
				if (auto _ = widget::begin(set_width_fixed(200) | set_height_fit() | set_align_center()))
				{
					if (tex_desc.is_3d)
					{
						widget::text("depth count");
					}
					else
					{
						widget::text("array count");
					}
				}

				if (auto _ = widget::begin(set_vertical() | set_width_grow() | set_height_fit()))
				{
					widget::numeric_field(tex_desc.array_or_depth_count);
				}
			}


			if (auto _ = widget::begin(set_horizontal() | set_width_grow() | set_height_fit()))
			{
				if (auto _ = widget::begin(set_width_fixed(200) | set_height_fit() | set_align_center()))
				{
					widget::text("extent");
				}

				if (auto _ = widget::begin(set_vertical() | set_width_grow() | set_height_fit()))
				{
					auto extent = vec2<uint32>{ tex_desc.width, tex_desc.height };
					widget::numeric_field(extent);
					tex_desc.width = extent.x, tex_desc.height = extent.y;
				}
			}

			if (auto _ = widget::begin(set_horizontal() | set_width_grow() | set_height_fit()))
			{
				if (auto _ = widget::begin(set_width_fixed(200) | set_height_fit() | set_align_center()))
				{
					widget::text("fit pow2 (0=source)");
				}

				if (auto _ = widget::begin(set_vertical() | set_width_grow() | set_height_fit()))
				{
					widget::checkbox(nullptr, tex_desc.fit_pow2);
				}
			}

			widget::separator_v();

			if (auto _ = widget::begin(set_horizontal() | set_width_grow() | set_height_fit()))
			{
				if (auto _ = widget::begin(set_width_fixed(200) | set_height_fit() | set_align_center()))
				{
					widget::text("mip count (0=full)");
				}

				if (auto _ = widget::begin(set_vertical() | set_width_grow() | set_height_fit()))
				{
					widget::numeric_field(tex_desc.mip_count);
				}
			}

			if (auto _ = widget::begin(set_horizontal() | set_width_grow() | set_height_fit()))
			{
				if (auto _ = widget::begin(set_width_fixed(200) | set_height_fit() | set_align_center()))
				{
					widget::text("mip filter");
				}

				if (auto _ = widget::begin(set_vertical() | set_width_grow() | set_height_fit()))
				{
					widget::dropdown<asset::e::mip_filter_kind>(tex_desc.filter, widget::make_dropdown_option_all<asset::e::mip_filter_kind>());
				}
			}

			if (auto _ = widget::begin(set_horizontal() | set_width_grow() | set_height_fit()))
			{
				if (auto _ = widget::begin(set_width_fixed(200) | set_height_fit() | set_align_center()))
				{
					widget::text("wrap mode");
				}

				if (auto _ = widget::begin(set_vertical() | set_width_grow() | set_height_fit()))
				{
					widget::dropdown<asset::e::wrap_mode_kind>(tex_desc.wrap, widget::make_dropdown_option_all<asset::e::wrap_mode_kind>());
				}
			}

			widget::separator_v();

			if (auto _ = widget::begin(set_horizontal() | set_width_grow() | set_height_fit()))
			{
				if (auto _ = widget::begin(set_width_fixed(200) | set_height_fit() | set_align_center()))
				{
					widget::text("h flip");
				}

				if (auto _ = widget::begin(set_vertical() | set_width_grow() | set_height_fit()))
				{
					widget::checkbox(nullptr, tex_desc.hflip);
				}
			}

			if (auto _ = widget::begin(set_horizontal() | set_width_grow() | set_height_fit()))
			{
				if (auto _ = widget::begin(set_width_fixed(200) | set_height_fit() | set_align_center()))
				{
					widget::text("v flip");
				}

				if (auto _ = widget::begin(set_vertical() | set_width_grow() | set_height_fit()))
				{
					widget::checkbox(nullptr, tex_desc.vflip);
				}
			}

			if (auto _ = widget::begin(set_horizontal() | set_width_grow() | set_height_fit()))
			{
				if (auto _ = widget::begin(set_width_fixed(200) | set_height_fit() | set_align_center()))
				{
					widget::text("invert y");
				}

				if (auto _ = widget::begin(set_vertical() | set_width_grow() | set_height_fit()))
				{
					widget::checkbox(nullptr, tex_desc.invert_y);
				}
			}

			widget::separator_v();

			if (auto _ = widget::begin(set_horizontal() | set_width_grow() | set_height_fit()))
			{
				if (auto _ = widget::begin(set_width_fixed(200) | set_height_fit() | set_align_center()))
				{
					widget::text("separate alpha");
				}

				if (auto _ = widget::begin(set_vertical() | set_width_grow() | set_height_fit()))
				{
					widget::checkbox(nullptr, tex_desc.separate_alpha);
				}
			}

			if (auto _ = widget::begin(set_horizontal() | set_width_grow() | set_height_fit()))
			{
				if (auto _ = widget::begin(set_width_fixed(200) | set_height_fit() | set_align_center()))
				{
					widget::text("alpha threshold (-1 = unset, used for bc1)");
				}

				if (auto _ = widget::begin(set_vertical() | set_width_grow() | set_height_fit()))
				{
					widget::numeric_field(tex_desc.alpha_threshold);
				}
			}

			if (auto _ = widget::begin(set_horizontal() | set_width_grow() | set_height_fit()))
			{
				if (auto _ = widget::begin(set_width_fixed(200) | set_height_fit() | set_align_center()))
				{
					widget::text("keep_coverage, -1 = unset");
				}

				if (auto _ = widget::begin(set_vertical() | set_width_grow() | set_height_fit()))
				{
					widget::numeric_field(tex_desc.keep_coverage);
				}
			}

			widget::separator_v();


			if (auto _ = widget::begin(set_width_fit() | set_height_fit() | set_align_center()))
			{
				widget::text_heading("src images");
			}

			if (auto _ = widget::begin(set_vertical() | set_width_grow() | set_height_fit()))
			{
				if (c_auto new_size = tex_desc.array_or_depth_count * (tex_desc.is_cube ? 6 : 1);
					src_vec.size() < new_size)
				{
					src_vec.reserve(new_size);
					for (auto i : views::loop(new_size))
					{
						src_vec.emplace_back();
					}
				}

				src_vec.resize(tex_desc.array_or_depth_count * (tex_desc.is_cube ? 6 : 1));

				if (tex_desc.is_cube)
				{
					for (auto i : views::loop(tex_desc.array_or_depth_count))
					{
						auto panel = widget::begin(style::panel() | set_height_fit());
						for (auto j : views::loop(6))
						{
							auto id = ui::id_begin();
							auto _	= widget::begin(set_horizontal() | set_height_fit() | set_width_grow());
							widget::path_picker(src_vec[i * 6 + j]);
							if (std::filesystem::is_regular_file(src_vec[i * 6 + j].data()) is_false)
							{
								is_valid = false;

								widget::begin(style::text("path does not exists") | set_body_brush_data(theme::color_text_red()));
							}
						}
					}
				}
				else
				{
					for (auto i : views::loop(tex_desc.array_or_depth_count))
					{
						auto id = ui::id_begin();
						auto _	= widget::begin(set_horizontal() | set_height_fit() | set_width_grow());
						widget::path_picker(src_vec[i]);
						if (std::filesystem::is_regular_file(src_vec[i].data()) is_false)
						{
							is_valid = false;

							widget::begin(style::text("path does not exists") | set_body_brush_data(theme::color_text_red()));
						}
					}
				}
			}

			widget::separator_v();

			if (is_valid is_false)
			{
				widget::begin(style::text("invalid option") | set_body_brush_data(theme::color_text_red()));
				return;
			}

			if (auto _ = widget::begin(set_horizontal() | set_height_fit() | set_align_center()))
			{
				if (auto _ = widget::begin(set_width_fixed(100 + 100) | set_height_fit()))
				{
					widget::text_input(detail::ui_modal_asset_name_input_scratch_buf());
				}

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
						// g::show_modal	 = false;
						c_auto name		 = fs::join(std::format("asset/{}", to_string(asset::e::kind::texture)), detail::ui_modal_asset_name_input_scratch_buf().data());
						c_auto full_path = asset::get_asset_full_path<texture>(name);

						auto src = age::vector<const char*>::gen_reserved(src_vec.size());

						for (auto& s : src_vec)
						{
							src.emplace_back(s.data());
						}

						bake_success = asset::texture::bake(std::span<const char* const>(src), full_path.data(), tex_desc);
						if (bake_success)
						{
							asset::registry::register_asset(texture, full_path.data());
							g::show_modal = false;
						}
					}
				}
			}
		}

		if (bake_success is_false)
		{
			widget::begin(style::text("texture bake failed") | set_body_brush_data(theme::color_text_red()));
		}
	}

	void
	ui_modal_new_asset_material() noexcept
	{
		using namespace age::ui;
		using enum age::asset::e::kind;

		static auto mat_desc = asset::material_desc{};

		if (asset::registry::is_registered(mat_desc.h_tex_base_color) is_false)
		{
			mat_desc.h_tex_base_color = {};
		}
		if (asset::registry::is_registered(mat_desc.h_tex_metallic_roughness) is_false)
		{
			mat_desc.h_tex_metallic_roughness = {};
		}
		if (asset::registry::is_registered(mat_desc.h_tex_normal) is_false)
		{
			mat_desc.h_tex_normal = {};
		}
		if (asset::registry::is_registered(mat_desc.h_tex_occlusion) is_false)
		{
			mat_desc.h_tex_occlusion = {};
		}
		if (asset::registry::is_registered(mat_desc.h_tex_emissive) is_false)
		{
			mat_desc.h_tex_emissive = {};
		}

		if (auto _ = widget::begin(set_vertical() | set_width_grow() | set_height_grow()))
		{
			if (auto _ = widget::begin(set_horizontal() | set_width_grow() | set_height_fit()))
			{
				if (auto _ = widget::begin(set_width_fixed(200) | set_height_fit() | set_align_center()))
				{
					widget::text("Base color");
				}

				if (auto _ = widget::begin(set_vertical() | set_width_grow() | set_height_fit()))
				{
					widget::color_field(mat_desc.base_color_factor);
					detail::ui_component_asset_dropdown<texture>(mat_desc.h_tex_base_color, true);
					widget::dropdown<graphics::e::sampler_kind>(mat_desc.base_color_sampler_kind, widget::make_dropdown_option_all<graphics::e::sampler_kind>());
				}
			}

			widget::separator_v();

			if (auto _ = widget::begin(set_horizontal() | set_width_grow() | set_height_fit()))
			{
				if (auto _ = widget::begin(set_width_fixed(200) | set_height_fit() | set_align_center()))
				{
					widget::text("Matallic");
				}

				if (auto _ = widget::begin(set_vertical() | set_width_grow() | set_height_fit()))
				{
					widget::numeric_field(mat_desc.metallic_factor, nullptr, 0.f, 1.f);
					widget::slider(mat_desc.metallic_factor, 0.f, 1.f);
				}
			}

			if (auto _ = widget::begin(set_horizontal() | set_width_grow() | set_height_fit()))
			{
				if (auto _ = widget::begin(set_width_fixed(200) | set_height_fit() | set_align_center()))
				{
					widget::text("Roughness");
				}

				if (auto _ = widget::begin(set_vertical() | set_width_grow() | set_height_fit()))
				{
					widget::numeric_field(mat_desc.roughness_factor, nullptr, 0.f, 1.f);
					widget::slider(mat_desc.roughness_factor, 0.f, 1.f);
				}
			}


			if (auto _ = widget::begin(set_horizontal() | set_width_grow() | set_height_fit()))
			{
				if (auto _ = widget::begin(set_width_fixed(200) | set_height_fit() | set_align_center()))
				{
					widget::text("MR Texture");
				}

				if (auto _ = widget::begin(set_vertical() | set_width_grow() | set_height_fit()))
				{
					detail::ui_component_asset_dropdown<texture>(mat_desc.h_tex_metallic_roughness, false);
					widget::dropdown<graphics::e::sampler_kind>(mat_desc.metallic_roughness_sampler_kind, widget::make_dropdown_option_all<graphics::e::sampler_kind>());
				}
			}

			widget::separator_v();

			if (auto _ = widget::begin(set_horizontal() | set_width_grow() | set_height_fit()))
			{
				if (auto _ = widget::begin(set_width_fixed(200) | set_height_fit() | set_align_center()))
				{
					widget::text("Normal");
				}

				if (auto _ = widget::begin(set_vertical() | set_width_grow() | set_height_fit()))
				{
					widget::numeric_field(mat_desc.normal_scale, nullptr, 0.f, 2.f);
					widget::slider(mat_desc.normal_scale, 0.f, 2.f);
					detail::ui_component_asset_dropdown<texture>(mat_desc.h_tex_normal, false);
					widget::dropdown<graphics::e::sampler_kind>(mat_desc.normal_sampler_kind, widget::make_dropdown_option_all<graphics::e::sampler_kind>());
				}
			}

			widget::separator_v();

			if (auto _ = widget::begin(set_horizontal() | set_width_grow() | set_height_fit()))
			{
				if (auto _ = widget::begin(set_width_fixed(200) | set_height_fit() | set_align_center()))
				{
					widget::text("Occlusion");
				}

				if (auto _ = widget::begin(set_vertical() | set_width_grow() | set_height_fit()))
				{
					widget::numeric_field(mat_desc.occlusion_strength, nullptr, 0.f, 1.f);
					widget::slider(mat_desc.occlusion_strength, 0.f, 1.f);
					detail::ui_component_asset_dropdown<texture>(mat_desc.h_tex_occlusion, false);
					widget::dropdown<graphics::e::sampler_kind>(mat_desc.occlusion_sampler_kind, widget::make_dropdown_option_all<graphics::e::sampler_kind>());
				}
			}

			widget::separator_v();

			if (auto _ = widget::begin(set_horizontal() | set_width_grow() | set_height_fit()))
			{
				if (auto _ = widget::begin(set_width_fixed(200) | set_height_fit() | set_align_center()))
				{
					widget::text("Emissive");
				}

				if (auto _ = widget::begin(set_vertical() | set_width_grow() | set_height_fit()))
				{
					widget::color_field(mat_desc.emissive_factor);
					detail::ui_component_asset_dropdown<texture>(mat_desc.h_tex_emissive, false);
					widget::dropdown<graphics::e::sampler_kind>(mat_desc.emissive_sampler_kind, widget::make_dropdown_option_all<graphics::e::sampler_kind>());
				}
			}

			widget::separator_v();

			if (auto _ = widget::begin(set_horizontal() | set_width_grow() | set_height_fit()))
			{
				if (auto _ = widget::begin(set_width_fixed(200) | set_height_fit() | set_align_center()))
				{
					widget::text("Shading Model");
				}

				if (auto _ = widget::begin(set_vertical() | set_width_grow() | set_height_fit()))
				{
					widget::dropdown<graphics::e::material_shading_model_kind>(mat_desc.shading_model, widget::make_dropdown_option_all<graphics::e::material_shading_model_kind>());
				}
			}

			widget::separator_v();

			if (auto _ = widget::begin(set_horizontal() | set_width_grow() | set_height_fit()))
			{
				if (auto _ = widget::begin(set_width_fixed(200) | set_height_fit() | set_align_center()))
				{
					widget::text("Double Sided");
				}

				if (auto _ = widget::begin(set_vertical() | set_width_grow() | set_height_fit()))
				{
					widget::checkbox(nullptr, mat_desc.double_sided);
				}
			}

			widget::separator_v();

			if (auto _ = widget::begin(set_horizontal() | set_align_center()))
			{
				if (auto _ = widget::begin(set_width_fixed(100 + 100) | set_height_fit()))
				{
					widget::text_input(detail::ui_modal_asset_name_input_scratch_buf());
				}

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
						g::show_modal	 = false;
						c_auto name		 = fs::join(std::format("asset/{}", to_string(asset::e::kind::material)), detail::ui_modal_asset_name_input_scratch_buf().data());
						c_auto full_path = asset::get_asset_full_path<material>(name);

						asset::material::build(full_path.data(), mat_desc);
						asset::registry::register_asset(material, full_path.data());
					}
				}
			}
		}
	}

	void
	ui_modal_new_asset_env_light() noexcept
	{
		using namespace age::ui;
		using enum age::asset::e::kind;

		static auto asset_desc = asset::env_light_desc{};
		static auto src_path   = age::array<char, config::max_asset_path_len>{};

		auto		is_valid	 = true;
		static auto bake_success = true;

		if (auto _ = widget::begin(set_vertical() | set_width_grow() | set_height_grow()))
		{
			auto h_scoll = widget::scroll_area_v();
			if (auto _ = widget::begin(set_horizontal() | set_width_grow() | set_height_fit()))
			{
				if (auto _ = widget::begin(set_width_fixed(200) | set_height_fit() | set_align_center()))
				{
					widget::text("cubemap format");
				}

				if (auto _ = widget::begin(set_vertical() | set_width_grow() | set_height_fit()))
				{
					widget::dropdown<graphics::e::texture_format>(asset_desc.format, widget::make_dropdown_option_all<graphics::e::texture_format>());
				}
			}

			widget::separator_v();

			if (auto _ = widget::begin(set_horizontal() | set_width_grow() | set_height_fit()))
			{
				if (auto _ = widget::begin(set_width_fixed(200) | set_height_fit() | set_align_center()))
				{
					widget::text("cubemap size");
				}

				if (auto _ = widget::begin(set_vertical() | set_width_grow() | set_height_fit()))
				{
					widget::numeric_field(asset_desc.cubemap_size, nullptr);
					if (util::popcount(asset_desc.cubemap_size) != 1)
					{
						widget::begin(style::text("cubemap size must be power of 2") | set_body_brush_data(theme::color_text_red()));
					}
				}
			}

			if (auto _ = widget::begin(set_horizontal() | set_width_grow() | set_height_fit()))
			{
				if (auto _ = widget::begin(set_width_fixed(200) | set_height_fit() | set_align_center()))
				{
					widget::text("prefilter size");
				}

				if (auto _ = widget::begin(set_vertical() | set_width_grow() | set_height_fit()))
				{
					widget::numeric_field(asset_desc.prefilter_size);
				}
			}

			if (auto _ = widget::begin(set_horizontal() | set_width_grow() | set_height_fit()))
			{
				if (auto _ = widget::begin(set_width_fixed(200) | set_height_fit() | set_align_center()))
				{
					widget::text("prefilter mip count");
				}

				if (auto _ = widget::begin(set_vertical() | set_width_grow() | set_height_fit()))
				{
					widget::numeric_field(asset_desc.prefilter_mip_count, nullptr, uint16{}, static_cast<uint16>(util::popcount(asset_desc.prefilter_size) - 1));
				}
			}

			widget::separator_v();

			if (auto _ = widget::begin(set_horizontal() | set_width_grow() | set_height_fit()))
			{
				if (auto _ = widget::begin(set_width_fixed(200) | set_height_fit() | set_align_center()))
				{
					widget::text("irradiance size");
				}

				if (auto _ = widget::begin(set_vertical() | set_width_grow() | set_height_fit()))
				{
					widget::numeric_field(asset_desc.irradiance_size);
					if (util::popcount(asset_desc.irradiance_size) != 1)
					{
						widget::begin(style::text("irradiance_size size must be power of 2") | set_body_brush_data(theme::color_text_red()));
					}
				}
			}

			if (auto _ = widget::begin(set_horizontal() | set_width_grow() | set_height_fit()))
			{
				if (auto _ = widget::begin(set_width_fixed(200) | set_height_fit() | set_align_center()))
				{
					widget::text("invert y");
				}

				if (auto _ = widget::begin(set_vertical() | set_width_grow() | set_height_fit()))
				{
					widget::checkbox(nullptr, asset_desc.invert_y);
				}
			}

			widget::separator_v();

			if (auto _ = widget::begin(set_width_fit() | set_height_fit() | set_align_center()))
			{
				widget::text_heading("src image");
			}

			if (auto _ = widget::begin(set_vertical() | set_width_grow() | set_height_fit()))
			{
				widget::path_picker(src_path);

				if (std::filesystem::is_regular_file(src_path.data()) is_false)
				{
					is_valid = false;

					widget::begin(style::text("path does not exists") | set_body_brush_data(theme::color_text_red()));
				}
			}

			widget::separator_v();

			if (is_valid is_false)
			{
				widget::begin(style::text("invalid option") | set_body_brush_data(theme::color_text_red()));
				return;
			}

			if (auto _ = widget::begin(set_horizontal() | set_height_fit() | set_align_center()))
			{
				if (auto _ = widget::begin(set_width_fixed(100 + 100) | set_height_fit()))
				{
					widget::text_input(detail::ui_modal_asset_name_input_scratch_buf());
				}

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
						c_auto name		 = fs::join(std::format("asset/{}", to_string(asset::e::kind::env_light)), detail::ui_modal_asset_name_input_scratch_buf().data());
						c_auto full_path = asset::get_asset_full_path<env_light>(name);

						if (bake_success = asset::env_light::bake(src_path, full_path, asset_desc))
						{
							asset::registry::register_asset(env_light, full_path.data());
							g::show_modal = false;
						}
					}
				}
			}
		}

		if (bake_success is_false)
		{
			widget::begin(style::text("env light bake failed") | set_body_brush_data(theme::color_text_red()));
		}
	}

	void
	ui_modal_new_asset_model() noexcept
	{
		using namespace age::ui;
		using enum age::asset::e::kind;

		static auto model_desc = asset::model_desc{};

		// handle asset destroy while creating new model
		if (asset::registry::is_registered(model_desc.h_mesh) is_false)
		{
			model_desc.h_mesh = {};
		}
		for (auto& h_mat : model_desc.h_materials)
		{
			if (asset::registry::is_registered(h_mat) is_false)
			{
				h_mat = {};
			}
		}

		auto submesh_count = 0;

		if (runtime::is_handle_invalid(model_desc.h_mesh) is_false)
		{
			auto& entry = model_desc.h_mesh.get_entry<mesh_baked>();

			if (entry.is_cpu_loaded() is_false)
			{
				asset::mesh_baked::cpu_load(model_desc.h_mesh);
				asset_mgr::add_asset_pin(asset::e::kind::mesh_baked, model_desc.h_mesh, 10);
			}

			submesh_count = model_desc.h_mesh.get_entry<mesh_baked>().submesh_count();
		}

		if (auto _ = widget::begin(set_vertical() | set_width_grow() | set_height_grow()))
		{
			if (auto _ = widget::begin(set_horizontal() | set_width_grow() | set_height_fit()))
			{
				if (auto _ = widget::begin(set_width_fixed(200) | set_height_fit() | set_align_center()))
				{
					widget::text("Mesh");
				}

				if (auto _ = widget::begin(set_vertical() | set_width_grow() | set_height_fit()))
				{
					widget::text(std::format("submesh_count : {}", submesh_count).data());
					detail::ui_component_asset_dropdown<mesh_baked>(model_desc.h_mesh);
				}
			}

			widget::separator_v();

			if (auto _ = widget::begin(set_horizontal() | set_width_grow() | set_height_fit()))
			{
				if (auto _ = widget::begin(set_width_fixed(200) | set_height_fit() | set_align_center()))
				{
					widget::text("Material");
				}

				if (auto _ = widget::begin(set_vertical() | set_width_grow() | set_height_fit()))
				{
					if (auto _ = widget::begin(set_horizontal() | set_width_grow() | set_height_fit()))
					{
						if (auto btn = widget::button("+"))
						{
							if (btn.clicked())
							{
								model_desc.h_materials.emplace_back(asset::handle{});
							}
						}

						if (auto btn = widget::button("-"))
						{
							if (btn.clicked())
							{
								model_desc.h_materials.pop_back();
							}
						}
					}


					{
						auto _id = ui::id_begin();

						if (submesh_count < model_desc.h_materials.size())
						{
							widget::begin(style::text("submesh_count < material_count, extra materials will be ignored") | set_body_brush_color(ui::theme::color_text_red()));
						}
						else if (submesh_count > model_desc.h_materials.size())
						{
							widget::begin(style::text("submesh_count > material_count, unassigned submeshes will fall back to the error material") | set_body_brush_color(ui::theme::color_text_red()));
						}
					}


					for (auto& h_mat : model_desc.h_materials)
					{
						auto _id = ui::id_begin();
						detail::ui_component_asset_dropdown<material>(h_mat);
					}
				}
			}

			widget::separator_v();

			if (auto _ = widget::begin(set_horizontal() | set_align_center()))
			{
				if (auto _ = widget::begin(set_width_fixed(100 + 100) | set_height_fit()))
				{
					widget::text_input(detail::ui_modal_asset_name_input_scratch_buf());
				}

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
						g::show_modal	 = false;
						c_auto name		 = fs::join(std::format("asset/{}", to_string(asset::e::kind::model)), detail::ui_modal_asset_name_input_scratch_buf().data());
						c_auto full_path = asset::get_asset_full_path<model>(name);

						asset::model::build(full_path.data(), model_desc);
						asset::registry::register_asset(model, full_path.data());
					}
				}
			}
		}
	}
}	 // namespace age::editor

namespace age::editor
{
	void
	ui_modal_new_asset() noexcept
	{
		using namespace age::ui;
		using enum age::asset::e::kind;

		static auto selected = mesh_baked;
		if (auto _ = widget::begin(style::panel() | set_horizontal() | set_width_grow() | set_height_grow()))
		{
			if (auto _ = widget::begin(style::section() | set_vertical() | set_width_fit() | set_height_grow()))
			{
				asset::for_each_kind(AGE_LAMBDA(
					<asset::e::kind e_kind>,
					{
						auto asset_btn = widget::button(asset::e::to_string(e_kind).data());
						if (asset_btn.clicked())
						{
							selected = e_kind;
						}
					}));
			}

			if (auto _ = widget::begin(style::section() | set_vertical() | set_width_grow() | set_height_grow()))
			{
				switch (selected)
				{
				case font:
				{
					ui_modal_new_asset_font();
					break;
				}
				case mesh_baked:
				{
					ui_modal_new_asset_mesh_baked();
					break;
				}
				case texture:
				{
					ui_modal_new_asset_texture();
					break;
				}
				case material:
				{
					ui_modal_new_asset_material();
					break;
				}
				case env_light:
				{
					ui_modal_new_asset_env_light();
					break;
				}
				case model:
				{
					ui_modal_new_asset_model();
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
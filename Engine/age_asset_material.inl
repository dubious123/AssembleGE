#pragma once
#include "age.hpp"

namespace age::asset::detail
{
	void
	handle_texture_unload(handle h_tex, auto& renderer) noexcept
	{
		if (runtime::is_handle_invalid(h_tex) is_false)
		{
			texture::remove_ref(h_tex);

			auto& entry = h_tex.get_entry<e::kind::texture>();

			if (entry.ref_counter == 0)
			{
				texture::full_unload(h_tex, renderer);
			}
		}
	}

	void
	handle_texture_load(asset::handle h_tex, auto& renderer) noexcept
	{
		if (runtime::is_handle_invalid(h_tex)) { return; }

		texture::add_ref(h_tex);
		texture::gpu_load(h_tex, renderer);
	}

	handle
	handle_texture_load(const age::array<char, config::max_asset_path_len>& full_path, auto& renderer) noexcept
	{
		c_auto h_tex = asset::find(e::kind::texture, full_path);

		if (runtime::is_handle_invalid(h_tex)) { return {}; }

		texture::add_ref(h_tex);
		texture::gpu_load(h_tex, renderer);

		return h_tex;
	}
}	 // namespace age::asset::detail

namespace age::asset::material
{
	void
	full_unload(handle h_mat, auto& renderer) noexcept
	{
		auto& entry = h_mat.get_entry<e::kind::material>();
		if (entry.is_loaded())
		{
			for (auto& h_tex : entry.all_textures() | views::deref)
			{
				detail::handle_texture_unload(h_tex, renderer);
			}

			renderer.release_material(entry.render_id);
		}

		AGE_ASSERT(entry.is_loaded() is_false);
	}

	void
	load(handle h_mat, auto& renderer) noexcept
	{
		auto& entry = h_mat.get_entry<e::kind::material>();

		if (entry.is_loaded())
		{
			return;
		}

		if (auto file_data = asset::read_asset_file(entry.get_path());
			file_data.is_valid())
		{
			switch (file_data.header.asset_version)
			{
			case 0:
			{
				auto alpha_mode = uint8{};
				file_data.buf.read(
					entry.base_color_factor,
					entry.metallic_factor,
					entry.roughness_factor,
					entry.emissive_factor,
					entry.normal_scale,
					entry.occlusion_strength,
					entry.alpha_cutoff,
					alpha_mode);
				entry.double_sided					  = false;
				entry.shading_model					  = graphics::e::material_shading_model_kind::pbr_default;
				entry.base_color_sampler_kind		  = graphics::e::sampler_kind::linear_wrap;
				entry.metallic_roughness_sampler_kind = graphics::e::sampler_kind::linear_wrap;
				entry.normal_sampler_kind			  = graphics::e::sampler_kind::linear_wrap;
				entry.occlusion_sampler_kind		  = graphics::e::sampler_kind::linear_wrap;
				entry.emissive_sampler_kind			  = graphics::e::sampler_kind::linear_wrap;

				break;
			}
			case config::material_asset_version:
			{
				file_data.buf.read(
					entry.double_sided,
					entry.base_color_factor,
					entry.metallic_factor,
					entry.roughness_factor,
					entry.emissive_factor,
					entry.normal_scale,
					entry.occlusion_strength,
					entry.alpha_cutoff,
					entry.shading_model,
					entry.base_color_sampler_kind,
					entry.metallic_roughness_sampler_kind,
					entry.normal_sampler_kind,
					entry.occlusion_sampler_kind,
					entry.emissive_sampler_kind);
				break;
			}
			default:
			{
				AGE_ASSERT(false);
				return;
			}
			}

			for (auto& h_tex : entry.all_textures() | views::deref)
			{
				h_tex = detail::handle_texture_load(file_data.buf.read<age::array<char, config::max_asset_path_len>>(), renderer);
			}
		}

		entry.render_id = renderer.upload_material(h_mat);
	}

	handle
	load(std::string_view mat_name, auto& renderer) noexcept
	{
		c_auto h_asset = detail::load_common<e::kind::material>(mat_name);

		load(h_asset, renderer);

		return h_asset;
	}
}	 // namespace age::asset::material
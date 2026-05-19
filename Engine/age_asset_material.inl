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
	handle_texture_load(const std::array<char, config::max_asset_path_len>& full_path, auto& renderer) noexcept
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

		if (auto buf = asset::read_asset_file(entry.get_path());
			buf.empty() is_false)
		{
			buf.read(
				entry.base_color_factor,
				entry.metallic_factor,
				entry.roughness_factor,
				entry.emissive_factor,
				entry.normal_scale,
				entry.occlusion_strength,
				entry.alpha_cutoff,
				entry.alpha_mode);

			for (auto& h_tex : entry.all_textures() | views::deref)
			{
				h_tex = detail::handle_texture_load(buf.read<std::array<char, config::max_asset_path_len>>(), renderer);
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
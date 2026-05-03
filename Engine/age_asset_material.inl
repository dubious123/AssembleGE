#pragma once
#include "age.hpp"

namespace age::asset::material
{
	void
	full_unload(handle h_mat, auto& renderer) noexcept
	{
		auto& entry = h_mat.get_entry<e::kind::material>();
		if (entry.is_loaded())
		{
			if (runtime::is_handle_invalid(entry.h_tex_base_color) is_false)
			{
				texture::remove_ref(entry.h_tex_base_color);
			}
			if (runtime::is_handle_invalid(entry.h_tex_metallic_roughness) is_false)
			{
				texture::remove_ref(entry.h_tex_metallic_roughness);
			}
			if (runtime::is_handle_invalid(entry.h_tex_normal) is_false)
			{
				texture::remove_ref(entry.h_tex_normal);
			}
			if (runtime::is_handle_invalid(entry.h_tex_occlusion) is_false)
			{
				texture::remove_ref(entry.h_tex_occlusion);
			}
			if (runtime::is_handle_invalid(entry.h_tex_emissive) is_false)
			{
				texture::remove_ref(entry.h_tex_emissive);
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

			entry.h_tex_base_color		   = detail::load_common_from_path<e::kind::texture>(buf.read<std::array<char, config::max_asset_path_len>>());
			entry.h_tex_metallic_roughness = detail::load_common_from_path<e::kind::texture>(buf.read<std::array<char, config::max_asset_path_len>>());
			entry.h_tex_normal			   = detail::load_common_from_path<e::kind::texture>(buf.read<std::array<char, config::max_asset_path_len>>());
			entry.h_tex_occlusion		   = detail::load_common_from_path<e::kind::texture>(buf.read<std::array<char, config::max_asset_path_len>>());
			entry.h_tex_emissive		   = detail::load_common_from_path<e::kind::texture>(buf.read<std::array<char, config::max_asset_path_len>>());
		}

		if (runtime::is_handle_invalid(entry.h_tex_base_color) is_false)
		{
			texture::add_ref(entry.h_tex_base_color);
			texture::gpu_load(entry.h_tex_base_color, renderer);
		}
		if (runtime::is_handle_invalid(entry.h_tex_metallic_roughness) is_false)
		{
			texture::add_ref(entry.h_tex_metallic_roughness);
			texture::gpu_load(entry.h_tex_metallic_roughness, renderer);
		}
		if (runtime::is_handle_invalid(entry.h_tex_normal) is_false)
		{
			texture::add_ref(entry.h_tex_normal);
			texture::gpu_load(entry.h_tex_normal, renderer);
		}
		if (runtime::is_handle_invalid(entry.h_tex_occlusion) is_false)
		{
			texture::add_ref(entry.h_tex_occlusion);
			texture::gpu_load(entry.h_tex_occlusion, renderer);
		}
		if (runtime::is_handle_invalid(entry.h_tex_emissive) is_false)
		{
			texture::add_ref(entry.h_tex_emissive);
			texture::gpu_load(entry.h_tex_emissive, renderer);
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
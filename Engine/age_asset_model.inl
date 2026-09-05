#pragma once
#include "age.hpp"

namespace age::asset::detail
{
	void
	handle_mesh_baked_unload(handle h_mesh, auto& renderer) noexcept
	{
		if (runtime::is_handle_invalid(h_mesh) is_false)
		{
			mesh_baked::remove_ref(h_mesh);

			auto& entry = h_mesh.get_entry<e::kind::mesh_baked>();

			if (entry.ref_counter == 0)
			{
				mesh_baked::full_unload(h_mesh, renderer);
			}
		}
	}

	void
	handle_material_unload(handle h_mat, auto& renderer) noexcept
	{
		if (runtime::is_handle_invalid(h_mat) is_false)
		{
			material::remove_ref(h_mat);

			auto& entry = h_mat.get_entry<e::kind::material>();

			if (entry.ref_counter == 0)
			{
				material::full_unload(h_mat, renderer);
			}
		}
	}
}	 // namespace age::asset::detail

namespace age::asset::model
{
	void
	full_unload(handle h_model, auto& renderer) noexcept
	{
		auto& entry = h_model.get_entry<e::kind::model>();
		if (entry.is_loaded())
		{
			detail::handle_mesh_baked_unload(entry.h_mesh, renderer);

			for (auto& h_material : entry.h_material_vec)
			{
				detail::handle_material_unload(h_material, renderer);
			}
		}

		entry.any_loaded = false;

		AGE_ASSERT(entry.is_loaded() is_false);
		AGE_ASSERT(entry.is_any_loaded() is_false);
	}

	void
	load(handle h_model, auto& renderer) noexcept
	{
		auto& entry = h_model.get_entry<e::kind::model>();

		if (entry.is_meta_loaded() and entry.is_loaded())
		{
			return;
		}

		if (entry.is_meta_loaded() is_false and entry.is_any_loaded())
		{
			full_unload(h_model, renderer);
		}

		if (entry.is_meta_loaded() is_false)
		{
			if (auto file_data = asset::read_asset_file(entry.get_path());
				file_data.is_valid())
			{
				auto& buf = file_data.buf;
				switch (file_data.header.asset_version)
				{
				case config::model_asset_version:
				{
					entry.h_mesh = asset::find(e::kind::mesh_baked, buf.read<age::array<char, config::max_asset_path_len>>());
					entry.h_material_vec.resize(buf.read<uint32>());
					break;
				}
				default:
				{
					AGE_ASSERT(false);
					return;
				}
				}

				for (auto& h_mat : entry.h_material_vec)
				{
					h_mat = asset::find(e::kind::material, buf.read<age::array<char, config::max_asset_path_len>>());
				}
			}
			else
			{
				AGE_ASSERT(false);
				return;
			}
		}

		entry.meta_loaded = true;

		if (entry.h_mesh) { mesh_baked::gpu_load(entry.h_mesh, renderer); }
		for (auto& h_mat : entry.h_material_vec)
		{
			if (h_mat) { material::load(h_mat, renderer); }
		}

		if (entry.is_any_loaded() is_false)
		{
			if (entry.h_mesh) { mesh_baked::add_ref(entry.h_mesh); }
			for (auto& h_mat : entry.h_material_vec)
			{
				if (h_mat) { material::add_ref(h_mat); }
			}
		}

		entry.any_loaded = true;

		AGE_ASSERT(entry.is_meta_loaded());
		AGE_ASSERT(entry.is_any_loaded());
		AGE_ASSERT(entry.is_loaded());
	}

	handle
	load(std::string_view model_name, auto& renderer) noexcept
	{
		c_auto h_asset = detail::load_common<e::kind::model>(model_name);

		load(h_asset, renderer);

		return h_asset;
	}

	handle
	load_common_from_path(const age::array<char, config::max_asset_path_len>& full_path, auto& renderer) noexcept
	{
		c_auto h_asset = detail::load_common_from_path<e::kind::model>(full_path);

		load(h_asset, renderer);

		return h_asset;
	}
}	 // namespace age::asset::model
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

	handle
	handle_mesh_baked_load(const age::array<char, config::max_asset_path_len>& full_path, auto& renderer) noexcept
	{
		c_auto h_mesh = asset::find(e::kind::mesh_baked, full_path);

		if (runtime::is_handle_invalid(h_mesh)) { return {}; }

		mesh_baked::add_ref(h_mesh);
		mesh_baked::gpu_load(h_mesh, renderer);

		return h_mesh;
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

	handle
	handle_material_load(const age::array<char, config::max_asset_path_len>& full_path, auto& renderer) noexcept
	{
		c_auto h_mat = asset::find(e::kind::material, full_path);

		if (runtime::is_handle_invalid(h_mat)) { return {}; }

		material::add_ref(h_mat);
		material::load(h_mat, renderer);

		return h_mat;
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

		AGE_ASSERT(entry.is_loaded() is_false);
	}

	void
	load(handle h_model, auto& renderer) noexcept
	{
		auto& entry = h_model.get_entry<e::kind::model>();

		if (entry.is_loaded())
		{
			return;
		}

		if (auto file_data = asset::read_asset_file(entry.get_path());
			file_data.is_valid())
		{
			auto& buf = file_data.buf;
			switch (file_data.header.asset_version)
			{
			case config::model_asset_version:
			{
				entry.h_mesh = detail::handle_mesh_baked_load(buf.read<age::array<char, config::max_asset_path_len>>(), renderer);
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
				h_mat = detail::handle_material_load(buf.read<age::array<char, config::max_asset_path_len>>(), renderer);
			}
		}
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
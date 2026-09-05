#include "age_pch.hpp"
#include "age.hpp"

namespace age::asset
{
	template <>
	bool
	validate_header<e::kind::model>(const file_header& header) noexcept
	{
		auto res = true;
		{
			c_auto tmp = header.asset_kind == e::kind::model;
			AGE_ASSERT(tmp);
			res &= tmp;
		}

		return res;
	}

	std::array<char, config::max_asset_path_len>&
	entry<e::kind::model>::get_path() const noexcept
	{
		return g::path_vec[path_id];
	}

	bool
	entry<e::kind::model>::is_meta_loaded() const noexcept
	{
		return meta_loaded;
	}

	bool
	entry<e::kind::model>::is_any_loaded() const noexcept
	{
		return any_loaded;
	}

	bool
	entry<e::kind::model>::is_loaded() const noexcept
	{
		if (is_any_loaded() is_false) { return false; }
		if (is_meta_loaded() is_false) { return false; }

		auto loaded = true;

		if (h_mesh)
		{
			loaded &= h_mesh.get_entry<e::kind::mesh_baked>().is_gpu_loaded();
		}

		for (c_auto& h_mat : h_material_vec)
		{
			if (h_mat)
			{
				loaded &= h_mat.get_entry<e::kind::material>().is_loaded();
			}
		}

		return loaded;
	}
}	 // namespace age::asset

namespace age::asset::model
{
	void
	update_mesh(handle h_model, handle h_mesh) noexcept
	{
		auto& entry = h_model.get_entry<e::kind::model>();

		if (runtime::is_handle_invalid(entry.h_mesh) is_false)
		{
			mesh_baked::remove_ref(entry.h_mesh);
		}

		if (runtime::is_handle_invalid(h_mesh) is_false)
		{
			mesh_baked::add_ref(h_mesh);
		}

		entry.h_mesh = h_mesh;
	}

	void
	update_material(handle h_model, uint32 idx, handle h_mat) noexcept
	{
		auto& entry = h_model.get_entry<e::kind::model>();

		if (runtime::is_handle_invalid(entry.h_material_vec[idx]) is_false)
		{
			material::remove_ref(entry.h_material_vec[idx]);
		}

		if (runtime::is_handle_invalid(h_mat) is_false)
		{
			material::add_ref(h_mat);
		}

		entry.h_material_vec[idx] = h_mat;
	}

	bool
	renderable(handle h_model) noexcept
	{
		auto& entry = model::get_entry(h_model);
		return entry.h_mesh and entry.is_loaded();
	}
}	 // namespace age::asset::model

namespace age::asset::model
{
	void
	build(std::string_view model_path, const model_desc& desc) noexcept
	{
		auto buf = byte_buf::gen_reserved(config::max_asset_path_len * 1
										  + sizeof(uint32)
										  + config::max_asset_path_len * desc.h_materials.size());

		buf.write(get_path_safe<e::kind::mesh_baked>(desc.h_mesh),
				  cast_to<uint32>(desc.h_materials.size()));

		for (c_auto& h_mat : desc.h_materials)
		{
			buf.write(get_path_safe<e::kind::material>(h_mat));
		}

		c_auto f_header = get_default_file_header<e::kind::model>(buf.size());
		write_asset_file(model_path.data(), f_header, buf.data());
	}

	void
	save(handle h_model) noexcept
	{
		if (runtime::is_handle_invalid(h_model))
		{
			AGE_ASSERT(false);
			return;
		}

		auto& entry = h_model.get_entry<e::kind::model>();

		auto buf = byte_buf::gen_reserved(config::max_asset_path_len * 1
										  + sizeof(uint32)
										  + config::max_asset_path_len * entry.h_material_vec.size());

		buf.write(get_path_safe<e::kind::mesh_baked>(entry.h_mesh),
				  entry.h_material_vec.size<uint32>());

		for (c_auto& h_mat : entry.h_material_vec)
		{
			buf.write(get_path_safe<e::kind::material>(h_mat));
		}

		c_auto f_header = get_default_file_header<e::kind::model>(buf.size());
		write_asset_file(h_model.get_path<e::kind::model>().data(), f_header, buf.data());
	}
}	 // namespace age::asset::model
#pragma once
#include "age.hpp"

namespace age::asset::mesh_baked::detail
{
	void
	build_mesh_baked(const std::array<char, config::max_asset_path_len>& path, const primitive_desc&, e::vertex_kind) noexcept;

	handle
	load_common(std::string_view mesh_name) noexcept;
}	 // namespace age::asset::mesh_baked::detail

namespace age::asset::mesh_baked
{
	void
	cpu_unload(handle h_mesh) noexcept;

	void
	cpu_load(handle h_mesh, const primitive_desc& desc, e::vertex_kind v_kind) noexcept;

	void
	cpu_load(handle h_mesh) noexcept;

	handle
	cpu_load(std::string_view mesh_name) noexcept;

	void
	gpu_unload(handle h_mesh, auto& renderer) noexcept
	{
		auto& entry = h_mesh.get_entry<e::kind::mesh_baked>();
		AGE_ASSERT(entry.is_gpu_loaded());

		renderer.release_mesh(entry.render_id);
		AGE_SET_INVALID_ID(entry.render_id);

		AGE_ASSERT(entry.is_gpu_loaded() is_false);
	}

	void
	full_unload(handle h_mesh, auto& renderer) noexcept
	{
		auto& entry = h_mesh.get_entry<e::kind::mesh_baked>();
		if (entry.is_cpu_loaded())
		{
			cpu_unload(h_mesh);
		}

		if (entry.is_gpu_loaded())
		{
			gpu_unload(h_mesh, renderer);
		}

		AGE_ASSERT(entry.is_gpu_loaded() is_false);
		AGE_ASSERT(entry.is_cpu_loaded() is_false);
	}

	void
	full_unload(std::string_view mesh_name, auto& renderer) noexcept
	{
		auto full_path = std::format("{}{}{}", mesh_name, config::mesh_baked_asset_tag, config::asset_extension);
		auto h_mesh	   = registry::find<e::kind::mesh_baked>(mesh_name.data());

		if (AGE_IS_INVALID_ID(h_mesh.id))
		{
			full_unload(h_mesh, renderer);
		}
	}

	void
	gpu_load(handle h_mesh, auto& renderer, const primitive_desc& desc, e::vertex_kind v_kind) noexcept
	{
		auto& entry = h_mesh.get_entry<e::kind::mesh_baked>();
		if (entry.is_gpu_loaded())
		{
			return;
		}

		if (entry.is_cpu_loaded())
		{
			entry.render_id = renderer.upload_mesh(h_mesh);
			return;
		}

		if (auto buf = asset::read_asset_file(entry.get_path());
			buf.empty() is_false)
		{
			entry.p_blob	= buf.release();
			entry.render_id = renderer.upload_mesh(h_mesh);
			cpu_unload(h_mesh);
			return;
		}

		detail::build_mesh_baked(entry.get_path(), desc, v_kind);

		if (auto buf = asset::read_asset_file(entry.get_path());
			buf.empty() is_false)
		{
			entry.p_blob	= buf.release();
			entry.render_id = renderer.upload_mesh(h_mesh);
			cpu_unload(h_mesh);
			return;
		}
		else
		{
			AGE_ASSERT(false);
		}
	}

	handle
	gpu_load(std::string_view mesh_name, auto& renderer, const primitive_desc& desc, e::vertex_kind v_kind) noexcept
	{
		c_auto h_mesh = detail::load_common(mesh_name);
		gpu_load(h_mesh, renderer, desc, v_kind);
		return h_mesh;
	}

	void
	gpu_load(handle& h_mesh, auto& renderer) noexcept
	{
		auto& entry = h_mesh.get_entry<e::kind::mesh_baked>();
		if (entry.is_gpu_loaded())
		{
			return;
		}

		if (entry.is_cpu_loaded())
		{
			entry.render_id = renderer.upload_mesh(h_mesh);
			return;
		}

		if (auto buf = asset::read_asset_file(entry.get_path());
			buf.empty() is_false)
		{
			entry.p_blob	= buf.release();
			entry.render_id = renderer.upload_mesh(h_mesh);
			return;
		}
	}

	handle
	gpu_load(std::string_view mesh_name, auto& renderer) noexcept
	{
		c_auto h_mesh = detail::load_common(mesh_name);

		gpu_load(h_mesh, renderer);

		return h_mesh;
	}

	void
	full_load(handle h_mesh, auto& renderer) noexcept
	{
		auto& entry = h_mesh.get_entry<e::kind::mesh_baked>();

		if (entry.is_cpu_loaded() is_false)
		{
			cpu_load(h_mesh);
		}

		if (entry.is_gpu_loaded() is_false)
		{
			gpu_load(h_mesh, renderer);
		}
	}

	handle
	full_load(std::string_view mesh_name, auto& renderer) noexcept
	{
		c_auto h_mesh = detail::load_common(mesh_name);

		full_load(h_mesh, renderer);
	}
}	 // namespace age::asset::mesh_baked
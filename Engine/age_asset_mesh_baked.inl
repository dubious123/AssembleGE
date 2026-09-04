#pragma once
#include "age.hpp"

namespace age::asset::mesh_baked::detail
{
	void
	build_mesh_baked(const age::array<char, config::max_asset_path_len>& path, std::span<const primitive_desc>, e::vertex_kind) noexcept;

	void
	build_mesh_baked(const age::array<char, config::max_asset_path_len>& path, const primitive_desc&, e::vertex_kind) noexcept;

	bool
	cpu_load_helper(entry<e::kind::mesh_baked>& entry) noexcept;
}	 // namespace age::asset::mesh_baked::detail

namespace age::asset::mesh_baked
{
	void
	gpu_unload(handle h_mesh, auto& renderer) noexcept
	{
		auto& entry = h_mesh.get_entry<e::kind::mesh_baked>();
		AGE_ASSERT(entry.is_gpu_loaded());

		renderer.release_mesh(entry.render_id);

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
		c_auto full_path = get_asset_full_path<e::kind::mesh_baked>(mesh_name);
		c_auto h_mesh	 = find(e::kind::mesh_baked, full_path);

		if (AGE_IS_INVALID_ID(h_mesh.id))
		{
			full_unload(h_mesh, renderer);
		}
	}

	void
	gpu_load(handle h_mesh, auto& renderer, std::span<const primitive_desc> descs, e::vertex_kind v_kind) noexcept
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

		if (detail::cpu_load_helper(entry))
		{
			entry.render_id = renderer.upload_mesh(h_mesh);
			cpu_unload(h_mesh);
			return;
		}

		detail::build_mesh_baked(entry.get_path(), descs, v_kind);

		if (detail::cpu_load_helper(entry))
		{
			entry.render_id = renderer.upload_mesh(h_mesh);
			cpu_unload(h_mesh);
			return;
		}
		else
		{
			AGE_ASSERT(false);
		}
	}

	void
	gpu_load(handle h_mesh, auto& renderer, const primitive_desc& desc, e::vertex_kind v_kind) noexcept
	{
		gpu_load(h_mesh, renderer, { &desc, 1 }, v_kind);
	}

	handle
	gpu_load(std::string_view mesh_name, auto& renderer, std::span<const primitive_desc> descs, e::vertex_kind v_kind) noexcept
	{
		c_auto h_mesh = asset::detail::load_common<e::kind::mesh_baked>(mesh_name);
		gpu_load(h_mesh, renderer, descs, v_kind);
		return h_mesh;
	}

	handle
	gpu_load(std::string_view mesh_name, auto& renderer, const primitive_desc& desc, e::vertex_kind v_kind) noexcept
	{
		return gpu_load(mesh_name, renderer, { &desc, 1 }, v_kind);
	}

	void
	gpu_load(handle h_mesh, auto& renderer) noexcept
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

		if (detail::cpu_load_helper(entry))
		{
			entry.render_id = renderer.upload_mesh(h_mesh);
			return;
		}

		AGE_ASSERT(false);
	}

	handle
	gpu_load(std::string_view mesh_name, auto& renderer) noexcept
	{
		c_auto h_mesh = asset::detail::load_common<e::kind::mesh_baked>(mesh_name);

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
		c_auto h_mesh = asset::detail::load_common<e::kind::mesh_baked>(mesh_name);

		full_load(h_mesh, renderer);
	}
}	 // namespace age::asset::mesh_baked
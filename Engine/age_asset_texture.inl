#pragma once
#include "age.hpp"

namespace age::asset::texture
{
	void
	gpu_unload(handle h_tex, auto& renderer) noexcept
	{
		auto& entry = h_tex.get_entry<e::kind::texture>();
		AGE_ASSERT(entry.is_gpu_loaded());

		renderer.release_texture(entry.render_id);

		AGE_ASSERT(entry.is_gpu_loaded() is_false);
	}

	void
	full_unload(handle h_tex, auto& renderer) noexcept
	{
		auto& entry = h_tex.get_entry<e::kind::texture>();
		if (entry.is_cpu_loaded())
		{
			cpu_unload(h_tex);
		}

		if (entry.is_gpu_loaded())
		{
			gpu_unload(h_tex, renderer);
		}

		AGE_ASSERT(entry.is_gpu_loaded() is_false);
		AGE_ASSERT(entry.is_cpu_loaded() is_false);
	}

	void
	gpu_load(handle h_tex, auto& renderer) noexcept
	{
		auto& entry = h_tex.get_entry<e::kind::texture>();
		if (entry.is_gpu_loaded())
		{
			return;
		}

		if (entry.is_cpu_loaded())
		{
			entry.render_id = renderer.upload_texture(h_tex);
			return;
		}

		if (auto buf = asset::read_asset_file(entry.get_path());
			buf.empty() is_false)
		{
			entry.p_blob	= buf.release();
			entry.render_id = renderer.upload_texture(h_tex);
			return;
		}
	}

	handle
	gpu_load(std::string_view tex_name, auto& renderer) noexcept
	{
		c_auto h_tex = detail::load_common<e::kind::texture>(tex_name);

		gpu_load(h_tex, renderer);

		return h_tex;
	}

	void
	full_load(handle h_tex, auto& renderer) noexcept
	{
		auto& entry = h_tex.get_entry<e::kind::texture>();

		if (entry.is_cpu_loaded() is_false)
		{
			cpu_load(h_tex);
		}

		if (entry.is_gpu_loaded() is_false)
		{
			gpu_load(h_tex, renderer);
		}
	}

	handle
	full_load(std::string_view tex_name, auto& renderer) noexcept
	{
		c_auto h_tex = detail::load_common<e::kind::texture>(tex_name);

		full_load(h_tex, renderer);
	}
}	 // namespace age::asset::texture
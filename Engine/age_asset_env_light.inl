#pragma once
#include "age.hpp"

namespace age::asset::env_light
{
	void
	gpu_unload(handle h_env_light, auto& renderer) noexcept
	{
		auto& entry = h_env_light.get_entry<e::kind::env_light>();
		AGE_ASSERT(entry.is_gpu_loaded());

		renderer.release_env_light(entry.render_id);

		AGE_ASSERT(entry.is_gpu_loaded() is_false);
	}

	void
	full_unload(handle h_env_light, auto& renderer) noexcept
	{
		auto& entry = h_env_light.get_entry<e::kind::env_light>();
		if (entry.is_cpu_loaded())
		{
			cpu_unload(h_env_light);
		}

		if (entry.is_gpu_loaded())
		{
			gpu_unload(h_env_light, renderer);
		}

		AGE_ASSERT(entry.is_gpu_loaded() is_false);
		AGE_ASSERT(entry.is_cpu_loaded() is_false);
	}

	void
	gpu_load(handle h_env_light, auto& renderer) noexcept
	{
		auto& entry = h_env_light.get_entry<e::kind::env_light>();
		if (entry.is_gpu_loaded())
		{
			return;
		}

		if (entry.is_cpu_loaded())
		{
			entry.render_id = renderer.upload_env_light(h_env_light);
			return;
		}

		if (auto buf = asset::read_asset_file(entry.get_path());
			buf.empty() is_false)
		{
			entry.p_blob	= buf.release();
			entry.render_id = renderer.upload_env_light(h_env_light);
			return;
		}
	}

	handle
	gpu_load(std::string_view tex_name, auto& renderer) noexcept
	{
		c_auto h_env_light = detail::load_common<e::kind::env_light>(tex_name);

		gpu_load(h_env_light, renderer);

		return h_env_light;
	}

	void
	full_load(handle h_env_light, auto& renderer) noexcept
	{
		auto& entry = h_env_light.get_entry<e::kind::env_light>();

		if (entry.is_cpu_loaded() is_false)
		{
			cpu_load(h_env_light);
		}

		if (entry.is_gpu_loaded() is_false)
		{
			gpu_load(h_env_light, renderer);
		}
	}

	handle
	full_load(std::string_view tex_name, auto& renderer) noexcept
	{
		c_auto h_env_light = detail::load_common<e::kind::env_light>(tex_name);

		full_load(h_env_light, renderer);
	}
}	 // namespace age::asset::env_light
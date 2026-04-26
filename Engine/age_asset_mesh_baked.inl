#pragma once
#include "age.hpp"

namespace age::asset::mesh_test::detail
{
	void
	rebuild_mesh_baked(std::string_view path, const primitive_desc&, e::vertex_kind) noexcept;
}

namespace age::asset::mesh_test
{
	void
	unload(handle h_mesh, auto& renderer) noexcept
	{
		auto& entry = h_mesh.get_entry<e::kind::font>();
		if (entry.is_loaded())
		{
			renderer.release_mesh(entry.renderer_id);
		}

		AGE_ASSERT(entry.is_loaded() is_false);
	}

	void
	load(handle h_mesh, auto& renderer, const primitive_desc& desc, e::vertex_kind v_kind) noexcept
	{
		auto& entry = h_mesh.get_entry<e::kind::mesh_baked>();

		if (entry.is_loaded() and entry.vertex_kind == v_kind)
		{
			return;
		}

		detail::rebuild_mesh_baked(entry.get_path(), desc, v_kind);

		if (auto buf = asset::read_asset_file(entry.get_path());
			buf.empty() is_false)
		{
			detail::read_entry(entry, buf);

			entry.atlas_id = renderer.upload_texture(buf.data() + buf.read_amount(),
													 { .width = entry.atlas_width, .height = entry.atlas_height },
													 age::graphics::e::texture_format::rgba8_unorm);
		}
		else
		{
			AGE_ASSERT(false);
		}
	}

	handle
	load(std::string_view mesh_name, auto& renderer) noexcept
	{
		auto full_path = std::format("{}{}{}", mesh_name, config::mesh_baked_asset_tag, config::asset_extension);
		auto h_asset   = registry::find<e::kind::mesh_baked>(mesh_name.data());

		if (AGE_IS_INVALID_ID(h_asset.id))
		{
			h_asset = asset::create_entry<e::kind::mesh_baked>(full_path);
		}

		load(h_asset, renderer);

		return h_asset;
	}
}	 // namespace age::asset::mesh_test
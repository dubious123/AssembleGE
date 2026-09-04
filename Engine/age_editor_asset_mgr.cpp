#include "age_pch.hpp"
#include "age.hpp"

namespace age::editor::asset_mgr
{
	void
	init() noexcept
	{
		static_assert(g::asset_default_frames_to_live > 0);
		AGE_ASSERT(g::asset_pin_vec.is_empty());
	}

	void
	add_asset_pin(asset::e::kind e_kind, asset::handle h_asset, uint16 frames_to_live) noexcept
	{
		AGE_ASSERT(e_kind == h_asset.get_kind());
		AGE_ASSERT(runtime::is_handle_invalid(h_asset) is_false);
		AGE_ASSERT(frames_to_live > 0);

		asset::visit(e_kind, [&]<asset::e::kind k> { asset::add_ref<k>(h_asset); });
		g::asset_pin_vec.emplace_back(asset_pin_data{
			.kind			= e_kind,
			.frames_to_live = frames_to_live,
			.h_asset		= h_asset });
	}

	bool
	is_pinned(asset::handle h_asset) noexcept
	{
		return std::ranges::find(g::asset_pin_vec, h_asset, &asset_pin_data::h_asset) != g::asset_pin_vec.end();
	}

	void
	deinit() noexcept
	{
		for (auto& pin : g::asset_pin_vec)
		{
			asset::visit(pin.kind, [&]<asset::e::kind k> {
				asset::remove_ref<k>(pin.h_asset);
			});
		}

		g::asset_pin_vec.clear();
	}
}	 // namespace age::editor::asset_mgr
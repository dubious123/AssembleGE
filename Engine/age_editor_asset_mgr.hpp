#pragma once
#include "age.hpp"

namespace age::editor::asset_mgr
{
	void
	init() noexcept;

	void
	deinit() noexcept;

	void
	update(auto& ecs_game, auto& renderer) noexcept;

	void
	add_asset_pin(asset::e::kind, asset::handle, uint16 frames_to_live = g::asset_default_frames_to_live) noexcept;

	bool
	is_pinned(asset::handle _) noexcept;
}	 // namespace age::editor::asset_mgr
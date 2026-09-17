#pragma once
#include "age.hpp"

namespace age::asset::registry
{
	// relative to asset_root_dir
	void
	load(std::string_view dir) noexcept;

	void
	save() noexcept;

	void
	register_asset(asset::handle _) noexcept;

	void
	register_asset(e::kind, std::string_view path) noexcept;

	void
	unregister_asset(asset::handle _) noexcept;

	void
	unregister_asset(e::kind, std::string_view path) noexcept;

	bool
	is_registered(asset::handle _) noexcept;

	template <e::kind>
	bool
	is_registered(asset::handle _) noexcept;

	std::span<const asset::handle>
	all(e::kind k) noexcept;

	void
	clear() noexcept;
}	 // namespace age::asset::registry

namespace age::asset::registry
{
	template <e::kind e_kind>
	bool
	is_registered(asset::handle h) noexcept
	{
		auto& vec = g::registry_map[to_idx(e_kind)];
		auto  it  = std::ranges::find(vec, h);
		return it != vec.end();
	}
}	 // namespace age::asset::registry

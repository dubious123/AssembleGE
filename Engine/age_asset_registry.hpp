#pragma once
#include "age.hpp"

namespace age::asset::registry
{
	void
	load(const char* root_dir) noexcept;

	void
	save() noexcept;

	void
	register_asset(asset::handle _) noexcept;

	void
	register_asset(e::kind, const char* path) noexcept;

	void
	unregister_asset(asset::handle _) noexcept;

	void
	unregister_asset(e::kind, const char* path) noexcept;

	asset::handle
	find(e::kind, const char* path) noexcept;

	std::span<const asset::handle>
	all(e::kind k) noexcept;
}	 // namespace age::asset::registry

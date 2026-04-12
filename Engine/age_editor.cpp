#include "age_pch.hpp"
#include "age.hpp"

namespace age::editor
{
	void
	init() noexcept
	{
	}

	void
	deinit() noexcept
	{
		g::select_vec.clear();
	}

	void
	add_select(uint64 ent_id) noexcept
	{
		if (is_selected(ent_id) is_false)
		{
			g::select_vec.emplace_back(ent_id);
		}
	}

	void
	remove_select(uint64 ent_id) noexcept
	{
		for (auto&& [idx, id] : g::select_vec | std::views::enumerate)
		{
			if (ent_id == id)
			{
				g::select_vec[idx] = g::select_vec.back();
				g::select_vec.pop_back();
				break;
			}
		}
	}

	bool
	is_selected(uint64 ent_id) noexcept
	{
		for (auto id : g::select_vec)
		{
			if (ent_id == id) { return true; }
		}
		return false;
	}

	void
	clear_select() noexcept
	{
		g::select_vec.clear();
	}
}	 // namespace age::editor
#include "age_pch.hpp"
#include "age.hpp"

namespace age::editor
{
	void
	init() noexcept
	{
		g::command_buf.clear();

		g::current_mode = e::mode_kind::edit;
		g::current_game = game_editor_data{};
	}

	void
	deinit() noexcept
	{
		g::select_vec.clear();

		if constexpr (age::config::debug_mode)
		{
			g::command_buf.validate();
		}

		g::command_buf.clear();
	}

	bool
	is_edit_mode() noexcept
	{
		return g::current_mode == e::mode_kind::edit;
	}

	bool
	is_play_mode() noexcept
	{
		return g::current_mode == e::mode_kind::play;
	}
}	 // namespace age::editor

// selete
namespace age::editor
{
	// void
	// add_select(uint32 storage_code_idx, uint64 ent_id) noexcept
	//{
	//	if (is_selected(storage_code_idx, ent_id) is_false)
	//	{
	//		g::select_vec[storage_code_idx].emplace_back(ent_id);
	//	}
	// }

	// void
	// remove_select(uint32 storage_code_idx, uint64 ent_id) noexcept
	//{
	//	for (auto&& [idx, id] : g::select_vec[storage_code_idx] | std::views::enumerate)
	//	{
	//		if (ent_id == id)
	//		{
	//			g::select_vec[storage_code_idx][idx] = g::select_vec[storage_code_idx].back();
	//			g::select_vec[storage_code_idx].pop_back();
	//			break;
	//		}
	//	}
	// }

	// bool
	// is_selected(uint32 storage_code_idx, uint64 ent_id) noexcept
	//{
	//	for (auto id : g::select_vec[storage_code_idx])
	//	{
	//		if (ent_id == id) { return true; }
	//	}
	//	return false;
	// }

	// void
	// clear_select() noexcept
	//{
	//	for (auto& vec : g::select_vec)
	//	{
	//		vec.clear();
	//	}
	// }

	void
	set_select_kind(e::select_kind new_kind) noexcept
	{
		if (g::current_select_kind != new_kind)
		{
			for (auto& vec : g::select_vec)
			{
				vec.clear();
			}
			g::current_select_kind = new_kind;
		}
	}

	void
	add_select(e::select_kind kind, uint32 group_idx, uint64 id) noexcept
	{
		set_select_kind(kind);

		if (is_selected(kind, group_idx, id) is_false)
		{
			g::select_vec[group_idx].emplace_back(id);
		}
	}

	void
	remove_select(e::select_kind kind, uint32 group_idx, uint64 id) noexcept
	{
		if (g::current_select_kind != kind) { return; }

		for (auto&& [idx, stored_id] : g::select_vec[group_idx] | std::views::enumerate)
		{
			if (id == stored_id)
			{
				g::select_vec[group_idx][idx] = g::select_vec[group_idx].back();
				g::select_vec[group_idx].pop_back();
				break;
			}
		}
	}

	bool
	is_selected(e::select_kind kind, uint32 group_idx, uint64 id) noexcept
	{
		if (g::current_select_kind != kind) { return false; }

		for (auto stored_id : g::select_vec[group_idx])
		{
			if (id == stored_id) { return true; }
		}
		return false;
	}

	void
	clear_select() noexcept
	{
		for (auto& vec : g::select_vec)
		{
			vec.clear();
		}
		g::current_select_kind = e::select_kind::none;
	}
}	 // namespace age::editor
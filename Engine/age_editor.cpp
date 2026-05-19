#include "age_pch.hpp"
#include "age.hpp"

namespace age::editor
{
	void
	init(util::function_ref<asset::handle(std::string_view, const asset::primitive_desc&, asset::e::vertex_kind)> fn_mesh_gpu_load) noexcept
	{
		// g::command_buf.clear();

		g::current_mode = e::mode_kind::edit;
		g::current_game = game_editor_data{};

		g::show_modal = false;
		g::set_focus  = false;

		g::h_mesh_cone = fn_mesh_gpu_load("editor_mesh_cone",
										  asset::primitive_desc{
											  .seg_u	 = 30,
											  .seg_v	 = 1,
											  .mesh_kind = asset::e::primitive_mesh_kind::cone,
										  },
										  asset::e::vertex_kind::pnt_uv0);

		g::h_mesh_cube = fn_mesh_gpu_load("editor_mesh_cube",
										  asset::primitive_desc{
											  .seg_u	 = 1,
											  .seg_v	 = 1,
											  .mesh_kind = asset::e::primitive_mesh_kind::cube,
										  },
										  asset::e::vertex_kind::pnt_uv0);
	}

	void
	deinit(util::function_ref<void(asset::handle)> fn_mesh_full_unload) noexcept
	{
		g::select_vec.clear();

		if constexpr (age::config::debug_mode)
		{
			// g::command_buf.validate();
		}

		// g::command_buf.clear();

		fn_mesh_full_unload(g::h_mesh_cone);
		fn_mesh_full_unload(g::h_mesh_cube);
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

	bool
	has_selection(e::select_kind kind, uint32 group_idx) noexcept
	{
		if (g::current_select_kind != kind) { return false; }

		return g::select_vec[group_idx].is_empty() is_false;
	}

	std::optional<uint64>
	last_selected(e::select_kind kind, uint32 group_idx) noexcept
	{
		if (has_selection(kind, group_idx))
		{
			return { g::select_vec[group_idx].back() };
		}
		else
		{
			return {};
		}
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
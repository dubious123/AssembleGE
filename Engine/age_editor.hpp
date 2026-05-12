#pragma once
#include "age.hpp"

namespace age::editor
{
	void
	init() noexcept;

	void
	deinit() noexcept;

	bool
	is_edit_mode() noexcept;

	bool
	is_play_mode() noexcept;

	void
	add_select(e::select_kind, uint32 group_idx, uint64 id) noexcept;

	void
	remove_select(e::select_kind, uint32 group_idx, uint64 id) noexcept;

	bool
	is_selected(e::select_kind, uint32 group_idx, uint64 id) noexcept;

	bool
	has_selection(e::select_kind, uint32 group_idx) noexcept;

	std::optional<uint64>
	last_selected(e::select_kind kind, uint32 group_idx) noexcept;

	void
	clear_select() noexcept;

	void
	load_game(auto& game, std::filesystem::path root_dir, auto& renderer) noexcept;

	void
	save_game(auto& game, auto& renderer) noexcept;

	void
	update_game(auto& ecs_game, auto& renderer) noexcept;
}	 // namespace age::editor

namespace age::editor::detail
{
	void
	register_entity(storage_editor_data& editor_storage,
					uint32				 editor_arch_idx,
					uint64				 editor_ent_idx,
					uint64				 ecs_entity_id) noexcept;

	void
	unregister_entity(storage_editor_data& editor_storage,
					  uint32			   editor_arch_idx,
					  uint64			   editor_ent_idx,
					  uint64			   ecs_entity_id) noexcept;

	void
	re_register_entity(storage_editor_data& editor_storage, uint64 ecs_entity_id, uint64 new_archetype) noexcept;
}	 // namespace age::editor::detail

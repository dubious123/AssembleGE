#pragma once
#include "age.hpp"

namespace age::editor
{
	void
	init(auto& ecs_game, auto& renderer) noexcept;

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
	load_game(auto& ecs_game, std::string_view root_parent_dir, auto& renderer) noexcept;

	void
	save_game(auto& ecs_game, auto& renderer) noexcept;

	void
	update_game(auto& ecs_game, auto& renderer) noexcept;

	void
	render_current_scene(auto& ecs_game, auto& renderer, platform::window_handle h_window) noexcept;
}	 // namespace age::editor

// ecs
namespace age::editor
{
	uint64
	add_entity(uint32 editor_scene_idx, uint32 editor_storage_idx, std::string_view name) noexcept;

	void
	remove_entity(uint32 editor_scene_idx, uint32 editor_storage_idx, uint64 ecs_entity_id) noexcept;

	uint64
	get_archetype(uint32 editor_scene_idx, uint32 editor_storage_idx, uint64 ecs_entity_id) noexcept;

	void
	add_components(uint32 editor_scene_idx, uint32 editor_storage_idx, uint64 ecs_entity_id, uint64 ecs_archetype_to_add) noexcept;

	void
	remove_components(uint32 editor_scene_idx, uint32 editor_storage_idx, uint64 ecs_entity_id, uint64 ecs_archetype_to_remove) noexcept;

	// return invalid_idx if the component not found
	uint32
	get_ecs_component_id(uint32 editor_scene_idx, uint32 editor_storage_idx, uint64 component_name_hash) noexcept;

	// if storages does not contains component, UB
	void*
	get_component_ptr(uint32 editor_scene_idx, uint32 editor_storage_idx, uint64 ecs_entity_id, uint32 ecs_cmponent_id) noexcept;

	// if storages does not contains component, UB
	void
	get_component_ptrs(uint32 editor_scene_idx, uint32 editor_storage_idx, uint64 ecs_entity_id, std::span<const uint64> cmp_name_hash_span, AGE_OUT std::span<void*> cmp_ptr_span) noexcept;

	template <typename... t_cmp>
	requires(sizeof...(t_cmp) >= 2)
	std::tuple<t_cmp&...>
	get_components(uint32 editor_scene_idx, uint32 editor_storage_idx, uint64 ecs_entity_id) noexcept;

	template <typename t_cmp>
	t_cmp&
	get_components(uint32 editor_scene_idx, uint32 editor_storage_idx, uint64 ecs_entity_id) noexcept;

	// add components and get components
	template <typename... t_cmp>
	decltype(auto)
	add_components(uint32 editor_scene_idx, uint32 editor_storage_idx, uint64 ecs_entity_id) noexcept;
}	 // namespace age::editor

namespace age::editor
{
	const std::string&
	get_asset_root_dir_path() noexcept;

	const std::string&
	get_asset_dir_path(asset::e::kind kind) noexcept;

	std::string
	get_asset_path(asset::e::kind kind, std::string_view asset_name) noexcept;

	age::array<char, config::max_asset_path_len>
	get_asset_full_path(asset::e::kind e_kind, std::string_view asset_name) noexcept;

	void
	asset_full_unload(asset::e::kind asset_kind, asset::handle h_asset) noexcept;
}	 // namespace age::editor

// ui
namespace age::editor
{
	void
	ui_inspector(auto& ecs_game, auto& renderer) noexcept;

	void
	ui_entity_hierarchy(auto& ecs_game, auto& renderer) noexcept;

	void
	ui_scene_view(auto& renderer) noexcept;

	void
	ui_asset_list_panel() noexcept;

	void
	ui_modal() noexcept;

	template <asset::e::kind>
	bool /*is_dirty*/
	ui_asset(asset::handle h) noexcept;

	template <>
	bool ui_asset<asset::e::kind::font>(asset::handle) noexcept;
	template <>
	bool ui_asset<asset::e::kind::mesh_baked>(asset::handle) noexcept;
	template <>
	bool ui_asset<asset::e::kind::material>(asset::handle) noexcept;
	template <>
	bool ui_asset<asset::e::kind::texture>(asset::handle) noexcept;
	template <>
	bool ui_asset<asset::e::kind::env_light>(asset::handle) noexcept;
	template <>
	bool ui_asset<asset::e::kind::model>(asset::handle) noexcept;

	void
	ui_asset(asset::e::kind, asset::handle h, auto& renderer) noexcept;

	void
	ui_modal_new_asset() noexcept;

	void
	ui_modal_import_asset() noexcept;
}	 // namespace age::editor

namespace age::editor::gizmo
{
	float3
	translation(const float cam_fov_y, const float3& cam_pos, const float3& cam_forward, const float3& world_pos, const float4& quat, const float screen_size) noexcept;

	// quat, pivot world pos, drag_started, dragging
	std::tuple<float4, float3, bool, bool>
	rotation(const float cam_fov_y, const float3& cam_pos, const float3& cam_forward, const float3& world_pos, const float4& quat, const float screen_size) noexcept;

	// scale ratio , drag_started, dragging
	std::tuple<float3, bool, bool>
	scale(const float cam_fov_y, const float3& cam_pos, const float3& cam_forward, const float3& world_pos, const float4& quat, const float screen_size) noexcept;
}	 // namespace age::editor::gizmo

namespace age::editor::detail
{
	scene_editor_data&
	find_scene_editor_data(uint32 ecs_idx) noexcept;

	storage_editor_data&
	find_storage_editor_data(uint32 ecs_scene_idx, uint32 ecs_storage_idx) noexcept;
}	 // namespace age::editor::detail

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

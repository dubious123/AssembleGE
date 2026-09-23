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
	save_game() noexcept;

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

	uint64
	add_entity(uint32 editor_scene_idx, uint32 editor_storage_idx, std::string_view name, uint64 new_ecs_archetype) noexcept;

	uint64
	copy_entity(uint32 editor_scene_idx, uint32 editor_storage_idx, uint64 ecs_entity_id) noexcept;

	void
	remove_entity(uint32 editor_scene_idx, uint32 editor_storage_idx, uint64 ecs_entity_id) noexcept;

	uint64
	get_archetype(uint32 editor_scene_idx, uint32 editor_storage_idx, uint64 ecs_entity_id) noexcept;

	// returns string_view {editor_storage_data::names[0].data()}
	std::string_view
	get_component_name(uint32 editor_scene_idx, uint32 editor_storage_idx, uint32 ecs_component_id) noexcept;

	uint32
	get_component_count(uint32 editor_scene_idx, uint32 editor_storage_idx) noexcept;

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

	// change editor entity location
	void
	relocate_editor_entity(uint32 editor_scene_idx, uint32 editor_storage_idx, uint64 ecs_entity_id, uint64 new_archetype) noexcept;
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
	ui_inspector() noexcept;

	void
	ui_entity_hierarchy() noexcept;

	void
	ui_scene_view(auto& renderer) noexcept;

	void
	ui_asset_list_panel() noexcept;

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
	ui_modal() noexcept;

	void
	ui_modal_new_asset() noexcept;

	void
	ui_modal_import_asset() noexcept;

	float3
	get_component_color(uint32 cmp_idx) noexcept;

	ui::widget_ctx
	ui_component_header(const char* p_name, AGE_OUT bool& close_out) noexcept;

	void
	ui_component(auto&& cmp) noexcept;

	void
	ui_component(ecs::position& pos) noexcept;
	void
	ui_component(ecs::render_object& obj) noexcept;
	void
	ui_component(ecs::rotation& rot) noexcept;
	void
	ui_component(ecs::scale& scale) noexcept;
	void
	ui_component(ecs::mesh& mesh) noexcept;
	void
	ui_component(asset::handle h_mat, asset::entry<asset::e::kind::material>& mat_entry) noexcept;
	void
	ui_component(ecs::material& mat) noexcept;
	void
	ui_component(ecs::model_render_option&) noexcept;
	void
	ui_component(ecs::model&) noexcept;
	void
	ui_component(ecs::directional_light& light) noexcept;
	void
	ui_component(ecs::point_light& light) noexcept;
	void
	ui_component(ecs::spot_light& light) noexcept;
	void
	ui_component(ecs::env_light& env_light) noexcept;
	void
	ui_component(ecs::camera& cam) noexcept;
	void
	ui_component(ecs::bloom& cmp) noexcept;
	void
	ui_component(ecs::gi_config& cmp) noexcept;
	void
	ui_component(age::ecs::editor_cam_setting& cmp) noexcept;
	void
	ui_component(age::ecs::ao_config& cmp) noexcept;
	void
	ui_component(age::ecs::aa_config& cmp) noexcept;
	void
	ui_component(age::ecs::debug_view_config& cmp) noexcept;
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

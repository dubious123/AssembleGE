#pragma once
#include "age.hpp"

namespace age::editor::g
{
	inline auto select_vec	= age::vector<uint64>{};
	inline auto command_buf = ecs::command_buffer{};

	inline auto entity_name_map = age::unordered_map<uint64, std::array<char, 64>>{};
}	 // namespace age::editor::g

namespace age::editor
{
	void
	init() noexcept;

	void
	deinit() noexcept;

	void
	add_select(uint64 ent_id) noexcept;

	void
	remove_select(uint64 ent_id) noexcept;

	bool
	is_selected(uint64 ent_id) noexcept;

	void
	clear_select() noexcept;

	// ui
	void
	ui_camera(age::ecs::position& pos, age::ecs::camera& cam, auto& renderer) noexcept;

	void
	ui_directional_light(age::ecs::directional_light& light, auto& renderer) noexcept;

	void
	ui_point_light(age::ecs::position& pos, age::ecs::point_light& light, auto& renderer) noexcept;

	void
	ui_spot_light(age::ecs::position& pos, age::ecs::spot_light& light, auto& renderer) noexcept;

	void
	ui_render_object(age::ecs::render_object render_obj,
					 age::ecs::position&	 pos,
					 age::ecs::rotation&	 rot,
					 age::ecs::scale&		 scale,
					 age::ecs::mesh&		 mesh,
					 age::ecs::material&	 mat,
					 auto&					 renderer) noexcept;
}	 // namespace age::editor
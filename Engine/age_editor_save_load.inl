#pragma once
#include "age.hpp"

// code -> editor_data
namespace age::editor::detail
{
	void
	gen_storage_data(auto& storage_editor, auto& storage, auto scene_idx, auto storage_idx) noexcept
	{
		using t_storage			 = BARE_OF(storage);
		using t_archetype_traits = typename t_storage::t_archetype_traits;

		storage_editor.component_data_vec.reserve(t_archetype_traits::cmp_count());

		[]<auto... i>(std::index_sequence<i...>, auto& storage_editor) {
			((storage_editor.component_data_vec.emplace_back(component_editor_data{
				 .names					  = ecs::get_component_name<typename t_archetype_traits::template t_component<i>>() | std::ranges::to<age::vector<age::array<char, config::max_component_name_len>>>(),
				 .version				  = ecs::get_component_version<typename t_archetype_traits::template t_component<i>>(),
				 .byte_size				  = ecs::get_byte_size<typename t_archetype_traits::template t_component<i>>(),
				 .ecs_component_id		  = cast_to<uint32>(i),
				 .ecs_component_name_hash = ecs::get_component_name_hash<typename t_archetype_traits::template t_component<i>>(),
			 })),
			 ...);
		}(std::make_index_sequence<t_archetype_traits::cmp_count()>{}, storage_editor);
	}

	void
	gen_scene_data(auto& scene_editor, auto& scene, auto scene_idx) noexcept
	{
		scene_editor.storage_data_vec.reserve(scene.storage_count());
		[]<auto... i>(std::index_sequence<i...>, auto& scene_editor, auto& scene, auto scene_idx) {
			((scene_editor.storage_data_vec.emplace_back(storage_editor_data{
				  .names	= std::get<i>(scene.storage_names()) | std::ranges::to<age::vector<age::array<char, config::max_entity_storage_name_len>>>(),
				  .code_idx = i,
			  }),

			  gen_storage_data(scene_editor.storage_data_vec.back(), std::get<i>(scene.storages()), scene_idx, i)),
			 ...);
		}(std::make_index_sequence<scene.storage_count()>{}, scene_editor, scene, scene_idx);
	}

	game_editor_data
	gen_game_data(auto& game) noexcept
	{
		auto res = game_editor_data{};

		res.names					 = game.age_editor_name_arr | std::ranges::to<age::vector<age::array<char, config::max_game_name_len>>>();
		res.default_active_scene_idx = 0;

		res.scene_data_vec.clear();

		res.scene_data_vec.reserve(game.scene_count());
		[]<auto... i>(std::index_sequence<i...>, auto& res, auto& game) {
			((res.scene_data_vec.emplace_back(scene_editor_data{
				  .names	= std::get<i>(game.scene_names()) | std::ranges::to<age::vector<age::array<char, config::max_scene_name_len>>>(),
				  .code_idx = i,
				  .loaded	= false,
			  }),

			  gen_scene_data(res.scene_data_vec.back(), std::get<i>(game.scenes()), i)),
			 ...);
		}(std::make_index_sequence<game.scene_count()>{}, res, game);

		return res;
	}

	void
	load_game_impl(std::string_view root_parent_dir, std::span<const age::array<char, age::config::max_game_name_len>> age_editor_name_span, game_editor_data&& code_game_data) noexcept;
}	 // namespace age::editor::detail

namespace age::editor
{
	void
	load_game(auto& game, std::string_view root_parent_dir) noexcept
	{
		detail::load_game_impl(root_parent_dir, { game.age_editor_name_arr }, detail::gen_game_data(game));
	}
}	 // namespace age::editor

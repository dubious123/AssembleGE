#include "age_pch.hpp"
#include "age.hpp"

namespace age::editor::detail
{
	uint32
	find_or_get_editor_archetype_idx(storage_editor_data& editor_storage, uint64 ecs_archetype) noexcept
	{
		for (auto&& [arch_idx, arch_data] : editor_storage.archetype_data_vec | std::views::enumerate)
		{
			if (arch_data.archetype == ecs_archetype)
			{
				return static_cast<uint32>(arch_idx);
			}
		}

		c_auto res		 = editor_storage.archetype_data_vec.size<uint32>();
		auto&  arch_data = editor_storage.archetype_data_vec.emplace_back(archetype_editor_data{
			.archetype = ecs_archetype,
		});

		// todo, change_sig
		util::integral_to_str<16>(AGE_OUT arch_data.name, ecs_archetype);
		return res;
	}
}	 // namespace age::editor::detail

namespace age::editor
{
	uint64
	add_entity(uint32 editor_scene_idx, uint32 editor_storage_idx, std::string_view name) noexcept
	{
		AGE_ASSERT(editor_scene_idx < g::current_game.scene_data_vec.size<uint32>());
		AGE_ASSERT(editor_storage_idx < g::current_game.scene_data_vec[editor_scene_idx].storage_data_vec.size<uint32>());

		auto& editor_scene	 = g::current_game.scene_data_vec[editor_scene_idx];
		auto& editor_storage = editor_scene.storage_data_vec[editor_storage_idx];

		c_auto ecs_entity_id = g::host_ops.p_add_entity(editor_scene.code_idx, editor_storage.code_idx);

		c_auto editor_arch_idx = detail::find_or_get_editor_archetype_idx(editor_storage, 0);

		// register_entity
		{
			auto&  editor_arch_data = editor_storage.archetype_data_vec[editor_arch_idx];
			c_auto editor_ent_idx	= editor_arch_data.entity_data_vec.size<uint32>();
			auto&  editor_ent_data	= editor_arch_data.entity_data_vec.emplace_back();
			editor_ent_data.id		= ecs_entity_id;
			util::to_fixed_str(name, AGE_OUT editor_ent_data.name);
			editor_storage.ecs_ent_id_to_editor_location_map[ecs_entity_id] = { editor_arch_idx, editor_ent_idx };
			++editor_storage.entity_count;
		}

		return ecs_entity_id;
	}

	void
	remove_entity(uint32 editor_scene_idx, uint32 editor_storage_idx, uint64 ecs_entity_id) noexcept
	{
		AGE_ASSERT(editor_scene_idx < g::current_game.scene_data_vec.size<uint32>());
		AGE_ASSERT(editor_storage_idx < g::current_game.scene_data_vec[editor_scene_idx].storage_data_vec.size<uint32>());

		auto& editor_scene	 = g::current_game.scene_data_vec[editor_scene_idx];
		auto& editor_storage = editor_scene.storage_data_vec[editor_storage_idx];

		c_auto[editor_arch_idx, editor_ent_idx] = editor_storage.ecs_ent_id_to_editor_location_map[ecs_entity_id];

		auto& arch_data = editor_storage.archetype_data_vec[editor_arch_idx];

		{
			auto& ent_data = arch_data.entity_data_vec[editor_ent_idx];
			g::host_ops.p_remove_entity(editor_scene.code_idx, editor_storage.code_idx, ent_data.id);
		}

		// todo, implement vector.erase
		{
			for (auto i = editor_ent_idx + 1; i < arch_data.entity_data_vec.size(); ++i)
			{
				arch_data.entity_data_vec[i - 1] = std::move(arch_data.entity_data_vec[i]);
			}
			arch_data.entity_data_vec.pop_back();
		}

		for (auto&& [i, ent] : arch_data.entity_data_vec | views::enumerate<uint32> | std::views::drop(editor_ent_idx))
		{
			editor_storage.ecs_ent_id_to_editor_location_map[ent.id].second = i;
		}

		editor_storage.ecs_ent_id_to_editor_location_map.erase(ecs_entity_id);
		--editor_storage.entity_count;
	}
}	 // namespace age::editor
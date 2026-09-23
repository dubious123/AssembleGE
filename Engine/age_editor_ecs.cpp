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

		c_auto ecs_entity_id = g::host_ops.p_new_entity(editor_scene.code_idx, editor_storage.code_idx);

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

	uint64
	add_entity(uint32 editor_scene_idx, uint32 editor_storage_idx, std::string_view name, uint64 new_ecs_archetype) noexcept
	{
		c_auto ecs_entity_id = add_entity(editor_scene_idx, editor_storage_idx, name);
		add_components(editor_scene_idx, editor_storage_idx, ecs_entity_id, new_ecs_archetype);
		return ecs_entity_id;
	}

	uint64
	copy_entity(uint32 editor_scene_idx, uint32 editor_storage_idx, uint64 ecs_entity_id) noexcept
	{
		AGE_ASSERT(editor_scene_idx < g::current_game.scene_data_vec.size<uint32>());
		AGE_ASSERT(editor_storage_idx < g::current_game.scene_data_vec[editor_scene_idx].storage_data_vec.size<uint32>());

		auto& editor_scene	 = g::current_game.scene_data_vec[editor_scene_idx];
		auto& editor_storage = editor_scene.storage_data_vec[editor_storage_idx];

		AGE_ASSERT(editor_storage.ecs_ent_id_to_editor_location_map.contains(ecs_entity_id));

		c_auto[editor_archetype_idx, editor_entity_idx] = editor_storage.ecs_ent_id_to_editor_location_map[ecs_entity_id];

		c_auto new_ecs_entity_id = g::host_ops.p_copy_entity(editor_scene.code_idx, editor_storage.code_idx, ecs_entity_id);

		auto& arch_data = editor_storage.archetype_data_vec[editor_archetype_idx];

		editor_storage.ecs_ent_id_to_editor_location_map[new_ecs_entity_id] = std::pair{ editor_archetype_idx, arch_data.entity_data_vec.size() };

		auto& ent_data = arch_data.entity_data_vec.emplace_back(entity_editor_data{
			.id	  = new_ecs_entity_id,
			.name = util::fixed_format<config::max_entity_name_len>("{}_clone", arch_data.entity_data_vec[editor_entity_idx].name),
		});

		++editor_storage.entity_count;

		return new_ecs_entity_id;
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

		for (auto&& [i, ent] : arch_data.entity_data_vec | views::enumerate_rng<uint32> | std::views::drop(editor_ent_idx))
		{
			editor_storage.ecs_ent_id_to_editor_location_map[ent.id].second = i;
		}

		editor_storage.ecs_ent_id_to_editor_location_map.erase(ecs_entity_id);
		--editor_storage.entity_count;
	}

	uint64
	get_archetype(uint32 editor_scene_idx, uint32 editor_storage_idx, uint64 ecs_entity_id) noexcept
	{
		AGE_ASSERT(editor_scene_idx < g::current_game.scene_data_vec.size<uint32>());
		AGE_ASSERT(editor_storage_idx < g::current_game.scene_data_vec[editor_scene_idx].storage_data_vec.size<uint32>());

		auto& editor_scene	 = g::current_game.scene_data_vec[editor_scene_idx];
		auto& editor_storage = editor_scene.storage_data_vec[editor_storage_idx];

		return g::host_ops.p_get_archetype(editor_scene.code_idx, editor_storage.code_idx, ecs_entity_id);
	}

	uint32
	get_component_count(uint32 editor_scene_idx, uint32 editor_storage_idx) noexcept
	{
		AGE_ASSERT(editor_scene_idx < g::current_game.scene_data_vec.size<uint32>());
		AGE_ASSERT(editor_storage_idx < g::current_game.scene_data_vec[editor_scene_idx].storage_data_vec.size<uint32>());

		auto& editor_scene	 = g::current_game.scene_data_vec[editor_scene_idx];
		auto& editor_storage = editor_scene.storage_data_vec[editor_storage_idx];

		return g::host_ops.p_get_component_count(editor_scene.code_idx, editor_storage.code_idx);
	}

	std::string_view
	get_component_name(uint32 editor_scene_idx, uint32 editor_storage_idx, uint32 ecs_component_id) noexcept
	{
		AGE_ASSERT(editor_scene_idx < g::current_game.scene_data_vec.size<uint32>());
		AGE_ASSERT(editor_storage_idx < g::current_game.scene_data_vec[editor_scene_idx].storage_data_vec.size<uint32>());

		auto& editor_scene	 = g::current_game.scene_data_vec[editor_scene_idx];
		auto& editor_storage = editor_scene.storage_data_vec[editor_storage_idx];

		c_auto it = std::ranges::find(editor_storage.component_data_vec, ecs_component_id, &component_editor_data::ecs_component_id);

		AGE_ASSERT(ecs_component_id < editor_storage.component_data_vec.size<uint32>());
		AGE_ASSERT(it != editor_storage.component_data_vec.end());

		return util::to_string_view(it->names[0]);
	}

	namespace detail
	{
		void
		modify_entity_archetype(storage_editor_data& editor_storage, uint64 ecs_entity_id, uint64 new_ecs_archetype) noexcept
		{
			auto&& [old_arch_idx, old_editor_ent_idx] = editor_storage.ecs_ent_id_to_editor_location_map[ecs_entity_id];
			auto& old_arch_data						  = editor_storage.archetype_data_vec[old_arch_idx];

			if (old_arch_data.archetype == new_ecs_archetype) { return; }

			auto ent_data = std::move(old_arch_data.entity_data_vec[old_editor_ent_idx]);

			for (auto i = old_editor_ent_idx + 1; i < old_arch_data.entity_data_vec.size(); ++i)
			{
				old_arch_data.entity_data_vec[i - 1]															 = std::move(old_arch_data.entity_data_vec[i]);
				editor_storage.ecs_ent_id_to_editor_location_map[old_arch_data.entity_data_vec[i - 1].id].second = i - 1;
			}
			old_arch_data.entity_data_vec.pop_back();

			for (auto&& [arch_idx, arch_data] : editor_storage.archetype_data_vec | std::views::enumerate)
			{
				if (arch_data.archetype == new_ecs_archetype)
				{
					editor_storage.ecs_ent_id_to_editor_location_map[ecs_entity_id] = { static_cast<uint32>(arch_idx), arch_data.entity_data_vec.size<uint64>() };
					arch_data.entity_data_vec.emplace_back(std::move(ent_data));
					return;
				}
			}

			editor_storage.ecs_ent_id_to_editor_location_map[ecs_entity_id] = { editor_storage.archetype_data_vec.size<uint32>(), 0ull };
			auto& new_arch_data												= editor_storage.archetype_data_vec.emplace_back();
			new_arch_data.archetype											= new_ecs_archetype;
			new_arch_data.entity_data_vec.emplace_back(std::move(ent_data));
		}
	}	 // namespace detail

	void
	add_components(uint32 editor_scene_idx, uint32 editor_storage_idx, uint64 ecs_entity_id, uint64 ecs_archetype_to_add) noexcept
	{
		AGE_ASSERT(editor_scene_idx < g::current_game.scene_data_vec.size<uint32>());
		AGE_ASSERT(editor_storage_idx < g::current_game.scene_data_vec[editor_scene_idx].storage_data_vec.size<uint32>());

		auto& editor_scene	 = g::current_game.scene_data_vec[editor_scene_idx];
		auto& editor_storage = editor_scene.storage_data_vec[editor_storage_idx];

		g::host_ops.p_add_components(editor_scene.code_idx, editor_storage.code_idx, ecs_entity_id, ecs_archetype_to_add);

		c_auto new_ecs_archetype = g::host_ops.p_get_archetype(editor_scene.code_idx, editor_storage.code_idx, ecs_entity_id);

		detail::modify_entity_archetype(editor_storage, ecs_entity_id, new_ecs_archetype);
	}

	void
	remove_components(uint32 editor_scene_idx, uint32 editor_storage_idx, uint64 ecs_entity_id, uint64 ecs_archetype_to_remove) noexcept
	{
		AGE_ASSERT(editor_scene_idx < g::current_game.scene_data_vec.size<uint32>());
		AGE_ASSERT(editor_storage_idx < g::current_game.scene_data_vec[editor_scene_idx].storage_data_vec.size<uint32>());

		auto& editor_scene	 = g::current_game.scene_data_vec[editor_scene_idx];
		auto& editor_storage = editor_scene.storage_data_vec[editor_storage_idx];

		g::host_ops.p_remove_components(editor_scene.code_idx, editor_storage.code_idx, ecs_entity_id, ecs_archetype_to_remove);

		c_auto new_ecs_archetype = g::host_ops.p_get_archetype(editor_scene.code_idx, editor_storage.code_idx, ecs_entity_id);

		detail::modify_entity_archetype(editor_storage, ecs_entity_id, new_ecs_archetype);
	}

	uint32
	get_ecs_component_id(uint32 editor_scene_idx, uint32 editor_storage_idx, uint64 component_name_hash) noexcept
	{
		AGE_ASSERT(editor_scene_idx < g::current_game.scene_data_vec.size<uint32>());
		AGE_ASSERT(editor_storage_idx < g::current_game.scene_data_vec[editor_scene_idx].storage_data_vec.size<uint32>());

		auto& editor_scene	 = g::current_game.scene_data_vec[editor_scene_idx];
		auto& editor_storage = editor_scene.storage_data_vec[editor_storage_idx];

		auto ecs_component_data_it = std::ranges::find(editor_storage.component_data_vec, component_name_hash, &component_editor_data::ecs_component_name_hash);
		if (ecs_component_data_it == editor_storage.component_data_vec.end())
		{
			return age::get_invalid_idx<uint32>();
		}

		return ecs_component_data_it->ecs_component_id;
	}

	void*
	get_component_ptr(uint32 editor_scene_idx, uint32 editor_storage_idx, uint64 ecs_entity_id, uint32 ecs_cmponent_id) noexcept
	{
		AGE_ASSERT(editor_scene_idx < g::current_game.scene_data_vec.size<uint32>());
		AGE_ASSERT(editor_storage_idx < g::current_game.scene_data_vec[editor_scene_idx].storage_data_vec.size<uint32>());

		auto& editor_scene	 = g::current_game.scene_data_vec[editor_scene_idx];
		auto& editor_storage = editor_scene.storage_data_vec[editor_storage_idx];

		return g::host_ops.p_get_components(editor_scene.code_idx, editor_storage.code_idx, ecs_entity_id, ecs_cmponent_id);
	}

	void
	get_component_ptrs(uint32 editor_scene_idx, uint32 editor_storage_idx, uint64 ecs_entity_id, std::span<const uint64> cmp_name_hash_span, AGE_OUT std::span<void*> cmp_ptr_span) noexcept
	{
		AGE_ASSERT(editor_scene_idx < g::current_game.scene_data_vec.size<uint32>());
		AGE_ASSERT(editor_storage_idx < g::current_game.scene_data_vec[editor_scene_idx].storage_data_vec.size<uint32>());
		AGE_ASSERT(cmp_name_hash_span.size() == cmp_ptr_span.size());

		auto& editor_scene	 = g::current_game.scene_data_vec[editor_scene_idx];
		auto& editor_storage = editor_scene.storage_data_vec[editor_storage_idx];

		for (auto&& [cmp_name_hash, cmp_ptr] : std::views::zip(cmp_name_hash_span, cmp_ptr_span))
		{
			// must success
			c_auto& ecs_component_data = *std::ranges::find(editor_storage.component_data_vec, cmp_name_hash, &component_editor_data::ecs_component_name_hash);
			cmp_ptr					   = get_component_ptr(editor_scene_idx, editor_storage_idx, ecs_entity_id, ecs_component_data.ecs_component_id);
		}
	}

	void
	relocate_editor_entity(uint32 editor_scene_idx, uint32 editor_storage_idx, uint64 ecs_entity_id, uint64 new_archetype) noexcept
	{
		AGE_ASSERT(editor_scene_idx < g::current_game.scene_data_vec.size<uint32>());
		AGE_ASSERT(editor_storage_idx < g::current_game.scene_data_vec[editor_scene_idx].storage_data_vec.size<uint32>());

		auto& editor_scene	 = g::current_game.scene_data_vec[editor_scene_idx];
		auto& editor_storage = editor_scene.storage_data_vec[editor_storage_idx];

		detail::modify_entity_archetype(editor_storage, ecs_entity_id, new_archetype);
	}
}	 // namespace age::editor
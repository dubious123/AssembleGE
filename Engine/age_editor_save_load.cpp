#include "age_pch.hpp"
#include "age.hpp"

// file -> editor_data, editor_data -> file
namespace age::editor::detail
{
	game_editor_data
	read_game_proj(std::filesystem::path proj_path) noexcept
	{
		auto read_buf = asset::read_asset_file(proj_path.string());

		auto res = game_editor_data{};

		auto&& [game_proj_version, game_name_count, game_default_active_scene_idx, game_scene_count] = read_buf.read<uint32, uint32, uint32, uint32>();

		if (game_proj_version != config::editor_game_proj_version)
		{
			// AGE_ASSERT(false);
			//  todo, handle game_proj_migrate
		}

		res.default_active_scene_idx = game_default_active_scene_idx;

		res.names.reserve(game_name_count);
		for (auto _ : views::loop(game_name_count))
		{
			res.names.emplace_back(read_buf.read<std::array<char, config::max_game_name_len>>());
		}

		res.scene_data_vec.reserve(game_scene_count);
		for (auto i : views::loop(game_scene_count))
		{
			auto& scene = res.scene_data_vec.emplace_back();

			// todo, handle game_proj_migrate
			if (game_proj_version >= 2)
			{
				scene.cam = read_buf.read<camera_data>();
			}

			auto&& [scene_name_count, scene_ent_storage_count] = read_buf.read<uint32, uint32>();

			scene.names.reserve(scene_name_count);
			for (auto _ : views::loop(scene_name_count))
			{
				scene.names.emplace_back(read_buf.read<std::array<char, config::max_scene_name_len>>());
			}

			scene.storage_data_vec.reserve(scene_ent_storage_count);
			for (auto _ : views::loop(scene_ent_storage_count))
			{
				auto& storage = scene.storage_data_vec.emplace_back();

				auto&& [storage_name_count, component_count, archetype_count] = read_buf.read<uint32, uint32, uint64>();

				storage.names.reserve(storage_name_count);
				for (auto _ : views::loop(storage_name_count))
				{
					storage.names.emplace_back(read_buf.read<std::array<char, config::max_entity_storage_name_len>>());
				}

				storage.component_data_vec.reserve(component_count);

				for (auto _ : views::loop(component_count))
				{
					auto&& [component_name_count, component_version, component_byte_size] = read_buf.read<uint32, uint32, uint32>();

					auto& cmp_data = storage.component_data_vec.emplace_back(component_editor_data{ .version   = component_version,
																									.byte_size = component_byte_size });
					cmp_data.names.reserve(component_name_count);

					for (auto _ : views::loop(component_name_count))
					{
						cmp_data.names.emplace_back(read_buf.read<std::array<char, config::max_component_name_len>>());
					}
				}

				storage.archetype_data_vec.reserve(archetype_count);

				auto arch_entity_sum = 0ull;
				for (auto _ : views::loop(archetype_count))
				{
					auto&& [archetype_bits, arch_entity_count, archetype_name] =
						read_buf.read<uint64, uint64, std::array<char, config::max_archetype_name_len>>();

					arch_entity_sum += arch_entity_count;

					auto& arch = storage.archetype_data_vec.emplace_back(archetype_editor_data{
						.name			 = archetype_name,
						.archetype		 = archetype_bits,
						.entity_data_vec = age::vector<entity_editor_data>::gen_reserved(arch_entity_count),
					});

					for (auto _ : views::loop(arch_entity_count))
					{
						arch.entity_data_vec.emplace_back(entity_editor_data{
							.name = read_buf.read<std::array<char, config::max_entity_name_len>>() });
					}
				}
			}
		}

		return res;
	}

	void
	save_game_proj(const game_editor_data& game) noexcept
	{
		auto buf = byte_buf{};

		buf.write(config::editor_game_proj_version,
				  game.names.size<uint32>(),
				  game.default_active_scene_idx,
				  game.scene_data_vec.size<uint32>());


		for (c_auto& name : game.names)
		{
			buf.write(name);
		}

		for (c_auto& scene : game.scene_data_vec)
		{
			if constexpr (config::editor_game_proj_version >= 2)
			{
				buf.write(scene.cam);
			}

			buf.write(scene.names.size<uint32>(), scene.storage_data_vec.size<uint32>());

			for (c_auto& name : scene.names)
			{
				buf.write(name);
			}

			for (c_auto& storage : scene.storage_data_vec)
			{
				buf.write(storage.names.size<uint32>(),
						  storage.component_data_vec.size<uint32>(),
						  storage.archetype_data_vec.size<uint64>());

				for (c_auto& name : storage.names)
				{
					buf.write(name);
				}

				for (c_auto& component : storage.component_data_vec)
				{
					buf.write(component.names.size<uint32>(), component.version, component.byte_size);

					for (c_auto& name : component.names)
					{
						buf.write(name);
					}
				}

				auto arch_entity_sum = 0ull;
				for (c_auto& archetype : storage.archetype_data_vec)
				{
					buf.write(archetype.archetype, archetype.entity_data_vec.size<uint64>(), archetype.name);

					arch_entity_sum += archetype.entity_data_vec.size<uint64>();

					for (c_auto& entity : archetype.entity_data_vec)
					{
						buf.write(entity.name);
					}
				}

				AGE_ASSERT(storage.entity_count == arch_entity_sum);
			}
		}

		asset::write_asset_file(game.dir_path / std::format("{}{}", config::game_asset_tag, config::asset_extension),
								asset::get_default_file_header<asset::e::kind::editor_game>(buf.size()),
								buf.data());
	}
}	 // namespace age::editor::detail

// merge code_data, file_data
namespace age::editor::detail
{
	uint32
	resolve_archetype_collapse_via_console(
		const age::vector<archetype_editor_data>& file_archetype_vec,
		const age::vector<uint32>&				  collapse_vec) noexcept
	{
		auto named_indices	  = age::vector<uint32>{};
		auto unnamed_count	  = uint32{ 0 };
		auto unnamed_entities = uint64{ 0 };

		for (c_auto idx : collapse_vec)
		{
			c_auto& arch = file_archetype_vec[idx];
			if (arch.name[0] == '\0')
			{
				++unnamed_count;
				unnamed_entities += arch.entity_data_vec.size();
			}
			else
			{
				named_indices.emplace_back(idx);
			}
		}

		if (named_indices.empty())
		{
			return collapse_vec[0];
		}

		if (named_indices.size() == 1)
		{
			return named_indices[0];
		}

		std::println("");
		std::println("Archetype collapse detected:");
		for (auto&& [gi, idx] : named_indices | std::views::enumerate)
		{
			c_auto& arch = file_archetype_vec[idx];
			std::println("  [{}] \"{}\"  ({} entities)",
						 gi, arch.name, arch.entity_data_vec.size());
		}
		if (unnamed_count > 0)
		{
			std::println("  (unnamed archetypes: {}, entities: {})",
						 unnamed_count, unnamed_entities);
		}
		std::println("Which name to keep?");

		while (true)
		{
			std::print("> ");

			auto line = std::string{};
			if (std::getline(std::cin, line).fail())
			{
				std::cin.clear();
				std::println("input error - try again");
				continue;
			}

			auto choice = int32{ 0 };
			auto stream = std::istringstream{ line };
			if ((stream >> choice).fail())
			{
				std::println("invalid input - enter a number");
				continue;
			}

			if (choice < 0 or static_cast<uint32>(choice) >= named_indices.size())
			{
				std::println("out of range - try again");
				continue;
			}

			return named_indices[static_cast<uint32>(choice)];
		}
	}

	storage_editor_data
	merge_storage_data(storage_editor_data& code_storage, storage_editor_data& file_storage) noexcept
	{
		auto res  = storage_editor_data{};
		res.names = std::move(file_storage.names);
		res.names.reserve(res.names.size() + code_storage.names.size());
		res.code_idx = code_storage.code_idx;

		for (auto i : views::loop(code_storage.names.size()))
		{
			auto found = false;
			for (auto j : views::loop(res.names.size()))
			{
				if (std::strcmp(code_storage.names[i].data(), res.names[j].data()) == 0)
				{
					found = true;
					break;
				}
			}

			if (found is_false) { res.names.emplace_back(std::move(code_storage.names[i])); }
		}

		// res.entity_count = file_storage.entity_count;

		auto&& [file_to_code, unmatched_code, unmatched_file] = match_editor_names(code_storage.component_data_vec, file_storage.component_data_vec);
		resolve_unmatched_via_console(code_storage.component_data_vec, file_storage.component_data_vec,
									  file_to_code, unmatched_code, unmatched_file,
									  "storage component");

		{
			for (auto&& [file_idx, code_idx] : file_to_code | std::views::enumerate)
			{
				if (code_idx == -1) { continue; }

				auto& code_names = code_storage.component_data_vec[code_idx].names;
				auto& file_names = file_storage.component_data_vec[file_idx].names;

				age::ranges::erase_if(code_names, [&](c_auto& code_name) {
					for (c_auto& file_name : file_names)
					{
						if (std::strcmp(code_name.data(), file_name.data()) == 0) { return true; }
					}
					return false;
				});

				file_names.append_range(code_names | std::views::as_rvalue);
				code_names = std::move(file_names);
			}

			res.component_data_vec = std::move(code_storage.component_data_vec);
		}

		{
			auto new_archetype_mask = 0ull;
			for (auto cmp_idx : unmatched_file)
			{
				new_archetype_mask |= (1ull << cmp_idx);
			}

			new_archetype_mask = ~new_archetype_mask;

			auto new_archetype_map = age::unordered_map<uint64, age::vector<uint32>>();
			new_archetype_map.reserve(file_storage.archetype_data_vec.size());

			for (auto i : views::loop(file_storage.archetype_data_vec.size<uint32>()))
			{
				auto new_archetype = file_storage.archetype_data_vec[i].archetype & new_archetype_mask;
				new_archetype_map[new_archetype].emplace_back(i);
				file_storage.archetype_data_vec[i].archetype = new_archetype;
			}


			auto merged_archetype_vec = age::vector<archetype_editor_data>::gen_reserved(new_archetype_map.size());
			for (auto&& [_, collapse_vec] : new_archetype_map)
			{
				if (collapse_vec.size() > 1)
				{
					auto entity_count_sum = 0ull;
					for (c_auto idx : collapse_vec)
					{
						entity_count_sum += file_storage.archetype_data_vec[idx].entity_data_vec.size();
					}

					uint32 chosen_idx = resolve_archetype_collapse_via_console(file_storage.archetype_data_vec, collapse_vec);

					auto merged_archetype = archetype_editor_data{
						.name			 = file_storage.archetype_data_vec[chosen_idx].name,
						.archetype		 = file_storage.archetype_data_vec[chosen_idx].archetype,
						.entity_data_vec = age::vector<entity_editor_data>::gen_reserved(entity_count_sum)
					};

					for (c_auto idx : collapse_vec)
					{
						merged_archetype.entity_data_vec.append_range(file_storage.archetype_data_vec[idx].entity_data_vec | std::views::as_rvalue);
					}

					merged_archetype_vec.emplace_back(std::move(merged_archetype));
				}
				else if (collapse_vec.size() == 1)
				{
					merged_archetype_vec.emplace_back(std::move(file_storage.archetype_data_vec[collapse_vec[0]]));
				}
				else
				{
					AGE_UNREACHABLE();
				}
			}

			for (auto& merged_archetype : merged_archetype_vec)
			{
				auto new_archetype = uint64{ 0 };
				for (auto bits = merged_archetype.archetype; bits != 0; bits &= bits - 1)
				{
					c_auto file_bit = std::countr_zero(bits);
					c_auto code_bit = file_to_code[file_bit];
					if (code_bit < 0) { continue; }
					new_archetype |= 1ull << static_cast<uint32>(code_bit);
				}

				merged_archetype.archetype = new_archetype;
			}

			res.archetype_data_vec = std::move(merged_archetype_vec);
		}


		return res;
	}

	scene_editor_data
	merge_scene_data(scene_editor_data& code_scene, scene_editor_data& file_scene) noexcept
	{
		auto res	 = scene_editor_data{};
		res.names	 = std::move(file_scene.names);
		res.code_idx = code_scene.code_idx;

		res.names.reserve(res.names.size() + code_scene.names.size());

		res.cam = file_scene.cam;

		for (auto i : views::loop(code_scene.names.size<uint32>()))
		{
			auto found = false;
			for (auto j : views::loop(res.names.size<uint32>()))
			{
				if (std::strcmp(code_scene.names[i].data(), res.names[j].data()) == 0)
				{
					found = true;
					break;
				}
			}

			if (found is_false) { res.names.emplace_back(std::move(code_scene.names[i])); }
		}

		auto&& [file_to_code, unmatched_code, unmatched_file] = match_editor_names(code_scene.storage_data_vec, file_scene.storage_data_vec);

		resolve_unmatched_via_console(code_scene.storage_data_vec, file_scene.storage_data_vec,
									  file_to_code, unmatched_code, unmatched_file,
									  "entity storage");

		res.storage_data_vec.reserve(file_to_code.size() + unmatched_code.size());

		for (auto&& [file_idx, code_idx] : file_to_code | std::views::enumerate)
		{
			if (code_idx < 0) { continue; }

			res.storage_data_vec.emplace_back(merge_storage_data(code_scene.storage_data_vec[code_idx], file_scene.storage_data_vec[file_idx]));
		}

		for (c_auto code_idx : unmatched_code)
		{
			res.storage_data_vec.emplace_back(std::move(code_scene.storage_data_vec[code_idx]));
		}

		return res;
	}

	game_editor_data
	merge_game_data(game_editor_data& code_game, game_editor_data& file_game) noexcept
	{
		auto res = game_editor_data{};

		{
			res.names = std::move(file_game.names);
			res.names.reserve(res.names.size() + code_game.names.size());

			for (auto i : views::loop(code_game.names.size<uint32>()))
			{
				auto found = false;
				for (auto j : views::loop(res.names.size<uint32>()))
				{
					if (std::strcmp(code_game.names[i].data(), res.names[j].data()) == 0)
					{
						found = true;
						break;
					}
				}

				if (found is_false) { res.names.emplace_back(std::move(code_game.names[i])); }
			}
		}


		auto&& [file_to_code, unmatched_code, unmatched_file] = match_editor_names(code_game.scene_data_vec, file_game.scene_data_vec);

		resolve_unmatched_via_console(code_game.scene_data_vec, file_game.scene_data_vec,
									  file_to_code, unmatched_code, unmatched_file,
									  "scene");

		res.scene_data_vec.reserve(file_to_code.size() + unmatched_code.size());

		auto active_scene_idx = 0u;

		for (auto&& [file_idx, code_idx] : file_to_code | std::views::enumerate)
		{
			if (code_idx < 0) { continue; }

			if (file_idx == file_game.default_active_scene_idx)
			{
				active_scene_idx = res.scene_data_vec.size<uint32>();
			}

			res.scene_data_vec.emplace_back(merge_scene_data(code_game.scene_data_vec[code_idx], file_game.scene_data_vec[file_idx]));
		}

		for (c_auto code_idx : unmatched_code)
		{
			res.scene_data_vec.emplace_back(std::move(code_game.scene_data_vec[code_idx]));
		}

		res.default_active_scene_idx = active_scene_idx;
		res.current_active_scene_idx = active_scene_idx;

		return res;
	}
}	 // namespace age::editor::detail

namespace age::editor::detail
{
	void
	register_entity(storage_editor_data& editor_storage, uint32 editor_arch_idx, uint64 editor_ent_idx, uint64 ecs_entity_id) noexcept
	{
		auto& editor_arch_data									= editor_storage.archetype_data_vec[editor_arch_idx];
		editor_arch_data.entity_data_vec[editor_ent_idx].id		= ecs_entity_id;
		editor_storage.id_to_editor_location_map[ecs_entity_id] = { editor_arch_idx, editor_ent_idx };
		++editor_storage.entity_count;
	}

	void
	unregister_entity(storage_editor_data& editor_storage, uint32 editor_arch_idx, uint64 editor_ent_idx, uint64 ecs_entity_id) noexcept
	{
		auto& arch_data = editor_storage.archetype_data_vec[editor_arch_idx];

		for (auto i = editor_ent_idx + 1; i < arch_data.entity_data_vec.size(); ++i)
		{
			arch_data.entity_data_vec[i - 1]													 = std::move(arch_data.entity_data_vec[i]);
			editor_storage.id_to_editor_location_map[arch_data.entity_data_vec[i - 1].id].second = i - 1;
		}
		arch_data.entity_data_vec.pop_back();
		editor_storage.id_to_editor_location_map.erase(ecs_entity_id);
		--editor_storage.entity_count;
	}

	void
	re_register_entity(storage_editor_data& editor_storage, uint64 ecs_entity_id, uint64 new_archetype) noexcept
	{
		auto&& [old_arch_idx, old_ent_idx] = editor_storage.id_to_editor_location_map[ecs_entity_id];
		auto& old_arch_data				   = editor_storage.archetype_data_vec[old_arch_idx];

		if (old_arch_data.archetype == new_archetype) { return; }

		auto ent_data = std::move(old_arch_data.entity_data_vec[old_ent_idx]);

		for (auto i = old_ent_idx + 1; i < old_arch_data.entity_data_vec.size(); ++i)
		{
			old_arch_data.entity_data_vec[i - 1]													 = std::move(old_arch_data.entity_data_vec[i]);
			editor_storage.id_to_editor_location_map[old_arch_data.entity_data_vec[i - 1].id].second = i - 1;
		}
		old_arch_data.entity_data_vec.pop_back();

		for (auto&& [arch_idx, arch_data] : editor_storage.archetype_data_vec | std::views::enumerate)
		{
			if (arch_data.archetype == new_archetype)
			{
				editor_storage.id_to_editor_location_map[ecs_entity_id] = { static_cast<uint32>(arch_idx), arch_data.entity_data_vec.size<uint64>() };
				arch_data.entity_data_vec.emplace_back(std::move(ent_data));
				return;
			}
		}

		editor_storage.id_to_editor_location_map[ecs_entity_id] = { editor_storage.archetype_data_vec.size<uint32>(), 0ull };
		auto& new_arch_data										= editor_storage.archetype_data_vec.emplace_back();
		new_arch_data.archetype									= new_archetype;
		new_arch_data.entity_data_vec.emplace_back(std::move(ent_data));
	}
}	 // namespace age::editor::detail
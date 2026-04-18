#pragma once
#include "age.hpp"

namespace age::editor
{
	struct entity_editor_data
	{
		uint64										  id;
		std::array<char, config::max_entity_name_len> name;
	};

	struct archetype_editor_data
	{
		std::array<char, config::max_archetype_name_len> name;	  // editor_only name
		uint64											 archetype;
		age::vector<entity_editor_data>					 entity_data_vec;
	};

	struct component_editor_data
	{
		age::vector<std::array<char, config::max_component_name_len>> names;
		uint32														  version;
		uint32														  byte_size;
	};

	struct storage_editor_data
	{
		age::vector<std::array<char, config::max_entity_storage_name_len>> names;
		uint32															   code_idx;
		uint64															   entity_count;
		age::vector<component_editor_data>								   component_data_vec;
		age::vector<archetype_editor_data>								   archetype_data_vec;

		age::unordered_map<uint64, std::pair<uint32, uint64>> id_to_editor_location_map;	// editor_only map
	};

	struct scene_editor_data
	{
		age::vector<std::array<char, config::max_scene_name_len>> names;
		uint32													  code_idx;
		bool													  loaded = false;
		age::vector<storage_editor_data>						  storage_data_vec;
	};

	struct game_editor_data
	{
		age::vector<std::array<char, config::max_game_name_len>> names;
		uint32													 default_active_scene_idx;
		age::vector<scene_editor_data>							 scene_data_vec;
	};
}	 // namespace age::editor

namespace age::editor::g
{
	inline auto select_vec	= age::vector<uint64>{};
	inline auto command_buf = ecs::command_buffer{};

	inline auto current_scene_idx	= 0u;
	inline auto current_storage_idx = 0u;

	inline auto entity_name_map = age::unordered_map<uint64, std::array<char, config::max_entity_name_len>>{};

	inline auto current_game = game_editor_data{};
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

namespace age::editor
{
	template <typename t_renderer>
	struct rw_context
	{
		uint32		version;
		t_renderer& renderer;
	};

	FORCE_INLINE constexpr auto
	get_rw_context(uint32 version, auto&& renderer) noexcept
	{
		return rw_context<BARE_OF(renderer)>{ .version = version, .renderer = FWD(renderer) };
	}
}	 // namespace age::editor

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
				 .names		= ecs::get_component_name<typename t_archetype_traits::template t_component<i>>() | std::ranges::to<age::vector<std::array<char, config::max_component_name_len>>>(),
				 .version	= ecs::get_component_version<typename t_archetype_traits::template t_component<i>>(),
				 .byte_size = ecs::get_byte_size<typename t_archetype_traits::template t_component<i>>(),
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
				  .names	= std::get<i>(scene.storage_names()) | std::ranges::to<age::vector<std::array<char, config::max_entity_storage_name_len>>>(),
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

		res.names					 = game.age_editor_name() | std::ranges::to<age::vector<std::array<char, config::max_game_name_len>>>();
		res.default_active_scene_idx = 0;

		res.scene_data_vec.clear();

		res.scene_data_vec.reserve(game.scene_count());
		[]<auto... i>(std::index_sequence<i...>, auto& res, auto& game) {
			((res.scene_data_vec.emplace_back(scene_editor_data{
				  .names	= std::get<i>(game.scene_names()) | std::ranges::to<age::vector<std::array<char, config::max_scene_name_len>>>(),
				  .code_idx = i,
				  .loaded	= false,
			  }),

			  gen_scene_data(res.scene_data_vec.back(), std::get<i>(game.scenes()), i)),
			 ...);
		}(std::make_index_sequence<game.scene_count()>{}, res, game);

		return res;
	}

	inline game_editor_data
	read_game_proj(std::filesystem::path proj_path) noexcept
	{
		auto h_proj_file = asset::load_from_path(proj_path.string());
		auto read_buf	 = h_proj_file->get_payload_read_buf();

		auto res = game_editor_data{};

		auto&& [game_proj_version, game_name_count, game_active_scene_idx, game_scene_count] = read_buf.read<uint32, uint32, uint32, uint32>();

		if (game_proj_version != config::editor_game_proj_version)
		{
			AGE_ASSERT(false);
			// todo, handle game_proj_migrate
		}

		res.default_active_scene_idx = game_active_scene_idx;

		res.names.reserve(game_name_count);
		for (auto _ : views::loop(game_name_count))
		{
			res.names.emplace_back(read_buf.read<std::array<char, config::max_game_name_len>>());
		}

		res.scene_data_vec.reserve(game_scene_count);
		for (auto i : views::loop(game_scene_count))
		{
			auto& scene = res.scene_data_vec.emplace_back();

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

				auto&& [storage_name_count, component_count, storage_entity_count, archetype_count] = read_buf.read<uint32, uint32, uint64, uint64>();
				storage.entity_count																= storage_entity_count;

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

				for (auto _ : views::loop(archetype_count))
				{
					auto&& [archetype_bits, arch_entity_count, archetype_name] =
						read_buf.read<uint64, uint64, std::array<char, config::max_archetype_name_len>>();

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

	template <typename t_editor_data>
	decltype(auto)
	match_editor_names(const age::vector<t_editor_data>& code_data_vec, const age::vector<t_editor_data>& file_data_vec) noexcept
	{
		auto file_to_code	= age::dynamic_array<int32>::gen_sized_copy(file_data_vec.size(), -1);
		auto unmatched_code = age::vector<uint32>{};
		auto unmatched_file = age::vector<uint32>{};

		auto code_matched = age::dynamic_array<bool>::gen_sized_copy(code_data_vec.size(), false);

		for (auto&& [file_idx, file_data] : file_data_vec | std::views::enumerate)
		{
			auto found = false;
			for (auto&& [code_idx, matched] : code_matched | std::views::enumerate)
			{
				if (matched) { continue; }

				if (std::ranges::find_first_of(file_data.names, code_data_vec[code_idx].names,
											   [](c_auto& lhs, c_auto& rhs) { return std::strcmp(lhs.data(), rhs.data()) == 0; })
					!= file_data.names.end())
				{
					file_to_code[file_idx] = static_cast<int32>(code_idx);
					matched				   = true;
					found				   = true;
					break;
				}
			}

			if (found is_false)
			{
				unmatched_file.emplace_back(static_cast<uint32>(file_idx));
			}
		}

		for (auto&& [code_idx, matched] : code_matched | std::views::enumerate)
		{
			if (matched is_false)
			{
				unmatched_code.emplace_back(static_cast<uint32>(code_idx));
			}
		}

		return std::make_tuple(std::move(file_to_code), std::move(unmatched_code), std::move(unmatched_file));
	}

	template <typename t_editor_data>
	void
	resolve_unmatched_via_console(
		const age::vector<t_editor_data>& code_data_vec,
		const age::vector<t_editor_data>& file_data_vec,
		age::dynamic_array<int32>&		  file_to_code,
		age::vector<uint32>&			  unmatched_code,
		age::vector<uint32>&			  unmatched_file,
		std::string_view				  kind_label) noexcept
	{
		auto new_unmatched_code = age::vector<uint32>{};
		auto matched_file_mask	= age::dynamic_array<bool>::gen_sized_copy(file_data_vec.size(), false);

		for (c_auto code_idx : unmatched_code)
		{
			// header
			std::println("");
			std::println("New code {} detected: \"{}\"",
						 kind_label,
						 code_data_vec[code_idx].names[0]);

			if (code_data_vec[code_idx].names.size() > 1)
			{
				std::print("  aliases: ");
				for (auto&& [i, code_alias] : code_data_vec[code_idx].names | std::views::drop(1) | std::views::enumerate)
				{
					if (i > 0) { std::print(", "); }
					std::print("\"{}\"", code_alias);
				}
				std::println("");
			}


			// available file candidates
			auto available_count = uint32{ 0 };
			for (c_auto file_idx : unmatched_file)
			{
				if (matched_file_mask[file_idx]) { continue; }
				std::println("  [{}] {}", file_idx, file_data_vec[file_idx].names[0]);
				++available_count;
			}

			if (available_count == 0)
			{
				std::println("  (no file data available - will be created fresh)");
			}

			std::println("  [-1] Create fresh (no pairing)");

			// input loop
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

				if (choice == -1)
				{
					new_unmatched_code.emplace_back(code_idx);
					break;
				}

				if (choice < 0 || static_cast<uint32>(choice) >= file_data_vec.size())
				{
					std::println("out of range - try again");
					continue;
				}

				c_auto file_idx = static_cast<uint32>(choice);
				if (matched_file_mask[file_idx])
				{
					std::println("already paired - try again");
					continue;
				}

				c_auto in_unmatched = std::ranges::find(unmatched_file, file_idx) != unmatched_file.end();
				if (in_unmatched is_false)
				{
					std::println("not an unmatched file - try again");
					continue;
				}

				file_to_code[file_idx]		= static_cast<int32>(code_idx);
				matched_file_mask[file_idx] = true;
				break;
			}
		}

		unmatched_code = std::move(new_unmatched_code);

		auto new_unmatched_file = age::vector<uint32>{};
		for (c_auto file_idx : unmatched_file)
		{
			if (matched_file_mask[file_idx] is_false)
			{
				new_unmatched_file.emplace_back(file_idx);
			}
		}
		unmatched_file = std::move(new_unmatched_file);

		if (unmatched_file.empty() is_false)
		{
			std::println("");
			std::println("The following {} entries exist in file data but not in code:", kind_label);
			for (c_auto file_idx : unmatched_file)
			{
				std::println("  \"{}\"", file_data_vec[file_idx].names[0]);
			}
			std::println("");
			std::println("These may have been intentionally deleted from code, or you forgot to add an alias after renaming.");
			std::println("  [y] Confirm deletion - data will be permanently lost on next save");
			std::println("  [n] Abort merge - fix code first, then retry");

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

				c_auto first = std::ranges::find_if(line, [](char c) { return std::isspace(static_cast<unsigned char>(c)) is_false; });
				if (first == line.end())
				{
					std::println("enter y or n");
					continue;
				}

				c_auto answer = static_cast<char>(std::tolower(static_cast<unsigned char>(*first)));

				if (answer == 'y')
				{
					std::println("confirmed - {} orphan entries will be dropped on next save", unmatched_file.size());
					break;
				}
				if (answer == 'n')
				{
					std::println("merge aborted - fix code and retry");
					std::exit(0);
				}

				std::println("invalid input - enter y or n");
			}
		}
	}

	inline uint32
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

	inline storage_editor_data
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

		res.entity_count = file_storage.entity_count;

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

	inline scene_editor_data
	merge_scene_data(scene_editor_data& code_scene, scene_editor_data& file_scene) noexcept
	{
		auto res	 = scene_editor_data{};
		res.names	 = std::move(file_scene.names);
		res.code_idx = code_scene.code_idx;

		res.names.reserve(res.names.size() + code_scene.names.size());

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

	inline game_editor_data
	merge_game_data(game_editor_data& code_game, game_editor_data& file_game) noexcept
	{
		auto res = game_editor_data{};

		auto&& [file_to_code, unmatched_code, unmatched_file] = match_editor_names(code_game.scene_data_vec, file_game.scene_data_vec);

		resolve_unmatched_via_console(code_game.scene_data_vec, file_game.scene_data_vec,
									  file_to_code, unmatched_code, unmatched_file,
									  "scene");

		res.scene_data_vec.reserve(file_to_code.size() + unmatched_code.size());

		for (auto&& [file_idx, code_idx] : file_to_code | std::views::enumerate)
		{
			if (code_idx < 0) { continue; }

			res.scene_data_vec.emplace_back(merge_scene_data(code_game.scene_data_vec[code_idx], file_game.scene_data_vec[file_idx]));
		}

		for (c_auto code_idx : unmatched_code)
		{
			res.scene_data_vec.emplace_back(std::move(code_game.scene_data_vec[code_idx]));
		}

		return res;
	}
}	 // namespace age::editor::detail

namespace age::editor
{
	namespace detail
	{
		template <bool is_dir = true>
		std::filesystem::path
		resolve_path_by_names(const std::filesystem::path& parent,
							  c_auto&					   names,
							  std::string_view			   suffix = {}) noexcept
		{
			auto make_path = [](c_auto& parent, c_auto& n, c_auto& suffix) {
				if constexpr (is_dir)
				{
					return parent / n.data();
				}
				else
				{
					return parent / std::format("{}{}", n.data(), suffix);
				}
			};

			auto primary = make_path(parent, names[0], suffix);
			auto found	 = std::filesystem::path{};

			for (c_auto& n : names)
			{
				auto candidate = make_path(parent, n, suffix);
				if (std::filesystem::exists(candidate))
				{
					found = candidate;
					break;
				}
			}

			if (found.empty())
			{
				if constexpr (is_dir)
				{
					std::filesystem::create_directories(primary);
				}
			}
			else if (found != primary)
			{
				std::filesystem::rename(found, primary);
			}

			return primary;
		}
	}	 // namespace detail

	inline age::byte_buf
	serialize_storage_data(c_auto& ecs_entity_storage, const storage_editor_data& storage, auto& renderer) noexcept
	{
		using t_storage			 = BARE_OF(ecs_entity_storage);
		using t_archetype_traits = typename t_storage::t_archetype_traits;
		using t_local_cmp_idx	 = typename t_storage::t_local_cmp_idx;
		auto buf				 = age::byte_buf{};

		// component section
		buf.write(storage.component_data_vec.size<uint32>());
		for (c_auto& cmp : storage.component_data_vec)
		{
			buf.write(cmp.names[0]);
			buf.write(cmp.byte_size);
			buf.write(cmp.version);
		}

		buf.write(storage.archetype_data_vec.size<uint32>());


		// archetype section
		for (c_auto& arch : storage.archetype_data_vec)
		{
			buf.write(arch.name);
			buf.write(arch.archetype);
			buf.write(arch.entity_data_vec.size<uint64>());

			auto archetype_byte_size = 0ull;
			for (auto storage_cmp_idx : views::each_set_bit_idx(arch.archetype))
			{
				archetype_byte_size += storage.component_data_vec[storage_cmp_idx].byte_size;
			}

			c_auto buf_base_pos = buf.size();
			buf.reserve(buf_base_pos + archetype_byte_size * arch.entity_data_vec.size());

			for (auto& block : ecs_entity_storage | ecs::each_block(arch.archetype))
			{
				AGE_ASSERT(arch.archetype == block.local_archetype());

				for (c_auto local_ent_id : views::loop(block.entity_count()))
				{
					c_auto ent_id = block.ent_id(local_ent_id);

					auto it = storage.id_to_editor_location_map.find(ent_id);
					AGE_ASSERT(it != storage.id_to_editor_location_map.end());
					AGE_ASSERT(arch.archetype == storage.archetype_data_vec[it->second.first].archetype);
					c_auto editor_idx = it->second.second;

					buf.move_write_pos(buf_base_pos + editor_idx * archetype_byte_size);

					for (auto [local_cmp_idx, storage_cmp_idx] : views::each_set_bit_idx(arch.archetype) | std::views::enumerate)
					{
						c_auto* p_cmp = block.cmp_ptr(static_cast<t_local_cmp_idx>(local_cmp_idx), local_ent_id);
						t_archetype_traits::visit_component_at(storage_cmp_idx, []<typename t_cmp>(auto&&... arg) { ecs::serialize_component<t_cmp>(FWD(arg)...); }, buf, p_cmp, get_rw_context(storage.component_data_vec[storage_cmp_idx].version, renderer));
					}
				}
			}

			buf.move_write_pos(buf_base_pos + archetype_byte_size * arch.entity_data_vec.size());
		}

		return buf;
	}

	void
	load_game(auto& game, std::filesystem::path root_dir, auto& renderer) noexcept
	{
		auto code_game_data = detail::gen_game_data(game);

		c_auto& names	 = game.age_editor_name();
		c_auto	game_dir = detail::resolve_path_by_names(root_dir, names);

		auto proj_file_name = game_dir / std::format("{}{}", config::game_asset_tag, config::asset_extension);

		if (std::filesystem::exists(proj_file_name))
		{
			auto file_game_data = detail::read_game_proj(proj_file_name);

			g::current_game = detail::merge_game_data(code_game_data, file_game_data);
		}
		else
		{
			g::current_game = std::move(code_game_data);
		}

		for (auto&& [scene_idx, scene] : g::current_game.scene_data_vec | std::views::enumerate)
		{
			c_auto scene_dir = detail::resolve_path_by_names(game_dir, scene.names);

			for (auto& storage : scene.storage_data_vec)
			{
				c_auto storage_path = detail::resolve_path_by_names<false>(scene_dir, storage.names, std::format("{}{}", config::editor_ent_storage_asset_tag, config::asset_extension));

				if (std::filesystem::exists(storage_path) is_false)
				{
					auto   buf				 = game.visit_storage_at(scene.code_idx, storage.code_idx, AGE_FUNC(serialize_storage_data), storage, renderer);
					c_auto asset_file_header = asset::get_default_file_header<asset::e::kind::editor_entity_storage>(buf.size());
					asset::write_to_file(storage_path, asset_file_header, *buf.data());
				}


				if (g::current_scene_idx == scene_idx)
				{
					// todo load?
				}
			}
		}

		std::filesystem::remove_all(root_dir);
	}
}	 // namespace age::editor

namespace age::editor
{
	struct component_data
	{
		uint32						  version = 2;
		std::string_view			  name;
		age::vector<std::string_view> alias;
		std::size_t					  byte_size;
	};

	template <typename t_cmp>
	component_data
	gen_component_data() noexcept
	{
		// return component_data{
		//	.version = ecs::get_component_version<t_cmp>(),
		//	.name	 = ecs::get_component_name<t_cmp>(),
		//	.alias	 = ecs::get_component_alias<t_cmp>()
		// };

		return component_data{
			.version = 0,
			.name	 = ecs::get_component_name<t_cmp>(),
			.alias	 = { "pos", "position_3d" }
		};
	}

	struct entity_storage_data
	{
		uint64												  entity_count = 0;
		std::array<char, config::max_entity_storage_name_len> storage_name;
		age::vector<const char*>							  component_name_vec;
		age::unordered_map<uint64, age::byte_buf>			  archetype_blob;
	};

	entity_storage_data
	gen_entity_storage_data(auto&& storage, auto&& rw_ctx) noexcept
	{
		using t_storage			 = BARE_OF(storage);
		using t_archetype_traits = typename t_storage::t_archetype_traits;
		using t_local_cmp_idx	 = typename t_archetype_traits::t_local_cmp_idx;
		using t_storage_cmp_idx	 = typename t_archetype_traits::t_storage_cmp_idx;

		auto res = entity_storage_data{ .entity_count = storage.entity_count() };


		//		for (auto cmp_id : views::loop(t_archetype_traits::cmp_count()))
		//		{
		//			res.component_name_vec.emplace_back(t_archetype_traits::get_component_name(cmp_id));
		//		}
		//
		//
		//		for (auto& block : storage | ecs::each_block(ecs::query<ecs::sv_archetype>()))
		//		{
		//			auto archetype = block.local_archetype();
		//
		//			auto& buf = res.archetype_blob[archetype];
		//
		//			for (auto local_ent_id : views::loop(block.entity_count()))
		//			{
		//				auto ent_id = block.ent_id(local_ent_id);
		//
		//				auto ent_name = g::entity_name_map[g::current_storage_idx][ent_id];
		//
		//				buf.write(ent_name);
		//
		//
		//				{
		//					for (auto [local_cmp_idx, storage_cmp_idx] : age::views::each_set_bit_idx(archetype) | std::views::enumerate)
		//					{
		//						switch (static_cast<t_storage_cmp_idx>(storage_cmp_idx))
		//						{
		// #define X(N)                                                                                                                                                              \
//	case N:                                                                                                                                                               \
//	{                                                                                                                                                                     \
//		if constexpr (N < t_archetype_traits::cmp_count())                                                                                                                \
//		{                                                                                                                                                                 \
//			ecs::serialize_component<typename t_archetype_traits::t_component<N>>(buf, block.cmp_ptr(static_cast<t_local_cmp_idx>(local_cmp_idx), local_ent_id), rw_ctx); \
//			break;                                                                                                                                                        \
//		}                                                                                                                                                                 \
//		else                                                                                                                                                              \
//		{                                                                                                                                                                 \
//			AGE_UNREACHABLE();                                                                                                                                            \
//		}                                                                                                                                                                 \
//	}
		//							__X_REPEAT_LIST_512
		// #undef X
		//						default:
		//						{
		//							AGE_UNREACHABLE();
		//						}
		//						}
		//					}
		//				}
		//			}
		//		}

		return res;
	}

	void
	save_scene_as(const std::array<char, config::max_scene_name_len>& scene_name, std::filesystem::path directory, auto& renderer, ecs::cx_entity_storage auto&&... storage) noexcept
	{
		auto buf = byte_buf{};

		// 1) Reserve slot for asset header (written last or patched)
		auto scene_header = asset::editor::scene_asset_header{
			.version = config::version_major,
			//.name[config::max_scene_name_len],
			.entity_storage_count = sizeof...(storage),
			//.component_count,
			//.archetype_count,
			//.entity_storage_buffer_byte_offset,
			//.component_data_idx_buffer_byte_offset,
			//.component_data_buffer_byte_offset,
			//.archetype_data_buffer_byte_offset,
		};


		buf.write(scene_header);

		//// 2) Per-storage blocks
		// auto rw_ctx = editor::rw_context{ .renderer = renderer, .version = 0 };
		//(editor::write_storage_to_buf(buf, storage, rw_ctx), ...);

		//// 3) Patch asset header now that we know final state
		// auto* p_header = std::start_lifetime_as<editor::scene_asset_header>(buf.data());
		//*p_header	   = editor::scene_asset_header{
		//	.magic		   = config::age_asset_magic,
		//	.version	   = editor::scene_asset_version,
		//	.asset_type	   = asset_type::editor_scene,
		//	.storage_count = sizeof...(storage),
		//	.name		   = scene_name,
		// };

		// 4) File write
		c_auto asset_path = directory / std::format("{}.editor_scene.age_asset", std::string_view{ scene_name.data() });
		asset::write_to_file(asset_path.string(), std::span{ buf.data(), buf.size() });
	}
}	 // namespace age::editor
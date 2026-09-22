#include "age_pch.hpp"
#include "age.hpp"

// common utils
namespace age::editor::detail
{
	// return relative to .exe
	template <bool is_dir = true>
	std::string
	resolve_path_by_names(std::string_view parent,
						  c_auto&		   names,
						  std::string_view suffix = {}) noexcept
	{
		auto make_path = [](c_auto& parent, c_auto& name, c_auto& suffix) {
			if constexpr (is_dir)
			{
				return fs::join(parent, name.data());
			}
			else
			{
				return fs::join(parent, std::format("{}{}", name.data(), suffix));
			}
		};

		auto primary = make_path(parent, names[0], suffix);
		auto found	 = std::string{};

		for (c_auto& name : names)
		{
			auto candidate = make_path(parent, name, suffix);
			if (fs::exists(candidate))
			{
				found = std::move(candidate);
				break;
			}
		}

		if (found.empty())
		{
			if constexpr (is_dir)
			{
				fs::create_dir(primary);
			}
		}
		else if (found != primary)
		{
			fs::rename(found, primary);
		}

		return primary;
	}
}	 // namespace age::editor::detail

// save_game
namespace age::editor::detail
{
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

		asset::write_asset_file(asset::to_root_relative(fs::join(game.dir_path, std::format("{}{}", config::game_asset_tag, config::asset_extension))),
								asset::get_default_file_header(asset::e::kind::editor_game, buf.size(), config::editor_game_proj_version),
								buf.data());
	}

	age::byte_buf
	serialize_storage_data(const uint32 editor_scene_idx, const uint32 editor_storage_idx) noexcept
	{
		c_auto& editor_scene   = g::current_game.scene_data_vec[editor_scene_idx];
		c_auto& editor_storage = editor_scene.storage_data_vec[editor_storage_idx];
		auto	buf			   = age::byte_buf{};

		// component section
		buf.write(editor_storage.component_data_vec.size<uint32>());
		for (c_auto& cmp : editor_storage.component_data_vec)
		{
			buf.write(cmp.names[0]);
			buf.write(cmp.byte_size);
			buf.write(cmp.version);
		}

		buf.write(editor_storage.archetype_data_vec.size<uint32>());

		// archetype section
		for (c_auto& arch : editor_storage.archetype_data_vec)
		{
			buf.write(arch.name);
			buf.write(arch.archetype);
			buf.write(arch.entity_data_vec.size<uint64>());

			auto archetype_byte_size = 0ull;
			for (c_auto storage_cmp_idx : views::each_set_bit_idx(arch.archetype))
			{
				archetype_byte_size += editor_storage.component_data_vec[storage_cmp_idx].byte_size;
			}

			c_auto buf_base_pos = buf.size();
			buf.reserve(buf_base_pos + archetype_byte_size * arch.entity_data_vec.size());

			g::host_ops.p_serialize_entity_storage(editor_storage, editor_scene.code_idx, editor_storage.code_idx, arch.archetype, archetype_byte_size, AGE_INOUT buf);

			buf.move_write_pos(buf_base_pos + archetype_byte_size * arch.entity_data_vec.size());
		}

		return buf;
	}
}	 // namespace age::editor::detail

namespace age::editor
{
	void
	save_game() noexcept
	{
		asset::registry::save();
		detail::save_game_proj(g::current_game);

		c_auto& active_scene = g::current_game.scene_data_vec[g::current_game.current_active_scene_idx];

		for (const auto&& [editor_storage_idx, editor_storage] : active_scene.storage_data_vec | views::enumerate<uint32>)
		{
			c_auto storage_path		 = detail::resolve_path_by_names<false>(active_scene.dir_path, editor_storage.names, std::format("{}{}", config::editor_ent_storage_asset_tag, config::asset_extension));
			c_auto buf				 = detail::serialize_storage_data(g::current_game.current_active_scene_idx, editor_storage_idx);
			c_auto asset_file_header = asset::get_default_file_header(asset::e::kind::editor_entity_storage, buf.size(), config::editor_ent_storage_asset_version);
			asset::write_asset_file(asset::to_root_relative(storage_path), asset_file_header, buf.data());
		}
	}
}	 // namespace age::editor

// load game
namespace age::editor::detail
{
	game_editor_data
	read_game_proj(std::string_view proj_path) noexcept
	{
		auto  file_data = asset::read_asset_file(proj_path);
		auto& buf		= file_data.buf;

		auto res = game_editor_data{};

		auto&& [game_proj_version, game_name_count, game_default_active_scene_idx, game_scene_count] = buf.read<uint32, uint32, uint32, uint32>();

		if (game_proj_version != config::editor_game_proj_version)
		{
			// AGE_ASSERT(false);
			//  todo, handle game_proj_migrate
		}

		res.default_active_scene_idx = game_default_active_scene_idx;

		res.names.reserve(game_name_count);
		for (auto _ : views::loop(game_name_count))
		{
			res.names.emplace_back(buf.read<age::array<char, config::max_game_name_len>>());
		}

		res.scene_data_vec.reserve(game_scene_count);
		for (auto i : views::loop(game_scene_count))
		{
			auto& scene = res.scene_data_vec.emplace_back();

			// todo, handle game_proj_migrate
			if (game_proj_version == 2)
			{
				buf.read(
					scene.cam.move_speed,
					scene.cam.sprint_mult,
					scene.cam.sensitivity,
					scene.cam.zoom_speed,
					scene.cam.zoom_distance,
					scene.cam.pan_speed,
					scene.cam.move_smoothing,
					scene.cam.look_smoothing,
					scene.cam.zoom_smoothing,
					scene.cam.move,
					scene.cam.look,
					scene.cam.zoom);

				scene.cam.sprint = buf.read<uint32>() > 0;

				buf.read(
					scene.cam.euler_x,
					scene.cam.euler_y,
					scene.cam.smoothed_move,
					scene.cam.smoothed_look,
					scene.cam.smoothed_zoom,
					scene.cam.smoothed_pan,
					scene.cam.pos,
					scene.cam.euler_deg,
					scene.cam.aspect_ratio);
			}
			else if (game_proj_version > 2)
			{
				scene.cam = buf.read<camera_data>();
			}

			auto&& [scene_name_count, scene_ent_storage_count] = buf.read<uint32, uint32>();

			scene.names.reserve(scene_name_count);
			for (auto _ : views::loop(scene_name_count))
			{
				scene.names.emplace_back(buf.read<age::array<char, config::max_scene_name_len>>());
			}

			scene.storage_data_vec.reserve(scene_ent_storage_count);
			for (auto _ : views::loop(scene_ent_storage_count))
			{
				auto& storage = scene.storage_data_vec.emplace_back();

				auto&& [storage_name_count, component_count, archetype_count] = buf.read<uint32, uint32, uint64>();

				storage.names.reserve(storage_name_count);
				for (auto _ : views::loop(storage_name_count))
				{
					storage.names.emplace_back(buf.read<age::array<char, config::max_entity_storage_name_len>>());
				}

				storage.component_data_vec.reserve(component_count);

				for (auto _ : views::loop(component_count))
				{
					auto&& [component_name_count, component_version, component_byte_size] = buf.read<uint32, uint32, uint32>();

					auto& cmp_data = storage.component_data_vec.emplace_back(component_editor_data{ .version   = component_version,
																									.byte_size = component_byte_size });
					cmp_data.names.reserve(component_name_count);

					for (auto _ : views::loop(component_name_count))
					{
						cmp_data.names.emplace_back(buf.read<age::array<char, config::max_component_name_len>>());
					}
				}

				storage.archetype_data_vec.reserve(archetype_count);

				for (auto _ : views::loop(archetype_count))
				{
					auto&& [archetype_bits, arch_entity_count, archetype_name] =
						buf.read<uint64, uint64, age::array<char, config::max_archetype_name_len>>();

					storage.entity_count += arch_entity_count;

					auto& arch = storage.archetype_data_vec.emplace_back(archetype_editor_data{
						.name			 = archetype_name,
						.archetype		 = archetype_bits,
						.entity_data_vec = age::vector<entity_editor_data>::gen_reserved(arch_entity_count),
					});

					for (auto _ : views::loop(arch_entity_count))
					{
						arch.entity_data_vec.emplace_back(entity_editor_data{
							.name = buf.read<age::array<char, config::max_entity_name_len>>() });
					}
				}
			}
		}

		return res;
	}
}	 // namespace age::editor::detail

// merge code_data, file_data
namespace age::editor::detail
{
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
			for (c_auto cmp_idx : unmatched_file)
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

		res.entity_count = 0u;
		for (c_auto& arch_data : res.archetype_data_vec)
		{
			res.entity_count += arch_data.entity_data_vec.size<uint32>();
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
	merge_game_data(game_editor_data&& code_game, game_editor_data&& file_game) noexcept
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

// deserialize
namespace age::editor::detail
{
	void
	deserialize_ecs_storage_data(const uint32 editor_scene_idx, const uint32 editor_storage_idx, aligned_byte_buf& buf, game_editor_data& dst_game) noexcept
	{
		auto& editor_storage = g::current_game.get_editor_storage(editor_scene_idx, editor_storage_idx);

		c_auto component_count	  = buf.read<uint32>();
		auto   component_data_arr = age::dynamic_array<component_editor_data>::gen_sized_default(component_count);

		auto cmp_file_id_to_ecs_id_arr = age::dynamic_array<uint32>::gen_sized_copy(component_count, age::get_invalid_idx<uint32>());

		for (auto&& [file_cmp_idx, cmp] : component_data_arr | views::enumerate<uint32>)
		{
			cmp.names.emplace_back(buf.read<age::array<char, config::max_component_name_len>>());
			buf.read(cmp.byte_size);
			buf.read(cmp.version);

			for (c_auto& cmp_data : editor_storage.component_data_vec)
			{
				for (c_auto& code_cmp_name : cmp_data.names)
				{
					if (std::strcmp(code_cmp_name.data(), cmp.names[0].data()) == 0)
					{
						cmp_file_id_to_ecs_id_arr[file_cmp_idx] = cmp_data.ecs_component_id;
						goto goto__break;
					}
				}
			}

		goto__break:;
		}

		c_auto archetype_count = buf.read<uint32>();

		for (auto _ : views::loop(archetype_count))
		{
			const auto&& [arch_name, file_archetype, entity_count] = buf.read<age::array<char, config::max_archetype_name_len>, uint64, uint64>();

			auto ecs_archetype = 0ull;

			for (auto file_cmp_idx : views::each_set_bit_idx(file_archetype))
			{
				c_auto ecs_component_id = cmp_file_id_to_ecs_id_arr[file_cmp_idx];
				if (ecs_component_id != age::get_invalid_idx<uint32>())
				{
					ecs_archetype |= (1ull << ecs_component_id);
				}
			}

			auto editor_arch_idx = age::get_invalid_id<uint32>();
			for (const auto&& [idx, arch_data] : editor_storage.archetype_data_vec | views::enumerate<uint32>)
			{
				if (arch_data.archetype == ecs_archetype)
				{
					editor_arch_idx = idx;
					break;
				}
			}

			AGE_ASSERT(runtime::is_invalid_idx(editor_arch_idx) is_false);

			for (c_auto ecs_scene_idx	= dst_game.scene_data_vec[editor_scene_idx].code_idx,
						ecs_storage_idx = dst_game.scene_data_vec[editor_scene_idx].storage_data_vec[editor_storage_idx].code_idx;
				 auto	editor_ent_idx : views::loop(entity_count))
			{
				c_auto ecs_entity_id = g::host_ops.p_new_entity_with_archetype(ecs_scene_idx, ecs_storage_idx, ecs_archetype);

				for (auto file_cmp_idx : views::each_set_bit_idx(file_archetype))
				{
					c_auto& file_cmp_data	 = component_data_arr[file_cmp_idx];
					c_auto	ecs_component_id = cmp_file_id_to_ecs_id_arr[file_cmp_idx];

					if (runtime::is_invalid_idx(ecs_component_id) is_false)
					{
						g::host_ops.p_deserialize_component(ecs_scene_idx, ecs_storage_idx, ecs_entity_id, ecs_component_id, file_cmp_data.version, buf);
					}
					else
					{
						buf.skip_read(file_cmp_data.byte_size);
					}
				}

				auto& editor_arch_data											= editor_storage.archetype_data_vec[editor_arch_idx];
				editor_arch_data.entity_data_vec[editor_ent_idx].id				= ecs_entity_id;
				editor_storage.ecs_ent_id_to_editor_location_map[ecs_entity_id] = { editor_arch_idx, editor_ent_idx };
			}
		}

		AGE_ASSERT(buf.read_amount() == buf.size());
	}
}	 // namespace age::editor::detail

namespace age::editor::detail
{
	void
	load_game_impl(std::string_view root_parent_dir, std::span<const age::array<char, age::config::max_game_name_len>> age_editor_name_span, game_editor_data&& code_game_data) noexcept
	{
		c_auto game_dir = detail::resolve_path_by_names(root_parent_dir, age_editor_name_span);

		if (fs::file_exists(fs::join(game_dir, std::format("{}{}", config::game_asset_tag, config::asset_extension))))
		{
			auto file_game_data = detail::read_game_proj(std::format("{}{}", config::game_asset_tag, config::asset_extension));

			g::current_game = detail::merge_game_data(std::move(code_game_data), std::move(file_game_data));
		}
		else
		{
			g::current_game = std::move(code_game_data);
		}

		g::current_game.dir_path			= fs::normalize_path(game_dir);
		g::current_game.asset_root_dir_path = fs::join(g::current_game.dir_path, "asset");
		e_visit_all(asset::e::kind{}, [&]<asset::e::kind e_kind> {
			// todo. mesh_baked -> mesh?
			if constexpr (e_kind == asset::e::kind::mesh_baked)
			{
				g::current_game.asset_dir_path_arr[to_idx(e_kind)] = fs::join(g::current_game.asset_root_dir_path, "mesh");
			}
			else
			{
				g::current_game.asset_dir_path_arr[to_idx(e_kind)] = fs::join(g::current_game.asset_root_dir_path, to_string(e_kind));
			}
		});

		age::asset::registry::load(std::string{});

		for (auto&& [editor_scene_idx, scene] : g::current_game.scene_data_vec | views::enumerate<uint32>)
		{
			scene.dir_path = detail::resolve_path_by_names(g::current_game.dir_path, scene.names);

			for (auto&& [editor_storage_idx, storage] : scene.storage_data_vec | views::enumerate<uint32>)
			{
				c_auto storage_path = detail::resolve_path_by_names<false>(scene.dir_path, storage.names, std::format("{}{}", config::editor_ent_storage_asset_tag, config::asset_extension));

				if (fs::exists(storage_path) is_false)
				{
					c_auto buf				 = detail::serialize_storage_data(editor_scene_idx, editor_storage_idx);
					c_auto asset_file_header = asset::get_default_file_header(asset::e::kind::editor_entity_storage, buf.size(), config::editor_ent_storage_asset_version);
					asset::write_asset_file(asset::to_root_relative(storage_path), asset_file_header, buf.data());
				}

				if (g::current_game.default_active_scene_idx == editor_scene_idx)
				{
					auto  file_data = asset::read_asset_file(asset::to_root_relative(storage_path));
					auto& buf		= file_data.buf;

					detail::deserialize_ecs_storage_data(editor_scene_idx, editor_storage_idx, buf, g::current_game);

					scene.loaded = true;
				}
			}
		}

		g::host_ops.p_renderer_init_main_cam(g::current_game.get_current_scene().cam);

		for (c_auto& scene : g::current_game.scene_data_vec)
		{
			g::select_vec.resize(scene.storage_data_vec.size());
		}
	}
}	 // namespace age::editor::detail

namespace age::editor::detail
{
	void
	re_register_entity(storage_editor_data& editor_storage, uint64 ecs_entity_id, uint64 new_archetype) noexcept
	{
		auto&& [old_arch_idx, old_ent_idx] = editor_storage.ecs_ent_id_to_editor_location_map[ecs_entity_id];
		auto& old_arch_data				   = editor_storage.archetype_data_vec[old_arch_idx];

		if (old_arch_data.archetype == new_archetype) { return; }

		auto ent_data = std::move(old_arch_data.entity_data_vec[old_ent_idx]);

		for (auto i = old_ent_idx + 1; i < old_arch_data.entity_data_vec.size(); ++i)
		{
			old_arch_data.entity_data_vec[i - 1]															 = std::move(old_arch_data.entity_data_vec[i]);
			editor_storage.ecs_ent_id_to_editor_location_map[old_arch_data.entity_data_vec[i - 1].id].second = i - 1;
		}
		old_arch_data.entity_data_vec.pop_back();

		for (auto&& [arch_idx, arch_data] : editor_storage.archetype_data_vec | std::views::enumerate)
		{
			if (arch_data.archetype == new_archetype)
			{
				editor_storage.ecs_ent_id_to_editor_location_map[ecs_entity_id] = { static_cast<uint32>(arch_idx), arch_data.entity_data_vec.size<uint64>() };
				arch_data.entity_data_vec.emplace_back(std::move(ent_data));
				return;
			}
		}

		editor_storage.ecs_ent_id_to_editor_location_map[ecs_entity_id] = { editor_storage.archetype_data_vec.size<uint32>(), 0ull };
		auto& new_arch_data												= editor_storage.archetype_data_vec.emplace_back();
		new_arch_data.archetype											= new_archetype;
		new_arch_data.entity_data_vec.emplace_back(std::move(ent_data));
	}
}	 // namespace age::editor::detail
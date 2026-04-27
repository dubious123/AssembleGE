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
}	 // namespace age::editor::detail

// file -> editor_data
namespace age::editor::detail
{
	game_editor_data
	read_game_proj(std::filesystem::path proj_path) noexcept;
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

	uint32
	resolve_archetype_collapse_via_console(const age::vector<archetype_editor_data>& file_archetype_vec,
										   const age::vector<uint32>&				 collapse_vec) noexcept;

	storage_editor_data
	merge_storage_data(storage_editor_data& code_storage, storage_editor_data& file_storage) noexcept;
	scene_editor_data
	merge_scene_data(scene_editor_data& code_scene, scene_editor_data& file_scene) noexcept;

	game_editor_data
	merge_game_data(game_editor_data& code_game, game_editor_data& file_game) noexcept;
}	 // namespace age::editor::detail

// serialize, deserialize ecs storage
namespace age::editor::detail
{
	age::byte_buf
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
						t_archetype_traits::visit_component(storage_cmp_idx, []<typename t_cmp>(auto&&... arg) { ecs::serialize_component_from_ptr<t_cmp>(FWD(arg)...); }, p_cmp, buf, get_rw_context(storage.component_data_vec[storage_cmp_idx].version, renderer));
					}
				}
			}

			buf.move_write_pos(buf_base_pos + archetype_byte_size * arch.entity_data_vec.size());
		}

		return buf;
	}

	void
	deserialize_storage_data(auto& ecs_entity_storage, auto& buf, storage_editor_data& editor_storage, auto& renderer) noexcept
	{
		using t_storage			 = BARE_OF(ecs_entity_storage);
		using t_archetype		 = typename t_storage::t_archetype;
		using t_archetype_traits = typename t_storage::t_archetype_traits;
		using t_local_cmp_idx	 = typename t_storage::t_local_cmp_idx;

		c_auto component_count	  = buf.read<uint32>();
		auto   component_data_arr = age::dynamic_array<component_editor_data>::gen_sized_default(component_count);

		auto cmp_file_to_code_arr = age::dynamic_array<int32>::gen_sized_copy(component_count, -1);

		for (auto&& [file_cmp_idx, cmp] : component_data_arr | std::views::enumerate)
		{
			cmp.names.emplace_back(buf.read<std::array<char, config::max_component_name_len>>());
			buf.read(cmp.byte_size);
			buf.read(cmp.version);

			for (const auto&& [code_cmp_idx, code_cmp_data] : editor_storage.component_data_vec | std::views::enumerate)
			{
				for (c_auto& code_cmp_name : code_cmp_data.names)
				{
					if (std::strcmp(code_cmp_name.data(), cmp.names[0].data()) == 0)
					{
						cmp_file_to_code_arr[file_cmp_idx] = static_cast<int32>(code_cmp_idx);
						goto goto__break;
					}
				}
			}

		goto__break:;
		}

		c_auto archetype_count = buf.read<uint32>();

		for (auto _ : views::loop(archetype_count))
		{
			auto&& [arch_name, file_archetype, entity_count] = buf.read<std::array<char, config::max_archetype_name_len>, uint64, uint64>();

			auto code_archetype = 0ull;

			for (auto file_cmp_idx : views::each_set_bit_idx(file_archetype))
			{
				c_auto code_cmp_idx = cmp_file_to_code_arr[file_cmp_idx];
				if (code_cmp_idx != -1)
				{
					code_archetype |= (1ull << code_cmp_idx);
				}
			}

			auto editor_arch_idx = age::get_invalid_id<uint32>();
			for (const auto&& [idx, arch_data] : editor_storage.archetype_data_vec | std::views::enumerate)
			{
				if (arch_data.archetype == code_archetype)
				{
					editor_arch_idx = static_cast<uint32>(idx);
					break;
				}
			}

			AGE_ASSERT(AGE_IS_INVALID_IDX(editor_arch_idx) is_false);

			for (auto editor_ent_idx : views::loop(entity_count))
			{
				c_auto ent_id = ecs_entity_storage.new_entity(static_cast<t_archetype>(code_archetype), get_ecs_context(renderer));

				for (auto file_cmp_idx : views::each_set_bit_idx(file_archetype))
				{
					c_auto& file_cmp_data = component_data_arr[file_cmp_idx];
					c_auto	code_cmp_idx  = cmp_file_to_code_arr[file_cmp_idx];

					if (code_cmp_idx != -1)
					{
						t_archetype_traits::visit_component(
							code_cmp_idx,
							AGE_LAMBDA(<typename t_cmp>(auto& buf, auto& storage, c_auto ent_id, auto&& rw_ctx),
									   {
										   auto&& [cmp] = storage.get_component<t_cmp>(ent_id);
										   ecs::deserialize_component<t_cmp>(cmp, buf, FWD(rw_ctx));
									   }),
							buf, ecs_entity_storage, ent_id, get_rw_context(file_cmp_data.version, renderer));
					}
					else
					{
						buf.skip_read(file_cmp_data.byte_size);
					}
				}

				register_entity(editor_storage, editor_arch_idx, editor_ent_idx, ent_id);
			}
		}

		AGE_ASSERT(buf.read_amount() == buf.size());
	}
}	 // namespace age::editor::detail

// file name
namespace age::editor::detail
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
}	 // namespace age::editor::detail

namespace age::editor
{
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

		g::current_game.dir_path = std::move(game_dir);

		for (auto&& [scene_idx, scene] : g::current_game.scene_data_vec | std::views::enumerate)
		{
			scene.dir_path = detail::resolve_path_by_names(g::current_game.dir_path, scene.names);

			for (auto& storage : scene.storage_data_vec)
			{
				c_auto storage_path = detail::resolve_path_by_names<false>(scene.dir_path, storage.names, std::format("{}{}", config::editor_ent_storage_asset_tag, config::asset_extension));

				if (std::filesystem::exists(storage_path) is_false)
				{
					c_auto buf				 = game.visit_storage_at(scene.code_idx, storage.code_idx, AGE_FUNC(detail::serialize_storage_data), storage, renderer);
					c_auto asset_file_header = asset::get_default_file_header<asset::e::kind::editor_entity_storage>(buf.size());
					asset::write_asset_file(storage_path, asset_file_header, buf.data());
				}

				if (g::current_game.default_active_scene_idx == scene_idx)
				{
					auto buf = asset::read_asset_file(storage_path.string());

					game.visit_storage_at(scene.code_idx, storage.code_idx, AGE_FUNC(detail::deserialize_storage_data), buf, storage, renderer);

					scene.loaded = true;
				}
			}
		}

		{
			auto& cam						  = g::current_game.get_current_scene().cam;
			auto  cam_desc					  = renderer.get_camera_desc(0);
			cam_desc.pos					  = cam.pos;
			cam_desc.quaternion				  = age::euler_deg_to_quat(cam.euler_deg);
			cam_desc.perspective.aspect_ratio = cam.aspect_ratio;
			renderer.update_camera(0, cam_desc);
			renderer.set_main_camera(0);
		}
		// std::filesystem::remove_all(root_dir);
	}
}	 // namespace age::editor

namespace age::editor::detail
{
	void
	save_game_proj(const game_editor_data& game) noexcept;
}	 // namespace age::editor::detail

namespace age::editor
{
	void
	save_game(auto& game, auto& renderer) noexcept
	{
		detail::save_game_proj(g::current_game);

		c_auto& active_scene = g::current_game.scene_data_vec[g::current_game.current_active_scene_idx];

		for (c_auto& editor_storage : active_scene.storage_data_vec)
		{
			c_auto storage_path		 = detail::resolve_path_by_names<false>(active_scene.dir_path, editor_storage.names, std::format("{}{}", config::editor_ent_storage_asset_tag, config::asset_extension));
			c_auto buf				 = game.visit_storage_at(active_scene.code_idx, editor_storage.code_idx, AGE_FUNC(detail::serialize_storage_data), editor_storage, renderer);
			c_auto asset_file_header = asset::get_default_file_header<asset::e::kind::editor_entity_storage>(buf.size());
			asset::write_asset_file(storage_path, asset_file_header, buf.data());
		}
	}
}	 // namespace age::editor
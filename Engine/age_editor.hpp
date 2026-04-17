#pragma once
#include "age.hpp"

namespace age::editor
{
	struct entity_editor_data
	{
		char name[config::max_entity_name_len];
	};

	struct storage_editor_data
	{
		age::vector<std::array<char, config::max_entity_storage_name_len>> names;
		age::unordered_map<uint64, entity_editor_data>					   entity_data_map;
	};

	struct scene_editor_data
	{
		age::vector<std::array<char, config::max_scene_name_len>> names;
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

namespace age::editor::detail
{
	void
	gen_storage_data(auto& storage, auto scene_idx, auto storage_idx) noexcept
	{
		// todo
	}

	void
	gen_scene_data(auto& scene, auto scene_idx) noexcept
	{
		[]<auto... i>(std::index_sequence<i...>, auto& scene, auto scene_idx) {
			((g::current_game.scene_data_vec[scene_idx].storage_data_vec.emplace_back(storage_editor_data{
				  .names = std::get<i>(scene.storage_names()) | std::ranges::to<age::vector<std::array<char, config::max_entity_storage_name_len>>>(),
			  }),

			  gen_storage_data(std::get<i>(scene.storages()), scene_idx, i)),
			 ...);
		}(std::make_index_sequence<scene.storage_count()>{}, scene, scene_idx);
	}

	void
	gen_game_data(auto& game) noexcept
	{
		g::current_game.names					 = game.age_editor_name() | std::ranges::to<age::vector<std::array<char, config::max_game_name_len>>>();
		g::current_game.default_active_scene_idx = 0;

		g::current_game.scene_data_vec.clear();

		[]<auto... i>(std::index_sequence<i...>, auto& game) {
			((g::current_game.scene_data_vec.emplace_back(scene_editor_data{
				  .names  = std::get<i>(game.scene_names()) | std::ranges::to<age::vector<std::array<char, config::max_scene_name_len>>>(),
				  .loaded = false,
			  }),

			  gen_scene_data(std::get<i>(game.scenes()), i)),
			 ...);
		}(std::make_index_sequence<game.scene_count()>{}, game);
	}

	template <typename t_str>
	void
	merge_names(age::vector<t_str>& dst, const age::vector<t_str>& rhs) noexcept
	{
		auto has_common = false;
		for (c_auto& rhs_name : rhs)
		{
			auto found = false;
			for (auto i : views::loop(dst.size()))
			{
				c_auto& dst_name = dst[i];
				if (std::strcmp(dst_name.data(), rhs_name.data()) == 0)
				{
					found = true;
					break;
				}
			}

			has_common |= found;
			if (found is_false)
			{
				dst.emplace_back(rhs_name);
			}
		}

		if (has_common is_false)
		{
			// todo better log
			AGE_DEBUG_LOG("merge_name failed");
		}
	}
}	 // namespace age::editor::detail

namespace age::editor
{
	void
	load_game(auto& game, std::filesystem::path root_dir) noexcept
	{
		detail::gen_game_data(game);

		c_auto& names	 = game.age_editor_name();
		auto	game_dir = std::filesystem::path{};
		{
			auto game_primary_dir = root_dir / names[0].data();

			for (c_auto name : names)
			{
				auto candidate = root_dir / name.data();

				std::println("{}", candidate.string());

				if (std::filesystem::exists(candidate))
				{
					game_dir = candidate;
					break;
				}
			}

			if (game_dir.empty())
			{
				game_dir = game_primary_dir;
				std::filesystem::create_directories(game_dir);
			}
			else if (game_dir != game_primary_dir)
			{
				std::filesystem::rename(game_dir, game_primary_dir);
			}

			game_dir = std::move(game_primary_dir);
		}

		auto proj_file_name = game_dir / std::format("{}{}", config::game_asset_tag, config::asset_extension);

		if (std::filesystem::exists(proj_file_name))
		{
			auto h_proj_file = asset::load_from_path(proj_file_name.string());

			auto read_buf = h_proj_file->get_payload_read_buf();

			auto&& [game_proj_version, game_name_count, game_active_scene_idx, game_scene_count] = read_buf.read<uint32, uint32, uint32, uint32>();

			if (game_proj_version != config::editor_game_proj_version)
			{
				AGE_ASSERT(false);
				// todo, handle game_proj_migrate
			}

			{
				auto file_game_name_vec = age::vector<std::array<char, config::max_game_name_len>>::gen_reserved(game_name_count);
				for (auto i : views::loop(game_name_count))
				{
					file_game_name_vec.emplace_back(read_buf.read<std::array<char, config::max_game_name_len>>());
				}

				detail::merge_names(file_game_name_vec, g::current_game.names);

				g::current_game.names = std::move(file_game_name_vec);
			}

			{
				auto scene_found = age::dynamic_array<bool>::gen_sized_copy(game.scene_count(), false);

				for (auto i : views::loop(game_scene_count))
				{
					auto&& [scene_name_count, scene_ent_storage_count] = read_buf.read<uint32, uint32>();

					auto file_scene_name_vec = age::vector<std::array<char, config::max_scene_name_len>>::gen_reserved(scene_name_count);

					for (auto _ : views::loop(scene_name_count))
					{
						file_scene_name_vec.emplace_back(read_buf.read<std::array<char, config::max_scene_name_len>>());
					}

					// todo
					// auto&& [found, idx] = detail::find_name(scene_found, g::current_game.scene_data_vec, file_scene_name_vec);

					// for (c_auto& file_scene_name : file_scene_name_vec)
					//{
					//	for (auto&& [idx, found] : scene_found | std::views::enumerate)
					//	{
					//		if (found) { continue; }

					//		for (c_auto& code_scene_name : g::current_game.scene_data_vec[idx].names)
					//		{
					//		}
					//	}
					//}
				}
			}
		}

		for (auto&& [scene_idx, scene] : g::current_game.scene_data_vec | std::views::enumerate)
		{
			auto scene_dir = std::filesystem::path{};
			{
				auto scene_primary_dir = game_dir / scene.names[0].data();

				for (c_auto& scene_name : scene.names)
				{
					auto candidate = game_dir / scene_name.data();
					if (std::filesystem::exists(scene_dir))
					{
						scene_dir = candidate;
						break;
					}
				}

				if (scene_dir.empty())
				{
					std::filesystem::create_directories(scene_primary_dir);
				}
				else if (scene_dir != scene_primary_dir)
				{
					std::filesystem::rename(scene_dir, scene_primary_dir);
				}

				scene_dir = std::move(scene_primary_dir);
			}

			for (auto& storage : scene.storage_data_vec)
			{
				auto storage_path = std::filesystem::path{};
				{
					auto storage_path_primary = scene_dir / storage.names[0].data();

					for (c_auto& storage_name : storage.names)
					{
						auto candidate = scene_dir / std::format("{}{}{}", storage_name.data(), config::editor_ent_storage_asset_tag, config::asset_extension);

						if (std::filesystem::exists(candidate))
						{
							storage_path = candidate;
							break;
						}
					}

					if (storage_path.empty())
					{
						// util::create_file(storage_path_primary);
						auto _		 = std::ofstream(storage_path_primary);
						storage_path = storage_path_primary;
						// todo, create_defaults?
					}
					else if (storage_path != storage_path_primary)
					{
						std::filesystem::rename(storage_path, storage_path_primary);
					}
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
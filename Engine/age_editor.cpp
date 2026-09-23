#include "age_pch.hpp"
#include "age.hpp"

namespace age::editor
{
	namespace detail
	{
		void
		init_impl() noexcept
		{
			g::current_mode = e::mode_kind::edit;
			g::current_game = game_editor_data{};

			g::show_modal = false;
			g::set_focus  = false;

			g::h_mesh_cone = g::host_ops.p_mesh_gpu_load("editor_asset/editor_mesh_cone",
														 asset::primitive_desc{
															 .seg_u		= 30,
															 .seg_v		= 1,
															 .mesh_kind = asset::e::primitive_mesh_kind::cone,
														 },
														 asset::e::vertex_kind::pnt_uv0);

			g::h_mesh_cube = g::host_ops.p_mesh_gpu_load("editor_asset/editor_mesh_cube",
														 asset::primitive_desc{
															 .seg_u		= 1,
															 .seg_v		= 1,
															 .mesh_kind = asset::e::primitive_mesh_kind::cube,
														 },
														 asset::e::vertex_kind::pnt_uv0);

			asset::mesh_baked::add_ref(g::h_mesh_cone);
			asset::mesh_baked::add_ref(g::h_mesh_cube);

			AGE_ASSERT(g::h_mesh_cone.get_entry<asset::e::kind::mesh_baked>().is_gpu_loaded());
			AGE_ASSERT(g::h_mesh_cube.get_entry<asset::e::kind::mesh_baked>().is_gpu_loaded());

			asset_mgr::init();
		}
	}	 // namespace detail

	void
	deinit() noexcept
	{
		g::select_vec.reset();

		if constexpr (age::config::debug_mode)
		{
			// g::command_buf.validate();
		}

		// g::command_buf.clear();

		asset::mesh_baked::remove_ref(g::h_mesh_cone);
		asset::mesh_baked::remove_ref(g::h_mesh_cube);

		g::host_ops.p_mesh_full_unload(g::h_mesh_cone);
		g::host_ops.p_mesh_full_unload(g::h_mesh_cube);

		AGE_ASSERT(g::h_mesh_cone.get_entry<asset::e::kind::mesh_baked>().is_gpu_loaded() is_false);
		AGE_ASSERT(g::h_mesh_cube.get_entry<asset::e::kind::mesh_baked>().is_gpu_loaded() is_false);

		g::h_mesh_cone = {};
		g::h_mesh_cube = {};

		g::host_ops = {};

		asset_mgr::deinit();
	}

	bool
	is_edit_mode() noexcept
	{
		return g::current_mode == e::mode_kind::edit;
	}

	bool
	is_play_mode() noexcept
	{
		return g::current_mode == e::mode_kind::play;
	}
}	 // namespace age::editor

// select
namespace age::editor
{
	// void
	// add_select(uint32 storage_code_idx, uint64 ent_id) noexcept
	//{
	//	if (is_selected(storage_code_idx, ent_id) is_false)
	//	{
	//		g::select_vec[storage_code_idx].emplace_back(ent_id);
	//	}
	// }

	// void
	// remove_select(uint32 storage_code_idx, uint64 ent_id) noexcept
	//{
	//	for (auto&& [idx, id] : g::select_vec[storage_code_idx] | std::views::enumerate)
	//	{
	//		if (ent_id == id)
	//		{
	//			g::select_vec[storage_code_idx][idx] = g::select_vec[storage_code_idx].back();
	//			g::select_vec[storage_code_idx].pop_back();
	//			break;
	//		}
	//	}
	// }

	// bool
	// is_selected(uint32 storage_code_idx, uint64 ent_id) noexcept
	//{
	//	for (auto id : g::select_vec[storage_code_idx])
	//	{
	//		if (ent_id == id) { return true; }
	//	}
	//	return false;
	// }

	// void
	// clear_select() noexcept
	//{
	//	for (auto& vec : g::select_vec)
	//	{
	//		vec.clear();
	//	}
	// }

	void
	set_select_kind(e::select_kind new_kind) noexcept
	{
		if (g::current_select_kind != new_kind)
		{
			for (auto& vec : g::select_vec)
			{
				vec.clear();
			}
			g::current_select_kind = new_kind;
		}
	}

	void
	add_select(e::select_kind kind, uint32 group_idx, uint64 id) noexcept
	{
		set_select_kind(kind);
		g::select_vec.resize(max(g::select_vec.size<uint32>(), group_idx + 1));

		if (is_selected(kind, group_idx, id) is_false)
		{
			g::select_vec[group_idx].emplace_back(id);
		}
	}

	void
	remove_select(e::select_kind kind, uint32 group_idx, uint64 id) noexcept
	{
		if (g::current_select_kind != kind) { return; }

		for (auto&& [idx, stored_id] : g::select_vec[group_idx] | std::views::enumerate)
		{
			if (id == stored_id)
			{
				g::select_vec[group_idx][idx] = g::select_vec[group_idx].back();
				g::select_vec[group_idx].pop_back();
				break;
			}
		}
	}

	bool
	is_selected(e::select_kind kind, uint32 group_idx, uint64 id) noexcept
	{
		if (g::current_select_kind != kind) { return false; }

		for (auto stored_id : g::select_vec[group_idx])
		{
			if (id == stored_id) { return true; }
		}
		return false;
	}

	bool
	has_selection(e::select_kind kind, uint32 group_idx) noexcept
	{
		if (g::current_select_kind != kind) { return false; }

		return g::select_vec[group_idx].is_empty() is_false;
	}

	std::optional<uint64>
	last_selected(e::select_kind kind, uint32 group_idx) noexcept
	{
		if (has_selection(kind, group_idx))
		{
			return { g::select_vec[group_idx].back() };
		}
		else
		{
			return {};
		}
	}

	void
	clear_select() noexcept
	{
		for (auto& vec : g::select_vec)
		{
			vec.clear();
		}
		g::current_select_kind = e::select_kind::none;
	}
}	 // namespace age::editor

// asset
namespace age::editor
{
	const std::string&
	get_asset_root_dir_path() noexcept
	{
		return g::current_game.asset_root_dir_path;
	}

	const std::string&
	get_asset_dir_path(asset::e::kind kind) noexcept
	{
		return g::current_game.asset_dir_path_arr[to_idx(kind)];
	}

	std::string
	get_asset_path(asset::e::kind kind, std::string_view asset_name) noexcept
	{
		return fs::join(get_asset_dir_path(kind), asset_name);
	}

	age::array<char, config::max_asset_path_len>
	get_asset_full_path(asset::e::kind kind, std::string_view asset_name) noexcept
	{
		return asset::e::visit(kind, [&]<asset::e::kind e_kind> {
			c_auto name		 = get_asset_path(e_kind, asset_name);
			c_auto full_path = asset::get_asset_full_path<e_kind>(name);
			return full_path;
		});
	}

	void
	asset_full_unload(asset::e::kind asset_kind, asset::handle h_asset) noexcept
	{
		AGE_ASSERT(h_asset is_true, "invalid asset handle should not exist here");

		if (h_asset.is_any_loaded()) { return; }

		switch (asset_kind)
		{
		case asset::e::kind::font:
		{
			AGE_UNREACHABLE("invalid asset type font");
			break;
		}
		case asset::e::kind::mesh_baked:
		{
			g::host_ops.p_mesh_full_unload(h_asset);
			break;
		}
		case asset::e::kind::material:
		{
			g::host_ops.p_material_full_unload(h_asset);
			break;
		}
		case asset::e::kind::texture:
		{
			g::host_ops.p_texture_full_unload(h_asset);
			break;
		}
		case asset::e::kind::env_light:
		{
			g::host_ops.p_env_light_full_unload(h_asset);
			break;
		}
		case asset::e::kind::model:
		{
			g::host_ops.p_model_full_unload(h_asset);
			break;
		}
		default:
			AGE_UNREACHABLE("invalid asset type {}", to_idx(asset_kind));
			break;
		}
	}
}	 // namespace age::editor

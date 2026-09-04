#include "age.hpp"

namespace age::editor::asset_mgr
{
	// delete asset => unregister asset, ref counter 0 => full unload, two concepts are independent
	// in the editor domain, every asset held by a component has ref_counter > 0
	// however, handles can be held without a ref and how ref_counter is used is domain specific.
	// a handle in other domains has no way to detect any destruction of an entry
	// asset entries are meant to be light, so keeping them throughout the game will (probably) cost little
	// therefore we don't destroy an asset entry here (except for the deinit phases)
	void
	update(auto& ecs_game, auto& renderer) noexcept
	{
		using namespace age::asset;
		using enum age::asset::e::kind;

		// update_asset_pin
		for (auto i : views::loop(g::asset_pin_vec.size<uint32>()) | std::views::reverse)
		{
			auto& pin = g::asset_pin_vec[i];
			AGE_ASSERT(runtime::is_handle_invalid(pin.h_asset) is_false);

			if (--pin.frames_to_live == 0)
			{
				asset::visit(pin.kind, [&]<asset::e::kind k> {
					asset::remove_ref<k>(pin.h_asset);
				});
				std::swap(g::asset_pin_vec[i], g::asset_pin_vec[g::asset_pin_vec.size() - 1]);
				g::asset_pin_vec.pop_back();

				continue;
			}
		}

		// handle asset child delete
		{
			for (auto h : asset::each_handle_of<material>())
			{
				auto& entry = h.get_entry<material>();
				if (entry.ref_counter == 0) { continue; }

				auto need_update = false;
				for (auto&& h_tex : entry.all_textures() | views::deref)
				{
					if (runtime::is_handle_invalid(h_tex)) { continue; }

					if (std::ranges::contains(g::asset_to_delete[to_idx(texture)], h_tex))
					{
						asset::material::update_texture(h_tex, asset::handle{});	// --ref
						need_update = true;
					}
				}

				if (need_update and entry.is_loaded())
				{
					renderer.update_material(h);
				}
			}

			for (auto h : asset::each_handle_of<model>())
			{
				auto& entry = h.get_entry<model>();
				if (entry.ref_counter == 0) { continue; }

				if (runtime::is_handle_invalid(entry.h_mesh) is_false
					and std::ranges::contains(g::asset_to_delete[to_idx(mesh_baked)], entry.h_mesh))
				{
					asset::model::update_mesh(h, asset::handle{});
				}

				for (auto&& [idx, h_mat] : entry.h_material_vec | views::enumerate<uint32>)
				{
					if (runtime::is_handle_invalid(h_mat)) { continue; }

					if (std::ranges::contains(g::asset_to_delete[to_idx(material)], h_mat))
					{
						asset::model::update_material(h, idx, asset::handle{});
					}
				}
			}
		}

		// handle asset unload
		asset::for_each_kind([&renderer]<asset::e::kind e_kind> noexcept {
			if constexpr (e_kind == font)
			{
				for (auto h : asset::each_handle_of<e_kind>())
				{
					AGE_ASSERT(asset::registry::is_registered(h) is_false, "font component is not implemented yet");
				}
			}
			else
			{
				for (auto h : asset::each_handle_of<e_kind>())
				{
					auto& entry = h.get_entry<e_kind>();

					if (entry.ref_counter == 0)
					{
						asset::full_unload<e_kind>(h, renderer);	// cascades
					}
					// else
					//{
					//	if constexpr (e_kind == material) { asset::material::load(h, renderer); }
					//	else if constexpr (e_kind == model) { asset::model::load(h, renderer); }
					//	else
					//	{
					//		asset::gpu_load<e_kind>(h, renderer);
					//	}
					// }
				}
			}
		});

		// handle asset delete
		asset::for_each_kind([]<asset::e::kind e_kind> noexcept {
			for (auto h : g::asset_to_delete[to_idx(e_kind)])
			{
				asset::registry::unregister_asset(h);
			}
			g::asset_to_delete[to_idx(e_kind)].clear();
		});

		// update ecs components
		ecs_game.visit_all_storages([&](auto& ecs_storage) {
			using namespace ecs;
			for (auto&& [cmp] : ecs_storage | each_entity_soft<ecs::mesh>())
			{
				if (runtime::is_handle_invalid(cmp.h_mesh)) { continue; }

				if (asset::registry::is_registered<asset::e::kind::mesh_baked>(cmp.h_mesh) is_false)
				{
					cmp.update_h_mesh(asset::handle{});
				}
			}

			for (auto&& [cmp] : ecs_storage | each_entity_soft<ecs::material>())
			{
				if (runtime::is_handle_invalid(cmp.h_mat)) { continue; }

				if (asset::registry::is_registered<asset::e::kind::material>(cmp.h_mat) is_false)
				{
					cmp.update_h_mat(asset::handle{});
				}
			}

			for (auto&& [cmp] : ecs_storage | each_entity_soft<ecs::env_light>())
			{
				if (runtime::is_handle_invalid(cmp.h_env_light)) { continue; }

				if (asset::registry::is_registered<asset::e::kind::env_light>(cmp.h_env_light) is_false)
				{
					cmp.update_h_env_light(asset::handle{});
				}
			}

			for (auto&& [cmp] : ecs_storage | each_entity_soft<ecs::model>())
			{
				if (runtime::is_handle_invalid(cmp.h_model)) { continue; }

				if (asset::registry::is_registered<asset::e::kind::model>(cmp.h_model) is_false)
				{
					cmp.update_h_model(asset::handle{});
				}
			}
		});
	}
}	 // namespace age::editor::asset_mgr
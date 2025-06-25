#pragma once
#include "__common.h"
#include "__ecs_common.h"
#include "__ecs_utility.h"
#include "__ecs_entity_group_basic.h"

// todo : free group tacking (instead of linear search)
// todo : custom map
// todo : custom malloc
// todo : custom vector
namespace ecs::entity_storage
{
	template <typename t_entity_id, ecs::component_type... t_cmp>
	struct basic
	{
		using t_archetype_traits = ecs::utility::archetype_traits<t_cmp...>;
		using t_archetype		 = t_archetype_traits::t_archetype;
		using t_storage_cmp_idx	 = t_archetype_traits::t_storage_cmp_idx;
		using t_local_cmp_idx	 = t_archetype_traits::t_local_cmp_idx;
		using t_entity_group_idx = t_entity_id;
		using t_entity_group	 = ecs::entity_group::basic<4_KiB, t_entity_id, t_entity_group_idx, t_cmp...>;
		using t_local_entity_idx = t_entity_group::t_local_entity_idx;

		struct entity_info
		{
			t_archetype archetype;

			// Index of the entity within its current entity_group.
			// Used to access component data in SoA blocks.
			// Must be updated when entities move between groups.
			t_local_entity_idx local_idx;

			t_entity_group* p_group;
		};

		struct entity_group_collection
		{
			//[ full ... | free ... ]
			data_structure::vector<t_entity_group*> ent_group_vec;
			std::size_t								free_group_count = 0;

			inline void
			deinit()
			{
				std::ranges::for_each(ent_group_vec, [](auto* p_group) { delete p_group; });
			}

			inline std::size_t
			free_group_idx()
			{
				return static_cast<std::size_t>(ent_group_vec.size() - free_group_count);
			}

			inline t_entity_group&
			free_group(t_archetype archetype)
			{
				if (free_group_count > 0)
				{
					return *ent_group_vec[free_group_idx()];
				}

				++free_group_count;
				if (ent_group_vec.empty())
				{
					auto p_entity_group = new t_entity_group();
					p_entity_group->init(archetype, static_cast<t_entity_group_idx>(ent_group_vec.size()));
					ent_group_vec.emplace_back(p_entity_group);
					return *p_entity_group;
				}
				else
				{
					auto p_entity_group = new t_entity_group();
					p_entity_group->init(*ent_group_vec.back(), static_cast<t_entity_group_idx>(ent_group_vec.size()));
					ent_group_vec.emplace_back(p_entity_group);
					return *p_entity_group;
				}
			}

			void
			update_free(std::size_t group_idx)
			{
				assert(ent_group_vec[group_idx]->is_full() is_false);
				assert(group_idx < free_group_idx());
				++free_group_count;

				// ent_group_vec[swap_idx] is full
				auto swap_idx = free_group_idx();
				std::swap(ent_group_vec[group_idx], ent_group_vec[swap_idx]);
				std::swap(ent_group_vec[group_idx]->entity_group_idx(), ent_group_vec[swap_idx]->entity_group_idx());
			}

			void
			update_full(std::size_t group_idx)
			{
				assert(ent_group_vec[group_idx]->is_full());
				assert(group_idx >= free_group_idx());

				// the first free_group
				auto swap_idx = free_group_idx();
				std::swap(ent_group_vec[group_idx], ent_group_vec[swap_idx]);
				std::swap(ent_group_vec[group_idx]->entity_group_idx(), ent_group_vec[swap_idx]->entity_group_idx());

				--free_group_count;
			}
		};

		data_structure::sparse_vector<entity_info>				  entity_info_vec;
		data_structure::map<t_archetype, entity_group_collection> entity_groups_map;

		std::size_t
		entity_count() const noexcept
		{
			return entity_info_vec.size();
		}

	  private:
		// find or create and init entity_group
		template <typename... t>
		inline t_entity_group&
		_get_or_init_entity_group()
		{
			return entity_groups_map[t_archetype_traits::template calc_archetype<t...>()].free_group();
		}

		inline t_entity_group&
		_get_or_init_entity_group(t_archetype archetype)
		{
			return entity_groups_map[archetype].free_group();
		}

	  public:
		template <typename... t, typename... t_arg>
		t_entity_id
		new_entity(t_arg&&... arg)
		{
			static_assert((sizeof...(t) == sizeof...(t_arg)) or (sizeof...(t_arg) == 0), "invalid template parameter");

			constexpr auto archetype = t_archetype_traits::template calc_archetype<t...>();

			auto& ent_group_collection = entity_groups_map[archetype];
			auto& entity_group		   = ent_group_collection.free_group(archetype);
			auto  entity_id			   = static_cast<t_entity_id>(entity_info_vec.emplace_back(entity_info{ archetype, 0, &entity_group }));

			entity_info_vec[entity_id].local_idx = entity_group.new_entity<t...>(entity_id, std::forward<t_arg>(arg)...);

			if (entity_group.is_full())
			{
				ent_group_collection.update_full(entity_group.entity_group_idx());
			}

			return entity_id;
		}

		void
		remove_entity(const t_entity_id id)
		{
			auto& ent_info = entity_info_vec[id];

			auto need_update = ent_info.p_group->is_full();

			auto& entity_id_last					  = ent_info.p_group->remove_entity(ent_info.local_idx);
			entity_info_vec[entity_id_last].local_idx = ent_info.local_idx;

			if (need_update)
			{
				entity_groups_map[ent_info.archetype].update_free(ent_info.p_group->entity_group_idx());
			}

			entity_info_vec.remove(id);
		}

		bool
		is_valid(t_entity_id id) const
		{
			return false;
		}

		// if dup cmp => UB
		template <typename... t, typename... t_arg>
		void
		add_component(const t_entity_id id, t_arg&&... arg)
		{
			using namespace std::ranges::views;

			auto& ent_info					= entity_info_vec[id];
			auto  new_archetype				= static_cast<t_archetype>(ent_info.archetype | t_archetype_traits::template calc_archetype<t...>());
			auto& src_ent_group_collection	= entity_groups_map[ent_info.archetype];
			auto& dest_ent_group_collection = entity_groups_map[new_archetype];
			auto& src_group					= *ent_info.p_group;
			auto& dst_group					= dest_ent_group_collection.free_group(new_archetype);
			auto  need_src_update			= src_group.is_full();

			for (auto [idx, storage_cmp_idx] : iota(0, std::bit_width(ent_info.archetype))
												   | filter([archetype = ent_info.archetype](auto idx) { return (archetype >> idx) & 1; })
												   | enumerate)
			{
				auto src_local_cmp_idx	= static_cast<t_local_cmp_idx>(idx);
				auto dest_local_cmp_idx = t_archetype_traits::calc_local_cmp_idx(new_archetype, storage_cmp_idx);
				src_group.evict_component(ent_info.local_idx, src_local_cmp_idx, dst_group.get_component_write_ptr(dest_local_cmp_idx));
			}

			if constexpr (sizeof...(t_arg) == 0)
			{
				(std::construct_at(reinterpret_cast<t*>(dst_group.get_component_write_ptr(t_archetype_traits::template calc_local_cmp_idx<t>(new_archetype)))), ...);
			}
			else if constexpr (sizeof...(t_arg) == sizeof...(t))
			{
				(std::construct_at(reinterpret_cast<t*>(dst_group.get_component_write_ptr(t_archetype_traits::template calc_local_cmp_idx<t>(new_archetype))), std::forward<t_arg>(arg)), ...);
			}
			else
			{
				static_assert(false, "invalid template parameter");
			}

			src_group.ent_id(ent_info.local_idx)	   = src_group.ent_id(--src_group.entity_count());
			dst_group.ent_id(dst_group.entity_count()) = id;

			ent_info.archetype = new_archetype;
			ent_info.p_group   = &dst_group;
			ent_info.local_idx = dst_group.entity_count()++;

			if (need_src_update)
			{
				src_ent_group_collection.update_free(src_group.entity_group_idx());
			}
			if (dst_group.is_full())
			{
				dest_ent_group_collection.update_full(dst_group.entity_group_idx());
			}
		}

		template <typename... t>
		void
		remove_component(t_entity_id id)
		{
			using namespace std::ranges::views;

			auto& ent_info					= entity_info_vec[id];
			auto  new_archetype				= static_cast<t_archetype>(ent_info.archetype ^ t_archetype_traits::template calc_archetype<t...>());
			auto& src_ent_group_collection	= entity_groups_map[ent_info.archetype];
			auto& dest_ent_group_collection = entity_groups_map[new_archetype];
			auto& src_group					= *ent_info.p_group;
			auto& dst_group					= dest_ent_group_collection.free_group(new_archetype);
			auto  need_src_update			= src_group.is_full();

			for (auto [idx, storage_cmp_idx] : iota(0, std::bit_width(new_archetype))
												   | filter([archetype = new_archetype](auto idx) { return (archetype >> idx) & 1; })
												   | enumerate)
			{
				auto dest_local_cmp_idx = static_cast<t_local_cmp_idx>(idx);
				auto src_local_cmp_idx	= t_archetype_traits::calc_local_cmp_idx(ent_info.archetype, storage_cmp_idx);
				src_group.evict_component(ent_info.local_idx, src_local_cmp_idx, dst_group.get_component_write_ptr(dest_local_cmp_idx));
			}

			(src_group.evict_component(ent_info.local_idx, t_archetype_traits::template calc_local_cmp_idx<t>(ent_info.archetype)), ...);

			src_group.ent_id(ent_info.local_idx)	   = src_group.ent_id(--src_group.entity_count());
			dst_group.ent_id(dst_group.entity_count()) = id;

			ent_info.archetype = new_archetype;
			ent_info.p_group   = &dst_group;
			ent_info.local_idx = dst_group.entity_count()++;

			if (need_src_update)
			{
				src_ent_group_collection.update_free(src_group.entity_group_idx());
			}
			if (dst_group.is_full())
			{
				dest_ent_group_collection.update_full(dst_group.entity_group_idx());
			}
		}

		template <typename... t>
		inline decltype(auto)
		get_component(t_entity_id id)
		{
			auto& ent_info = entity_info_vec[id];
			return ent_info.p_group->template get_component<t...>(ent_info.local_idx);
		}

		template <typename... t>
		bool
		has_component(t_entity_id id)
		{
			constexpr auto archetype = t_archetype_traits::template calc_archetype<t...>();
			return (entity_info_vec[id].archetype & archetype) == archetype;
		}

		void
		load_from_file(const char* path)
		{
		}

		void
		save_to_file(const char* path) const
		{
		}

		void
		load_from_memory(const void* data, std::size_t size)
		{
		}

		template <typename... t, typename t_lambda>
		void
		each_entity(t_lambda&& fn)
		{
			// std::ranges::for_each(entity_groups_map[t_archetype_traits::template calc_archetype<t...>()],
			//					  fn)
		}

		template <typename... t, typename t_lambda>
		void
		each_group(t_lambda&& fn)
		{
		}

		void
		deinit()
		{
			std::ranges::for_each(entity_groups_map | std::views::values | std::views::join, [](auto& ent_groups) { ent_groups.deinit(); });
		}
	};
}	 // namespace ecs::entity_storage
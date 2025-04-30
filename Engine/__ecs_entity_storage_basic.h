#pragma once
#include "__common.h"
#include "__ecs_common.h"
#include "__ecs_utility.h"
#include "__ecs_entity_group_basic.h"

namespace ecs::entity_storage
{
	template <typename t_entity_id, ecs::component_type... t_cmp>
	struct basic
	{
		using t_archetype_traits = ecs::utility::archetype_traits<t_cmp...>;
		using t_archetype		 = t_archetype_traits::t_archetype;
		using t_storage_cmp_idx	 = t_archetype_traits::t_storage_cmp_idx;
		using t_local_cmp_idx	 = t_archetype_traits::t_local_cmp_idx;
		using t_local_entity_idx = uint16;
		using t_entity_group	 = ecs::entity_group::basic<4_KiB, t_entity_id, t_local_entity_idx, t_cmp...>;

		struct entity_info
		{
			t_archetype archetype;

			// Index of the entity within its current entity_group.
			// Used to access component data in SoA blocks.
			// Must be updated when entities move between groups.
			t_local_entity_idx local_idx;

			t_entity_group& group;
		};

		data_structure::sparse_vector<entity_info>								  entity_info_vec;
		data_structure::map<t_archetype, data_structure::vector<t_entity_group*>> entity_groups_map;

		std::size_t entity_count() const noexcept
		{
			return entity_info_vec.size();
		}

	  private:
		// find or create and init entity_group
		template <typename... t>
		inline t_entity_group& _get_or_init_entity_group()
		{
			auto* p_entity_group   = (t_entity_group*)nullptr;
			auto& entity_group_vec = entity_groups_map[t_archetype_traits::template calc_archetype<t...>()];

			auto res = std::ranges::find_if_not(entity_group_vec, [](auto* p_group) { return p_group->is_full(); });
			if (res != entity_group_vec.end())
			{
				p_entity_group = *res;
			}
			else
			{
				p_entity_group = (t_entity_group*)malloc(sizeof(t_entity_group));
				p_entity_group->init<t...>();
				entity_group_vec.emplace_back(p_entity_group);
			}

			return *p_entity_group;
		}

		inline t_entity_group& _get_or_init_entity_group(t_archetype archetype)
		{
			auto* p_entity_group   = (t_entity_group*)nullptr;
			auto& entity_group_vec = entity_groups_map[archetype];

			auto res = std::ranges::find_if_not(entity_group_vec, [](auto* p_group) { return p_group->is_full(); });
			if (res != entity_group_vec.end())
			{
				p_entity_group = *res;
			}
			else
			{
				p_entity_group = (t_entity_group*)malloc(sizeof(t_entity_group));
				p_entity_group->init(archetype);
				entity_group_vec.emplace_back(p_entity_group);
			}

			return *p_entity_group;
		}

	  public:
		template <typename... t>
		t_entity_id new_entity()
		{
			constexpr auto archetype	= t_archetype_traits::template calc_archetype<t...>();
			auto&		   entity_group = _get_or_init_entity_group<t...>();
			auto		   entity_id	= static_cast<t_entity_id>(entity_info_vec.size());

			{
				auto entity_local_idx = entity_group.new_entity<t...>(entity_id);
				entity_info_vec.emplace_back(entity_info { archetype, entity_local_idx, entity_group });
			}

			return entity_id;
		}

		template <typename... t>
		t_entity_id new_entity(t&&... cmp)
		{
			constexpr auto archetype	= t_archetype_traits::template calc_archetype<t...>();
			auto&		   entity_group = _get_or_init_entity_group<t...>();
			auto		   entity_id	= static_cast<t_entity_id>(entity_info_vec.size());

			{
				auto entity_local_idx = entity_group.new_entity<t...>(entity_id, std::forward<t>(cmp)...);
				entity_info_vec.emplace_back(archetype, entity_local_idx, entity_group);
			}

			return entity_id;
		}

		void remove_entity(const t_entity_id id)
		{
			auto& ent_info = entity_info_vec[id];

			auto entity_id_last						  = ent_info.group.remove_entity(ent_info.local_idx);
			entity_info_vec[entity_id_last].local_idx = ent_info.local_idx;

			entity_info_vec.remove(id);
		}

		bool is_valid(t_entity_id id) const { return false; }

		// if dup cmp => UB
		template <typename... t, typename... t_arg>
		void add_component(const t_entity_id id, t_arg&&... arg)
		{
			using namespace std::ranges::views;

			auto& ent_info		= entity_info_vec[id];
			auto  new_archetype = static_cast<t_archetype>(ent_info.archetype | t_archetype_traits::template calc_archetype<t...>());
			auto& src_group		= ent_info.group;
			auto& dst_group		= _get_or_init_entity_group(new_archetype);

			for (auto			   prev_local_cmp_idx = (t_local_cmp_idx)0;
				 t_storage_cmp_idx storage_cmp_idx : iota(0, std::bit_width(ent_info.archetype))
														 | filter([archetype = ent_info.archetype](auto idx) { return (archetype >> idx) & 1; }))
			{
				auto next_local_cmp_idx = t_archetype_traits::calc_local_cmp_idx(new_archetype, storage_cmp_idx);

				src_group.evict_component(ent_info.local_idx, prev_local_cmp_idx++, dst_group.get_component_write_ptr(next_local_cmp_idx));
			}

			if constexpr (sizeof...(t_arg) == 0)
			{
				(std::construct_at(reinterpret_cast<t*>(dst_group.get_component_write_ptr(t_archetype_traits::template calc_local_cmp_idx<t>(new_archetype)))), ...);
			}
			else if constexpr (sizeof...(t_arg) == sizeof...(t))
			{
				(std::construct_at(reinterpret_cast<t*>(dst_group.get_component_write_ptr(t_archetype_traits::template calc_local_cmp_idx<t>(new_archetype))), std::forward<t_arg>(arg)), ...);
			}

			src_group.entity_id(ent_info.local_idx)			= src_group.entity_id(--src_group.entity_count());
			dst_group.entity_id(dst_group.entity_count()++) = id;
		}

		template <typename... t>
		void remove_component(t_entity_id id)
		{
			using namespace std::ranges::views;

			auto& ent_info		= entity_info_vec[id];
			auto  new_archetype = static_cast<t_archetype>(ent_info.archetype ^ t_archetype_traits::template calc_archetype<t...>());
			auto& src_group		= ent_info.group;
			auto& dst_group		= _get_or_init_entity_group(new_archetype);

			for (auto			   next_local_cmp_idx = (t_local_cmp_idx)0;
				 t_storage_cmp_idx storage_cmp_idx : iota(0, std::bit_width(new_archetype))
														 | filter([archetype = new_archetype](auto idx) { return (archetype >> idx) & 1; }))
			{
				auto prev_local_cmp_idx = t_archetype_traits::calc_local_cmp_idx(ent_info.archetype, storage_cmp_idx);

				src_group.evict_component(ent_info.local_idx, prev_local_cmp_idx, dst_group.get_component_write_ptr(next_local_cmp_idx++));
			}

			(src_group.evict_component(ent_info.local_idx, t_archetype_traits::template calc_local_cmp_idx<t>(ent_info.archetype)), ...);

			src_group.entity_id(ent_info.local_idx)			= src_group.entity_id(--src_group.entity_count());
			dst_group.entity_id(dst_group.entity_count()++) = id;
		}

		template <typename... t>
		inline decltype(auto) get_component(t_entity_id id)
		{
			auto& ent_info = entity_info_vec[id];
			return ent_info.group.get_component<t...>(ent_info.local_idx);
		}

		template <typename... t>
		bool has_component(t_entity_id id) const
		{
			constexpr auto archetype = t_archetype_traits::template calc_archetype<t...>();
			return (entity_info_vec[id].archetype & archetype) == archetype;
		}

		void load_from_file(const char* path) { }

		void save_to_file(const char* path) const { }

		void load_from_memory(const void* data, std::size_t size) { }

		template <typename... t_cmp, typename t_lambda>
		void each_entity(t_lambda&& fn)
		{
		}

		template <typename... t_cmp, typename t_lambda>
		void each_group(t_lambda&& fn)
		{
		}
	};
}	 // namespace ecs::entity_storage
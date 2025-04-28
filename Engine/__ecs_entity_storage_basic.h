#pragma once
#include "__common.h"
#include "__ecs_common.h"
#include "__ecs_utility.h"
#include "__ecs_entity_group_basic.h"

namespace ecs::entity_storage
{
	template <typename t_entity_id, typename... t_cmp>
	struct basic
	{
		using t_archetype_traits = ecs::utility::archetype_traits<t_cmp...>;
		using t_archetype		 = t_archetype_traits::t_archetype;
		using t_group_idx		 = uint32;
		using t_local_idx		 = uint16;
		using t_entity_group	 = entity_group::basic<t_entity_id, t_local_idx, t_cmp...>;

		struct entity_info
		{
			t_archetype archetype;
			// t_group_idx		entity_group_idx;
			t_local_idx		entity_local_idx;
			t_entity_group* p_entity_group;
		};

		data_structure::sparse_vector<entity_info>								  entity_info_vec;
		data_structure::map<t_archetype, data_structure::vector<t_entity_group*>> entity_groups_map;

		std::size_t entity_count() const noexcept
		{
			return entity_info_vec.size();
		}

		// Entity Lifecycle
		template <typename... t>
		t_entity_id new_entity()
		{
			constexpr auto archetype		= t_archetype_traits::template calc_archetype<t...>();
			auto&		   entity_group_vec = entity_groups_map[archetype];
			auto*		   p_entity_group	= (t_entity_group*)nullptr;
			auto		   entity_id		= static_cast<t_entity_id>(entity_info_vec.size());

			{
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
			}

			{
				auto entity_local_idx = p_entity_group->new_entity<t...>(entity_id);
				entity_info_vec.emplace_back(archetype, entity_local_idx, p_entity_group);
			}

			return entity_id;
		}

		template <typename... t>
		t_entity_id new_entity(t&&... cmp)
		{
			constexpr auto archetype		= t_archetype_traits::template calc_archetype<t...>();
			auto&		   entity_group_vec = entity_groups_map[archetype];
			auto*		   p_entity_group	= (t_entity_group*)nullptr;
			auto		   entity_id		= static_cast<t_entity_id>(entity_info_vec.size());

			{
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
			}

			{
				auto entity_local_idx = p_entity_group->new_entity<t...>(entity_id, std::forward<t>(cmp)...);
				entity_info_vec.emplace_back(archetype, entity_local_idx, p_entity_group);
			}

			return entity_id;
		}

		void remove_entity(t_entity_id id)
		{
			auto& e_info = entity_info_vec[id];

			auto entity_id_last								 = e_info.p_entity_group->remove_entity(e_info.entity_local_idx);
			entity_info_vec[entity_id_last].entity_local_idx = e_info.entity_local_idx;

			entity_info_vec.remove(id);
		}

		bool is_valid(t_entity_id id) const { return false; }

		// Component Lifecycle
		template <typename t_cmp>
		void add_component(t_entity_id id, t_cmp&& cmp)
		{
		}

		template <typename t_cmp>
		void remove_component(t_entity_id id)
		{
		}

		template <typename t_cmp>
		t_cmp& get_component(t_entity_id id)
		{
			static t_cmp dummy;
			return dummy;
		}

		template <typename t_cmp>
		const t_cmp& get_component(t_entity_id id) const
		{
			static t_cmp dummy;
			return dummy;
		}

		template <typename t_cmp>
		bool has_component(t_entity_id id) const
		{
			return false;
		}

		// Loading / Saving
		void load_from_file(const char* path) { }

		void save_to_file(const char* path) const { }

		void load_from_memory(const void* data, std::size_t size) { }

		// Iteration
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
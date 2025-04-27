#pragma once
#include "__common.h"
#include "__ecs_common.h"
#include "__ecs_crtp_entity_storage.h"

namespace ecs::entity_storage
{
	template <typename... t_cmp>
	struct basic : ecs::crtp::entity_storage<basic<t_cmp...>, uint32>
	{
		struct entity_info
		{
			t_archetype archetype;
			uint32		entity_group_idx;
		};

		data_structure::sparse_vector<entity_info>						 entity_info_vec;
		data_structure::map<t_archetype, data_structure::vector<uint32>> entity_groups_map;

		// Entity Lifecycle
		template <typename... t_cmp>
		t_entity_id add_entity(t_cmp&&... comps)
		{
			return 0;
		}

		void remove_entity(t_entity_id id) { }

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
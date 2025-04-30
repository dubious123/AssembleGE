#pragma once

namespace ecs::entity_group
{
	template <typename t_entity_id, typename t_entity_local_idx, typename... t_cmp>
	struct basic
	{
		using t_archetype_traits = ecs::utility::archetype_traits<t_cmp...>;
		using t_archetype		 = t_archetype_traits::t_archetype;

		template <typename... t>
		void init()
		{
		}

		void init(t_archetype archetype)
		{
		}

		template <typename... t>
		t_entity_local_idx new_entity(t_entity_id entity_id)
		{
			return t_entity_local_idx {};
		}

		template <typename... t>
		t_entity_local_idx new_entity(t_entity_id entity_id, t&&... arg)
		{
			return t_entity_local_idx {};
		}

		t_entity_local_idx remove_entity(t_entity_local_idx local_idx)
		{
			return t_entity_local_idx {};
		}

		bool is_full() const
		{
			return false;
		}
	};
}	 // namespace ecs::entity_group
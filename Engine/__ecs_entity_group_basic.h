#pragma once

namespace ecs::entity_group
{
	template <std::size_t size, typename t_entity_id, typename t_local_entity_idx, ecs::component_type... t_cmp>
	struct basic
	{
		using t_archetype_traits = ecs::utility::archetype_traits<t_cmp...>;
		using t_archetype		 = t_archetype_traits::t_archetype;
		using t_local_cmp_idx	 = t_archetype_traits::t_local_cmp_idx;

		alignas(ecs::utility::max_alignof<t_cmp...>())
			std::byte storage[size];

		template <typename... t>
		void init()
		{
		}

		void init(t_archetype archetype)
		{
		}

		t_local_entity_idx& entity_count()
		{
			static auto _ = t_local_entity_idx {};
			return _;
		}

		t_entity_id& entity_id(t_local_entity_idx local_ent_idx)
		{
			static auto _ = t_entity_id {};
			return _;
		}

		template <typename... t>
		t_local_entity_idx new_entity(t_entity_id entity_id)
		{
			return t_local_entity_idx {};
		}

		template <typename... t>
		t_local_entity_idx new_entity(t_entity_id entity_id, t&&... arg)
		{
			return t_local_entity_idx {};
		}

		t_local_entity_idx remove_entity(t_local_entity_idx local_ent_idx)
		{
			return t_local_entity_idx {};
		}

		void evict_component(const t_local_entity_idx local_ent_idx, const t_local_cmp_idx local_cmp_idx, void* p_dest)
		{
		}

		void evict_component(const t_local_entity_idx local_ent_idx, const t_local_cmp_idx local_cmp_idx)
		{
		}

		void* get_component_write_ptr(const t_local_cmp_idx local_cmp_idx)
		{
			return nullptr;
		}

		template <typename... t>
		decltype(auto) get_component(const t_local_entity_idx local_ent_idx)
		{
			using ret_t = std::conditional_t<
				std::is_const_v<decltype(*this)>,
				std::conditional_t<
					(sizeof...(t) == 1),
					const meta::variadic_at_t<0, t&...>,
					const std::tuple<t&...>>,
				std::conditional_t<
					(sizeof...(t) == 1),
					meta::variadic_at_t<0, t&...>,
					std::tuple<t&...>>>;
			static ret_t _;

			return _;
		}

		bool is_full() const
		{
			return false;
		}
	};
}	 // namespace ecs::entity_group
#pragma once
#include "__ecs_common.h"
#ifndef INCLUDED_FROM_ECS_SYSTEM_HEADER
	#error "Do not include this file directly. Include <__ecs_system.h> instead."
#endif

namespace ecs::system
{
	template <component_type... t>
	struct with_clause
	{
		using types = std::tuple<t...>;

		constexpr with_clause() = default;
	};

	template <component_type... t>
	struct without_clause
	{
		using types = std::tuple<t...>;

		constexpr without_clause() = default;
	};

	template <component_type... t>
	static constexpr inline auto with = with_clause<t...>{};

	template <component_type... t>
	static constexpr inline auto without = without_clause<t...>{};

	namespace detail
	{
		template <typename t>
		struct is_with_clause : std::false_type
		{
		};

		template <typename... t>
		struct is_with_clause<with_clause<t...>> : std::true_type
		{
		};

		template <typename t>
		struct is_without_clause : std::false_type
		{
		};

		template <typename... t>
		struct is_without_clause<without_clause<t...>> : std::true_type
		{
		};

		template <typename with, typename without>
		consteval bool
		validate_query()
		{
			constexpr auto duplicated =
				[]<auto... i>(std::index_sequence<i...>) {
					return ((meta::tuple_empty_v<
							 meta::filtered_tuple_t<
								 meta::pred<std::tuple_element_t<i, typename with::types>>::template is_same,
								 without::types>>)and...);
				}(std::make_index_sequence<std::tuple_size_v<typename with::types>>{});

			static_assert(duplicated, "query: same component(s) in with<> and without<>");
			return duplicated;
		}
	}	 // namespace detail

	template <typename... t_clause>
	struct query
	{
		using with	  = meta::variadic_find_t<detail::is_with_clause, with_clause<>, std::decay_t<t_clause>...>;
		using without = meta::variadic_find_t<detail::is_without_clause, without_clause<>, std::decay_t<t_clause>...>;

		static_assert(detail::validate_query<with, without>(), "query: invalid query");

		query(t_clause&... clause) requires(sizeof...(t_clause) > 0)
		{};

		constexpr query() = default;
	};

	template <typename t_query, typename t_sys>
	struct each_group
	{
		no_unique_addr t_sys sys;

		each_group(t_query&& query, t_sys&& sys) : sys(FWD(sys)){};

		constexpr each_group() = default;

		FORCE_INLINE constexpr decltype(auto)
		operator()(auto&& groups)
		{
			using t_ent_storage = std::decay_t<decltype(groups)>;
			using t_ent_group	= t_ent_storage::t_entity_group;

			using sys_trait = meta::function_traits<&std::decay_t<t_sys>::template operator()<t_ent_group&>>;
			static_assert(
				std::tuple_size_v<typename sys_trait::argument_types> == 1
					and meta::is_specialization_of_v<
						std::tuple_element_t<0, typename sys_trait::argument_types>,
						i_entity_group>,
				"each_group : The system must be callable with exactly one parameter of type i_entity_group<T>.");

			groups.each_group(t_query{}, sys);
		}
	};

	template <typename t>
	concept is_interface_entity_group = requires(t&& _) {
		typename i_entity_group<t>;
		// typename t::t_ent_id;
		// typename t::t_ent_group_idx;
		// typename t::t_local_entity_idx;
		// typename t::t_storage_cmp_idx;
		// typename t::t_archetype;
		// typename t::t_entity_count;
		// typename t::t_capacity;
		// typename t::t_local_cmp_idx;
		// typename t::t_component_count;
		// typename t::t_component_size;
		// typename t::t_component_offset;
		// typename t::t_cmp_offset_arr_base;
		// typename t::t_cmp_size_arr_base;
		// typename t::t_entity_id_arr_base;
		{
			i_entity_group<t>{ FWD(_) }
		};
	};

	template <typename t>
	concept is_interface_entity_storage = requires(t&& _) { i_entity_storage<t>{ FWD(_) }; };

	template <typename t_query, typename t_sys>
	struct each_entity
	{
		no_unique_addr t_sys sys;

		each_entity(t_query&& query, t_sys&& sys) : sys(FWD(sys)){};

		constexpr each_entity() = default;

		// template <typename g>
		// requires requires(g obj) { i_entity_group<g>{ obj }; }
		// FORCE_INLINE constexpr decltype(auto)
		// operator()(i_entity_group<g> i_group)
		//{
		// }

		// template <typename s>
		// requires requires(s obj) { i_entity_storage<s>{ obj }; }
		// FORCE_INLINE constexpr decltype(auto)
		// operator()(i_entity_storage<s> i_storage)
		//{
		// }

		template <typename t_arg>
		FORCE_INLINE constexpr decltype(auto)
		operator()(t_arg&& arg)
		{
			if constexpr (is_interface_entity_group<t_arg>)
			{
			}
			else if constexpr (is_interface_entity_storage<t_arg>)
			{
			}
			else
			{
				static_assert(false, "each_entity : invalid arguement");
			}
		}
	};
}	 // namespace ecs::system
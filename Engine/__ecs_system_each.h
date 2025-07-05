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
		constexpr with_clause() = default;
	};

	template <component_type... t>
	struct without_clause
	{
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
	}	 // namespace detail

	template <typename... t_clause>
	struct query
	{
		using with1	   = meta::variadic_find_t<detail::is_with_clause, with_clause<>, std::decay_t<t_clause>...>;
		using without1 = meta::variadic_find_t<detail::is_without_clause, without_clause<>, std::decay_t<t_clause>...>;

		query(t_clause&... clause) { };

		constexpr query() = default;
	};

	template <typename t_query, typename t_sys>
	struct each_group
	{
		no_unique_addr t_sys sys;

		each_group(t_query&& query, t_sys&& sys) : sys(FWD(sys)) { };

		constexpr each_group() = default;

		FORCE_INLINE constexpr decltype(auto)
		operator()(auto&& groups)
		{
			groups.each_group(t_query{}, sys);
		}
	};
}	 // namespace ecs::system
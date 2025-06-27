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
	};

	template <component_type... t>
	struct without_clause
	{
	};

	template <typename... t_clause>
	struct query
	{
		query(t_clause&... clause) { };

		constexpr query() = default;
	};

	template <component_type... t>
	static constexpr inline auto with = with_clause<t...>{};

	template <component_type... t>
	static constexpr inline auto without = without_clause<t...>{};

	template <typename t_query, typename t_sys>
	struct each_group
	{
		no_unique_addr t_sys sys;

		each_group(t_query&& qeury, t_sys&& sys) { };

		constexpr each_group() = default;

		template <typename... t_arg>
		FORCE_INLINE constexpr decltype(auto)
		operator()(t_arg&&... arg)
		{
		}
	};
}	 // namespace ecs::system
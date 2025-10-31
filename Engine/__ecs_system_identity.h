#pragma once
#ifndef INCLUDED_FROM_ECS_SYSTEM_HEADER
	#error "Do not include this file directly. Include <__ecs_system.h> instead."
#endif

namespace ecs::system
{
	using namespace detail;

	template <typename t_data>
	struct identity
	{
		no_unique_addr t_data data;

		template <typename t_arg>
		constexpr identity(t_arg&& arg) noexcept : data(FWD(arg)){};

		constexpr identity() noexcept requires(std::is_empty_v<t_data> and std::is_default_constructible_v<t_data>)
		= default;

		FORCE_INLINE constexpr decltype(auto)
		operator()() noexcept
		{
			return FWD(data);
		}
	};

	template <typename t_arg>
	identity(t_arg&&) -> identity<meta::value_or_ref_t<t_arg&&>>;
}	 // namespace ecs::system
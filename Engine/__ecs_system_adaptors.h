#pragma once
#ifndef INCLUDED_FROM_ECS_SYSTEM_HEADER
	#error "Do not include this file directly. Include <__ecs_system.h> instead."
#endif

namespace ecs::system
{
	using namespace detail;

	template <typename t_fn>
	struct for_each
	{
		no_unique_addr t_fn fn;

		constexpr for_each(t_fn&& f) noexcept : fn(FWD(f)) { };

		constexpr for_each() noexcept requires(std::is_empty_v<t_fn> and std::is_default_constructible_v<t_fn>)
		= default;

		FORCE_INLINE constexpr decltype(auto)
		operator()(auto&& rng) noexcept
		{
			std::ranges::for_each(FWD(rng), fn);
		}
	};

	struct sum
	{
		FORCE_INLINE constexpr decltype(auto)
		operator()(auto&& rng) noexcept
		{
			return std::ranges::fold_left(FWD(rng), 0, std::plus{});
		}
	};

#define def_unary_adaptor(name, std_name)                                                                   \
	template <typename t_fn>                                                                                \
	struct name                                                                                             \
	{                                                                                                       \
		no_unique_addr t_fn fn;                                                                             \
                                                                                                            \
		constexpr name(t_fn&& f) noexcept : fn(FWD(f)) { };                                                 \
                                                                                                            \
		constexpr name() noexcept requires(std::is_empty_v<t_fn> and std::is_default_constructible_v<t_fn>) \
		= default;                                                                                          \
                                                                                                            \
		FORCE_INLINE constexpr decltype(auto)                                                               \
		operator()(auto&& rng) noexcept                                                                     \
		{                                                                                                   \
			return FWD(rng) | std::ranges::views::std_name(fn);                                             \
		}                                                                                                   \
	};                                                                                                      \
                                                                                                            \
	template <typename t_arg>                                                                               \
	name(t_arg&&) -> name<meta::value_or_ref_t<t_arg&&>>;

	// clang-format off
	def_unary_adaptor(filter, filter)

	def_unary_adaptor(take, take)

	def_unary_adaptor(map, transform)
	// clang-format on

#undef def_unary_adaptor
}	 // namespace ecs::system

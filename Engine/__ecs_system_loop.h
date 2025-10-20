#pragma once
#ifndef INCLUDED_FROM_ECS_SYSTEM_HEADER
	#error "Do not include this file directly. Include <__ecs_system.h> instead."
#endif

namespace ecs::system
{
	using namespace detail;

	template <typename t_sys_cond>
	struct break_if
	{
		no_unique_addr t_sys_cond sys_cond;

		constexpr break_if(t_sys_cond&& sys_cond) : sys_cond(FWD(sys_cond)) { }

		constexpr break_if() = default;

		template <typename... t_arg>
		FORCE_INLINE constexpr decltype(auto)
		operator()(t_arg&&... arg)
		{
			using t_res_cond = decltype(run_sys(sys_cond, FWD(arg)...));
			static_assert(std::is_convertible_v<t_res_cond, bool>,
						  "[cond] sys_cond must return a type convertible to bool - check that it's callable and produces a condition-like value");

			return run_sys(sys_cond, FWD(arg)...);
		}
	};

	template <typename t_sys_cond>
	struct continue_if
	{
		no_unique_addr t_sys_cond sys_cond;

		constexpr continue_if(t_sys_cond&& sys_cond) : sys_cond(FWD(sys_cond)) { }

		constexpr continue_if() = default;

		template <typename... t_arg>
		FORCE_INLINE constexpr decltype(auto)
		operator()(t_arg&&... arg)
		{
			using t_res_cond = decltype(run_sys(sys_cond, FWD(arg)...));
			static_assert(std::is_convertible_v<t_res_cond, bool>,
						  "[cond] sys_cond must return a type convertible to bool - check that it's callable and produces a condition-like value");

			return run_sys(sys_cond, FWD(arg)...);
		}
	};

	template <typename t>
	inline constexpr bool is_break_if = false;

	template <typename t_sys_cond>
	inline constexpr bool is_break_if<break_if<t_sys_cond>> = true;

	template <typename t>
	inline constexpr bool is_continue_if = false;

	template <typename t_sys_cond>
	inline constexpr bool is_continue_if<continue_if<t_sys_cond>> = true;

	template <typename t>
	constexpr bool is_break_or_continue = is_break_if<std::decay_t<t>> || is_continue_if<std::decay_t<t>>;

	template <typename t_sys_cond, typename... t_sys>
	struct loop
	{
		using t_not_empty_idx_seq = meta::arr_to_seq_t<not_empty_sys_idx_arr<t_sys...>()>;
		using t_sys_not_empty	  = meta::filtered_variadic_t<meta::is_not_empty, t_sys...>;

		no_unique_addr t_sys_cond	   sys_cond;
		no_unique_addr t_sys_not_empty systems;

		constexpr loop(t_sys_cond&& sys_cond, t_sys&&... sys)
			: sys_cond(FWD(sys_cond)),
			  systems(meta::make_filtered_tuple<meta::is_not_empty, t_sys...>(FWD(sys)...)) { };

		constexpr loop() requires(std::is_empty_v<t_sys_cond> and ... and std::is_empty_v<t_sys>)
		= default;

		template <std::size_t i, typename... t_arg>
		FORCE_INLINE constexpr decltype(auto)
		run_impl(t_arg&&... arg)
		{
			using t_sys_now = meta::variadic_at_t<i, t_sys...>;
			if constexpr (std::is_empty_v<t_sys_now>)
			{
				return run_sys(t_sys_now{}, FWD(arg)...);
			}
			else
			{
				return run_sys(std::get<meta::index_sequence_at_v<i, t_not_empty_idx_seq>>(systems), FWD(arg)...);
			}
		}

#define __SYS_LOOP_IMPL(N)                                          \
	if constexpr (N < sizeof...(t_sys))                             \
	{                                                               \
		using t_sys_now = meta::variadic_at_t<N, t_sys...>;         \
		if constexpr (is_break_if<std::decay_t<t_sys_now>>)         \
		{                                                           \
			if (run_impl<N>(FWD(arg)...))                           \
				break;                                              \
		}                                                           \
		else if constexpr (is_continue_if<std::decay_t<t_sys_now>>) \
		{                                                           \
			if (run_impl<N>(FWD(arg)...))                           \
				continue;                                           \
		}                                                           \
		else                                                        \
		{                                                           \
			run_impl<N>(FWD(arg)...);                               \
		}                                                           \
	}

		template <typename... t_arg>
		FORCE_INLINE constexpr decltype(auto)
		operator()(t_arg&&... arg)
		{
			using t_res_cond = decltype(run_sys(sys_cond, FWD(arg)...));
			static_assert(std::is_convertible_v<t_res_cond, bool>,
						  "[loop] sys_cond must return a type convertible to bool - check that it's callable and produces a condition-like value");

			[this]<auto... i>(std::index_sequence<i...>) {
				static_assert(((not std::is_same_v<decltype(run_impl<i>(FWD(arg)...)), invalid_sys_call>) && ...),
							  "[loop] invalid_sys_call - check that system i is callable with given arguments.");
			}(std::index_sequence_for<t_sys...>{});

			static_assert(sizeof...(t_sys) <= 512, "too many systems");

			auto args = make_arg_tpl(FWD(arg)...);

			std::apply(
				[this](auto&&... arg) {
					while (run_sys(sys_cond, FWD(arg)...))
					{
#define X(N) __SYS_LOOP_IMPL(N)
						__X_REPEAT_LIST_512
#undef X
					}
				},
				args);
		}

#undef __SYS_LOOP_IMPL
	};
}	 // namespace ecs::system
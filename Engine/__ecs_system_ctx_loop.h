#pragma once
#ifndef INCLUDED_FROM_ECS_SYSTEM_HEADER
	#error "Do not include this file directly. Include <__ecs_system.h> instead."
#endif

namespace ecs::system::ctx
{
	using namespace detail;

	template <typename t_sys_cond>
	struct break_if
	{
		no_unique_addr t_sys_cond sys_cond;

		constexpr break_if(auto&& arg) noexcept : sys_cond{ FWD(arg) } { }

		FORCE_INLINE constexpr decltype(auto)
		operator()(cx_ctx auto&& ctx, auto&&... arg) noexcept
		{
			static_assert(
				requires { { run_sys(FWD(ctx), sys_cond, FWD(arg)...) } -> std::convertible_to<bool>; },
				"[break_if] sys_cond must return a type convertible to bool - check that it's callable and produces a condition-like value");

			return run_sys(FWD(ctx), sys_cond, FWD(arg)...);
		}
	};

	template <typename t_sys>
	break_if(t_sys&&) -> break_if<t_sys>;

	template <typename t_sys_cond>
	struct continue_if
	{
		no_unique_addr t_sys_cond sys_cond;

		constexpr continue_if(auto&& arg) noexcept : sys_cond{ FWD(arg) } { }

		FORCE_INLINE constexpr decltype(auto)
		operator()(cx_ctx auto&& ctx, auto&&... arg) noexcept
		{
			static_assert(
				requires { { run_sys(FWD(ctx), sys_cond, FWD(arg)...) } -> std::convertible_to<bool>; },
				"[continue_if] sys_cond must return a type convertible to bool - check that it's callable and produces a condition-like value");

			return run_sys(FWD(ctx), sys_cond, FWD(arg)...);
		}
	};

	template <typename t_sys>
	continue_if(t_sys&&) -> continue_if<t_sys>;

	template <typename t>
	inline constexpr bool is_break_if = false;

	template <typename t_sys_cond>
	inline constexpr bool is_break_if<break_if<t_sys_cond>> = true;

	template <typename t>
	inline constexpr bool is_continue_if = false;

	template <typename t_sys_cond>
	inline constexpr bool is_continue_if<continue_if<t_sys_cond>> = true;

	template <typename t_sys_cond, typename... t_sys>
	struct loop
	{
		using t_ctx_tag = ctx_tag<tag_adaptor, tag_ctx_bound>;

		no_unique_addr t_sys_cond sys_cond;

		no_unique_addr compressed_pack<t_sys...> systems;

		constexpr loop(auto&& sys_cond_arg, auto&&... sys_arg) noexcept
			: sys_cond{ FWD(sys_cond_arg) },
			  systems{ FWD(sys_arg)... } { };

#define __SYS_LOOP_IMPL(N)                                          \
	if constexpr (N < sizeof...(t_sys))                             \
	{                                                               \
		using t_sys_now = meta::variadic_at_t<N, t_sys...>;         \
		if constexpr (is_break_if<std::decay_t<t_sys_now>>)         \
		{                                                           \
			if (run_sys(ctx, systems.get<N>(), arg...))             \
				break;                                              \
		}                                                           \
		else if constexpr (is_continue_if<std::decay_t<t_sys_now>>) \
		{                                                           \
			if (run_sys(ctx, systems.get<N>(), arg...))             \
				continue;                                           \
		}                                                           \
		else if constexpr (N < (sizeof...(t_sys) - 1))              \
		{                                                           \
			run_sys(ctx, systems.get<N>(), arg...);                 \
		}                                                           \
		else                                                        \
		{                                                           \
			run_sys(FWD(ctx), systems.get<N>(), FWD(arg)...);       \
		}                                                           \
	}

	  private:
		template <std::size_t nth, typename t_ctx, typename t_sys, typename... t_arg>
		static consteval bool
		validate_nth_sys()
		{
			static_assert(
				requires { run_sys(std::declval<t_ctx>(), std::declval<t_sys>(), std::declval<t_arg>()...); },
				"[loop] invalid_sys_call - check that system i is callable with given arguments.");
			return true;
		}

	  public:
		FORCE_INLINE constexpr decltype(auto)
		operator()(cx_ctx auto&& ctx, auto&&... arg) noexcept
		{
			static_assert(
				requires { { run_sys(ctx, sys_cond, arg...) } -> std::convertible_to<bool>; },
				"[loop] sys_cond must return a type convertible to bool - check that it's callable and produces a condition-like value");


			//[this, &ctx, &arg...]<std::size_t... i>(std::index_sequence<i...>) {
			//	(validate_nth_sys<i, decltype(ctx), decltype(this->systems.template get<i>()), decltype(arg)...>(), ...);
			//}(std::index_sequence_for<t_sys...>{});

			static_assert(sizeof...(t_sys) <= 512, "[loop] too many systems");

			while (run_sys(ctx, sys_cond, arg...))
			{
#define X(N) __SYS_LOOP_IMPL(N)
				__X_REPEAT_LIST_512
#undef X
			}
		}

#undef __SYS_LOOP_IMPL
	};

	template <typename t_sys_cond, typename... t_sys>
	loop(t_sys_cond&&, t_sys&&...) -> loop<t_sys_cond, t_sys...>;
}	 // namespace ecs::system::ctx
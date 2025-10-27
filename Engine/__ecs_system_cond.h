#pragma once
#ifndef INCLUDED_FROM_ECS_SYSTEM_HEADER
	#error "Do not include this file directly. Include <__ecs_system.h> instead."
#endif

namespace ecs::system
{
	using namespace detail;

	template <typename t_sys_cond, typename t_sys_then, typename t_sys_else>
	struct cond
	{
		no_unique_addr t_sys_cond sys_cond;
		no_unique_addr t_sys_then sys_then;
		no_unique_addr t_sys_else sys_else;

		constexpr cond(t_sys_cond&& sys_cond, t_sys_then&& sys_then, t_sys_else&& sys_else) noexcept
			: sys_cond(FWD(sys_cond)),
			  sys_then(FWD(sys_then)),
			  sys_else(FWD(sys_else))
		{
		}

		constexpr cond()
			requires(std::is_empty_v<t_sys_cond> and std::is_empty_v<t_sys_then> and std::is_empty_v<t_sys_else>)
			= default;

		template <typename... t_arg>
		FORCE_INLINE constexpr decltype(auto)
		operator()(t_arg&&... arg) noexcept
		{
			using t_res_then = decltype(run_sys(sys_then, FWD(arg)...));
			using t_res_else = decltype(run_sys(sys_else, FWD(arg)...));
			static_assert(std::is_same_v<t_res_then, t_res_else>, "cond: 'then' and 'else' systems must return the same type");

			auto args = make_arg_tpl(FWD(arg)...);

			return std::apply(
				[this](auto&&... arg) noexcept {
					if (run_sys(sys_cond, FWD(arg)...))
					{
						return run_sys(sys_then, FWD(arg)...);
					}
					else
					{
						return run_sys(sys_else, FWD(arg)...);
					}
				},
				args);
		}
	};

	// constexpr inline auto when = sys_when<>();

	template <typename t_sys_cond, typename t_sys_then>
	struct cond<t_sys_cond, t_sys_then, void>
	{
		no_unique_addr t_sys_cond sys_cond;
		no_unique_addr t_sys_then sys_then;

		constexpr cond(t_sys_cond&& sys_cond, t_sys_then&& sys_then) noexcept
			: sys_cond(FWD(sys_cond)),
			  sys_then(FWD(sys_then))
		{
		}

		constexpr cond()
			requires(std::is_empty_v<t_sys_cond> and std::is_empty_v<t_sys_then>)
			= default;

		template <typename... t_arg>
		FORCE_INLINE constexpr decltype(auto)
		operator()(t_arg&&... arg) noexcept
		{
			using t_res_cond = decltype(run_sys(sys_cond, FWD(arg)...));
			using t_res_then = decltype(run_sys(sys_then, FWD(arg)...));
			static_assert(std::is_same_v<t_res_cond, bool>, "cond: system sys_cond is invalid - check if the system is callable with the given arguments or if the system returns bool");
			static_assert(not std::is_same_v<t_res_then, invalid_sys_call>, "cond: system sys_then is invalid - check if the system is callable with the given arguments");
			static_assert(std::is_same_v<t_res_then, void>, "cond: 'then' system must return void");

			auto args = make_arg_tpl(FWD(arg)...);

			return std::apply(
				[this](auto&&... arg) noexcept {
					if (run_sys(sys_cond, FWD(arg)...))
					{
						return run_sys(sys_then, FWD(arg)...);
					}
				},
				args);
		}
	};

	// Explicit deduction guides for cond because CTAD does not consider partial specializations
	template <typename t_sys_cond, typename t_sys_then>
	cond(t_sys_cond&&, t_sys_then&&) -> cond<t_sys_cond, t_sys_then, void>;

	template <typename t_sys_cond, typename t_sys_then, typename t_sys_else>
	cond(t_sys_cond&&, t_sys_then&&, t_sys_else&&) -> cond<t_sys_cond, t_sys_then, t_sys_else>;
}	 // namespace ecs::system
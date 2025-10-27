#pragma once
#ifndef INCLUDED_FROM_ECS_SYSTEM_HEADER
	#error "Do not include this file directly. Include <__ecs_system.h> instead."
#endif

namespace ecs::system
{
	using namespace detail;

	template <typename t_sys_l, typename t_sys_r>
	struct pipe
	{
		no_unique_addr t_sys_l sys_l;
		no_unique_addr t_sys_r sys_r;

		constexpr pipe(t_sys_l&& sys_l, t_sys_r&& sys_r) noexcept
			: sys_l(FWD(sys_l)),
			  sys_r(FWD(sys_r)) { }

		constexpr pipe() noexcept requires(std::is_empty_v<t_sys_l> and std::is_empty_v<t_sys_r>)
		= default;

		template <typename... t_arg>
		FORCE_INLINE constexpr decltype(auto)
		operator()(t_arg&&... arg) noexcept
		{
			using t_ret_l = decltype(run_sys(sys_l, FWD(arg)...));
			static_assert(meta::is_not_same_v<t_ret_l, invalid_sys_call>,
						  "[pipe] invalid_sys_call - check that system left is callable with given arguments.");

			if constexpr (std::is_void_v<t_ret_l>)
			{
				using t_ret_r = decltype(run_sys(sys_r));
				static_assert(meta::is_not_same_v<t_ret_r, invalid_sys_call>,
							  "[pipe] invalid_sys_call - check that system right is chain-able");

				run_sys(sys_l, FWD(arg)...);
				return run_sys(sys_r);
			}
			else if constexpr (meta::is_tuple_like_v<t_ret_l>)
			{
				using t_ret_r = decltype(meta::tuple_unpack([this](auto&&... l_ref_arg) { return run_sys(sys_r, FWD(l_ref_arg)...); }, run_sys(sys_l, FWD(arg)...)));
				if constexpr (meta::is_not_same_v<t_ret_r, invalid_sys_call>)
				{
					return meta::tuple_unpack([this](auto&&... l_ref_arg) noexcept { return run_sys(sys_r, FWD(l_ref_arg)...); }, run_sys(sys_l, FWD(arg)...));
				}
				else
				{
					static_assert(meta::is_not_same_v<decltype(run_sys(sys_r, run_sys(sys_l, FWD(arg)...))), invalid_sys_call>,
								  "[pipe] invalid_sys_call - check that system right is chain-able");
					return run_sys(sys_r, run_sys(sys_l, FWD(arg)...));
				}
			}
			else
			{
				static_assert(meta::is_not_same_v<decltype(run_sys(sys_r, run_sys(sys_l, FWD(arg)...))), invalid_sys_call>,
							  "[pipe] invalid_sys_call - check that system right is chain-able");
				return run_sys(sys_r, run_sys(sys_l, FWD(arg)...));
			}
		}
	};

	template <typename t_sys_l, typename t_sys_r>
	FORCE_INLINE constexpr decltype(auto)
	operator|(t_sys_l&& sys_l, t_sys_r&& sys_r) noexcept
	{
		return pipe{ FWD(sys_l), FWD(sys_r) };
	}
}	 // namespace ecs::system
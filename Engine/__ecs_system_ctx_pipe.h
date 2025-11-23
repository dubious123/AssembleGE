#pragma once
#ifndef INCLUDED_FROM_ECS_SYSTEM_HEADER
	#error "Do not include this file directly. Include <__ecs_system.h> instead."
#endif

namespace ecs::system::ctx
{
	using namespace detail;

	template <typename t_sys_l, typename t_sys_r>
	struct pipe : sys_ctx_bound
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
		operator()(cx_ctx auto&& ctx, t_arg&&... arg) noexcept
		{
			static_assert(
				meta::is_not_same_v<decltype(ctx.execute(sys_l, FWD(arg)...)), system::detail::invalid_sys_call>,
				"[pipe] invalid_sys_call - check that system left is callable with given arguments.");
			using t_ret_l = decltype(ctx.execute(sys_l, FWD(arg)...));

			if constexpr (std::is_void_v<t_ret_l>)
			{
				static_assert(
					meta::is_not_same_v<decltype(ctx.execute(sys_r)), system::detail::invalid_sys_call>,
					"[pipe] invalid_sys_call - check that system right is chain-able");

				ctx.execute(sys_l, FWD(arg)...);
				return ctx.execute(sys_r);
			}
			else if constexpr (meta::is_tuple_like_v<t_ret_l>)
			{
				if constexpr (
					meta::is_not_same_v<decltype(meta::tuple_unpack([this](auto&&... l_ref_arg) { return ctx.execute(sys_r, FWD(l_ref_arg)...); }, ctx.execute(sys_l, FWD(arg)...))),
										system::detail::invalid_sys_call>)
				{
					return meta::tuple_unpack([this](auto&&... l_ref_arg) { return ctx.execute(sys_r, FWD(l_ref_arg)...); }, ctx.execute(sys_l, FWD(arg)...));
				}
				else
				{
					static_assert(
						meta::is_not_same_v<decltype(ctx.execute(sys_r, ctx.execute(sys_l, FWD(arg)...))),
											system::detail::invalid_sys_call>,
						"[pipe] invalid_sys_call - check that system right is chain-able");
					return ctx.execute(sys_r, ctx.execute(sys_l, FWD(arg)...));
				}
			}
			else
			{
				static_assert(
					meta::is_not_same_v<decltype(ctx.execute(sys_r, ctx.execute(sys_l, FWD(arg)...))),
										system::detail::invalid_sys_call>,
					"[pipe] invalid_sys_call - check that system right is chain-able");

				return ctx.execute(sys_r, ctx.execute(sys_l, FWD(arg)...));
			}
		}
	};

	template <typename t_sys_l, typename t_sys_r>
	FORCE_INLINE constexpr decltype(auto)
	operator|(t_sys_l&& sys_l, t_sys_r&& sys_r) noexcept
	{
		return pipe{ FWD(sys_l), FWD(sys_r) };
	}
}	 // namespace ecs::system::ctx
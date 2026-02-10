#pragma once

namespace age::ecs::system
{
	template <typename t_sys_cond, typename t_sys_then, typename t_sys_else>
	struct cond
	{
		using t_ctx_tag = ctx_tag<tag_adaptor, tag_ctx_bound>;

		no_unique_addr t_sys_cond sys_cond;
		no_unique_addr t_sys_then sys_then;
		no_unique_addr t_sys_else sys_else;

		FORCE_INLINE constexpr cond(auto&& sys_cond, auto&& sys_then, auto&& sys_else) noexcept
			: sys_cond{ FWD(sys_cond) },
			  sys_then{ FWD(sys_then) },
			  sys_else{ FWD(sys_else) }
		{
		}

		FORCE_INLINE constexpr decltype(auto)
		operator()(this auto&& self, cx_ctx auto&& ctx, auto&&... arg) noexcept
		{
			static_assert(
				requires { { run_sys(ctx,  FWD(self).sys_cond, arg...) } -> std::convertible_to<bool>; },
				"[cond] system sys_cond is invalid - check if the system is callable with the given arguments or if the system returns bool");

			static_assert(
				requires {
					{ run_sys(FWD(ctx), FWD(self).sys_then, FWD(arg)...) };
					{ run_sys(FWD(ctx), FWD(self).sys_else, FWD(arg)...) };

					requires std::same_as<
						decltype(run_sys(FWD(ctx), FWD(self).sys_then, FWD(arg)...)),
						decltype(run_sys(FWD(ctx), FWD(self).sys_else, FWD(arg)...))>;
				},
				"[cond] 'then' and 'else' systems must return the same type");

			if (run_sys(ctx, FWD(self).sys_cond, arg...))
			{
				return run_sys(FWD(ctx), FWD(self).sys_then, FWD(arg)...);
			}
			else
			{
				return run_sys(FWD(ctx), FWD(self).sys_else, FWD(arg)...);
			}
		}
	};

	template <typename t_sys_cond, typename t_sys_then>
	struct cond<t_sys_cond, t_sys_then, void>
	{
		using t_ctx_tag = ctx_tag<tag_adaptor, tag_ctx_bound>;

		no_unique_addr t_sys_cond sys_cond;
		no_unique_addr t_sys_then sys_then;

		FORCE_INLINE constexpr cond(auto&& sys_cond, auto&& sys_then) noexcept
			: sys_cond{ FWD(sys_cond) },
			  sys_then{ FWD(sys_then) }
		{
		}

		FORCE_INLINE constexpr decltype(auto)
		operator()(this auto&& self, cx_ctx auto&& ctx, auto&&... arg) noexcept
		{
			static_assert(
				requires { { run_sys(ctx, FWD(self).sys_cond, arg...) } -> std::convertible_to<bool>; },
				"[cond] system sys_cond is invalid - check if the system is callable with the given arguments or if the system returns bool");

			static_assert(
				requires { { run_sys(FWD(ctx),  FWD(self).sys_then, FWD(arg)...) } -> std::same_as<void>; },
				"[cond] 'then' system must return void");

			if (run_sys(ctx, FWD(self).sys_cond, arg...))
			{
				run_sys(FWD(ctx), FWD(self).sys_then, FWD(arg)...);
			}
		}
	};

	template <typename t_sys_cond, typename t_sys_then>
	cond(t_sys_cond&&, t_sys_then&&) -> cond<t_sys_cond, t_sys_then, void>;

	template <typename t_sys_cond, typename t_sys_then, typename t_sys_else>
	cond(t_sys_cond&&, t_sys_then&&, t_sys_else&&) -> cond<t_sys_cond, t_sys_then, t_sys_else>;
}	 // namespace age::ecs::system
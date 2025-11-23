#pragma once
#ifndef INCLUDED_FROM_ECS_SYSTEM_HEADER
	#error "Do not include this file directly. Include <__ecs_system.h> instead."
#endif

namespace ecs::system::ctx
{
	template <typename t>
	inline constexpr auto is_compile_time_constructible_v = is_constexpr_default_constructible<std::remove_reference_t<t>>();

	struct ctx_base
	{
	};

	template <typename t>
	concept cx_ctx = std::is_base_of_v<ctx_base, std::remove_reference_t<t>>;

	struct sys_ctx_bound
	{
	};

	template <typename t>
	concept is_ctx_bound = std::is_base_of_v<sys_ctx_bound, std::remove_reference_t<t>>;

	namespace detail
	{
		template <typename t_sys, cx_ctx t_ctx>
		FORCE_INLINE constexpr decltype(auto)
		run_sys(t_sys&& sys, t_ctx&& ctx, auto&&... arg) noexcept
		{
			if constexpr (is_ctx_bound<t_sys>)
			{
				if constexpr (requires { sys(FWD(ctx), FWD(arg)...); })
				{
					return sys(FWD(ctx), FWD(arg)...);
				}
				else
				{
					return ecs::system::detail::invalid_sys_call();
				}
			}
			else if constexpr (requires { sys(FWD(arg)...); })
			{
				return sys(FWD(arg)...);
			}
			else if constexpr (requires { sys(FWD(ctx), FWD(arg)...); })
			{
				return sys(FWD(ctx), FWD(arg)...);
			}
			else
			{
				return ecs::system::detail::invalid_sys_call();
			}
		}

		template <typename t_sys, cx_ctx t_ctx>
		FORCE_INLINE constexpr decltype(auto)
		execute(t_sys&& sys, t_ctx&& ctx, auto&&... arg) noexcept
		{
			if constexpr (requires { ctx.execute(FWD(sys), FWD(arg)...); })
			{
				return ctx.execute(FWD(sys), FWD(arg)...);
			}
			else
			{
				return system::detail::invalid_sys_call();
			}
		}

		template <typename t_sys>
		FORCE_INLINE constexpr decltype(auto)
		execute(t_sys&& sys, auto&&... arg) noexcept
		{
			if constexpr (is_ctx_bound<t_sys> is_false and requires { sys(FWD(ctx), FWD(arg)...); })
			{
				return sys(FWD(arg)...);
			}
			else
			{
				return system::detail::invalid_sys_call();
			}
		}
	}	 // namespace detail

	template <typename t>
	constexpr bool
	is_constexpr_default_constructible()
	{
		if constexpr (std::is_default_constructible_v<t> and requires { t{}; })
		{
			return requires { []() consteval { t x{}; (void)x; return true; }(); };
		}
		else
		{
			return false;
		}
	}

	template <auto ith, typename t_sys>
	struct __sys : t_sys
	{
		using t_sys::operator();

		constexpr __sys(t_sys&& s) noexcept
			: t_sys(FWD(s)) { }

		constexpr __sys() noexcept requires(is_compile_time_constructible_v<t_sys>)
		= default;
	};

	template <typename t_sys>
	struct with_ctx : sys_ctx_bound
	{
		no_unique_addr t_sys sys;

		constexpr FORCE_INLINE
		with_ctx(auto&& sys) noexcept : sys(FWD(sys)){};

		constexpr with_ctx() noexcept requires(is_compile_time_constructible_v<t_sys> or std::is_empty_v<t_sys>)
		= default;

		template <typename t_sys, typename... t_arg>
		static consteval bool
		validate(meta::type_pack<t_arg...>)
		{
			{
				constexpr auto valid = std::invocable<t_sys, t_arg...>;

				static_assert(valid, "[with_ctx] systems(...) returned invalid_sys_call - check that system is callable with given arguments.");
			}

			return true;
		}

		FORCE_INLINE constexpr decltype(auto)
		operator()(cx_ctx auto&& ctx, auto&&... arg) noexcept
		{
			static_assert(validate<t_sys>(meta::type_pack<decltype(ctx), decltype(arg)...>{}), "[with_ctx]: invalid with_ctx");

			return detail::run_sys(sys, FWD(ctx), FWD(arg)...);
		}
	};

	template <typename t_sys>
	with_ctx(t_sys&&) -> with_ctx<meta::value_or_ref_t<t_sys&&>>;
}	 // namespace ecs::system::ctx

namespace ecs::system::ctx
{
	struct exec_inline
	{
		template <typename t_ctx, typename t_sys>
		FORCE_INLINE constexpr decltype(auto)
		__run_single(t_ctx&& ctx, t_sys&& sys, auto&&... arg) noexcept(noexcept(detail::run_sys(FWD(sys), FWD(ctx), FWD(arg)...)))
		{
			return detail::run_sys(FWD(sys), FWD(ctx), FWD(arg)...);
		}

		template <typename t_ctx, typename t_arg_tpl>
		FORCE_INLINE constexpr decltype(auto)
		__run_impl_tpl(t_ctx&& ctx, auto&& sys, t_arg_tpl&& arg_tpl) noexcept
		{
			return meta::tuple_unpack([this, sys = FWD(sys), &ctx](auto&&... arg) mutable noexcept -> decltype(auto) { return __run_single(FWD(ctx), sys, FWD(arg)...); }, FWD(arg_tpl));
		}

		template <typename t_ctx, typename t_arg_tpl, std::size_t... sys_idx>
		FORCE_INLINE constexpr decltype(auto)
		__run_impl_seq(t_ctx&& ctx, t_arg_tpl&& arg_tpl, std::index_sequence<sys_idx...>, auto&&... sys) noexcept
		{
			return (__run_impl_tpl(ctx, meta::variadic_get<sys_idx>(sys...), arg_tpl), ...);
		}

		template <typename t_ctx, typename t_arg_tpl, std::size_t... sys_idx, typename... t_sys>
		FORCE_INLINE constexpr decltype(auto)
		__run_helper_2(t_ctx&& ctx, t_arg_tpl&& arg_tpl, std::index_sequence<sys_idx...>, t_sys&&... sys) noexcept
		{
			using t_sys_res_not_void =
				meta::filtered_index_sequence_t<
					meta::is_not_void,
					decltype(__run_impl_tpl(ctx, meta::variadic_get<sys_idx>(sys...), arg_tpl))...>;

			static_assert(meta::index_sequence_empty_v<t_sys_res_not_void> is_false);

			if constexpr (constexpr auto has_trailing_voids =
							  (meta::is_not_void_v<
								  decltype(__run_impl_tpl(ctx, meta::variadic_get<variadic_auto_back_v<sys_idx...>>(sys...), arg_tpl))>
								   is_false))
			{
				constexpr auto idx_head			= meta::variadic_auto_front_v<sys_idx...>;
				constexpr auto idx_res_not_void = meta::variadic_auto_at_v<meta::index_sequence_front_v<t_sys_res_not_void>, sys_idx...>;
				constexpr auto idx_tail			= meta::variadic_auto_back_v<sys_idx...>;

				// auto after_return = detail::scope_guard{ [this, &ctx, &arg_tpl, &sys...]() mutable noexcept { __run_impl_seq(ctx, arg_tpl, meta::offset_sequence<idx_res_not_void + 1, idx_tail - idx_res_not_void>{}, sys...); } };
				return __run_impl_seq(ctx, arg_tpl, meta::offset_sequence<idx_head, idx_res_not_void - idx_head + 1>{}, sys...);
			}
			else
			{
				return __run_impl_seq(ctx, arg_tpl, std::index_sequence<sys_idx...>{}, sys...);
			}
		}

		template <typename t_ctx, typename t_arg_tpl, typename... t_idx_seq, typename... t_sys>
		FORCE_INLINE constexpr decltype(auto)
		__run_helper_1(t_ctx&& ctx, t_arg_tpl&& arg_tpl, meta::type_pack<t_idx_seq...>, t_sys&&... sys) noexcept
		{
			return std::tuple{ __run_helper_2(FWD(ctx), arg_tpl, t_idx_seq{}, sys...)... };
		}

		template <typename t_ctx, typename... t_sys>
		FORCE_INLINE constexpr decltype(auto)
		run(t_ctx&& ctx, t_sys&&... sys, auto&&... arg) noexcept
		{
			static_assert(sizeof...(t_sys) > 0);
			using t_sys_res_not_void =
				meta::filtered_index_sequence_t<
					meta::is_not_void,
					decltype(__run_single(FWD(ctx), FWD(sys), FWD(arg)...))...>;

			if constexpr (sizeof...(t_sys) <= 1 or meta::index_sequence_empty_v<t_sys_res_not_void>)
			{
				return (__run_single(FWD(ctx), FWD(sys), FWD(arg)...), ...);
			}
			else
			{
				return __run_helper_1(FWD(ctx), make_arg_tpl(FWD(arg)...), ecs::system::detail::template index_ranges_seq_t<sizeof...(t_sys) - 1, t_sys_res_not_void>{}, FWD(sys)...);
			}
		}
	};

	struct exec_async
	{
		template <typename t_sys>
		FORCE_INLINE constexpr decltype(auto)
		dispatch(this auto&& self_ctx, auto&&... arg) noexcept
		{
			if constexpr (is_ctx_bound<t_sys>)
			{
				return static_cast<meta::copy_cv_ref_t<decltype(self_ctx), t_sys>>(self_ctx)(self_ctx, FWD(arg)...);
			}
			else
			{
				return static_cast<meta::copy_cv_ref_t<decltype(self_ctx), t_sys>>(self_ctx)(FWD(arg)...);
			}
		}
	};
}	 // namespace ecs::system::ctx

namespace ecs::system::ctx
{
	using namespace detail;

	template <typename t_executor, typename... t_sys>
	struct __ctx : ctx_base, t_sys...
	{
		no_unique_addr t_executor exec;

		constexpr __ctx(t_executor&& exec, t_sys&&... sys) noexcept
			: exec(FWD(exec)), t_sys(FWD(sys))... {};

		constexpr __ctx() noexcept requires(is_compile_time_constructible_v<t_executor> and (is_compile_time_constructible_v<t_sys> and ...))
		= default;

		template <typename t>
		FORCE_INLINE constexpr decltype(auto)
		get(this auto&& self) noexcept
		{
			return static_cast<meta::copy_cv_ref_t<decltype(self), t>>(self);
		}

		template <typename t_sys>
		FORCE_INLINE constexpr decltype(auto)
		execute(t_sys&& sys, auto&&... arg) noexcept
		{
			return exec.run<decltype(*this), t_sys&&>(*this, FWD(sys), FWD(arg)...);
		}

		template <typename... t_arg>
		FORCE_INLINE constexpr decltype(auto)
		operator()(t_arg&&... arg) noexcept
		{
			return exec.run<decltype(*this), decltype(get<t_sys>())...>(*this, get<t_sys>()..., FWD(arg)...);
		}

		template <typename... t_arg>
		FORCE_INLINE constexpr decltype(auto)
		operator()(cx_ctx auto&& ctx, t_arg&&... arg) noexcept
		{
			return exec.run<decltype(*this), decltype(get<t_sys>())...>(*this, get<t_sys>()..., FWD(arg)...);
		}
	};

	template <typename... t_sys>
	__ctx(t_sys&&...) -> __ctx<meta::value_or_ref_t<t_sys&&>...>;

	template <typename t_exec, typename... t_sys, auto... i>
	FORCE_INLINE constexpr decltype(auto)
	__make_ctx(std::index_sequence<i...>, t_exec&& exec, t_sys&&... sys) noexcept
	{
		return __ctx{ FWD(exec), __sys<i, meta::value_or_ref_t<t_sys&&>>(FWD(sys))... };
	}

	template <typename t_executor, typename... t_sys>
	struct on_ctx
	{
		using t_ctx = decltype(__make_ctx(std::index_sequence_for<t_sys...>{}, std::declval<t_executor>(), std::declval<t_sys>()...));

		no_unique_addr t_ctx _ctx;

		constexpr on_ctx(auto&& exec, auto&&... sys) noexcept : _ctx{ __make_ctx(std::index_sequence_for<t_sys...>{}, FWD(exec), FWD(sys)...) } {};

		constexpr on_ctx() noexcept requires(std::is_empty_v<t_sys> and ...)
		= default;

		template <typename... t_sys, typename... t_arg>
		static consteval bool
		validate(meta::type_pack<t_arg...>)
		{
			{
				constexpr auto valid = sizeof...(t_sys) > 0;
				static_assert(valid, "[on_ctx]: requires at least one system");
			}
			{
				constexpr auto valid = meta::is_not_same_v<decltype(_ctx(std::declval<t_arg&&>()...)), invalid_sys_call>;

				static_assert(valid, "[on_ctx] systems(...) returned invalid_sys_call - check that system i is callable with given arguments.");
			}

			return true;
		}

		template <typename... t_arg>
		FORCE_INLINE constexpr decltype(auto)
		operator()(t_arg&&... arg) noexcept
		{
			static_assert(validate<t_sys...>(meta::type_pack<t_arg&&...>{}), "[on_ctx]: invalid on_ctx");

			return _ctx(FWD(arg)...);
		}
	};

	template <typename... t_sys>
	on_ctx(t_sys&&...) -> on_ctx<meta::value_or_ref_t<t_sys&&>...>;
}	 // namespace ecs::system::ctx

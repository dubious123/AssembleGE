#pragma once

namespace age::ecs::system
{
	struct tag_ctx
	{
	};

	struct tag_ctx_bound
	{
	};

	struct tag_adaptor
	{
	};

	struct tag_executor
	{
	};

	template <typename t>
	concept cx_ctx = requires { requires std::remove_reference_t<t>::t_ctx_tag::template has_tag<tag_ctx>; };

	template <typename t>
	using cx_ctx_pred = std::bool_constant<cx_ctx<t>>;

	template <typename t>
	concept cx_ctx_bound = requires { requires std::remove_reference_t<t>::t_ctx_tag::template has_tag<tag_ctx_bound>; };

	template <typename t>
	using cx_ctx_bound_pred = std::bool_constant<cx_ctx_bound<t>>;

	template <typename t>
	concept cx_adaptor = requires { requires std::remove_reference_t<t>::t_ctx_tag::template has_tag<tag_adaptor>; };

	template <typename t>
	using cx_adaptor_pred = std::bool_constant<cx_adaptor<t>>;

	template <typename t>
	concept cx_executor = requires { requires std::remove_reference_t<t>::t_ctx_tag::template has_tag<tag_executor>; };

	template <typename t>
	using cx_executor_pred = std::bool_constant<cx_executor<t>>;

	template <typename... t_tag>
	struct ctx_tag
	{
		template <typename t>
		static inline constexpr bool has_tag = meta::variadic_contains_v<t, t_tag...>;
	};
}	 // namespace age::ecs::system

namespace age::ecs::system
{
	namespace detail
	{
		struct invalid_sys_call
		{
		};

		template <typename t>
		concept cx_valid_sys_call =
			!std::same_as<std::remove_cvref_t<t>, invalid_sys_call>;

		template <typename t_from, typename t_dst>
		concept cx_convertible_to = std::is_convertible_v<t_from, t_dst> || requires(t_from&& arg) { t_dst{ FWD(arg) }; };

		template <typename t_tpl_from, typename t_tpl_to>
		concept cx_tpl_convertible_to = requires {
			requires std::tuple_size_v<t_tpl_from> == std::tuple_size_v<t_tpl_to>;
			requires[]<std::size_t... i>(std::index_sequence<i...>)
			{
				return (cx_convertible_to<std::tuple_element_t<i, t_tpl_from>, std::tuple_element_t<i, t_tpl_to>> and ...);
				// return true && (... && std::is_convertible_v<std::tuple_element_t<i, t_tpl_from>, std::tuple_element_t<i, t_tpl_to>>);
			}
			(std::make_index_sequence<std::tuple_size_v<t_tpl_from>>{});
		};

		template <typename t_dst, typename t_from>
		FORCE_INLINE constexpr decltype(auto)
		make_param(t_from&& arg) noexcept
		{
			if constexpr (std::is_convertible_v<t_from, t_dst>)
			{
				return FWD(arg);
			}
			else
			{
				return t_dst{ FWD(arg) };
			}
		}

		template <typename t_sys, typename... t_arg>
		FORCE_INLINE constexpr decltype(auto)
		sys_invoke_impl(t_sys&& sys, t_arg&&... arg) noexcept
		{
			if constexpr (std::is_invocable_v<t_sys&&, t_arg&&...>)
			{
				return FWD(sys)(FWD(arg)...);
			}
			else if constexpr (std::is_invocable_v<t_sys&&>)
			{
				return FWD(sys)();
			}
			else if constexpr (requires { &std::decay_t<t_sys>::template operator()<t_arg && ...>; })
			{
				using to_tpl = typename meta::function_traits<&std::decay_t<t_sys>::template operator()<t_arg&&...>>::argument_types;

				if constexpr (cx_tpl_convertible_to<std::tuple<t_arg&&...>, to_tpl>)
				{
					return []<auto... i> INLINE_LAMBDA_FRONT(std::index_sequence<i...>, auto&& sys, auto&&... arg) noexcept INLINE_LAMBDA_BACK -> decltype(auto) {
						return FWD(sys).template operator()<t_arg...>((make_param<std::tuple_element_t<i, to_tpl>>(FWD(arg)))...);
					}(std::make_index_sequence<sizeof...(arg)>{}, FWD(sys), FWD(arg)...);
				}
				else if constexpr (requires { FWD(sys).template operator()<t_arg...>(); })
				{
					return FWD(sys).template operator()<t_arg...>();
				}
				else
				{
					return invalid_sys_call{};
				}
			}
			else
			{
				return invalid_sys_call{};
			}
		}

		FORCE_INLINE constexpr decltype(auto)
		sys_invoke(auto&& sys, auto&& ctx, auto&&... arg) noexcept
		{
			using t_sys = decltype(sys);
			if constexpr (cx_ctx<decltype(ctx)> is_false)
			{
				return sys_invoke_impl(FWD(sys), FWD(ctx), FWD(arg)...);
			}
			else if constexpr (cx_ctx_bound<t_sys>)
			{
				return sys_invoke_impl(FWD(sys), FWD(ctx), FWD(arg)...);
			}
			else if constexpr (requires { {sys_invoke_impl(FWD(sys), FWD(arg)...)} -> detail::cx_valid_sys_call; })
			{
				return sys_invoke_impl(FWD(sys), FWD(arg)...);
			}
			else
			{
				return sys_invoke_impl(FWD(sys), FWD(ctx), FWD(arg)...);
			}
		}

		FORCE_INLINE constexpr decltype(auto)
		sys_invoke(auto&& sys, auto&& ctx, meta::tuple_like auto&& arg_tpl) noexcept
		{
			if constexpr (
				requires { {sys_invoke_impl(FWD(sys), FWD(ctx), FWD(arg_tpl))} -> detail::cx_valid_sys_call; }
				or requires { {sys_invoke_impl(FWD(sys), FWD(arg_tpl))} -> detail::cx_valid_sys_call; })
			{
				if constexpr (cx_ctx<decltype(ctx)> is_false)
				{
					return sys_invoke_impl(FWD(sys), FWD(ctx), FWD(arg_tpl));
				}
				else if constexpr (cx_ctx_bound<decltype(FWD(sys))>)
				{
					return sys_invoke_impl(FWD(sys), FWD(ctx), FWD(arg_tpl));
				}
				else if constexpr (requires { {sys_invoke_impl(FWD(sys),  FWD(arg_tpl))} -> detail::cx_valid_sys_call; })
				{
					return sys_invoke_impl(FWD(sys), FWD(arg_tpl));
				}
				else
				{
					return sys_invoke_impl(FWD(sys), FWD(ctx), FWD(arg_tpl));
				}
			}
			else
			{
				return []<std::size_t... idx> INLINE_LAMBDA_FRONT(std::index_sequence<idx...>, auto&& sys, auto&& ctx, auto&& arg_tpl) noexcept(
						   noexcept(sys_invoke(FWD(sys), FWD(ctx), std::get<idx>(FWD(arg_tpl))...))) INLINE_LAMBDA_BACK -> decltype(auto) {
					return sys_invoke(FWD(sys), FWD(ctx), std::get<idx>(FWD(arg_tpl))...);
				}(std::make_index_sequence<std::tuple_size_v<BARE_OF(arg_tpl)>>{}, FWD(sys), FWD(ctx), FWD(arg_tpl));
			}
		}
	}	 // namespace detail

}	 // namespace age::ecs::system

namespace age::ecs::system
{
	FORCE_INLINE constexpr decltype(auto)
	run_sys(cx_ctx auto&& ctx, auto&& sys, auto&&... arg) noexcept
	{
		static_assert(
			requires {{detail::sys_invoke(FWD(sys), FWD(ctx), FWD(arg)...)} ->detail::cx_valid_sys_call; }
			or requires { {FWD(ctx).get_executor().execute(FWD(ctx), FWD(sys), FWD(arg)...)} ->detail::cx_valid_sys_call; });
		if constexpr (cx_adaptor<decltype(sys)>)
		{
			return detail::sys_invoke(FWD(sys), FWD(ctx), FWD(arg)...);
		}
		else
		{
			return FWD(ctx).get_executor().execute(FWD(ctx), FWD(sys), FWD(arg)...);
		}
	}

	FORCE_INLINE constexpr decltype(auto)
	run_sys(auto&& sys, auto&&... arg) noexcept
	{
		static_assert(requires {{detail::sys_invoke_impl(FWD(sys), FWD(arg)...)} ->detail::cx_valid_sys_call; });
		return detail::sys_invoke_impl(FWD(sys), FWD(arg)...);
	}

	FORCE_INLINE constexpr decltype(auto)
	run_sys(auto&& sys, meta::tuple_like auto&& arg_tpl) noexcept
	{
		if constexpr (requires { {detail::sys_invoke_impl(FWD(sys), FWD(arg_tpl))} -> detail::cx_valid_sys_call; })
		{
			return detail::sys_invoke_impl(FWD(sys), FWD(arg_tpl));
		}
		else
		{
			using t_ret = decltype([]<std::size_t... idx> INLINE_LAMBDA_FRONT(std::index_sequence<idx...>, auto&& sys, auto&& arg_tpl) noexcept(
									   noexcept(detail::sys_invoke(FWD(sys), std::get<idx>(FWD(arg_tpl))...))) INLINE_LAMBDA_BACK -> decltype(auto) {
				return detail::sys_invoke(FWD(sys), std::get<idx>(FWD(arg_tpl))...);
			}(std::make_index_sequence<std::tuple_size_v<BARE_OF(arg_tpl)>>{}, FWD(sys), FWD(arg_tpl)));
			static_assert(detail::cx_valid_sys_call<t_ret>);

			return []<std::size_t... idx> INLINE_LAMBDA_FRONT(std::index_sequence<idx...>, auto&& sys, auto&& arg_tpl) noexcept(
					   noexcept(detail::sys_invoke(FWD(sys), std::get<idx>(FWD(arg_tpl))...))) INLINE_LAMBDA_BACK -> decltype(auto) {
				return detail::sys_invoke(FWD(sys), std::get<idx>(FWD(arg_tpl))...);
			}(std::make_index_sequence<std::tuple_size_v<BARE_OF(arg_tpl)>>{}, FWD(sys), FWD(arg_tpl));
		}
	}
}	 // namespace age::ecs::system
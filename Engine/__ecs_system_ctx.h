#pragma once
#ifndef INCLUDED_FROM_ECS_SYSTEM_HEADER
	#error "Do not include this file directly. Include <__ecs_system.h> instead."
#endif

namespace ecs::system::ctx
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
		static inline constexpr bool has_tag = meta::variadic_constains_v<t, t_tag...>;
	};
}	 // namespace ecs::system::ctx

namespace ecs::system::ctx
{
	namespace detail
	{
		FORCE_INLINE constexpr decltype(auto)
		sys_invoke(auto&& sys, auto&& ctx, auto&&... arg) noexcept
		{
			using t_sys = decltype(sys);

			if constexpr (cx_ctx_bound<t_sys>)
			{
				return sys(FWD(ctx), FWD(arg)...);
			}
			else if constexpr (requires { sys(FWD(ctx), FWD(arg)...); })
			{
				return sys(FWD(ctx), FWD(arg)...);
			}
			else
			{
				return sys(FWD(arg)...);
			}
		}

		FORCE_INLINE constexpr decltype(auto)
		sys_invoke(auto&& sys, auto&& ctx, meta::tuple_like auto&& arg_tpl) noexcept
		{
			if constexpr (requires { FWD(sys)(FWD(ctx), FWD(arg_tpl)); } or requires { FWD(sys)(FWD(arg_tpl)); })
			{
				if constexpr (cx_ctx_bound<decltype(FWD(sys))>)
				{
					return FWD(sys)(FWD(ctx), FWD(arg_tpl));
				}
				else if constexpr (requires { FWD(sys)(FWD(ctx), FWD(arg_tpl)); })
				{
					return FWD(sys)(FWD(ctx), FWD(arg_tpl));
				}
				else
				{
					return FWD(sys)(FWD(arg_tpl));
				}
			}
			else
			{
				return []<std::size_t... idx> INLINE_LAMBDA_FRONT(std::index_sequence<idx...>, auto&& sys, auto&& ctx, auto&& arg_tpl) noexcept(
						   noexcept(sys_invoke(FWD(sys), FWD(ctx), std::get<idx>(FWD(arg_tpl))...))) INLINE_LAMBDA_BACK -> decltype(auto) {
					return sys_invoke(FWD(sys), FWD(ctx), std::get<idx>(FWD(arg_tpl))...);
				}(std::make_index_sequence<std::tuple_size_v<std::remove_cvref_t<decltype(arg_tpl)>>>{}, FWD(sys), FWD(ctx), FWD(arg_tpl));
			}
		}

		FORCE_INLINE constexpr decltype(auto)
		run_sys(cx_ctx auto&& ctx, auto&& sys, auto&&... arg) noexcept
		{
			if constexpr (cx_adaptor<decltype(sys)>)
			{
				return sys_invoke(FWD(sys), FWD(ctx), FWD(arg)...);
			}
			else
			{
				return FWD(ctx).get_executor().execute(FWD(ctx), FWD(sys), FWD(arg)...);
			}
		}

		FORCE_INLINE constexpr decltype(auto)
		run_sys(auto&& sys, auto&&... arg) noexcept
		{
			return FWD(sys)(FWD(arg)...);
		}

		FORCE_INLINE constexpr decltype(auto)
		run_sys(auto&& sys, meta::tuple_like auto&& arg_tpl) noexcept
		{
			if constexpr (requires { FWD(sys)(FWD(arg_tpl)); })
			{
				return FWD(sys)(FWD(arg_tpl));
			}
			else
			{
				return meta::tuple_unpack(FWD(sys), FWD(arg_tpl));
			}
		}
	}	 // namespace detail

	struct exec_inline
	{
		using t_ctx_tag = ctx_tag<tag_executor>;

	  private:
		template <std::size_t sys_idx, typename t_arg_tpl>
		FORCE_INLINE static constexpr decltype(auto)
		__run_sys_impl_tpl(auto&& ctx, t_arg_tpl&& arg_tpl) noexcept
		{
			return []<auto... i> INLINE_LAMBDA_FRONT(std::index_sequence<i...>, auto&& ctx, auto&& arg_tpl) noexcept(noexcept(ecs::system::ctx::detail::run_sys(FWD(ctx), FWD(ctx).template get_sys<sys_idx>(), std::get<i>(arg_tpl)...))) INLINE_LAMBDA_BACK -> decltype(auto) {
				return ecs::system::ctx::detail::run_sys(FWD(ctx), FWD(ctx).template get_sys<sys_idx>(), std::get<i>(arg_tpl)...);
			}(std::make_index_sequence<std::tuple_size_v<std::remove_cvref_t<t_arg_tpl>>>{}, FWD(ctx), FWD(arg_tpl));
		}

		template <std::size_t... sys_idx>
		FORCE_INLINE static constexpr decltype(auto)
		__run_sys_impl_seq(auto&& ctx, std::index_sequence<sys_idx...>, auto&& arg_tpl) noexcept
		{
			return (__run_sys_impl_tpl<sys_idx>(FWD(ctx), arg_tpl), ...);
		}

		template <std::size_t... sys_idx>
		FORCE_INLINE static constexpr decltype(auto)
		__run_all_impl_2(auto&& ctx, std::index_sequence<sys_idx...>, auto&& arg_tpl) noexcept
		{
			using t_sys_res_not_void =
				meta::filtered_index_sequence_t<
					meta::is_not_void,
					decltype(__run_sys_impl_tpl<sys_idx>(FWD(ctx), FWD(arg_tpl)))...>;

			static_assert(meta::index_sequence_empty_v<t_sys_res_not_void> is_false);

			if constexpr (constexpr auto has_trailing_voids =
							  (meta::is_not_void_v<
								  decltype(__run_sys_impl_tpl<meta::variadic_auto_back_v<sys_idx...>>(ctx, FWD(arg_tpl)))>
								   is_false))
			{
				constexpr auto idx_head			= meta::variadic_auto_front_v<sys_idx...>;
				constexpr auto idx_res_not_void = meta::variadic_auto_at_v<meta::index_sequence_front_v<t_sys_res_not_void>, sys_idx...>;
				constexpr auto idx_tail			= meta::variadic_auto_back_v<sys_idx...>;

				auto after_return = ecs::system::detail::scope_guard{ [&ctx, &arg_tpl] INLINE_LAMBDA_FRONT mutable noexcept INLINE_LAMBDA_BACK {
					__run_sys_impl_seq(ctx, meta::offset_sequence<idx_res_not_void + 1, idx_tail - idx_res_not_void>{}, arg_tpl);
				} };

				return __run_sys_impl_seq(ctx, meta::offset_sequence<idx_head, idx_res_not_void - idx_head + 1>{}, arg_tpl);
			}
			else
			{
				return __run_sys_impl_seq(ctx, std::index_sequence<sys_idx...>{}, arg_tpl);
			}
		}

		template <typename... t_sys_idx_seq>
		FORCE_INLINE static constexpr decltype(auto)
		__run_all_impl_1(auto&& ctx, meta::type_pack<t_sys_idx_seq...>, auto&& arg_tpl) noexcept
		{
			return std::tuple{ __run_all_impl_2(FWD(ctx), t_sys_idx_seq{}, arg_tpl)... };
		}

	  public:
		FORCE_INLINE constexpr decltype(auto)
		execute(this auto&& self, cx_ctx auto&& ctx, auto&& sys, auto&&... arg) noexcept
		{
			// std::println("execute");
			return detail::sys_invoke(FWD(sys), FWD(ctx), FWD(arg)...);
		}

		template <std::size_t... sys_idx>
		FORCE_INLINE constexpr decltype(auto)
		run_all(this auto&& self, cx_ctx auto&& ctx, std::index_sequence<sys_idx...>, auto&&... arg) noexcept
		{
			using t_sys_res_not_void =
				meta::filtered_index_sequence_t<
					meta::is_not_void,
					decltype(ecs::system::ctx::detail::run_sys(FWD(ctx), FWD(ctx).template get_sys<sys_idx>(), arg...))...>;

			if constexpr (sizeof...(sys_idx) <= 1)
			{
				return (ecs::system::ctx::detail::run_sys(FWD(ctx), FWD(ctx).template get_sys<sys_idx>(), FWD(arg)...), ...);
			}
			else if constexpr (meta::index_sequence_empty_v<t_sys_res_not_void>)
			{
				(ecs::system::ctx::detail::run_sys(FWD(ctx), FWD(ctx).template get_sys<sys_idx>(), arg...), ...);
			}
			else
			{
				return __run_all_impl_1(FWD(ctx), ecs::system::detail::template index_ranges_seq_t<sizeof...(sys_idx) - 1, t_sys_res_not_void>{}, make_arg_tpl(FWD(arg)...));
			}
		}
	};

	struct exec_async
	{
		using t_ctx_tag = ctx_tag<tag_executor>;
	};
}	 // namespace ecs::system::ctx

namespace ecs::system::ctx
{
	template <typename... t>
	struct compressed_pack;

	template <>
	struct compressed_pack<>
	{
	};

	template <typename t_head>
	struct compressed_pack<t_head>
	{
		no_unique_addr t_head head;

		constexpr compressed_pack(auto&& h_arg)
			: head{ FWD(h_arg) }
		{
		}

		template <std::size_t nth>
		FORCE_INLINE constexpr decltype(auto)
		get(this auto&& self) noexcept
		{
			if constexpr (nth == 0)
			{
				return static_cast<meta::copy_cv_ref_t<decltype(self), t_head>>(FWD(self).head);
			}
			else
			{
				static_assert(false, "index out of range");
			}
		}
	};

	template <typename t_head, typename... t_tail>
	struct compressed_pack<t_head, t_tail...>
	{
		no_unique_addr t_head head;
		no_unique_addr compressed_pack<t_tail...> tail;

		constexpr compressed_pack() = default;

		constexpr compressed_pack(auto&& h_arg, auto&&... t_arg)
			: head{ FWD(h_arg) }, tail{ FWD(t_arg)... }
		{
		}

		template <std::size_t nth>
		FORCE_INLINE constexpr decltype(auto)
		get(this auto&& self) noexcept
		{
			if constexpr (nth == 0)
			{
				return static_cast<meta::copy_cv_ref_t<decltype(self), t_head>>(FWD(self).head);
			}
			else
			{
				return FWD(self).tail.template get<nth - 1>();
			}
		}
	};

	template <typename... t>
	compressed_pack(t&&... arg) -> compressed_pack<t...>;
}	 // namespace ecs::system::ctx

namespace ecs::system::ctx
{
	template <typename... t>
	struct on_ctx
	{
		using t_ctx_tag = ctx_tag<tag_ctx>;

		template <typename t_other>
		using sys_pred = std::bool_constant<not cx_executor<t_other>>;

		static inline constexpr auto executor_idx = meta::variadic_index_v<cx_executor_pred, t...>;
		static inline constexpr auto sys_idx_seq  = meta::make_filtered_index_sequence<sys_pred, t...>();
		static inline constexpr auto sys_count	  = meta::index_sequence_size_v<decltype(sys_idx_seq)>;

		no_unique_addr compressed_pack<t...> storage;

		constexpr on_ctx(auto&&... arg) noexcept : storage{ FWD(arg)... } { };

		FORCE_INLINE constexpr decltype(auto)
		get_executor(this auto&& self) noexcept
		{
			return FWD(self).storage.get<executor_idx>();
		}

		template <std::size_t nth>
		FORCE_INLINE constexpr decltype(auto)
		get_sys(this auto&& self) noexcept
		{
			static_assert(nth < meta::index_sequence_size_v<decltype(sys_idx_seq)>);

			return FWD(self).storage.get<meta::index_sequence_at_v<nth, decltype(sys_idx_seq)>>();
		}

		FORCE_INLINE constexpr decltype(auto)
		operator()(this auto&& self, auto&&... arg) noexcept
		{
			return FWD(self).get_executor().run_all(FWD(self), std::make_index_sequence<sys_count>{}, FWD(arg)...);
		}
	};

	template <typename... t>
	on_ctx(t&&...) -> on_ctx<t...>;
}	 // namespace ecs::system::ctx

namespace ecs::system::ctx
{
	template <typename t_sys>
	struct with_ctx
	{
		using t_ctx_tag = ctx_tag<tag_ctx_bound, tag_adaptor>;

		no_unique_addr t_sys sys;

		constexpr with_ctx(auto&&... arg) noexcept(std::is_nothrow_constructible_v<t_sys, decltype(arg)...>)
			: sys{ FWD(arg)... }
		{
		}

		template <typename... t_arg>
		static consteval bool
		validate(meta::type_pack<t_sys, t_arg...>)
		{
			{
				constexpr auto valid = std::invocable<t_sys, t_arg...>;

				static_assert(valid, "[with_ctx] systems(...) returned invalid_sys_call - check that system is callable with given arguments.");
			}

			return true;
		}

		FORCE_INLINE constexpr decltype(auto)
		operator()(this auto&& self, cx_ctx auto&& ctx, auto&&... arg) noexcept
		{
			static_assert(validate(meta::type_pack<t_sys, decltype(ctx), decltype(arg)...>{}), "[with_ctx]: invalid with_ctx");

			return ctx.get_executor().execute(FWD(ctx), FWD(self).sys, FWD(ctx), FWD(arg)...);
		}
	};

	template <typename t_sys>
	with_ctx(t_sys&&) -> with_ctx<t_sys>;
}	 // namespace ecs::system::ctx

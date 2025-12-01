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

	template <typename t>
	using cx_ctx_pred = std::bool_constant<cx_ctx<t>>;

	struct sys_ctx_bound
	{
	};

	template <typename t>
	concept cx_ctx_bound = std::is_base_of_v<sys_ctx_bound, std::remove_reference_t<t>>;

	template <typename t>
	using cx_ctx_bound_pred = std::bool_constant<cx_ctx_bound<t>>;

	struct adaptor_base
	{
	};

	template <typename t>
	concept cx_adaptor = std::is_base_of_v<adaptor_base, std::remove_reference_t<t>>;

	template <typename t>
	using cx_adaptor_pred = std::bool_constant<cx_adaptor<t>>;

	struct executor_base
	{
	};

	template <typename t>
	concept cx_executor = std::is_base_of_v<executor_base, std::remove_reference_t<t>>;

	template <typename t>
	using cx_executor_pred = std::bool_constant<cx_executor<t>>;

	namespace detail
	{
		template <typename t_sys, cx_ctx t_ctx>
		FORCE_INLINE constexpr decltype(auto)
		run_sys(t_sys&& sys, t_ctx&& ctx, auto&&... arg) noexcept
		{
			if constexpr (cx_adaptor<t_sys>)
			{
				if constexpr (cx_ctx_bound<t_sys>)
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
			else
			{
				return execute(FWD(sys), FWD(ctx), FWD(arg)...);
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
		with_ctx(auto&& sys) noexcept : sys(FWD(sys)) { };

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
	struct exec_inline : executor_base
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

		FORCE_INLINE constexpr decltype(auto)
		run_all(this auto&& ctx /*, auto&&... arg*/) noexcept
		{
		}
	};

	struct exec_async : executor_base
	{
		template <typename t_sys>
		FORCE_INLINE constexpr decltype(auto)
		dispatch(this auto&& self_ctx, auto&&... arg) noexcept
		{
			if constexpr (cx_ctx_bound<t_sys>)
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
			: exec(FWD(exec)), t_sys(FWD(sys))... { };

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
}	 // namespace ecs::system::ctx

// 모든 시스템은 상속된다.
// deducing this 때문에 상속된 sys에 접근 불가능
// 모든 sys에 unique id부여
// ctx.get<id>() 를 통해 sys를 찾을 수 있음
namespace ecs::system::ctx
{
	template <typename t>
	static consteval std::size_t
	get_unique_base_count()
	{
		if constexpr (requires { t::unique_base_count(); })
		{
			return t::unique_base_count();
		}
		else
		{
			return 1;
		}
	}

	template <std::size_t id, typename t_base>
	struct unique_base : t_base
	{
		using t_base::t_base;
		using t_self = unique_base<id, t_base>;

		template <typename t>
		struct is_compatible_unique_base : std::is_convertible<t, t_base>
		{
		};

		template <std::size_t other_id>
		struct is_compatible_unique_base<unique_base<other_id, t_base>> : std::true_type
		{
		};

		template <typename t>
		static constexpr auto is_compatible_unique_base_v = is_compatible_unique_base<t>::value;

		template <typename t_other>
		constexpr unique_base(t_other&& other) noexcept(
			noexcept(t_base(static_cast<meta::copy_cv_ref_t<t_other&&, t_base>>(FWD(other)))))
			requires is_compatible_unique_base_v<t_other>
			: t_base(static_cast<meta::copy_cv_ref_t<t_other&&, t_base>>(FWD(other)))
		{
		}

		constexpr unique_base(auto&&... arg) noexcept(
			noexcept(t_base(FWD(arg)...)))
			: t_base(FWD(arg)...)
		{
		}

		static consteval auto
		get_id()
		{
			return id;
		}

		static consteval auto
		unique_base_count()
		{
			return get_unique_base_count<t_base>();
		}

		FORCE_INLINE constexpr decltype(auto)
		get_base(this auto&& self) noexcept
		{
			return static_cast<meta::copy_cv_ref_t<decltype(self), t_base>>(FWD(self));
		}

		template <std::size_t id_other>
		FORCE_INLINE constexpr decltype(auto)
		get(this auto&& self) noexcept
		{
			if constexpr (id == id_other)
			{
				return FWD(self).get_base();
			}
			else if constexpr (id < id_other)
			{
				// what?
				static constexpr auto _ = requires { FWD(self).get_base().get<id_other>(); };
				if constexpr (_)
				{
					return FWD(self).get_base().get<id_other>();
				}
			}
			else
			{
				return;
			}
		}
	};

	template <typename... t_base>
	struct unique_bases;

	template <typename t>
	concept has_unique_bases =
		requires {
			typename std::remove_cvref_t<t>::t_unique_bases;
		};

	template <std::size_t offset, typename t_base>
	struct make_unique_base
	{
		using type = unique_base<offset, t_base>;
	};

	template <std::size_t offset, has_unique_bases t_base>
	struct make_unique_base<offset, t_base>
	{
		using type = typename make_unique_base<offset, typename std::remove_cvref_t<t_base>::t_unique_bases>::type;
	};

	template <std::size_t offset, std::size_t... n, typename... t_base>
	struct make_unique_base<offset, unique_bases<std::index_sequence<n...>, t_base...>>
	{
		using type = unique_base<offset, unique_bases<std::index_sequence<(offset + 1 + n)...>, t_base...>>;
	};

	template <std::size_t offset, typename t_base>
	using make_unique_base_t = typename make_unique_base<offset, t_base>::type;

	template <std::size_t offset, typename... t_base>
	static consteval decltype(auto)
	make_id_seq()
	{
		return make_offset_sequence<offset>(
			meta::index_sequence_exclusive_scan_t<std::index_sequence<get_unique_base_count<t_base>()...>>{});
	}

	template <typename... t_base>
	struct unique_bases;

	template <std::size_t... n, typename... t_base>
	struct unique_bases<std::index_sequence<n...>, t_base...> : make_unique_base_t<n, t_base>...
	{
		template <std::size_t id>
		struct pred
		{
			template <typename t>
			struct type : meta::is_not_void<decltype(std::declval<t>().get<id>())> /*std::bool_constant<(t::get_id() == id)>*/
			{
			};
		};

		template <std::size_t id>
		using t_unique_base = meta::variadic_find_t<pred<id>::template type, void, make_unique_base_t<n, t_base>...>;

		template <std::size_t... n_other, typename... t_base_other>
		constexpr unique_bases(unique_bases<std::index_sequence<n_other...>, t_base_other...>&& other)
			: make_unique_base_t<n, t_base>(FWD(other).get<n_other>())...
		{
		}

		constexpr unique_bases(auto&&... arg) noexcept(
			(noexcept(make_unique_base_t<n, t_base>(FWD(arg))) && ...))
			requires(std::is_constructible_v<make_unique_base_t<n, t_base>, decltype(FWD(arg))> && ...)
			: make_unique_base_t<n, t_base>(FWD(arg))...
		{
		}

		static consteval auto
		unique_base_count()
		{
			return (get_unique_base_count<t_base>() + ... + 1);
		}

		template <std::size_t nth>
		FORCE_INLINE constexpr decltype(auto)
		get_base(this auto&& self) noexcept
		{
			return static_cast<meta::copy_cv_ref_t<decltype(self), meta::variadic_at_t<nth, make_unique_base_t<n, t_base>...>>>(FWD(self));
		}

		template <std::size_t id>
		FORCE_INLINE constexpr decltype(auto)
		get(this auto&& self) noexcept
		{
			if constexpr (meta::is_not_void_v<t_unique_base<id>>)
			{
				return static_cast<meta::copy_cv_ref_t<decltype(self), t_unique_base<id>>>(FWD(self)).get<id>();
			}
			else
			{
				return;
			}
		}

		template <std::size_t nth>
		using t_nth_base = meta::variadic_at_t<nth, make_unique_base_t<n, t_base>...>;
	};

	// template <typename... t_base>
	// unique_bases(t_base&&...) -> unique_bases<std::index_sequence_for<t_base...>, t_base...>;

	template <typename... t_base>
	unique_bases(t_base&&...) -> unique_bases<decltype(make_id_seq<0, t_base...>()), t_base...>;

	template <typename... t_base>
	using make_unique_bases = unique_bases<decltype(make_id_seq<0, t_base...>()), meta::universal_wrapper<t_base>...>;

	// using make_unique_bases = decltype(unique_bases(std::declval<t_base>()...));

	template <typename... t_sys>
	struct test_adaptor : make_unique_bases<t_sys...>
	{
	};
}	 // namespace ecs::system::ctx

namespace ecs::system::ctx
{
	template <typename... t>
	struct on_ctx;

	template <typename t>
	struct ctx_info;

	template <typename... t>
	struct ctx_info<on_ctx<t...>>
	{
		// 1. get_id
		// 2. make_system_id_sequence or system_type_pack

		// validate :
		//  1. one unique executor
		//  2. at least one system

		template <typename t_other>
		using sys_pred = std::bool_constant<not cx_executor<t_other>>;

		static consteval bool
		validate()
		{
			return true;
		}

		static consteval std::size_t
		get_executor_idx()
		{
			return meta::variadic_index_v<cx_executor_pred, t...>;
		}

		static consteval auto
		sys_idx_seq()
		{
			return meta::filtered_index_sequence_t<sys_pred, t...>{};
		}

		static consteval std::size_t
		sys_count()
		{
			meta::index_sequence_size_v<decltype(sys_idx_seq())>;
		}

		using t_executor = meta::variadic_at_t<get_executor_idx(), t...>;
	};

	template <typename... t>
	struct on_ctx : make_unique_bases<t...>
	{
		using t_unique_bases = make_unique_bases<t...>;
		using t_unique_bases::t_unique_bases;

		using t_ctx_info = ctx_info<on_ctx<t...>>;
		using t_executor = typename t_ctx_info::t_executor;

		template <std::size_t nth>
		static consteval auto
		get_nth_unique_base_id()
		{
			return t_unique_bases::t_nth_base<nth>::get_id();
		}

		static consteval auto
		get_sys_id_seq()
		{
			return []<auto... nth>(std::index_sequence<nth...>) {
				return std::index_sequence<get_nth_unique_base_id<nth>()...>{};
			}(t_ctx_info::sys_idx_seq());
		}

		template <typename... t_arg>
		FORCE_INLINE constexpr decltype(auto)
		get_executor(this auto&& self) noexcept
		{
			constexpr auto nth_base	 = t_ctx_info::get_executor_idx();
			constexpr auto unique_id = get_nth_unique_base_id<nth_base>();
			return FWD(self).get<unique_id>();
		}

		FORCE_INLINE constexpr decltype(auto)
		operator()(this auto&& self, auto&&... arg) noexcept
		{
			// meta::print_type<decltype(FWD(self).get_executor())>();
			//  return t_ctx_info::get_executor_idx();
			//			return t_executor::run_all();

			// return t_executor::run_all(FWD(self), FWD(arg)...);
		}
	};

	template <typename... t>
	on_ctx(t&&...) -> on_ctx<t...>;
}	 // namespace ecs::system::ctx

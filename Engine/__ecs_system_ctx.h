#pragma once
#ifndef INCLUDED_FROM_ECS_SYSTEM_HEADER
	#error "Do not include this file directly. Include <__ecs_system.h> instead."
#endif

namespace ecs::system::ctx
{
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

	template <typename t_sys>
	struct with_ctx : sys_ctx_bound, adaptor_base
	{
		no_unique_addr t_sys sys;

		constexpr FORCE_INLINE
		with_ctx(auto&& sys) noexcept : sys(FWD(sys)) { };

		constexpr with_ctx() noexcept = default;

		template <typename t_sys, typename... t_arg>
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
		operator()(cx_ctx auto&& ctx, auto&&... arg) noexcept
		{
			static_assert(validate(meta::type_pack<t_sys, decltype(ctx), decltype(arg)...>{}), "[with_ctx]: invalid with_ctx");

			return FWD(ctx).execute(sys, FWD(arg)...);
		}
	};

	template <typename t_sys>
	with_ctx(t_sys&&) -> with_ctx<meta::value_or_ref_t<t_sys&&>>;
}	 // namespace ecs::system::ctx

namespace ecs::system::ctx
{
	namespace detail
	{
		template <std::size_t sys_id>
		FORCE_INLINE constexpr decltype(auto)
		run_sys(cx_ctx auto&& ctx, auto&&... arg) noexcept
		{
			using t_sys = decltype(FWD(ctx).get<sys_id>().value);

			if constexpr (cx_adaptor<t_sys>)
			{
				return FWD(ctx).template invoke<sys_id>(FWD(arg)...);
			}
			else
			{
				return FWD(ctx).template execute<sys_id>(FWD(arg)...);
			}
		}
	}	 // namespace detail

	struct exec_inline : executor_base
	{
	  private:
		template <std::size_t sys_id, typename t_arg_tpl>
		FORCE_INLINE static constexpr decltype(auto)
		__run_sys_impl_tpl(auto&& ctx, t_arg_tpl&& arg_tpl) noexcept
		{
			return []<auto... i> INLINE_LAMBDA_FRONT(std::index_sequence<i...>, auto&& ctx, auto&& arg_tpl) noexcept(noexcept(run_sys<sys_id>(FWD(ctx), std::get<i>(arg_tpl)...))) INLINE_LAMBDA_BACK -> decltype(auto) {
				return ecs::system::ctx::detail::run_sys<sys_id>(FWD(ctx), std::get<i>(arg_tpl)...);
			}(std::make_index_sequence<std::tuple_size_v<std::remove_cvref_t<t_arg_tpl>>>{}, FWD(ctx), FWD(arg_tpl));
		}

		template <std::size_t... sys_id>
		FORCE_INLINE static constexpr decltype(auto)
		__run_sys_impl_seq(auto&& ctx, std::index_sequence<sys_id...>, auto&& arg_tpl) noexcept
		{
			return (__run_sys_impl_tpl<sys_id>(FWD(ctx), arg_tpl), ...);
		}

		template <std::size_t... sys_id>
		FORCE_INLINE static constexpr decltype(auto)
		__run_all_impl_2(auto&& ctx, std::index_sequence<sys_id...>, auto&& arg_tpl) noexcept
		{
			using t_sys_res_not_void =
				meta::filtered_index_sequence_t<
					meta::is_not_void,
					decltype(__run_sys_impl_tpl<sys_id>(FWD(ctx), FWD(arg_tpl)))...>;

			static_assert(meta::index_sequence_empty_v<t_sys_res_not_void> is_false);

			if constexpr (constexpr auto has_trailing_voids =
							  (meta::is_not_void_v<
								  decltype(__run_sys_impl_tpl<meta::variadic_auto_back_v<sys_id...>>(ctx, FWD(arg_tpl)))>
								   is_false))
			{
				constexpr auto idx_head			= meta::variadic_auto_front_v<sys_id...>;
				constexpr auto idx_res_not_void = meta::variadic_auto_at_v<meta::index_sequence_front_v<t_sys_res_not_void>, sys_id...>;
				constexpr auto idx_tail			= meta::variadic_auto_back_v<sys_id...>;

				auto after_return = ecs::system::detail::scope_guard{ [&ctx, &arg_tpl] INLINE_LAMBDA_FRONT mutable noexcept INLINE_LAMBDA_BACK {
					__run_sys_impl_seq(ctx, meta::offset_sequence<idx_res_not_void + 1, idx_tail - idx_res_not_void>{}, arg_tpl);
				} };

				return __run_sys_impl_seq(ctx, meta::offset_sequence<idx_head, idx_res_not_void - idx_head + 1>{}, arg_tpl);
			}
			else
			{
				return __run_sys_impl_seq(ctx, std::index_sequence<sys_id...>{}, arg_tpl);
			}
		}

		template <typename... t_id_seq>
		FORCE_INLINE static constexpr decltype(auto)
		__run_all_impl_1(auto&& ctx, meta::type_pack<t_id_seq...>, auto&& arg_tpl) noexcept
		{
			return std::tuple{ __run_all_impl_2(FWD(ctx), t_id_seq{}, arg_tpl)... };
		}

	  public:
		template <std::size_t sys_id>
		FORCE_INLINE static constexpr decltype(auto)
		execute(cx_ctx auto&& ctx, auto&&... arg) noexcept
		{
			std::println("hi");
			return FWD(ctx).template invoke<sys_id>(FWD(arg)...);
		}

		FORCE_INLINE static constexpr decltype(auto)
		execute(cx_ctx auto&& ctx, auto&& sys, auto&&... arg) noexcept
		{
			std::println("hi");
			return FWD(ctx).invoke(FWD(sys), FWD(arg)...);
		}

		template <std::size_t... sys_id>
		FORCE_INLINE static constexpr decltype(auto)
		run_all(cx_ctx auto&& ctx, std::index_sequence<sys_id...>, auto&&... arg) noexcept
		{
			using t_sys_res_not_void =
				meta::filtered_index_sequence_t<
					meta::is_not_void,
					decltype(ecs::system::ctx::detail::run_sys<sys_id>(FWD(ctx), FWD(arg)...))...>;

			if constexpr (sizeof...(sys_id) <= 1)
			{
				return (ecs::system::ctx::detail::run_sys<sys_id>(FWD(ctx), FWD(arg)...), ...);
			}
			else if constexpr (meta::index_sequence_empty_v<t_sys_res_not_void>)
			{
				(ecs::system::ctx::detail::run_sys<sys_id>(FWD(ctx), arg...), ...);
			}
			else
			{
				return __run_all_impl_1(FWD(ctx), ecs::system::detail::template index_ranges_seq_t<sizeof...(sys_id) - 1, t_sys_res_not_void>{}, make_arg_tpl(FWD(arg)...));
			}
		}
	};

	struct exec_async : executor_base
	{
	};
}	 // namespace ecs::system::ctx

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
		get_nth_base(this auto&& self) noexcept
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

	template <typename... t_base>
	unique_bases(t_base&&...) -> unique_bases<decltype(make_id_seq<0, t_base...>()), t_base...>;

	template <typename... t_base>
	using make_unique_bases = unique_bases<decltype(make_id_seq<0, t_base...>()), meta::universal_wrapper<t_base>...>;
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

		using t_executor = std::remove_reference_t<meta::variadic_at_t<get_executor_idx(), t...>>;
	};

	template <typename... t>
	struct on_ctx : make_unique_bases<t...>, ctx_base
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

		template <typename t_sys>
		FORCE_INLINE constexpr decltype(auto)
		invoke(this auto&& self, t_sys&& sys, auto&&... arg) noexcept
		{
			if constexpr (cx_ctx_bound<t_sys>)
			{
				return FWD(sys)(FWD(self), FWD(arg)...);
			}
			else if constexpr (requires { FWD(sys)(FWD(self), FWD(arg)...); })
			{
				return FWD(sys)(FWD(self), FWD(arg)...);
			}
			else
			{
				return FWD(sys)(FWD(arg)...);
			}
		}

		template <std::size_t sys_id>
		FORCE_INLINE constexpr decltype(auto)
		invoke(this auto&& self, auto&&... arg) noexcept
		{
			using t_sys = decltype(FWD(self).get<sys_id>().value);

			if constexpr (cx_ctx_bound<t_sys>)
			{
				return FWD(self).get<sys_id>().value(FWD(self), FWD(arg)...);
			}
			else if constexpr (requires { FWD(self).get<sys_id>().value(FWD(self), FWD(arg)...); })
			{
				return FWD(self).get<sys_id>().value(FWD(self), FWD(arg)...);
			}
			else
			{
				return FWD(self).get<sys_id>().value(FWD(arg)...);
			}
		}

		FORCE_INLINE constexpr decltype(auto)
		execute(this auto&& self, auto&& sys, auto&&... arg) noexcept(noexcept(t_executor::execute(FWD(self), FWD(sys), FWD(arg)...)))
		{
			return t_executor::execute(FWD(self), FWD(sys), FWD(arg)...);
		}

		template <std::size_t sys_id>
		FORCE_INLINE constexpr decltype(auto)
		execute(this auto&& self, auto&&... arg) noexcept(noexcept(t_executor::template execute<sys_id>(FWD(self), FWD(arg)...)))
		{
			return t_executor::template execute<sys_id>(FWD(self), FWD(arg)...);
		}

		FORCE_INLINE constexpr decltype(auto)
		operator()(this auto&& self, auto&&... arg) noexcept
		{
			return t_executor::run_all(FWD(self), get_sys_id_seq(), FWD(arg)...);
		}
	};

	template <typename... t>
	on_ctx(t&&...) -> on_ctx<t...>;
}	 // namespace ecs::system::ctx

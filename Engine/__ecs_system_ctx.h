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

	struct adaptor_base
	{
	};

	template <typename t>
	concept cx_adaptor = std::is_base_of_v<adaptor_base, std::remove_reference_t<t>>;

	struct executor_base
	{
	};

	template <typename t>
	concept cx_executor = std::is_base_of_v<executor_base, std::remove_reference_t<t>>;

	namespace detail
	{
		template <typename t_sys, cx_ctx t_ctx>
		FORCE_INLINE constexpr decltype(auto)
		run_sys(t_sys&& sys, t_ctx&& ctx, auto&&... arg) noexcept
		{
			if constexpr (cx_adaptor<t_sys>)
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
}	 // namespace ecs::system::ctx

// 모든 시스템은 상속된다.
// deducing this 때문에 상속된 sys에 접근 불가능
// 모든 sys에 unique id부여
// ctx.get<id>() 를 통해 sys를 찾을 수 있음
namespace ecs::system::ctx
{
	// template <typename t>
	// struct leaf
	//{
	// };

	// template <typename t_root, typename... t_child>
	// struct tree
	//{
	//	struct type
	//	{
	//	};
	// };

	// template <std::size_t id, typename t>
	// struct unique_leaf
	//{
	//	static constexpr std::size_t id = id;
	// };

	// template <std::size_t id, typename t_root, typename... t_child>
	// struct unique_tree
	//{
	//	static constexpr std::size_t id = id;

	//	template <std::size_t id>
	//	FORCE_INLINE constexpr decltype(auto)
	//	get(this auto&& self) noexcept
	//	{
	//		// return static_cast<meta::copy_cv_ref_t<decltype(self), t_base_at<id>>>(FWD(self));
	//	}
	//};

	// template <std::size_t offset, typename t>
	// struct flatten_one
	//{
	//	using t_base_list						 = meta::type_pack<unique_leaf<offset, t>>;
	//	static constexpr std::size_t next_offset = offset + 1;
	// };

	// template <std::size_t offset, typename... t>
	// struct flatten_one<offset, tree<t...>>
	//{
	//   private:
	//	template <std::size_t offset, typename... t>
	//	struct helper;

	//	template <std::size_t offset>
	//	struct helper<offset>
	//	{
	//		using t_base_list						 = meta::type_pack<>;
	//		static constexpr std::size_t next_offset = offset;
	//	};

	//	template <std::size_t offset, typename t_head, typename... t_tail>
	//	struct helper<offset, t_head, t_tail...>
	//	{
	//		using t_flat_head = flatten_one<offset, t_head>;
	//		using t_flat_tail = helper<t_flat_head::next_offset, t_tail...>;

	//		using t_base_list = meta::type_pack_cat_t<typename t_flat_head::t_base_list, typename t_flat_tail::t_base_list>;

	//		static constexpr std::size_t next_offset = t_flat_tail::next_offset;
	//	};

	//	using t_result = helper<offset, t...>;

	//  public:
	//	using t_base_list						 = typename t_result::t_base_list;
	//	static constexpr std::size_t next_offset = t_result::next_offset;
	//};

	// template <typename t_base_tree>
	// struct flatten_tree
	//{
	//	using type								= flatten_one<0, t_base_tree>;
	//	using t_base_list						= typename type::t_base_list;
	//	static constexpr std::size_t base_count = type::next_offset;
	// };

	// template <typename t_base_pack>
	// struct unique_base_tree;

	// template <typename... t_unique_base>
	// struct unique_base_tree<meta::type_pack<t_unique_base...>>
	//{
	//	template <std::size_t id>
	//	struct pred
	//	{
	//		template <typename t>
	//		struct type : std::bool_constant<(t::id == id)>
	//		{
	//		};
	//	};

	//	struct type : t_unique_base...
	//	{
	//		using t_self = type;

	//		template <std::size_t id>
	//		using t_base_at = meta::variadic_find_t<pred<id>::template type, void, t_unique_base...>;

	//		template <std::size_t id>
	//		FORCE_INLINE constexpr decltype(auto)
	//		get(this auto&& self) noexcept
	//		{
	//			return static_cast<meta::copy_cv_ref_t<decltype(self), t_base_at<id>>>(FWD(self));
	//		}
	//	};
	//};

	// template <typename t_base_tree>
	// using unique_base_tree_t = typename unique_base_tree<typename flatten_tree<t_base_tree>::t_base_list>::type;

	// template <typename... t_base>
	// struct make_unique_base_tree : unique_base_tree_t<tree<t_base...>>
	//{
	//	using unique_base_tree_t<tree<t_base...>>::get;
	// };

	// template <typename... t_arg>
	// make_unique_base_tree(t_arg&&...)
	//	-> make_unique_base_tree<meta::value_or_ref_t<t_arg&&>...>;

}	 // namespace ecs::system::ctx

namespace ecs::system::ctx
{
	// template <typename... t>
	// struct unique_base
	//{
	// };

	// template <typename t>
	// struct is_unique_base : std::false_type
	//{
	// };

	// template <template <typename...> class t_derived, typename... t_arg>
	// struct is_unique_base<t_derived<t_arg...>>
	//	: std::bool_constant<std::is_base_of_v<unique_base<t_arg...>, t_derived<t_arg...>>>
	//{
	// };

	// template <typename t>
	// inline constexpr bool unique_base_like_v = is_unique_base<t>::value;

	// template <typename... t_child>
	// struct base_tree : t_child...
	//{
	// };

	// template <std::size_t id, typename t>
	// struct base_node
	//{
	//	static constexpr std::size_t node_id = id;
	// };

	// template <typename t>
	// struct is_base_node : std::false_type
	//{
	// };

	// template <std::size_t id, typename t>
	// struct is_base_node<base_node<id, t>> : std::true_type
	//{
	// };

	// template <typename t>
	// inline constexpr bool is_base_node_v = is_base_node<t>::value;

	// template <std::size_t id, typename t_spec>
	// struct rebind_leaf
	//{
	//	using type = std::conditional_t<
	//		is_base_node_v<t_spec>,
	//		t_spec,
	//		base_node<id, t_spec>>;

	//	static constexpr std::size_t next_id = id + 1;
	//};

	// template <std::size_t id, typename t_spec>
	// struct rebind_unique_base
	//{
	//	using type							 = typename rebind_leaf<id, t_spec>::type;
	//	static constexpr std::size_t next_id = rebind_leaf<id, t_spec>::next_id;
	// };

	// template <std::size_t id, typename... t_base>
	// struct rebind_unique_base<id, unique_base<t_base...>>
	//{
	//   private:
	//	template <std::size_t... n>
	//	FORCE_INLINE static constexpr auto
	//	make(std::index_sequence<n...>) noexcept -> unique_base<base_node<id + n, t_base>...>
	//	{
	//		return {};
	//	}

	//  public:
	//	using type							 = decltype(make(std::make_index_sequence<sizeof...(t_base)>{}));
	//	static constexpr std::size_t next_id = id + sizeof...(t_base);
	//};

	// template <std::size_t id, typename... t_child>
	// struct rebind_unique_base<id, base_tree<t_child...>>
	//{
	//   private:
	//	template <std::size_t cur_id, typename t_done_pack, typename... t_rest>
	//	struct impl;

	//	template <std::size_t cur_id, typename... t_done>
	//	struct impl<cur_id, meta::type_pack<t_done...>>
	//	{
	//		using type							 = base_tree<t_done...>;
	//		static constexpr std::size_t next_id = cur_id;
	//	};

	//	template <std::size_t cur_id, typename... t_done, typename t_head, typename... t_tail>
	//	struct impl<cur_id, meta::type_pack<t_done...>, t_head, t_tail...>
	//	{
	//		using t_head_rebind = rebind_unique_base<cur_id, t_head>;
	//		using t_next		= impl<t_head_rebind::next_id, meta::type_pack<t_done..., typename t_head_rebind::type>, t_tail...>;

	//		using type							 = typename t_next::type;
	//		static constexpr std::size_t next_id = t_next::next_id;
	//	};

	//	using t_impl = impl<id, meta::type_pack<>, t_child...>;

	//  public:
	//	using type							 = typename t_impl::type;
	//	static constexpr std::size_t next_id = t_impl::next_id;
	//};

	// template <std::size_t id, template <typename...> class t_derived, typename... t_arg>
	// requires(unique_base_like_v<t_derived<t_arg...>>)
	// struct rebind_unique_base<id, t_derived<t_arg...>>
	//{
	//   private:
	//	template <std::size_t cur_id, typename t_done_pack, typename... t_rest>
	//	struct impl;

	//	template <std::size_t cur_id, typename... t_done>
	//	struct impl<cur_id, meta::type_pack<t_done...>>
	//	{
	//		using type							 = t_derived<t_done...>;
	//		static constexpr std::size_t next_id = cur_id;
	//	};

	//	template <std::size_t cur_id, typename... t_done, typename t_head, typename... t_tail>
	//	struct impl<cur_id, meta::type_pack<t_done...>, t_head, t_tail...>
	//	{
	//		using t_head_rebind = rebind_unique_base<cur_id, t_head>;
	//		using t_next		= impl<t_head_rebind::next_id, meta::type_pack<t_done..., typename t_head_rebind::type>, t_tail...>;

	//		using type							 = typename t_next::type;
	//		static constexpr std::size_t next_id = t_next::next_id;
	//	};

	//	using t_impl = impl<id + 1, meta::type_pack<>, t_arg...>;

	//  public:
	//	using type							 = base_node<id, typename t_impl::type>;
	//	static constexpr std::size_t next_id = t_impl::next_id;
	//};

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
			// return static_cast<meta::copy_cv_ref_t<decltype(self), t_unique_base<id>>>(self);
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

	// template <typename... t_base>
	// struct unique_bases_impl
	//{
	//	static consteval decltype(auto)
	//	base_count_seq()
	//	{
	//		return std::index_sequence<get_unique_base_count<t_base>()...>{};
	//	}
	// };

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

		constexpr unique_bases(auto&&... base)
			requires(std::is_convertible_v<decltype(base), t_base> && ...)
			: make_unique_base_t<n, t_base>(FWD(base))...
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
	};

	// template <typename... t_base>
	// unique_bases(t_base&&...) -> unique_bases<std::index_sequence_for<t_base...>, t_base...>;

	template <typename... t_base>
	unique_bases(t_base&&...) -> unique_bases<decltype(make_id_seq<0, t_base...>()), t_base...>;

	template <typename... t_base>
	using make_unique_bases = unique_bases<decltype(make_id_seq<0, t_base...>()), t_base...>;

	// using make_unique_bases = decltype(unique_bases(std::declval<t_base>()...));

	template <typename... t_sys>
	struct test_adaptor : make_unique_bases<t_sys...>
	{
	};
}	 // namespace ecs::system::ctx

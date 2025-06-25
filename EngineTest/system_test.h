#pragma once

// namespace ecs::system::test
//{
// #define FWD(x) std::forward<decltype(x)>(x)
//
// #if defined(_MSC_VER)
//	#define FORCE_INLINE __forceinline
// #elif defined(__GNUC__) || defined(__clang__)
//	#define FORCE_INLINE inline __attribute__((always_inline))
// #else
//	#define FORCE_INLINE inline
// #endif
//
//	struct invalid_sys_call
//	{
//		invalid_sys_call() = delete;
//	};
//
//	template <typename t_sys, typename... t_arg>
//	FORCE_INLINE constexpr decltype(auto)
//	run_sys(t_sys&& sys, t_arg&&... arg)
//	{
//		if constexpr (ecs::detail::has_operator<t_sys>)
//		{
//			if constexpr (ecs::detail::invocable<&std::decay_t<t_sys>::operator(), decltype(arg)...>)
//			{
//				return sys(FWD(arg)...);
//			}
//			else if constexpr (ecs::detail::invocable<&std::decay_t<t_sys>::operator()>)
//			{
//				return sys();
//			}
//			else
//			{
//				return invalid_sys_call();
//			}
//		}
//		else if constexpr (ecs::detail::has_operator_templated<t_sys, decltype(arg)...>)
//		{
//			if constexpr (ecs::detail::invocable<&std::decay_t<t_sys>::template operator()<t_arg...>, decltype(arg)...>)
//			{
//				return sys.template operator()<t_arg...>(FWD(arg)...);
//			}
//			else if constexpr (ecs::detail::invocable<&std::decay_t<t_sys>::template operator()<t_arg...>>)
//			{
//				return sys.template operator()<t_arg...>();
//			}
//			else
//			{
//				return invalid_sys_call();
//			}
//		}
//		else if constexpr (sizeof...(arg) == 0)
//		{
//			return FWD(sys);
//		}
//		else
//		{
//			return invalid_sys_call();
//		}
//	}
//
//	template <typename... t>
//	consteval std::array<std::size_t, sizeof...(t)>
//	not_empty_sys_idx_arr()
//	{
//		constexpr const bool flags[] = { meta::is_not_empty<t>::value... };
//		auto				 arr	 = std::array<std::size_t, sizeof...(t)>{};
//		for (auto i = 0, idx = 0; i < sizeof...(t); ++i)
//		{
//			if (flags[i])
//			{
//				arr[i] = idx++;
//			}
//		}
//
//		return arr;
//	}
//
//	// Flattens a tuple of tuples into a single flat tuple.
//	// tuple_cat is used because some systems may return void (as empty tuples).
//	// We wrap system calls in a tuple (brace-initializer-list style) to guarantee left-to-right execution order,
//	// as required by the C++ standard, avoiding evaluation order issues with pack expansion.
//	template <typename... t>
//	FORCE_INLINE constexpr decltype(auto)
//	tuple_cat_all(std::tuple<t...>&& tpl)
//	{
//		return std::apply([](auto&&... arg) { return std::tuple_cat(FWD(arg)...); }, FWD(tpl));
//	}
//
//	template <typename... t>
//	FORCE_INLINE constexpr decltype(auto)
//	unwrap_tpl(std::tuple<t...>&& tpl)
//	{
//		if constexpr (sizeof...(t) == 0)
//		{
//			return;
//		}
//		else if constexpr (sizeof...(t) == 1)
//		{
//			return std::get<0>(FWD(tpl));
//		}
//		else
//		{
//			return FWD(tpl);
//		}
//	}
//
//	// For multiple systems, rvalue arguments are moved into storage once
//	// and then passed to each system as lvalue references.
//	template <typename... t_arg>
//	FORCE_INLINE constexpr decltype(auto)
//	make_arg_tpl(t_arg&&... arg)
//	{
//		using t_arg_tpl = std::tuple<
//			std::conditional_t<
//				std::is_lvalue_reference_v<t_arg&&>,
//				t_arg&&,
//				std::remove_reference_t<t_arg>>...>;
//
//		return t_arg_tpl{ FWD(arg)... };
//	}
//
//	template <typename... t_sys>
//	struct seq
//	{
//		using t_self			  = seq<t_sys...>;
//		using t_not_empty_idx_seq = meta::arr_to_seq_t<not_empty_sys_idx_arr<t_sys...>()>;
//		using t_sys_not_empty	  = meta::filtered_tuple_t<meta::is_not_empty, t_sys...>;
//
//		no_unique_addr t_sys_not_empty systems;
//
//		constexpr seq(t_sys&&... sys) : systems(meta::make_filtered_tuple<meta::is_not_empty, t_sys...>(FWD(sys)...)){};
//
//		constexpr seq() requires(std::is_empty_v<t_sys> and ...)
//		= default;
//
//		template <std::size_t i, typename... t_arg>
//		FORCE_INLINE constexpr decltype(auto)
//		run_impl(t_arg&&... arg)
//		{
//			using t_sys_now = meta::variadic_at_t<i, t_sys...>;
//			if constexpr (std::is_empty_v<t_sys_now>)
//			{
//				return run_sys(t_sys_now{}, FWD(arg)...);
//			}
//			else
//			{
//				return run_sys(std::get<meta::index_sequence_at_v<i, t_not_empty_idx_seq>>(systems), FWD(arg)...);
//			}
//		}
//
//		template <std::size_t i, typename... t_arg>
//		FORCE_INLINE constexpr decltype(auto)
//		run_as_tpl(t_arg&&... arg)
//		{
//			using t_ret = decltype(run_impl<i>(FWD(arg)...));
//			if constexpr (std::is_void_v<t_ret>)
//			{
//				run_impl<i>(FWD(arg)...);
//				return std::tuple<>{};
//			}
//			else
//			{
//				return std::make_tuple(run_impl<i>(FWD(arg)...));
//			}
//		}
//
//		template <typename... t_arg>
//		FORCE_INLINE constexpr decltype(auto)
//		operator()(t_arg&&... arg)
//		{
//			[this]<auto... i>(std::index_sequence<i...>) {
//				static_assert(((not std::is_same_v<decltype(run_impl<i>(FWD(arg)...)), invalid_sys_call>)&&...),
//							  "[seq] run_impl<i>(...) returned invalid_sys_call - check that system i is callable with given arguments.");
//			}(std::index_sequence_for<t_sys...>{});
//
//			return unwrap_tpl([this, args = make_arg_tpl(FWD(arg)...)]<auto... i>(std::index_sequence<i...>) {
//				return std::apply(
//					[this](auto&&... l_ref_arg) {
//						return tuple_cat_all(std::tuple{ run_as_tpl<i>(FWD(l_ref_arg)...)... });
//					},
//					args);
//			}(std::index_sequence_for<t_sys...>{}));
//		}
//	};
//
//	template <typename... t_sys>
//	struct par
//	{
//		static_assert(sizeof...(t_sys) > 1, "[par] requires at least 2 systems to be meaningful");
//
//		using t_this			  = par<t_sys...>;
//		using t_not_empty_idx_seq = meta::arr_to_seq_t<not_empty_sys_idx_arr<t_sys...>()>;
//		using t_sys_not_empty	  = meta::filtered_tuple_t<meta::is_not_empty, t_sys...>;
//
//		no_unique_addr t_sys_not_empty systems;
//
//		constexpr par(t_sys&&... sys) : systems(meta::make_filtered_tuple<meta::is_not_empty, t_sys...>(FWD(sys)...)){};
//
//		constexpr par() requires(std::is_empty_v<t_sys> and ...)
//		= default;
//
//		template <typename t_arg>
//		static constexpr bool has_par_exec_member = requires(t_arg arg) {
//			requires std::is_base_of_v<__parallel_executor_base, std::decay_t<decltype(arg.__parallel_executor)>>;
//		};
//
//		template <typename t_arg>
//		struct has_par_exec : std::bool_constant<has_par_exec_member<t_arg>>
//		{
//		};
//
//		template <std::size_t i, typename... t_arg>
//		FORCE_INLINE constexpr decltype(auto)
//		run_impl(t_arg&&... arg)
//		{
//			using t_sys_now = meta::variadic_at_t<i, t_sys...>;
//			if constexpr (std::is_empty_v<t_sys_now>)
//			{
//				return run_sys(t_sys_now{}, FWD(arg)...);
//			}
//			else
//			{
//				return run_sys(std::get<meta::index_sequence_at_v<i, t_not_empty_idx_seq>>(systems), FWD(arg)...);
//			}
//		}
//
//		template <std::size_t i, typename t_tpl>
//		FORCE_INLINE constexpr decltype(auto)
//		run_impl_tpl(t_tpl& tpl)
//		{
//			return std::apply([this](auto&&... arg) { return run_impl<i>(FWD(arg)...); }, tpl);
//		}
//
//		template <std::size_t i, typename t_tpl>
//		FORCE_INLINE constexpr decltype(auto)
//		run_as_tpl(t_tpl& tpl)
//		{
//			using t_ret = decltype(std::apply([this](auto&&... arg) { return run_impl<i>(FWD(arg)...); }, tpl));
//			if constexpr (std::is_void_v<t_ret>)
//			{
//				std::apply([this](auto&&... arg) { return run_impl<i>(FWD(arg)...); }, tpl);
//				return std::tuple<>{};
//			}
//			else
//			{
//				return std::make_tuple(std::apply([this](auto&&... arg) { return run_impl<i>(FWD(arg)...); }, tpl));
//			}
//		}
//
//		template <typename... t_arg>
//		FORCE_INLINE constexpr decltype(auto)
//		operator()(t_arg&&... arg)
//		{
//			[this]<auto... i>(std::index_sequence<i...>) {
//				static_assert(((not std::is_same_v<decltype(run_impl<i>(FWD(arg)...)), invalid_sys_call>)&&...),
//							  "[par] invalid_sys_call - check that system i is callable with given arguments.");
//			}(std::index_sequence_for<t_sys...>{});
//
//
//			if constexpr (meta::index_sequence_size_v<meta::filtered_index_sequence_t<has_par_exec, t_arg...>> == 0)
//			{
//				// default
//				return unwrap_tpl(
//					[this, args = make_arg_tpl(FWD(arg)...)]<auto... i>(std::index_sequence<i...>) {
//						return [](auto&&... async_op) {
//							return tuple_cat_all(std::tuple{ async_op.get()... });
//						}(std::async(std::launch::async, [this, &args]() { return run_as_tpl<i>(args); })...);
//					}(std::index_sequence_for<t_sys...>{}));
//			}
//			else
//			{
//				constexpr auto par_exec_idx = meta::index_sequence_front_v<meta::filtered_index_sequence_t<has_par_exec, t_arg...>>;
//				return unwrap_tpl(
//					[this, args = make_arg_tpl(FWD(arg)...)]<auto... i>(std::index_sequence<i...>) {
//						auto& par_exe = std::get<par_exec_idx>(args).__parallel_executor;
//						return par_exe.run_par(([this, &args] { return run_as_tpl<i>(args); })...);
//					}(std::index_sequence_for<t_sys...>{}));
//			}
//		}
//	};
//
//	template <typename t_sys_l, typename t_sys_r>
//	struct pipe
//	{
//		no_unique_addr t_sys_l sys_l;
//		no_unique_addr t_sys_r sys_r;
//
//		constexpr pipe(t_sys_l&& sys_l, t_sys_r&& sys_r)
//			: sys_l(FWD(sys_l)),
//			  sys_r(FWD(sys_r)) { }
//
//		constexpr pipe() requires(std::is_empty_v<t_sys_l> and std::is_empty_v<t_sys_r>)
//		= default;
//
//		template <typename... t_arg>
//		FORCE_INLINE constexpr decltype(auto)
//		operator()(t_arg&&... arg)
//		{
//			using t_ret_l = decltype(run_sys(sys_l, FWD(arg)...));
//			static_assert(not std::is_same_v<t_ret_l, invalid_sys_call>,
//						  "[pipe] invalid_sys_call - check that system left is callable with given arguments.");
//
//			if constexpr (std::is_void_v<t_ret_l>)
//			{
//				using t_ret_r = decltype(run_sys(sys_r));
//				static_assert(not std::is_same_v<t_ret_r, invalid_sys_call>,
//							  "[pipe] invalid_sys_call - check that system right is chain-able");
//
//				run_sys(sys_l, FWD(arg)...);
//				return run_sys(sys_r);
//			}
//			else
//			{
//				using t_ret_r = decltype(run_sys(sys_r, run_sys(sys_l, FWD(arg)...)));
//				static_assert(not std::is_same_v<t_ret_r, invalid_sys_call>,
//							  "[pipe] invalid_sys_call - check that system right is chain-able");
//
//				return run_sys(sys_r, run_sys(sys_l, FWD(arg)...));
//			}
//		}
//	};
//
//	template <typename t_sys_l, typename t_sys_r>
//	FORCE_INLINE constexpr decltype(auto)
//	operator|(t_sys_l&& sys_l, t_sys_r&& sys_r)
//	{
//		return pipe{ FWD(sys_l), FWD(sys_r) };
//	}
//
//	template <typename t_sys_cond, typename t_sys_then, typename t_sys_else>
//	struct cond
//	{
//		no_unique_addr t_sys_cond sys_cond;
//		no_unique_addr t_sys_then sys_then;
//		no_unique_addr t_sys_else sys_else;
//
//		constexpr cond(t_sys_cond&& sys_cond, t_sys_then&& sys_then, t_sys_else&& sys_else)
//			: sys_cond(FWD(sys_cond)),
//			  sys_then(FWD(sys_then)),
//			  sys_else(FWD(sys_else))
//		{
//		}
//
//		constexpr cond()
//			requires(std::is_empty_v<t_sys_cond> and std::is_empty_v<t_sys_then> and std::is_empty_v<t_sys_else>)
//			= default;
//
//		template <typename... t_arg>
//		FORCE_INLINE constexpr decltype(auto)
//		operator()(t_arg&&... arg)
//		{
//			using t_res_then = decltype(run_sys(sys_then, FWD(arg)...));
//			using t_res_else = decltype(run_sys(sys_else, FWD(arg)...));
//			static_assert(std::is_same_v<t_res_then, t_res_else>, "cond: 'then' and 'else' systems must return the same type");
//
//			auto args = make_arg_tpl(FWD(arg)...);
//
//			return std::apply(
//				[this](auto&&... arg) {
//					if (run_sys(sys_cond, FWD(arg)...))
//					{
//						return run_sys(sys_then, FWD(arg)...);
//					}
//					else
//					{
//						return run_sys(sys_else, FWD(arg)...);
//					}
//				},
//				args);
//		}
//	};
//
//	// constexpr inline auto when = sys_when<>();
//
//	template <typename t_sys_cond, typename t_sys_then>
//	struct cond<t_sys_cond, t_sys_then, void>
//	{
//		no_unique_addr t_sys_cond sys_cond;
//		no_unique_addr t_sys_then sys_then;
//
//		constexpr cond(t_sys_cond&& sys_cond, t_sys_then&& sys_then)
//			: sys_cond(FWD(sys_cond)),
//			  sys_then(FWD(sys_then))
//		{
//		}
//
//		constexpr cond()
//			requires(std::is_empty_v<t_sys_cond> and std::is_empty_v<t_sys_then>)
//			= default;
//
//		template <typename... t_arg>
//		FORCE_INLINE constexpr decltype(auto)
//		operator()(t_arg&&... arg)
//		{
//			using t_res_cond = decltype(run_sys(sys_cond, FWD(arg)...));
//			using t_res_then = decltype(run_sys(sys_then, FWD(arg)...));
//			static_assert(std::is_same_v<t_res_cond, bool>, "cond: system sys_cond is invalid - check if the system is callable with the given arguments or if the system returns bool");
//			static_assert(not std::is_same_v<t_res_then, invalid_sys_call>, "cond: system sys_then is invalid - check if the system is callable with the given arguments");
//			static_assert(std::is_same_v<t_res_then, void>, "cond: 'then' system must return void");
//
//			auto args = make_arg_tpl(FWD(arg)...);
//
//			return std::apply(
//				[this](auto&&... arg) {
//					if (run_sys(sys_cond, FWD(arg)...))
//					{
//						return run_sys(sys_then, FWD(arg)...);
//					}
//				},
//				args);
//		}
//	};
//
//	// Explicit deduction guides for cond because CTAD does not consider partial specializations
//	template <typename t_sys_cond, typename t_sys_then>
//	cond(t_sys_cond&&, t_sys_then&&) -> cond<t_sys_cond, t_sys_then, void>;
//
//	template <typename t_sys_cond, typename t_sys_then, typename t_sys_else>
//	cond(t_sys_cond&&, t_sys_then&&, t_sys_else&&) -> cond<t_sys_cond, t_sys_then, t_sys_else>;
//
//	template <typename t_sys_cond>
//	struct break_if
//	{
//		no_unique_addr t_sys_cond sys_cond;
//
//		constexpr break_if(t_sys_cond&& sys_cond) : sys_cond(FWD(sys_cond)) { }
//
//		constexpr break_if() = default;
//
//		template <typename... t_arg>
//		FORCE_INLINE constexpr decltype(auto)
//		operator()(t_arg&&... arg)
//		{
//			using t_res_cond = decltype(run_sys(sys_cond, FWD(arg)...));
//			static_assert(std::is_convertible_v<t_res_cond, bool>,
//						  "[cond] sys_cond must return a type convertible to bool - check that it's callable and produces a condition-like value");
//
//			return run_sys(sys_cond, FWD(arg)...);
//		}
//	};
//
//	template <typename t_sys_cond>
//	struct continue_if
//	{
//		no_unique_addr t_sys_cond sys_cond;
//
//		constexpr continue_if(t_sys_cond&& sys_cond) : sys_cond(FWD(sys_cond)) { }
//
//		constexpr continue_if() = default;
//
//		template <typename... t_arg>
//		FORCE_INLINE constexpr decltype(auto)
//		operator()(t_arg&&... arg)
//		{
//			using t_res_cond = decltype(run_sys(sys_cond, FWD(arg)...));
//			static_assert(std::is_convertible_v<t_res_cond, bool>,
//						  "[cond] sys_cond must return a type convertible to bool - check that it's callable and produces a condition-like value");
//
//			return run_sys(sys_cond, FWD(arg)...);
//		}
//	};
//
//	template <typename t>
//	inline constexpr bool is_break_if = false;
//
//	template <typename t_sys_cond>
//	inline constexpr bool is_break_if<break_if<t_sys_cond>> = true;
//
//	template <typename t>
//	inline constexpr bool is_continue_if = false;
//
//	template <typename t_sys_cond>
//	inline constexpr bool is_continue_if<continue_if<t_sys_cond>> = true;
//
//	template <typename t>
//	constexpr bool is_break_or_continue = is_break_if<std::decay_t<t>> || is_continue_if<std::decay_t<t>>;
//
//	template <typename t_sys_cond, typename... t_sys>
//	struct loop
//	{
//		using t_not_empty_idx_seq = meta::arr_to_seq_t<not_empty_sys_idx_arr<t_sys...>()>;
//		using t_sys_not_empty	  = meta::filtered_tuple_t<meta::is_not_empty, t_sys...>;
//
//		no_unique_addr t_sys_cond	   sys_cond;
//		no_unique_addr t_sys_not_empty systems;
//
//		constexpr loop(t_sys_cond&& sys_cond, t_sys&&... sys)
//			: sys_cond(FWD(sys_cond)),
//			  systems(meta::make_filtered_tuple<meta::is_not_empty, t_sys...>(FWD(sys)...)){};
//
//		constexpr loop() requires(std::is_empty_v<t_sys_cond> and ... and std::is_empty_v<t_sys>)
//		= default;
//
//		template <std::size_t i, typename... t_arg>
//		FORCE_INLINE constexpr decltype(auto)
//		run_impl(t_arg&&... arg)
//		{
//			using t_sys_now = meta::variadic_at_t<i, t_sys...>;
//			if constexpr (std::is_empty_v<t_sys_now>)
//			{
//				return run_sys(t_sys_now{}, FWD(arg)...);
//			}
//			else
//			{
//				return run_sys(std::get<meta::index_sequence_at_v<i, t_not_empty_idx_seq>>(systems), FWD(arg)...);
//			}
//		}
//
// #define __SYS_LOOP_IMPL(N)                                          \
//	if constexpr (N < sizeof...(t_sys))                             \
//	{                                                               \
//		using t_sys_now = meta::variadic_at_t<N, t_sys...>;         \
//		if constexpr (is_break_if<std::decay_t<t_sys_now>>)         \
//		{                                                           \
//			if (run_impl<N>(FWD(arg)...))                           \
//				break;                                              \
//		}                                                           \
//		else if constexpr (is_continue_if<std::decay_t<t_sys_now>>) \
//		{                                                           \
//			if (run_impl<N>(FWD(arg)...))                           \
//				continue;                                           \
//		}                                                           \
//		else                                                        \
//		{                                                           \
//			run_impl<N>(FWD(arg)...);                               \
//		}                                                           \
//	}
//
//		template <typename... t_arg>
//		FORCE_INLINE constexpr decltype(auto)
//		operator()(t_arg&&... arg)
//		{
//			using t_res_cond = decltype(run_sys(sys_cond, FWD(arg)...));
//			static_assert(std::is_convertible_v<t_res_cond, bool>,
//						  "[loop] sys_cond must return a type convertible to bool - check that it's callable and produces a condition-like value");
//
//			[this]<auto... i>(std::index_sequence<i...>) {
//				static_assert(((not std::is_same_v<decltype(run_impl<i>(FWD(arg)...)), invalid_sys_call>)&&...),
//							  "[loop] invalid_sys_call - check that system i is callable with given arguments.");
//			}(std::index_sequence_for<t_sys...>{});
//
//			static_assert(sizeof...(t_sys) <= 512, "too many systems");
//
//			auto args = make_arg_tpl(FWD(arg)...);
//
//			std::apply(
//				[this](auto&&... arg) {
//					while (run_sys(sys_cond, FWD(arg)...))
//					{
// #define X(N) __SYS_LOOP_IMPL(N)
//						__X_REPEAT_LIST_512
// #undef X
//					}
//				},
//				args);
//		}
//
// #undef __SYS_LOOP_IMPL
//	};
//
//	template <auto n, typename t_sys>
//	struct sys_case
//	{
//		no_unique_addr t_sys sys;
//
//		static constexpr decltype(n) case_value = n;
//
//		constexpr sys_case(t_sys&& sys) : sys(FWD(sys)) { }
//
//		constexpr sys_case() { }
//
//		template <typename... t_arg>
//		FORCE_INLINE constexpr decltype(auto)
//		operator()(t_arg&&... arg)
//		{
//			return run_sys(sys, FWD(arg)...);
//		}
//	};
//
//	template <auto n>
//	struct sys_case<n, void>
//	{
//		static constexpr decltype(n) case_value = n;
//
//		template <typename t_sys>
//		constexpr decltype(auto)
//		operator=(t_sys&& sys) const
//		{
//			return sys_case<n, t_sys>{ FWD(sys) };
//		}
//	};
//
//	template <auto n>
//	inline constexpr sys_case<n, void> on = sys_case<n, void>{};
//
//	template <typename t_sys>
//	struct sys_default
//	{
//		no_unique_addr t_sys sys;
//
//		constexpr sys_default(t_sys&& sys) : sys(FWD(sys)) { }
//
//		constexpr sys_default() { }
//
//		template <typename... t_arg>
//		FORCE_INLINE constexpr decltype(auto)
//		operator()(t_arg&&... arg)
//		{
//			return run_sys(sys, FWD(arg)...);
//		}
//	};
//
//	template <>
//	struct sys_default<void>
//	{
//		template <typename t_sys>
//		constexpr decltype(auto)
//		operator=(t_sys&& sys) const
//		{
//			return sys_default<t_sys>{ FWD(sys) };
//		}
//	};
//
//	inline constexpr sys_default<void> default_to = sys_default<void>{};
//
//	template <typename t>
//	constexpr inline bool is_sys_default_v = false;
//
//	template <typename t_sys>
//	constexpr bool is_sys_default_v<sys_default<t_sys>> = true;
//
//	template <typename t>
//	constexpr inline bool is_sys_case_v = false;
//
//	template <auto n, typename t_sys>
//	constexpr inline bool is_sys_case_v<sys_case<n, t_sys>> = not std::is_void_v<t_sys>;
//
//	template <typename... t_sys_case>
//	FORCE_INLINE constexpr bool
//	has_sys_default()
//	{
//		return (is_sys_default_v<t_sys_case> || ...);
//	}
//
// #define MAX_CASE_COUNT 512
//
//	template <typename... t_sys_case>
//	FORCE_INLINE constexpr bool
//	validate_match()
//	{
//		constexpr auto default_to_count = ((is_sys_default_v<t_sys_case> ? 1 : 0) + ...);
//		constexpr auto case_count		= sizeof...(t_sys_case) - default_to_count;
//
//		static_assert(((is_sys_default_v<t_sys_case> or is_sys_case_v<t_sys_case>)and...),
//					  "match: all arguments must be either on<n> = system or default_to = system");
//
//		static_assert(default_to_count <= 1, "match: only one default_to is allowed");
//
//		if constexpr (default_to_count == 1)
//		{
//			static_assert(is_sys_default_v<meta::variadic_back_t<t_sys_case...>>, "match: default_to must be the last entry");
//		}
//
//		static_assert(case_count <= MAX_CASE_COUNT,
//					  "match: too many cases for switch generation. Increase MAX_CASE_COUNT and expand __X_REPEAT_LIST_512 macro accordingly.");
//
//		constexpr auto all_cases_unique = []<auto... idx>(std::index_sequence<idx...>) {
//			return meta::variadic_auto_unique<meta::variadic_at_t<idx, t_sys_case...>::case_value...>;
//		}(std::make_index_sequence<case_count>());
//
//		static_assert(all_cases_unique, "match: duplicate on<n> detected. Each on<n> must have a unique value.");
//
//		return true;
//	}
//
//	template <typename t_sys_selector, typename... t_sys_case>
//	struct match
//	{
//		static_assert(validate_match<t_sys_case...>(), "match: invalid template arguements");
//
//		using t_not_empty_idx_seq = meta::arr_to_seq_t<not_empty_sys_idx_arr<t_sys_case...>()>;
//		using t_sys_not_empty	  = meta::filtered_tuple_t<meta::is_not_empty, t_sys_case...>;
//
//		no_unique_addr t_sys_selector  sys_selector;
//		no_unique_addr t_sys_not_empty sys_cases;
//
//		constexpr match(t_sys_selector&& sys_selector, t_sys_case&&... sys_case)
//			: sys_selector(FWD(sys_selector)),
//			  sys_cases(meta::make_filtered_tuple<meta::is_not_empty, t_sys_case...>(FWD(sys_case)...)){};
//
//		constexpr match() requires(std::is_empty_v<t_sys_selector> and ... and std::is_empty_v<t_sys_case>)
//		= default;
//
//		FORCE_INLINE static constexpr decltype(auto)
//		case_id_arr()
//		{
//			constexpr auto sys_case_count = sizeof...(t_sys_case) - ((is_sys_default_v<t_sys_case> ? 1 : 0) + ...);
//			auto		   arr			  = meta::seq_to_arr(std::make_index_sequence<MAX_CASE_COUNT>());
//
//			[&arr]<auto... idx>(std::index_sequence<idx...>) {
//				(
//					([&arr] {
//						constexpr auto case_id = meta::variadic_at_t<idx, t_sys_case...>::case_value;
//						arr[idx]			   = case_id;
//						if constexpr (case_id >= 0 and case_id < MAX_CASE_COUNT)
//						{
//							arr[case_id] = idx;
//						}
//					}()),
//					...);
//			}(std::make_index_sequence<sys_case_count>());
//
//			return arr;
//		}
//
//		template <std::size_t i>
//		FORCE_INLINE static constexpr decltype(auto)
//		case_id()
//		{
//			constexpr auto arr = case_id_arr();
//			return arr[i];
//		}
//
//		template <std::size_t i, typename... t_arg>
//		FORCE_INLINE constexpr decltype(auto)
//		run_impl(t_arg&&... arg)
//		{
//			using t_sys_case_now = meta::variadic_at_t<i, t_sys_case...>;
//			if constexpr (std::is_empty_v<t_sys_case_now>)
//			{
//				return run_sys(t_sys_case_now{}, FWD(arg)...);
//			}
//			else
//			{
//				return run_sys(std::get<meta::index_sequence_at_v<i, t_not_empty_idx_seq>>(sys_cases), FWD(arg)...);
//			}
//		}
//
// #define __SYS_CASE_IMPL(N)                   \
//	case case_id<N>():                       \
//	{                                        \
//		if constexpr (N < sys_case_count)    \
//		{                                    \
//			return run_impl<N>(FWD(arg)...); \
//		}                                    \
//	}
//
//		template <typename... t_arg>
//		FORCE_INLINE constexpr decltype(auto)
//		operator()(t_arg&&... arg)
//		{
//			constexpr auto default_to_exists = (is_sys_default_v<t_sys_case> | ...);
//			constexpr auto sys_case_count	 = sizeof...(t_sys_case) - (default_to_exists ? 0 : 1);
//
//			if constexpr (default_to_exists)
//			{
//				[this]<auto... idx>(std::index_sequence<idx...>) {
//					using t_ret_default = decltype(run_impl<sizeof...(t_sys_case) - 1>(FWD(arg)...));
//					static_assert((std::is_same_v<t_ret_default, decltype(run_impl<idx>(FWD(arg)...))> and ...),
//								  "match: when a default_to is provided, all cases and the default_to must return the same type");
//				}(std::make_index_sequence<sys_case_count>());
//			}
//			else
//			{
//				[this]<auto... idx>(std::index_sequence<idx...>) {
//					static_assert((std::is_same_v<void, decltype(run_impl<idx>(FWD(arg)...))> and ...),
//								  "match: when no default_to is provided, all cases must return void");
//				}(std::make_index_sequence<sys_case_count>());
//			}
//
//			auto args = make_arg_tpl(FWD(arg)...);
//
//			return std::apply(
//				[this](auto&&... arg) {
//					auto key = run_sys(sys_selector, FWD(arg)...);
//
//					switch (key)
//					{
// #define X(N) __SYS_CASE_IMPL(N)
//						__X_REPEAT_LIST_512
// #undef X
//					default:
//					{
//						if constexpr (default_to_exists)
//						{
//							return run_impl<sizeof...(t_sys_case) - 1>(FWD(arg)...);
//						}
//						break;
//					}
//					}
//				},
//				args);
//		}
//
// #undef __SYS_CASE_IMPL
// #undef MAX_CASE_COUNT
//	};	  // namespace ecs::system::test
//
// #undef FWD
// #undef FORCE_INLINE
// }	 // namespace ecs::system::test
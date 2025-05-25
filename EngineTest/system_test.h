#pragma once

namespace ecs::system::test
{
#define FWD(x) std::forward<decltype(x)>(x)

#if defined(_MSC_VER)
	#define FORCE_INLINE __forceinline
#elif defined(__GNUC__) || defined(__clang__)
	#define FORCE_INLINE inline __attribute__((always_inline))
#else
	#define FORCE_INLINE inline
#endif

	template <typename t_sys, typename... t_arg>
	FORCE_INLINE decltype(auto)
	run_sys(t_sys&& sys, t_arg&&... arg)
	{
		if constexpr (ecs::detail::has_operator<t_sys>)
		{
			if constexpr (ecs::detail::invocable<&std::decay_t<t_sys>::operator(), decltype(arg)...>)
			{
				return sys(FWD(arg)...);
			}
			else if constexpr (ecs::detail::invocable<&std::decay_t<t_sys>::operator()>)
			{
				return sys();
			}
			else
			{
				static_assert(false, "sys is data type but tries to run with arguments or sys cannot be invoked with given arguments");
			}
		}
		else if constexpr (ecs::detail::has_operator_templated<t_sys, decltype(arg)...>)
		{
			if constexpr (ecs::detail::invocable<&std::decay_t<t_sys>::template operator()<t_arg...>, decltype(arg)...>)
			{
				return sys.template operator()<t_arg...>(FWD(arg)...);
			}
			else if constexpr (ecs::detail::invocable<&std::decay_t<t_sys>::template operator()<t_arg...>>)
			{
				return sys.template operator()<t_arg...>();
			}
			else
			{
				static_assert(false, "sys is data type but tries to run with arguments or sys cannot be invoked with given arguments");
			}
		}
		else if constexpr (sizeof...(arg) == 0)
		{
			return std::forward<t_sys>(sys);
		}
		else
		{
			static_assert(false, "sys is data type but tries to run with arguments or sys cannot be invoked with given arguments");
			// return;
		}
	}

	template <typename... t>
	consteval std::array<std::size_t, sizeof...(t)>
	not_empty_sys_idx_arr()
	{
		constexpr const bool flags[] = { meta::is_not_empty<t>::value... };
		auto				 arr	 = std::array<std::size_t, sizeof...(t)>{};
		for (auto i = 0, idx = 0; i < sizeof...(t); ++i)
		{
			if (flags[i])
			{
				arr[i] = idx++;
			}
		}

		return arr;
	}

	// Flattens a tuple of tuples into a single flat tuple.
	// tuple_cat is used because some systems may return void (as empty tuples).
	// We wrap system calls in a tuple (brace-initializer-list style) to guarantee left-to-right execution order,
	// as required by the C++ standard, avoiding evaluation order issues with pack expansion.
	template <typename... t>
	FORCE_INLINE decltype(auto)
	tuple_cat_all(std::tuple<t...>&& tpl)
	{
		return std::apply([](auto&&... arg) { return std::tuple_cat(FWD(arg)...); }, FWD(tpl));
	}

	// For multiple systems, rvalue arguments are moved into storage once
	// and then passed to each system as lvalue references.
	template <typename... t_arg>
	FORCE_INLINE decltype(auto)
	make_arg_tpl(t_arg&&... arg)
	{
		using t_arg_tpl = std::tuple<
			std::conditional_t<
				std::is_lvalue_reference_v<t_arg&&>,
				t_arg&&,
				std::remove_reference_t<t_arg>>...>;

		return t_arg_tpl{ std::forward<t_arg>(arg)... };
	}

	template <typename... t_sys>
	struct seq
	{
		using t_not_empty_idx_seq = meta::arr_to_seq_t<not_empty_sys_idx_arr<t_sys...>()>;

		using t_sys_not_empty = meta::filtered_tuple_t<meta::is_not_empty, t_sys...>;

		no_unique_addr t_sys_not_empty systems;

		seq(t_sys&&... sys) : systems(meta::make_filtered_tuple<meta::is_not_empty, t_sys...>(FWD(sys)...)) { };

		seq() requires(std::is_empty_v<t_sys> and ...)
		= default;

		template <std::size_t i, typename... t_arg>
		FORCE_INLINE decltype(auto)
		run_impl(t_arg&&... arg)
		{
			using t_sys_now = meta::variadic_at_t<i, t_sys...>;
			if constexpr (std::is_empty_v<t_sys_now>)
			{
				if constexpr (std::is_same_v<decltype(run_sys(t_sys_now{}, FWD(arg)...)), void>)
				{
					run_sys(t_sys_now{}, FWD(arg)...);
					return std::tuple<>{};
				}
				else
				{
					return std::make_tuple(run_sys(t_sys_now{}, FWD(arg)...));
				}
			}
			else
			{
				if constexpr (std::is_same_v<decltype(run_sys(std::get<meta::index_sequence_at_v<i, t_not_empty_idx_seq>>(systems), FWD(arg)...)), void>)
				{
					run_sys(std::get<meta::index_sequence_at_v<i, t_not_empty_idx_seq>>(systems), FWD(arg)...);
					return std::tuple<>{};
				}
				else
				{
					return std::make_tuple(run_sys(std::get<meta::index_sequence_at_v<i, t_not_empty_idx_seq>>(systems), FWD(arg)...));
				}
			}
		}

		template <typename... t_arg>
		FORCE_INLINE decltype(auto)
		operator()(t_arg&&... arg)
		{
			if constexpr (sizeof...(t_sys) == 1)
			{
				return run_impl<0>(FWD(arg)...);
			}
			else
			{
				return [this, args = make_arg_tpl(FWD(arg)...)]<auto... i>(std::index_sequence<i...>) {
					return std::apply(
						[this](auto&&... l_ref_arg) {
							return tuple_cat_all(std::tuple{ run_impl<i>(FWD(l_ref_arg)...)... });
						},
						args);
				}(std::index_sequence_for<t_sys...>{});
			}
		}
	};

	template <typename... t_sys>
	struct par
	{
		using t_this			  = par<t_sys...>;
		using t_not_empty_idx_seq = meta::arr_to_seq_t<not_empty_sys_idx_arr<t_sys...>()>;
		using t_sys_not_empty	  = meta::filtered_tuple_t<meta::is_not_empty, t_sys...>;

		no_unique_addr t_sys_not_empty systems;

		par(t_sys&&... sys) : systems(meta::make_filtered_tuple<meta::is_not_empty, t_sys...>(FWD(sys)...)) { };

		par() requires(std::is_empty_v<t_sys> and ...)
		= default;

		template <typename t_arg>
		static constexpr bool has_par_exec_member = requires(t_arg arg) {
			requires std::is_base_of_v<__parallel_executor_base, std::decay_t<decltype(arg.__parallel_executor)>>;
		};

		template <typename t_arg>
		struct has_par_exec : std::bool_constant<has_par_exec_member<t_arg>>
		{
		};

		template <std::size_t i, typename... t_arg>
		FORCE_INLINE decltype(auto)
		run_impl(t_arg&&... arg)
		{
			using t_sys_now = meta::variadic_at_t<i, t_sys...>;
			if constexpr (std::is_empty_v<t_sys_now>)
			{
				if constexpr (std::is_same_v<decltype(run_sys(t_sys_now{}, FWD(arg)...)), void>)
				{
					run_sys(t_sys_now{}, FWD(arg)...);
					return std::tuple<>{};
				}
				else
				{
					return std::make_tuple(run_sys(t_sys_now{}, FWD(arg)...));
				}
			}
			else
			{
				if constexpr (std::is_same_v<decltype(run_sys(std::get<meta::index_sequence_at_v<i, t_not_empty_idx_seq>>(systems), FWD(arg)...)), void>)
				{
					run_sys(std::get<meta::index_sequence_at_v<i, t_not_empty_idx_seq>>(systems), FWD(arg)...);
					return std::tuple<>{};
				}
				else
				{
					return std::make_tuple(run_sys(std::get<meta::index_sequence_at_v<i, t_not_empty_idx_seq>>(systems), FWD(arg)...));
				}
			}
		}

		template <std::size_t i, typename t_tpl>
		FORCE_INLINE decltype(auto)
		run_impl_tpl(t_tpl& tpl)
		{
			return std::apply([this](auto&&... arg) { return run_impl<i>(FWD(arg)...); }, tpl);
		}

		template <typename... t_arg>
		FORCE_INLINE decltype(auto)
		operator()(t_arg&&... arg)
		{
			if constexpr (sizeof...(t_sys) == 0)
			{
				return;
			}
			else if constexpr (sizeof...(t_sys) == 1)
			{
				return run_impl<0>(FWD(arg)...);
			}
			else if constexpr (meta::index_sequence_size_v<meta::filtered_index_sequence_t<has_par_exec, t_arg...>> == 0)
			{
				// default
				auto args = make_arg_tpl(FWD(arg)...);
				return [this, &args]<auto... i>(std::index_sequence<i...>) {
					return [](auto&&... async_op) {
						return tuple_cat_all(std::tuple{ async_op.get()... });
					}(std::async(std::launch::async, [this, &args]() { return run_impl_tpl<i>(args); })...);
				}(std::index_sequence_for<t_sys...>{});
			}
			else
			{
				constexpr auto par_exec_idx = meta::index_sequence_front_v<meta::filtered_index_sequence_t<has_par_exec, t_arg...>>;
				auto		   args			= make_arg_tpl(FWD(arg)...);

				return [this, &args]<auto... i>(std::index_sequence<i...>) {
					auto& par_exe = std::get<par_exec_idx>(args).__parallel_executor;
					return [&par_exe](auto&&... func) {
						return par_exe.run_par(FWD(func)...);
					}(([this, &args] { return run_impl_tpl<i>(args); })...);
				}(std::index_sequence_for<t_sys...>{});
			}
		}
	};

	template <typename t_sys_l, typename t_sys_r>
	struct pipe
	{
		no_unique_addr t_sys_l sys_l;
		no_unique_addr t_sys_r sys_r;

		pipe(t_sys_l&& sys_l, t_sys_r&& sys_r) : sys_l(FWD(sys_l)), sys_r(FWD(sys_r)) { }

		pipe() requires(std::is_empty_v<t_sys_l> and std::is_empty_v<t_sys_r>)
		= default;

		template <typename... t_arg>
		FORCE_INLINE decltype(auto)
		operator()(t_arg&&... arg)
		{
			if constexpr (std::is_void_v<decltype(run_sys(sys_l, FWD(arg)...))>)
			{
				run_sys(sys_l, FWD(arg)...);
				return run_sys(sys_r);
			}
			else
			{
				return run_sys(sys_r, run_sys(sys_l, FWD(arg)...));
			}
		}
	};

	template <typename t_sys_l, typename t_sys_r>
	FORCE_INLINE decltype(auto)
	operator|(t_sys_l&& sys_l, t_sys_r&& sys_r)
	{
		return pipe{ FWD(sys_l), FWD(sys_r) };
	}

	template <typename t_sys_cond, typename t_sys_then, typename t_sys_else>
	struct cond
	{
		no_unique_addr t_sys_cond sys_cond;
		no_unique_addr t_sys_then sys_then;
		no_unique_addr t_sys_else sys_else;

		constexpr cond(t_sys_cond&& sys_cond, t_sys_then&& sys_then, t_sys_else&& sys_else)
			: sys_cond(FWD(sys_cond)),
			  sys_then(FWD(sys_then)),
			  sys_else(FWD(sys_else)) { }

		constexpr cond() requires(std::is_empty_v<t_sys_cond> and std::is_empty_v<t_sys_then> and std::is_empty_v<t_sys_else>)
		= default;

		template <typename... t_arg>
		void run(t_arg&&... arg)
		{
			// todo double FWD
			if (_run_sys(sys_cond, FWD(arg)...))
			{
				_run_sys(sys_then, FWD(arg)...);
			}
			else
			{
				_run_sys(sys_else, FWD(arg)...);
			}
		}
	};

	template <typename t_sys_cond, typename t_sys_then>
	struct cond<t_sys_cond, t_sys_then, void>
	{
		no_unique_addr t_sys_cond sys_cond;
		no_unique_addr t_sys_then sys_then;

		constexpr cond(t_sys_cond&& sys_cond, t_sys_then&& sys_then)
			: sys_cond(FWD(sys_cond)),
			  sys_then(FWD(sys_then)) { }

		constexpr cond() requires(std::is_empty_v<t_sys_cond> and std::is_empty_v<t_sys_then>)
		= default;

		template <typename... t_arg>
		void run(t_arg&&... arg)
		{
			if (_run_sys(sys_cond, FWD(arg)...))
			{
				_run_sys(sys_then, FWD(arg)...);
			}
		}
	};

#undef FWD
}	 // namespace ecs::system::test
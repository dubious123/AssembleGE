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

	template <typename... t_sys>
	struct seq
	{
		using t_not_empty_idx_seq = meta::arr_to_seq_t<not_empty_sys_idx_arr<t_sys...>()>;

		using t_sys_not_empty = meta::filtered_tuple_t<meta::is_not_empty, t_sys...>;

		no_unique_addr t_sys_not_empty systems;

		seq(t_sys&&... sys) : systems(meta::make_filtered_tuple<meta::is_not_empty, t_sys...>(FWD(sys)...)){};

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
		inline decltype(auto)
		operator()(t_arg&&... arg)
		{
			if constexpr (sizeof...(t_sys) == 1)
			{
				return run_impl<0>(FWD(arg)...);
			}
			else
			{
				return [this, &arg...]<auto... i>(std::index_sequence<i...>) {
					return tuple_cat_all(std::tuple{ run_impl<i>(FWD(arg)...)... });
				}(std::index_sequence_for<t_sys...>{});
			}
		}
	};

	template <typename... t_sys>
	struct par
	{
		using t_not_empty_idx_seq = meta::arr_to_seq_t<not_empty_sys_idx_arr<t_sys...>()>;

		using t_sys_not_empty = meta::filtered_tuple_t<meta::is_not_empty, t_sys...>;

		no_unique_addr t_sys_not_empty systems;

		par(t_sys&&... sys) : systems(meta::make_filtered_tuple<meta::is_not_empty, t_sys...>(FWD(sys)...)){};

		template <typename t>
		concept has_par_exec_member = requires(t v) {
			requires std::is_base_of_v<__parallel_executor_base, std::decay_t<decltype(v.__parallel_executor)>>;
		};

		template <typename t>
		struct has_par_exec : std::bool_constant<bool, has_par_exec_member<t>>
		{
		};

		template <typename... t_arg>
		constexpr decltype(auto) extract_par_exec(t_arg&&... arg)
		{
			return [tpl = std::make_tuple(FWD(arg)...)]<auto... i>(std::index_sequence<i...>) {
				return std::get<meta::variadic_auto_at_v<0, i...>>(FWD(tpl));
			}(meta::make_filtered_index_sequence<template has_par_exec, t_arg...>());
		}

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
		inline decltype(auto)
		operator()(t_arg&&... arg)
		{
			if constexpr (sizeof...(t_sys) == 1)
			{
				return run_impl<0>(FWD(arg)...);
			}
			else
			{
				return [this, &arg...]<auto... i>(std::index_sequence<i...>) {
					return tuple_cat_all(std::tuple{ run_impl<i>(FWD(arg)...)... });
				}(std::index_sequence_for<t_sys...>{});
			}
		}

		template <std::size_t... i, typename... t_arg>
		decltype(auto) run_with_default(std::index_sequence<i...>, t_arg&&... arg)
		{
			auto futures = std::make_tuple(
				std::async(std::launch::async, [&] {
					using t_sys_now = std::tuple_element_t<i, t_all_sys_tpl>;
					if constexpr (not std::is_empty_v<t_sys_now>)
					{
						_run_sys(std::get<not_empty_sys_idx_arr[i]>(systems), std::forward<t_arg>(arg)...);
					}
					else
					{
						_run_sys(t_sys_now{}, std::forward<t_arg>(arg)...);
					}
				})...);
			(..., (std::get<i>(futures).wait()));
		}

		template <typename t_par_exec, std::size_t... i, typename... t_arg>
		decltype(auto) run_with_par_exec(t_par_exec&& par_exec, std::index_sequence<i...>, t_arg&&... arg)
		{
			return par_exec.run_par(
				([&] {
					using t_sys_now = std::tuple_element_t<i, t_all_sys_tpl>;
					if constexpr (not std::is_empty_v<t_sys_now>)
					{
						_run_sys(std::get<not_empty_sys_idx_arr[i]>(systems), std::forward<t_arg>(arg)...);
					}
					else
					{
						_run_sys(t_sys_now{}, std::forward<t_arg>(arg)...);
					}
				})...);
		}
	};

#undef FWD
}	 // namespace ecs::system::test
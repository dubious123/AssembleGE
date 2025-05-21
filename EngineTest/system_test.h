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

	// template <typename t_sys, typename... t_data>
	// decltype(auto) _run_sys(t_sys&& sys, t_data&&... data)
	//{
	//	if constexpr (ecs::detail::has_run<t_sys, decltype(data)...>)
	//	{
	//		if constexpr (ecs::detail::invocable<&std::decay_t<t_sys>::run, decltype(data)...>)
	//		{
	//			return sys.run(std::forward<decltype(data)>(data)...);
	//		}
	//		else if constexpr (ecs::detail::invocable<&std::decay_t<t_sys>::run>)
	//		{
	//			return sys.run();
	//		}
	//		else
	//		{
	//			static_assert(false, "sys is data type but tries to run with arguments or sys cannot be invoked with given arguments");
	//		}
	//	}
	//	else if constexpr (ecs::detail::has_run_templated<t_sys, decltype(data)...>)
	//	{
	//		if constexpr (ecs::detail::invocable<&std::decay_t<t_sys>::template run<t_data...>, decltype(data)...>)
	//		{
	//			return sys.run<t_data...>(std::forward<decltype(data)>(data)...);
	//		}
	//		else if constexpr (ecs::detail::invocable<&std::decay_t<t_sys>::template run<decltype(data)...>>)
	//		{
	//			return sys.run<>();
	//		}
	//		else
	//		{
	//			static_assert(false, "sys is data type but tries to run with arguments or sys cannot be invoked with given arguments");
	//		}
	//	}
	//	else if constexpr (ecs::detail::has_operator<t_sys>)
	//	{
	//		if constexpr (ecs::detail::invocable<&std::decay_t<t_sys>::operator(), decltype(data)...>)
	//		{
	//			return sys(std::forward<decltype(data)>(data)...);
	//		}
	//		else if constexpr (ecs::detail::invocable<&std::decay_t<t_sys>::operator()>)
	//		{
	//			return sys();
	//		}
	//		else
	//		{
	//			static_assert(false, "sys is data type but tries to run with arguments or sys cannot be invoked with given arguments");
	//		}
	//	}
	//	else if constexpr (ecs::detail::has_operator_templated<t_sys, decltype(data)...>)
	//	{
	//		if constexpr (ecs::detail::invocable<&std::decay_t<t_sys>::template operator()<t_data...>, decltype(data)...>)
	//		{
	//			return sys.template operator()<t_data...>(std::forward<decltype(data)>(data)...);
	//		}
	//		else if constexpr (ecs::detail::invocable<&std::decay_t<t_sys>::template operator()<t_data...>>)
	//		{
	//			return sys.template operator()<t_data...>();
	//		}
	//		else
	//		{
	//			static_assert(false, "sys is data type but tries to run with arguments or sys cannot be invoked with given arguments");
	//		}
	//	}
	//	else if constexpr (sizeof...(data) == 0)
	//	{
	//		return std::forward<t_sys>(sys);
	//	}
	//	else
	//	{
	//		static_assert(false, "sys is data type but tries to run with arguments or sys cannot be invoked with given arguments");
	//		// return;
	//	}
	// }

	template <typename... t>
	consteval std::array<std::size_t, sizeof...(t)> not_empty_sys_idx_arr()
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
				return t_sys_now{}.operator()(FWD(arg)...);
			}
			else
			{
				return std::get<meta::index_sequence_at_v<i, t_not_empty_idx_seq>>(systems)(FWD(arg)...);
			}
		}

		template <typename... t_arg>
		inline decltype(auto)
		operator()(t_arg&&... arg)
		{
			if constexpr (sizeof...(t_sys) == 0)
			{
				return run_impl<0>(FWD(arg)...);
			}
			else
			{
				return [this, &arg...]<auto... i>(std::index_sequence<i...>) {
					return std::tuple_cat(
						(std::is_void_v<decltype(run_impl<i>(FWD(arg)...))>
							 ? std::tuple<>{}
							 : std::make_tuple(run_impl<i>(FWD(arg)...)))...);
				}(std::index_sequence_for<t_sys...>{});
			}
		}
	};

#undef FWD
}	 // namespace ecs::system::test
#pragma once
#include "__common.h"

namespace ecs
{
	namespace detail
	{
		template <typename t_tpl_from, typename t_tpl_to>
		concept tpl_convertible_from = requires {
			requires std::tuple_size_v<t_tpl_from> == std::tuple_size_v<t_tpl_to>;
			requires[]<std::size_t... i>(std::index_sequence<i...>)
			{
				return true && (... && std::is_convertible_v<std::tuple_element_t<i, t_tpl_from>, std::tuple_element_t<i, t_tpl_to>>);
			}
			(std::make_index_sequence<std::tuple_size_v<t_tpl_from>>{});
		};

		template <auto f, typename... t_data>
		concept invocable = requires {
			requires tpl_convertible_from<std::tuple<t_data...>, typename meta::function_traits<f>::argument_types>;
		};

		template <typename t_sys, typename... t_data>
		concept has_run_templated = requires {
			&std::remove_cvref_t<t_sys>::template run<t_data...>;
		};

		template <typename t_sys, typename... t_data>
		concept has_run = requires {
			&std::remove_cvref_t<t_sys>::run;
		};

		template <typename t_callable, typename... t_data>
		concept has_operator_templated = requires {
			&std::remove_cvref_t<t_callable>::template operator()<t_data...>;
		};

		template <typename t_callable, typename... t_data>
		concept has_operator = requires {
			&std::remove_cvref_t<t_callable>::operator();
		};
	}	 // namespace detail

	template <typename t_sys, typename... t_data>
	decltype(auto) _run_sys(t_sys&& sys, t_data&&... data)
	{
		if constexpr (ecs::detail::has_run<t_sys, decltype(data)...>)
		{
			if constexpr (ecs::detail::invocable<&std::decay_t<t_sys>::run, decltype(data)...>)
			{
				return sys.run(std::forward<decltype(data)>(data)...);
			}
			else if constexpr (ecs::detail::invocable<&std::decay_t<t_sys>::run>)
			{
				return sys.run();
			}
			else
			{
				static_assert(false, "sys is data type but tries to run with arguments or sys cannot be invoked with given arguments");
			}
		}
		else if constexpr (ecs::detail::has_run_templated<t_sys, decltype(data)...>)
		{
			if constexpr (ecs::detail::invocable<&std::decay_t<t_sys>::template run<t_data...>, decltype(data)...>)
			{
				return sys.run<t_data...>(std::forward<decltype(data)>(data)...);
			}
			else if constexpr (ecs::detail::invocable<&std::decay_t<t_sys>::template run<decltype(data)...>>)
			{
				return sys.run<>();
			}
			else
			{
				static_assert(false, "sys is data type but tries to run with arguments or sys cannot be invoked with given arguments");
			}
		}
		else if constexpr (ecs::detail::has_operator<t_sys>)
		{
			if constexpr (ecs::detail::invocable<&std::decay_t<t_sys>::operator(), decltype(data)...>)
			{
				return sys(std::forward<decltype(data)>(data)...);
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
		else if constexpr (ecs::detail::has_operator_templated<t_sys, decltype(data)...>)
		{
			if constexpr (ecs::detail::invocable<&std::decay_t<t_sys>::template operator()<t_data...>, decltype(data)...>)
			{
				return sys.template operator()<t_data...>(std::forward<decltype(data)>(data)...);
			}
			else if constexpr (ecs::detail::invocable<&std::decay_t<t_sys>::template operator()<t_data...>>)
			{
				return sys.template operator()<t_data...>();
			}
			else
			{
				static_assert(false, "sys is data type but tries to run with arguments or sys cannot be invoked with given arguments");
			}
		}
		else if constexpr (sizeof...(data) == 0)
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
	consteval std::array<std::size_t, sizeof...(t)> make_not_empty_sys_idx_arr()
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
}	 // namespace ecs

#include "__ecs_system_loop.h"
#include "__ecs_system_seq.h"
#include "__ecs_system_par.h"
#include "__ecs_system_pipeline.h"
#include "__ecs_system_cond.h"
#include "__ecs_system_match.h"

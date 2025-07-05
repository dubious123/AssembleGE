#pragma once
#include "__common.h"
#include "__meta.h"

namespace ecs::system
{
	namespace detail
	{
		struct invalid_sys_call
		{
			invalid_sys_call() = delete;
		};

		template <typename t_from, typename t_to>
		concept convertible_from = requires {
			requires std::is_convertible_v<t_from, t_to>
						 || (requires(t_from&& arg) { t_to{ FWD(arg) }; });
		};

		template <typename t_tpl_from, typename t_tpl_to>
		concept tpl_convertible_from = requires {
			requires std::tuple_size_v<t_tpl_from> == std::tuple_size_v<t_tpl_to>;
			requires[]<std::size_t... i>(std::index_sequence<i...>)
			{
				return (convertible_from<std::tuple_element_t<i, t_tpl_from>, std::tuple_element_t<i, t_tpl_to>> and ...);
				// return true && (... && std::is_convertible_v<std::tuple_element_t<i, t_tpl_from>, std::tuple_element_t<i, t_tpl_to>>);
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

		template <typename t_to, typename t_from>
		FORCE_INLINE constexpr decltype(auto)
		make_param(t_from&& arg)
		{
			if constexpr (std::is_convertible_v<t_from, t_to>)
			{
				return FWD(arg);
			}
			else
			{
				return t_to{ FWD(arg) };
			}
		}

		template <typename t_from, typename t_to>
		FORCE_INLINE constexpr decltype(auto)
		run_sys_helper(t_from&& arg)
		{
		}

		template <typename t_sys, typename... t_arg>
		FORCE_INLINE constexpr decltype(auto)
		run_sys(t_sys&& sys, t_arg&&... arg)
		{
			if constexpr (has_operator<t_sys>)
			{
				if constexpr (invocable<&std::decay_t<t_sys>::operator(), decltype(arg)...>)
				{
					return sys(FWD(arg)...);
				}
				else if constexpr (invocable<&std::decay_t<t_sys>::operator()>)
				{
					return sys();
				}
				else
				{
					return invalid_sys_call();
				}
			}
			else if constexpr (has_operator_templated<t_sys, decltype(arg)...>)
			{
				if constexpr (invocable<&std::decay_t<t_sys>::template operator()<t_arg...>, decltype(arg)...>)
				{
					using from_tpl = std::tuple<decltype(FWD(arg))...>;
					using to_tpl   = typename meta::function_traits<&std::decay_t<t_sys>::template operator()<t_arg...>>::argument_types;
					return [&]<auto... i>(std::index_sequence<i...>) -> decltype(auto) {
						return sys.template operator()<t_arg...>(
							(make_param<std::tuple_element_t<i, to_tpl>>(FWD(arg)))...);
					}(std::make_index_sequence<sizeof...(arg)>{});
					// return sys.template operator()<t_arg...>(FWD(arg)...);
				}
				else if constexpr (invocable<&std::decay_t<t_sys>::template operator()<t_arg...>>)
				{
					return sys.template operator()<t_arg...>();
				}
				else
				{
					return invalid_sys_call();
				}
			}
			else if constexpr (sizeof...(arg) == 0)
			{
				return FWD(sys);
			}
			else
			{
				return invalid_sys_call();
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
		template <typename t_tpl>
		FORCE_INLINE constexpr decltype(auto)
		tuple_cat_all(t_tpl&& tpl)
		{
			return std::apply([](auto&&... arg) { return std::tuple_cat(FWD(arg)...); }, FWD(tpl));
		}

		template <typename... t>
		FORCE_INLINE constexpr decltype(auto)
		unwrap_tpl(std::tuple<t...>&& tpl)
		{
			if constexpr (sizeof...(t) == 0)
			{
				return;
			}
			else if constexpr (sizeof...(t) == 1)
			{
				return std::get<0>(FWD(tpl));
			}
			else
			{
				return FWD(tpl);
			}
		}

		// For multiple systems, rvalue arguments are moved into storage once
		// and then passed to each system as lvalue references.
		template <typename... t_arg>
		FORCE_INLINE constexpr decltype(auto)
		make_arg_tpl(t_arg&&... arg)
		{
			using t_arg_tpl = std::tuple<
				std::conditional_t<
					std::is_lvalue_reference_v<t_arg&&>,
					t_arg&&,
					std::remove_reference_t<t_arg>>...>;

			return t_arg_tpl{ FWD(arg)... };
		}

	}	 // namespace detail

}	 // namespace ecs::system

#define INCLUDED_FROM_ECS_SYSTEM_HEADER
#include "__ecs_system_seq.h"
#include "__ecs_system_par.h"
#include "__ecs_system_pipeline.h"
#include "__ecs_system_cond.h"
#include "__ecs_system_loop.h"
#include "__ecs_system_match.h"

#include "__ecs_system_each.h"
#undef INCLUDED_FROM_ECS_SYSTEM_HEADER

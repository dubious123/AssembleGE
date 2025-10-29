#pragma once
#include "__common.h"
#include "__meta.h"

namespace ecs::system
{
	namespace detail
	{
		template <typename t_func>
		struct scope_guard
		{
			static_assert(std::is_nothrow_destructible_v<t_func> and noexcept(std::declval<t_func&>()()),
						  "scope_guard: fn() should be noexcept and you must ensure no-throw in dtor.");

			no_unique_addr t_func func;

			explicit constexpr scope_guard(t_func f) noexcept(std::is_nothrow_move_constructible_v<t_func>)
				: func(FWD(f)) { }

			scope_guard(const scope_guard&) = delete;
			scope_guard&
			operator=(const scope_guard&) = delete;

			// constexpr scope_guard(scope_guard&&) = delete;
			// scope_guard&
			// operator=(scope_guard&&) = delete;

			constexpr ~scope_guard() noexcept
			{
				func();
			}
		};

		// template <typename... t_val>
		// struct sys_result
		//{
		//	no_unique_addr std::tuple<t_val...> data;

		//	constexpr sys_result() = default;

		//	constexpr sys_result(std::tuple<t_val...>&& tpl) : data(FWD(tpl)){};

		//	constexpr sys_result(const std::tuple<t_val...>& tpl) : data(FWD(tpl)){};

		//	constexpr sys_result(sys_result&&) noexcept = default;

		//	constexpr sys_result&
		//	operator=(sys_result&&) noexcept = default;

		//	constexpr sys_result(const sys_result&) = delete;

		//	constexpr sys_result&
		//	operator=(const sys_result&) = delete;
		//};

		struct invalid_sys_call
		{
#ifdef _DEBUG
			invalid_sys_call()
			{
				assert(false);
			}
#else
			invalid_sys_call() = delete;
#endif
		};

		// template <typename t>
		// struct is_sys_result_impl : std::false_type
		//{
		// };

		// template <typename... t_val>
		// struct is_sys_result_impl<sys_result<t_val...>> : std::true_type
		//{
		// };

		// template <typename t>
		// concept is_sys_result = is_sys_result_impl<t>::value;

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

		template <typename t_callable, typename... t_data>
		concept has_operator_templated = requires {
			&std::remove_cvref_t<t_callable>::template operator()<t_data...>;
		};

		template <typename t_callable, typename... t_data>
		concept has_operator = requires {
			&std::remove_cvref_t<t_callable>::operator();
		};

		// return (a, a+1, ..., b-1, b)
		template <std::size_t a, std::size_t b>
		using index_range_t = decltype([]<auto... i>(std::index_sequence<i...>) {
			return std::index_sequence<(a + i)...>{};
		}(std::make_index_sequence<b - a + 1>{}));

		template <std::size_t arr_size, typename t_filtered_idx_seq>
		struct index_ranges_seq;

		template <std::size_t arr_size>
		struct index_ranges_seq<arr_size, std::index_sequence<>>
		{
			using type = meta::type_pack<index_range_t<0, arr_size>>;
		};

		template <std::size_t arr_size, std::size_t idx_head>
		struct index_ranges_seq<arr_size, std::index_sequence<idx_head>>
		{
			using type = meta::type_pack<>;
		};

		template <std::size_t arr_size, std::size_t idx_head, std::size_t idx_next, std::size_t... idx_tail>
		struct index_ranges_seq<arr_size, std::index_sequence<idx_head, idx_next, idx_tail...>>
		{
			using type = meta::type_pack_cat_t<
				meta::type_pack<index_range_t<idx_head, idx_next>>,
				typename index_ranges_seq<arr_size, std::index_sequence<idx_next + 1, idx_tail...>>::type>;
		};

		template <std::size_t arr_size, typename t_filtered_idx_seq>
		using index_ranges_seq_t = index_ranges_seq<
			arr_size,
			meta::index_sequence_cat_t<
				std::index_sequence<0>,
				decltype([] {
					if constexpr (meta::index_sequence_size_v<t_filtered_idx_seq> == 0)
					{
						return std::index_sequence<>{};
					}
					else
					{
						return meta::pop_back_seq_t<t_filtered_idx_seq>{};
					}
				}()),
				std::index_sequence<arr_size>>>::type;

		template <typename t_to, typename t_from>
		FORCE_INLINE constexpr decltype(auto)
		make_param(t_from&& arg) noexcept
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

		template <typename t_sys, typename... t_arg>
		FORCE_INLINE constexpr decltype(auto)
		run_sys(t_sys&& sys, t_arg&&... arg) noexcept
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
					// static_assert(false, "invalid_sys_call");
					return invalid_sys_call();
				}
			}
			else if constexpr (has_operator_templated<t_sys, decltype(arg)...>)
			{
				if constexpr (invocable<&std::decay_t<t_sys>::template operator()<t_arg...>, decltype(arg)...>)
				{
					using to_tpl = typename meta::function_traits<&std::decay_t<t_sys>::template operator()<t_arg...>>::argument_types;

					return []<auto... i>(std::index_sequence<i...>, auto&& sys, auto&&... arg) noexcept -> decltype(auto) INLINE_LAMBDA {
						return sys.template operator()<t_arg...>(
							(make_param<std::tuple_element_t<i, to_tpl>>(FWD(arg)))...);
					}(std::make_index_sequence<sizeof...(arg)>{}, FWD(sys), FWD(arg)...);
					// return sys.template operator()<t_arg...>(FWD(arg)...);
				}
				else if constexpr (invocable<&std::decay_t<t_sys>::template operator()<t_arg...>>)
				{
					return sys.template operator()<t_arg...>();
				}
				else
				{
					// static_assert(false, "invalid_sys_call");
					return invalid_sys_call();
				}
			}
			else if constexpr (sizeof...(arg) == 0)
			{
				return invalid_sys_call();
				// return FWD(sys);
			}
			else
			{
				// static_assert(false, "invalid_sys_call");
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

		// For multiple systems, rvalue arguments are moved into storage once
		// and then passed to each system as lvalue references.
		template <typename... t_arg>
		FORCE_INLINE constexpr decltype(auto)
		make_arg_tpl(t_arg&&... arg) noexcept
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

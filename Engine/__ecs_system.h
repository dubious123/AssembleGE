#pragma once
#include "__common.h"

namespace ecs
{
	namespace detail
	{
		struct unsupported
		{
		};

		template <typename t>
		struct extract_interface_template;

		template <template <typename> typename... t_interface, typename t>
		struct extract_interface_template<std::tuple<t_interface<t>...>>
		{
			template <typename t_data>
			using tpl_interfaces = std::tuple<t_interface<t_data>...>;

			template <typename t_data, std::size_t... i>
			constexpr static tpl_interfaces<t_data> get_interfaces_imp(t_data&& data, std::index_sequence<i...>)
			{
				return std::make_tuple((std::tuple_element_t<i, tpl_interfaces<t_data>>(std::forward<decltype(data)>(data)))...);
			}

			template <typename t_data>
			constexpr static tpl_interfaces<t_data> get_interfaces(t_data&& data)
			{
				constexpr auto size = std::tuple_size_v<tpl_interfaces<t_data>>;
				return get_interfaces_imp(std::forward<decltype(data)>(data), std::make_index_sequence<size> {});
			}
		};

		template <typename t_callable, typename t_data>
		using lambda_interface_templates = extract_interface_template<typename meta::function_traits<&t_callable::template operator()<t_data>>::argument_types>;

		// template <typename t_sys, typename... t_data>
		// auto sys_test_run() -> decltype(std::declval<t_sys>().template run<t_data...>(std::declval<t_data>()...), std::true_type {});

		template <typename t_sys, typename... t_data>
		constexpr auto sys_test_run() -> decltype(std::declval<std::remove_cvref_t<t_sys>>().template run<std::remove_cvref_t<t_data>...>(std::declval<t_data>()...), true)
		{
			using t_ret = decltype(std::declval<t_sys>().template run<t_data...>(std::declval<t_data>()...));
			if constexpr (std::is_same_v<t_ret, unsupported>)
			{
				return false;
			}
			else
			{
				return true;
			}
		}

		// template <typename t_sys>
		// auto sys_test_run() -> decltype(std::declval<t_sys>().run(), std::true_type {});

		template <typename t_sys>
		constexpr auto sys_test_run() -> decltype(std::declval<std::remove_cvref_t<t_sys>>().run(), true)
		{
			using t_ret = decltype(std::declval<t_sys>().run());
			if constexpr (std::is_same_v<t_ret, unsupported>)
			{
				return false;
			}
			else
			{
				return true;
			}
		}

		// template <typename, typename...>
		// auto sys_test_run(...) -> std::false_type;

		template <typename, typename...>
		constexpr auto sys_test_run(...)
		{
			return false;
		}

		// template <typename t_callable, typename... t_data>
		// auto callable_test_run() -> decltype(std::declval<t_callable>().template operator()<t_data...>(std::declval<t_data>()...), std::true_type {});

		// template <typename t_callable>
		// auto callable_test_run() -> decltype(std::declval<t_callable>()(), std::true_type {});

		// template <typename, typename...>
		// auto callable_test_run(...) -> std::false_type;

		template <typename t_callable, typename... t_data>
		constexpr auto callable_test_run() -> decltype(std::declval<t_callable>().template operator()<std::remove_cvref_t<t_data>...>(std::declval<t_data>()...), true)
		{
			using t_ret = decltype(std::declval<t_callable>().template operator()<t_data...>(std::declval<t_data>()...));
			if constexpr (std::is_same_v<t_ret, unsupported>)
			{
				return false;
			}
			else
			{
				return true;
			}
		}

		template <typename t_callable>
		constexpr auto callable_test_run() -> decltype(std::declval<t_callable>()(), true)
		{
			using t_ret = decltype(std::declval<t_callable>()());
			if constexpr (std::is_same_v<t_ret, unsupported>)
			{
				return false;
			}
			else
			{
				return true;
			}
		}

		template <typename, typename...>
		constexpr auto callable_test_run(...)
		{
			return false;
		}

		// template <typename t_sys, typename... t_data>
		// constexpr bool sys_can_compile = decltype(sys_test_run<t_sys, t_data...>())::value;

		template <typename t_sys, typename... t_data>
		constexpr bool sys_can_compile = sys_test_run<t_sys, t_data...>();

		// template <typename t_callable, typename... t_data>
		// constexpr bool callable_can_compile = decltype(callable_test_run<t_callable, t_data...>())::value;

		template <typename t_callable, typename... t_data>
		constexpr bool callable_can_compile = callable_test_run<t_callable, t_data...>();

		template <typename t_tpl_from, typename t_tpl_to>
		concept tpl_convertible_from = requires {
			requires std::tuple_size_v<t_tpl_from> == std::tuple_size_v<t_tpl_to>;
			requires[]<std::size_t... i>(std::index_sequence<i...>)
			{
				return true && (... && std::is_convertible_v<std::tuple_element_t<i, t_tpl_from>, std::tuple_element_t<i, t_tpl_to>>);
			}
			(std::make_index_sequence<std::tuple_size_v<t_tpl_from>> {});
		};


		// template <typename t_sys, typename... t_data>
		// concept is_system_templated = requires(t_sys sys, t_data&&... data) {
		//	//&t_sys::template run<t_data>;
		//	typename std::enable_if_t<not std::is_same_v<decltype(sys.template run<t_data...>(std::forward<t_data>(data)...)), unsupported>>;
		//	// sys.template run<t_data...>(std::forward<t_data>(data)...);
		//	//{
		//	//	sys.template run<t_data...>(std::forward<t_data>(data)...)
		//	//}
		//	// &t_sys::template run<t_data...>;
		//	//{
		//	//	t_sys::template run<t_data>(std::forward<t_data>(data))
		//	//	// sys.template run<t_data>(std::forward<t_data>(data))
		//	//};
		// };

		// template <typename t_sys, typename... t_data>
		// concept is_system_templated =
		//	requires {
		//		requires sys_can_compile<t_sys, t_data...>;
		//		requires tpl_convertible_from<std::tuple<t_data...>, typename meta::function_traits<&t_sys::template run<t_data...>>::argument_types>;
		//	};
		template <typename t_sys, typename... t_data>
		concept is_system_templated = requires {
			requires sys_can_compile<t_sys, t_data...>;
		};

		//&& tpl_convertible_from<std::tuple<t_data...>, typename meta::function_traits<&t_sys::template run<t_data...>>::argument_types>;

		// template <typename t_sys, typename... t_data>
		// concept is_system = requires(t_sys sys, t_data... data) {
		//	// typename std::enable_if_t<not std::is_same_v<decltype(sys.run(std::forward<t_data>(data)...)), unsupported>>;
		//	typename std::enable_if_t<std::is_same_v<typename meta::function_traits<&t_sys::run>::argument_types, std::tuple<t_data...>>>;
		// };

		template <typename t_sys, typename... t_data>
		concept is_system = requires {
			requires sys_can_compile<t_sys, t_data...>;
			// requires tpl_convertible_from<std::tuple<t_data...>, typename meta::function_traits<&t_sys::run>::argument_types>;
		};

		// template <typename t_callable, typename... t_data>
		// concept is_callable_templated = requires(t_callable sys, t_data... data) {
		//	typename std::enable_if_t<not std::is_same_v<decltype(sys.template operator()<t_data...>(std::forward<t_data>(data)...)), unsupported>>;
		//	// callable.template operator()<t_data...>(std::forward<t_data>(data)...);
		//	//  std::apply(callable, lambda_interface_templates<t_callable, t_data>::get_interfaces(std::forward<t_data>(data)));
		// };

		template <typename t_callable, typename... t_data>
		concept is_callable_templated = requires {
			requires callable_can_compile<t_callable, t_data...>;
			// requires tpl_convertible_from<std::tuple<t_data...>, typename meta::function_traits<&t_callable::template operator()<t_data...>>::argument_types>;
		};

		// template <typename t_callable>
		// concept is_callable = requires(t_callable callable) {
		//	// std::apply(callable, typename meta::callable_traits<[]() { return (scene_t1*)nullptr; }>::argument_types());
		//	&t_callable::operator();
		//	// std::apply(callable, typename meta::callable_traits<callable>::argument_types { std::forward<t_data>(data) });
		//	//  std::apply(callable, std::tuple<>());
		// };

		// template <typename t_data, typename t_sys>
		// concept is_callable = requires {
		//	typename std::enable_if_t<std::is_same_v<t_data, std::tuple_element_t<0, typename meta::function_traits<&t_sys::operator()>::argument_types>>>;
		// };

		// template <typename t_sys, typename... t_data>
		// concept is_callable = requires(t_sys sys, t_data... data) {
		//	typename std::enable_if_t<not std::is_same_v<decltype(sys(std::forward<t_data>(data)...)), unsupported>>;
		//	// sys(std::forward<t_data>(data)...);
		//	//  typename std::enable_if_t<std::is_same_v<typename meta::function_traits<&t_sys::operator()>::argument_types, std::tuple<t_data...>>>;
		// };

		template <typename t_callable, typename... t_data>
		concept is_callable = requires {
			requires callable_can_compile<t_callable, t_data...>;
			// requires tpl_convertible_from<std::tuple<t_data...>, typename meta::function_traits<&t_callable::operator()>::argument_types>;
		};

		template <typename t_sys, typename... t_data>
		concept has_run_templated = requires {
			typename decltype(&std::remove_cvref_t<t_sys>::template run<t_data...>);
		};

		template <typename t_sys, typename... t_data>
		concept has_run = requires {
			typename decltype(&std::remove_cvref_t<t_sys>::run);
		};

		template <typename t_callable, typename... t_data>
		concept has_operator_templated = requires {
			typename decltype(&std::remove_cvref_t<t_callable>::template operator()<t_data...>);
			// requires tpl_convertible_from<std::tuple<t_data...>, typename meta::function_traits<&t_callable::template operator()<t_data...>>::argument_types>;
		};

		template <typename t_callable, typename... t_data>
		concept has_operator = requires {
			typename decltype(&std::remove_cvref_t<t_callable>::operator());
			// requires tpl_convertible_from<std::tuple<t_data...>, typename meta::function_traits<&t_callable::template operator()<t_data...>>::argument_types>;
		};

		template <typename t_sys, typename t_data>
		decltype(auto) run_system(t_sys& sys, t_data&& data)
		{
			if constexpr (has_run_templated<decltype(sys), decltype(data)>)
			{
				// return sys.template run<t_data>(p_data);
				return sys.run<t_data>(std::forward<decltype(data)>(data));
			}
			else if constexpr (has_run<decltype(sys)>)
			{
				if constexpr (meta::function_traits<&t_sys::run>::arity == 0ull)
				{
					return sys.run();
				}
				else
				{
					return sys.run(std::forward<decltype(data)>(data));
				}
			}
			else if constexpr (has_operator_templated<decltype(sys), decltype(data)>)
			{
				return sys.template operator()<t_data>(std::forward<decltype(data)>(data));
			}
			else if constexpr (has_operator<decltype(sys)>)
			{
				if constexpr (meta::function_traits<&t_sys::operator()>::arity == 0ull)
				{
					return sys();
				}
				else
				{
					return sys(std::forward<decltype(data)>(data));
				}
			}
			else
			{
				return unsupported {};
				// static_assert(false and "System does not provide a run method that can be called.");
			}

			// if constexpr (is_system_templated<decltype(sys), decltype(data)>)
			//{
			//	// return sys.template run<t_data>(p_data);
			//	return sys.run<t_data>(std::forward<decltype(data)>(data));
			// }
			// else if constexpr (is_system<decltype(sys)>)
			//{
			//	if constexpr (meta::function_traits<&t_sys::run>::arity == 0ull)
			//	{
			//		return sys.run();
			//	}
			//	else
			//	{
			//		return sys.run(std::forward<decltype(data)>(data));
			//	}
			// }
			// else if constexpr (is_callable_templated<decltype(sys), decltype(data)>)
			//{
			//	return sys.template operator()<t_data>(std::forward<decltype(data)>(data));
			// }
			// else if constexpr (is_callable<decltype(sys)>)
			//{
			//	if constexpr (meta::function_traits<&t_sys::operator()>::arity == 0ull)
			//	{
			//		return sys();
			//	}
			//	else
			//	{
			//		return sys(std::forward<decltype(data)>(data));
			//	}
			// }
			// else
			//{
			//	return unsupported {};
			//	// static_assert(false and "System does not provide a run method that can be called.");
			// }
		}

		template <typename t_sys>
		decltype(auto) run_system(t_sys& sys)
		{
			if constexpr (is_system<t_sys>)
			{
				return sys.run();
			}
			else if constexpr (is_callable<t_sys>)
			{
				return sys();
			}
			else
			{
				return unsupported {};
				// static_assert(false and "System does not provide a run method that can be called.");
			}
		}

		template <unsigned int n, typename t_sys_loop>
		struct loop_impl_1
		{
			t_sys_loop sys_loop;

			template <typename t_data>
			void run(t_data&& data)
			{
				DEBUG_LOG("---loop start (func)---");
				for (auto _ : std::views::iota(0) | std::views::take(n))
				{
					run_system(sys_loop, std::forward<decltype(data)>(data));
				}
				DEBUG_LOG("---loop end (func)---");
			}
		};

		template <typename t_sys_cond, typename t_sys_loop>
		struct loop_impl_2
		{
			t_sys_cond sys_cond;
			t_sys_loop sys_loop;

			template <typename t_data>
			decltype(auto) run(t_data&& data)
			{
				using sys_cond_ret_type = decltype(run_system(sys_cond, std::forward<decltype(data)>(data)));

				DEBUG_LOG("---loop start (func)---");
				if constexpr (std::is_same_v<sys_cond_ret_type, bool>)
				{
					while (run_system(sys_cond, std::forward<decltype(data)>(data)))
					{
						run_system(sys_loop, std::forward<decltype(data)>(data));
					}
				}
				else if constexpr (std::is_integral_v<sys_cond_ret_type>)
				{
					for (auto _ : std::views::iota(0) | std::views::take(run_system(sys_cond, std::forward<decltype(data)>(data))))
					{
						run_system(sys_loop, std::forward<decltype(data)>(data));
					}
				}
				else
				{
					return unsupported {};
				}
				DEBUG_LOG("---loop end (func)---");
			}
		};

		template <auto sys_cond, auto sys_loop>
		constexpr decltype(auto) get_loop_impl()
		{
			if constexpr (std::is_integral_v<decltype(sys_cond)>)
			{
				return loop_impl_1<sys_cond, decltype(sys_loop)>();
			}
			else
			{
				return loop_impl_2<decltype(sys_cond), decltype(sys_loop)>();
			}
		}

		// i'm using custom tuple because std::tuple is not a structural type and cannot be used as a NTTP
		template <std::size_t i, typename t>
		struct _tpl_leaf
		{
			t val;
		};

		template <typename seq, typename... t>
		struct _tpl_helper;

		template <std::size_t... i, typename... t>
		struct _tpl_helper<std::index_sequence<i...>, t...> : public _tpl_leaf<i, t>...
		{
		};

		template <typename... t>
		struct _tpl : public _tpl_helper<std::index_sequence_for<t...>, t...>
		{
		};

		template <std::size_t i, typename... t>
		constexpr auto& _get(_tpl<t...>& tpl)
		{
			return static_cast<_tpl_leaf<i, typename std::tuple_element<i, std::tuple<t...>>::type>&>(tpl).val;
		}

		template <std::size_t i, typename... t>
		constexpr const auto& _get(const _tpl<t...>& tpl)
		{
			return static_cast<const _tpl_leaf<i, typename std::tuple_element<i, std::tuple<t...>>::type>&>(tpl).val;
		}

		template <std::size_t i, typename... t>
		constexpr auto&& _get(_tpl<t...>&& tpl)
		{
			return static_cast<_tpl_leaf<i, typename std::tuple_element<i, std::tuple<t...>>::type>&&>(tpl).val;
		}

		template <typename t>
		struct _tpl_size;

		template <typename... t>
		struct _tpl_size<_tpl<t...>> : std::integral_constant<std::size_t, sizeof...(t)>
		{
		};

		template <typename t>
		const inline constinit std::size_t _tpl_size_v = _tpl_size<t>::value;

		template <typename f, typename tpl_t, std::size_t... i>
		constexpr decltype(auto) _apply_impl(f&& func, tpl_t&& tpl, std::index_sequence<i...>)
		{
			return std::forward<f>(func)(_get<i>(std::forward<tpl_t>(tpl))...);
		}

		template <typename f, typename tpl_t>
		constexpr decltype(auto) _apply(f&& func, tpl_t&& tpl)
		{
			return _apply_impl(
				std::forward<f>(func),
				std::forward<tpl_t>(tpl),
				std::make_index_sequence<_tpl_size_v<std::decay_t<tpl_t>>> {});
		}
	}	 // namespace detail

	template <auto... sys>
	struct seq
	{
		detail::_tpl<decltype(sys)...> systems;

		constexpr seq() /*: systems(sys...)*/ { }

		template <typename t_data>
		void run(t_data&& data)
		{
			DEBUG_LOG("---new seq start (func)---");

			//([p_data]() {
			//	run_system(sys, p_data);
			//}(),
			// ...);
			detail::_apply(
				[&data](auto&... _sys) {
					((detail::run_system(std::forward<decltype(_sys)>(_sys), std::forward<decltype(data)>(data))), ...);
				},
				systems);
			DEBUG_LOG("---new seq end (func)---");
		}
	};

	template <auto... sys>
	struct par
	{
		detail::_tpl<decltype(sys)...> systems;

		constexpr par() { }

		// constexpr _par() :

		template <typename tpl_sys, typename t_data, std::size_t... i>
		void parallel_apply(tpl_sys& systems, t_data&& data, std::index_sequence<i...>)
		{
			auto futures = std::array<std::future<void>, sizeof...(i)> {
				(std::async(std::launch::async, [&data, &systems]() { detail::run_system(detail::_get<i>(systems), data); }))...
			};

			std::ranges::for_each(futures, [](auto&& fut) { fut.get(); });
		}

		template <typename t_data>
		void run(t_data&& data)
		{
			DEBUG_LOG("---new par start (func)---");
			parallel_apply(systems, data, std::make_index_sequence<sizeof...(sys)>());
			DEBUG_LOG("---new par end (func)---");
		}
	};

	template <auto sys_cond, auto sys_true, auto sys_false>
	struct cond
	{
		decltype(sys_cond)	_sys_cond;
		decltype(sys_true)	_sys_true;
		decltype(sys_false) _sys_false;

		constexpr cond() : _sys_cond(sys_cond), _sys_true(sys_true), _sys_false(sys_false) { }

		template <typename t_data>
		decltype(auto) run(t_data&& data)
		{
			DEBUG_LOG("---new cond start (func)---");
			if (detail::run_system(_sys_cond, std::forward<decltype(data)>(data)))
			{
				return detail::run_system(_sys_true, std::forward<decltype(data)>(data));
			}
			else
			{
				return detail::run_system(_sys_false, std::forward<decltype(data)>(data));
			}
			DEBUG_LOG("---new cond end (func)---");
		}
	};

	// because of msvc bug, cannot use NTTP partial initialization
	template <auto sys_cond, auto sys_loop>
	struct loop
	{
		decltype(detail::get_loop_impl<sys_cond, sys_loop>()) impl;

		constexpr loop() { };

		template <typename t_data>
		decltype(auto) run(t_data&& data)
		{
			return detail::run_system(impl, std::forward<decltype(data)>(data));
		}
	};

#define TUPLE_CASE(i)                                                                            \
	case i:                                                                                      \
	{                                                                                            \
		if constexpr (i < n)                                                                     \
		{                                                                                        \
			detail::run_system(detail::_get<i>(_sys_cases), std::forward<decltype(data)>(data)); \
		}                                                                                        \
		break;                                                                                   \
	}


#define MAX_TUPLE_SIZE 10

#define TUPLE_CASES \
	TUPLE_CASE(0)   \
	TUPLE_CASE(1)   \
	TUPLE_CASE(2)   \
	TUPLE_CASE(3)   \
	TUPLE_CASE(4)   \
	TUPLE_CASE(5)   \
	TUPLE_CASE(6)   \
	TUPLE_CASE(7)   \
	TUPLE_CASE(8)   \
	TUPLE_CASE(9)

	template <auto i, auto sys>
	struct _case
	{
		static const constinit decltype(i) _idx = i;
		decltype(sys)					   _sys = sys;
	};

	template <auto sys_select, auto... sys_cases>
	struct _switch
	{
		static_assert(sizeof...(sys_cases) <= MAX_TUPLE_SIZE, "Tuple size exceeds maximum supported size");

		decltype(sys_select)				 _sys_select;
		detail::_tpl<decltype(sys_cases)...> _sys_cases;

		constexpr _switch() { };

		template <typename t_data>
		decltype(auto) run(t_data&& data)
		{
			constexpr auto n = sizeof...(sys_cases);

			auto idx = detail::run_system(_sys_select, std::forward<decltype(data)>(data));
			switch (idx)
			{
				TUPLE_CASES
			default:
				break;
			}
		}
	};

	template <auto sys, auto sys_producer>
	struct bind

	{
		decltype(sys)		   _sys;
		decltype(sys_producer) _sys_producer;

		constexpr bind() : _sys(sys), _sys_producer(sys_producer)
		{
		}

		template <typename t_data>
		decltype(auto) run(t_data&& data)
		{
			static_assert(std::is_same_v<decltype(detail::run_system(_sys_producer, std::forward<t_data>(data))), void> == false, "system or lmabda must return something to bind");
			return detail::run_system(_sys, detail::run_system(_sys_producer, std::forward<t_data>(data)));
		}
	};
}	 // namespace ecs

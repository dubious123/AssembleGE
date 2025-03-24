#pragma once
#include "__common.h"

namespace ecs
{
	namespace detail
	{
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
					static_assert(false, "invalid condition system");
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

		template <typename t_sys, typename t_data>
		concept is_system_templated = requires(t_sys sys, t_data&& data) {
			{
				sys.template run<t_data>(std::forward<t_data>(data))
			};
		};

		template <typename t_sys>
		concept is_system = requires(t_sys sys) {
			{
				&t_sys::run
			};
		};

		template <typename t_callable, typename t_data>
		concept is_callable_templated = requires(t_callable callable, t_data&& data) {
			std::apply(callable, lambda_interface_templates<t_callable, t_data>::get_interfaces(std::forward<t_data>(data)));
		};

		template <typename t_callable>
		concept is_callable = requires(t_callable callable) {
			// std::apply(callable, typename meta::callable_traits<[]() { return (scene_t1*)nullptr; }>::argument_types());
			&t_callable::operator();
			// std::apply(callable, typename meta::callable_traits<callable>::argument_types { std::forward<t_data>(data) });
			//  std::apply(callable, std::tuple<>());
		};

		template <typename t_sys, typename t_data>
		decltype(auto) run_system(t_sys& sys, t_data&& data)
		{
			if constexpr (is_system_templated<t_sys, t_data>)
			{
				// return sys.template run<t_data>(p_data);
				return sys.run<t_data>(std::forward<decltype(data)>(data));
			}
			else if constexpr (is_system<t_sys>)
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
			else if constexpr (is_callable_templated<t_sys, t_data>)
			{
				return sys.template operator()<t_data>(std::forward<t_data>(data));
			}
			else if constexpr (is_callable<t_sys>)
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
				static_assert(false and "System does not provide a run method that can be called.");
			}
		}
	}	 // namespace detail

	template <auto... sys>
	struct seq
	{
		std::tuple<decltype(sys)...> systems;

		constexpr seq() : systems(sys...) { }

		template <typename t_data>
		void run(t_data&& data)
		{
			DEBUG_LOG("---new seq start (func)---");

			//([p_data]() {
			//	run_system(sys, p_data);
			//}(),
			// ...);
			std::apply(
				[&data](auto&... _sys) {
					((run_system(std::forward<decltype(_sys)>(_sys), std::forward<std::remove_const_t<t_data>>(data))), ...);
				},
				systems);
			DEBUG_LOG("---new seq end (func)---");
		}
	};

	template <auto... sys>
	struct par
	{
		std::tuple<decltype(sys)...> systems;

		constexpr par() : systems(sys...) { }

		// constexpr _par() :

		template <typename tpl_sys, typename t_data, std::size_t... i>
		void parallel_apply(tpl_sys& systems, t_data&& data, std::index_sequence<i...>)
		{
			auto futures = std::array<std::future<void>, sizeof...(i)> {
				(std::async(std::launch::async, [data, &systems]() { run_system(std::get<i>(systems), data); }))...
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
			if (run_system(_sys_cond, std::forward<decltype(data)>(data)))
			{
				return run_system(_sys_true, std::forward<decltype(data)>(data));
			}
			else
			{
				return run_system(_sys_false, std::forward<decltype(data)>(data));
			}
			DEBUG_LOG("---new cond end (func)---");
		}
	};

	// because of msvc bug, cannot use NTTP partial initialization
	template <auto sys_cond, auto sys_loop>
	struct loop
	{
		decltype(detail::get_loop_impl<sys_cond, sys_loop>()) impl;

		constexpr loop() {};

		template <typename t_data>
		decltype(auto) run(t_data&& data)
		{
			return run_system(impl, std::forward<decltype(data)>(data));
		}
	};

	template <auto sys, auto sys_producer>
	struct bind
	{
		decltype(sys)		   _sys;
		decltype(sys_producer) _sys_producer;

		constexpr bind() : _sys(sys), _sys_producer(sys_producer) { }

		template <typename t_data>
		decltype(auto) run(t_data&& data)
		{
			static_assert(std::is_same_v<decltype(run_system(_sys_producer, std::forward<t_data>(data))), void> == false, "system or lmabda must return something to bind");
			return run_system(_sys, run_system(_sys_producer, std::forward<t_data>(data)));
		}
	};
}	 // namespace ecs

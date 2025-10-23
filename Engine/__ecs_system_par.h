#pragma once
#ifndef INCLUDED_FROM_ECS_SYSTEM_HEADER
	#error "Do not include this file directly. Include <__ecs_system.h> instead."
#endif

namespace ecs::system::detail
{
	struct __parallel_executor_base
	{
	};
}	 // namespace ecs::system::detail

namespace ecs::system
{

	template <typename... t_sys>
	struct par
	{
		static_assert(sizeof...(t_sys) > 1, "[par] requires at least 2 systems to be meaningful");

		using t_this			  = par<t_sys...>;
		using t_not_empty_idx_seq = meta::arr_to_seq_t<not_empty_sys_idx_arr<t_sys...>()>;
		using t_sys_not_empty	  = meta::filtered_variadic_t<meta::is_not_empty, t_sys...>;

		no_unique_addr t_sys_not_empty systems;

		constexpr par(t_sys&&... sys) : systems(meta::make_filtered_tuple<meta::is_not_empty, t_sys...>(FWD(sys)...)){};

		constexpr par() requires(std::is_empty_v<t_sys> and ...)
		= default;

		template <typename t_arg>
		static constexpr bool has_par_exec_member = requires(t_arg arg) {
			requires std::is_base_of_v<detail::__parallel_executor_base, std::decay_t<decltype(arg.__parallel_executor)>>;
		};

		template <typename t_arg>
		struct has_par_exec : std::bool_constant<has_par_exec_member<t_arg>>
		{
		};

		template <std::size_t i, typename... t_arg>
		FORCE_INLINE constexpr decltype(auto)
		run_impl(t_arg&&... arg)
		{
			using t_sys_now = meta::variadic_at_t<i, t_sys...>;
			if constexpr (std::is_empty_v<t_sys_now>)
			{
				return run_sys(t_sys_now{}, FWD(arg)...);
			}
			else
			{
				return run_sys(std::get<meta::index_sequence_at_v<i, t_not_empty_idx_seq>>(systems), FWD(arg)...);
			}
		}

		template <std::size_t i, typename t_tpl>
		FORCE_INLINE constexpr decltype(auto)
		run_impl_tpl(t_tpl& tpl)
		{
			using t_ret = decltype(std::apply([this](auto&&... arg) { return run_impl<i>(FWD(arg)...); }, tpl));
			if constexpr (std::is_void_v<t_ret> or std::is_same_v<t_ret, detail::sys_result<>>)
			{
				std::apply([this](auto&&... arg) { return run_impl<i>(FWD(arg)...); }, tpl);
				return detail::__result_void{};
				// return std::tuple<>{};
			}
			else
			{
				return std::apply([this](auto&&... arg) -> decltype(auto) { return run_impl<i>(FWD(arg)...); }, tpl);
				// return std::tuple{ std::apply([this](auto&&... arg) -> decltype(auto) { return run_impl<i>(FWD(arg)...); }, tpl) };
			}

			// return std::apply([this](auto&&... arg) { return run_impl<i>(FWD(arg)...); }, tpl);
		}

		// template <std::size_t i, typename t_tpl>
		// FORCE_INLINE constexpr decltype(auto)
		// run_as_tpl(t_tpl& tpl)
		//{
		//	using t_ret = decltype(std::apply([this](auto&&... arg) { return run_impl<i>(FWD(arg)...); }, tpl));
		//	if constexpr (std::is_void_v<t_ret> or std::is_same_v<t_ret, detail::sys_result<>>)
		//	{
		//		std::apply([this](auto&&... arg) { return run_impl<i>(FWD(arg)...); }, tpl);
		//		return std::tuple<>{};
		//	}
		//	else
		//	{
		//		return std::tuple{ std::apply([this](auto&&... arg) -> decltype(auto) { return run_impl<i>(FWD(arg)...); }, tpl) };
		//	}
		// }

		template <typename... t_arg>
		FORCE_INLINE constexpr decltype(auto)
		operator()(t_arg&&... arg)
		{
			[this]<auto... i>(std::index_sequence<i...>) {
				static_assert(((not std::is_same_v<decltype(run_impl<i>(FWD(arg)...)), invalid_sys_call>)&&...),
							  "[par] invalid_sys_call - check that system i is callable with given arguments.");
			}(std::index_sequence_for<t_sys...>{});


			if constexpr (meta::index_sequence_size_v<meta::filtered_index_sequence_t<has_par_exec, t_arg...>> == 0)
			{
				// default
				return [this, args = make_arg_tpl(FWD(arg)...)]<auto... i>(std::index_sequence<i...>) {
					return [](auto&&... async_op) {
						return sys_result{
							meta::make_filtered_tuple_from_tuple<detail::is_result_not_void>(
								std::tuple<decltype(async_op.get())...>{ async_op.get()... })
						};

						// return sys_result{ tuple_cat_all(std::tuple{ async_op.get()... }) };
					}(std::async(std::launch::async, [this, &args]() { return run_impl_tpl<i>(args); })...);
				}(std::index_sequence_for<t_sys...>{});
			}
			else
			{
				constexpr auto par_exec_idx = meta::index_sequence_front_v<meta::filtered_index_sequence_t<has_par_exec, t_arg...>>;

				return sys_result{
					meta::make_filtered_tuple_from_tuple<detail::is_result_not_void>(
						[this, args = make_arg_tpl(FWD(arg)...)]<auto... i>(std::index_sequence<i...>) {
							return std::get<par_exec_idx>(args).__parallel_executor.run_par(
								([this, &args]() { return run_impl_tpl<i>(args); })...);
						}(std::index_sequence_for<t_sys...>{}))


					// meta::make_filtered_tuple_from_tuple<detail::is_not_result_void>(
					//	par_exe.run_par(


					//		([this, &args]() {
					//			return []<auto... i>(std::index_sequence<i...>) {
					//				return run_impl_tpl<i>(args);
					//			}(std::index_sequence_for<t_sys...>{});
					//		})...)


					//)
				};

				// return [this, args = make_arg_tpl(FWD(arg)...)]<auto... i>(std::index_sequence<i...>) {
				//	auto& par_exe = std::get<par_exec_idx>(args).__parallel_executor;

				//	return sys_result{ par_exe.run_par(([this, &args] { return run_impl_tpl<i>(args); })...) };
				//}(std::index_sequence_for<t_sys...>{});
			}
		}
	};
}	 // namespace ecs::system
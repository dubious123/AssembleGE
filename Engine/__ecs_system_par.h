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

		constexpr par(t_sys&&... sys) : systems(meta::make_filtered_tuple<meta::is_not_empty, t_sys...>(FWD(sys)...)) { };

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
		run_impl(t_arg&&... arg) noexcept
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
		run_impl_tpl(t_tpl&& tpl) noexcept
		{
			return meta::tuple_unpack([this](auto&&... arg) { return run_impl<i>(FWD(arg)...); }, FWD(tpl));
		}

		template <typename t_tpl, std::size_t... i>
		FORCE_INLINE constexpr decltype(auto)
		run_helper(t_tpl&& arg_tpl, std::index_sequence<i...>) noexcept
		{
			using t_sys_res_not_void =
				meta::filtered_index_sequence_t<
					meta::is_not_void,
					decltype(run_impl_tpl<i>(arg_tpl))...>;

			using t_sys_res_void =
				meta::filtered_index_sequence_t<
					std::is_void,
					decltype(run_impl_tpl<i>(arg_tpl))...>;

			if constexpr (meta::index_sequence_empty_v<t_sys_res_not_void>)
			{
				[](auto&&... async_op) noexcept -> decltype(auto) {
					(async_op.get(), ...);
				}(std::async(std::launch::async, [this, &arg_tpl]() { return run_impl_tpl<i>(arg_tpl); })...);
			}
			else
			{
				return [](auto&& async_op_tpl) noexcept -> decltype(auto) {
					[]<auto... sys_idx>(std::index_sequence<sys_idx...>, auto&& async_op_tpl) noexcept {
						((std::get<sys_idx>(async_op_tpl).get()), ...);
					}(t_sys_res_void{}, FWD(async_op_tpl));

					return []<auto... sys_idx>(std::index_sequence<sys_idx...>, auto&& async_op_tpl) noexcept INLINE_LAMBDA {
						return std::tuple{ std::get<sys_idx>(async_op_tpl).get()... };
					}(t_sys_res_not_void{}, FWD(async_op_tpl));
				}(std::tuple{ std::async(std::launch::async, [this, &arg_tpl]() { return run_impl_tpl<i>(arg_tpl); })... });
			}
		}

		template <typename... t_arg>
		FORCE_INLINE constexpr decltype(auto)
		operator()(t_arg&&... arg) noexcept
		{
			[this]<auto... i>(std::index_sequence<i...>) {
				static_assert(((not std::is_same_v<decltype(run_impl<i>(FWD(arg)...)), invalid_sys_call>) && ...),
							  "[par] invalid_sys_call - check that system i is callable with given arguments.");
			}(std::index_sequence_for<t_sys...>{});

			if constexpr (meta::index_sequence_size_v<meta::filtered_index_sequence_t<has_par_exec, t_arg...>> == 0)
			{
				return run_helper(make_arg_tpl(FWD(arg)...), std::index_sequence_for<t_sys...>{});
			}
			else
			{
				constexpr auto par_exec_idx = meta::index_sequence_front_v<meta::filtered_index_sequence_t<has_par_exec, t_arg...>>;

				return [this, arg_tpl = make_arg_tpl(FWD(arg)...)]<auto... i>(std::index_sequence<i...>) mutable noexcept INLINE_LAMBDA {
					return std::get<par_exec_idx>(arg_tpl).__parallel_executor.run_par(([this, &arg_tpl]() { return run_impl_tpl<i>(arg_tpl); })...);
				}(std::index_sequence_for<t_sys...>{});
			}
		}
	};
}	 // namespace ecs::system
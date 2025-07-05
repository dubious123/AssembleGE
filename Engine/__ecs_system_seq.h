#pragma once
#ifndef INCLUDED_FROM_ECS_SYSTEM_HEADER
	#error "Do not include this file directly. Include <__ecs_system.h> instead."
#endif

namespace ecs::system
{
	using namespace detail;

	template <typename... t_sys>
	struct seq
	{
		using t_self			  = seq<t_sys...>;
		using t_not_empty_idx_seq = meta::arr_to_seq_t<not_empty_sys_idx_arr<t_sys...>()>;
		using t_sys_not_empty	  = meta::filtered_tuple_t<meta::is_not_empty, t_sys...>;

		no_unique_addr t_sys_not_empty systems;

		constexpr seq(t_sys&&... sys) : systems(meta::make_filtered_tuple<meta::is_not_empty, t_sys...>(FWD(sys)...)) { };

		constexpr seq() requires(std::is_empty_v<t_sys> and ...)
		= default;

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

		template <std::size_t i, typename... t_arg>
		FORCE_INLINE constexpr decltype(auto)
		run_as_tpl(t_arg&&... arg)
		{
			using t_ret = decltype(run_impl<i>(FWD(arg)...));
			if constexpr (std::is_void_v<t_ret>)
			{
				run_impl<i>(FWD(arg)...);
				return std::tuple<>{};
			}
			else
			{
				return std::make_tuple(run_impl<i>(FWD(arg)...));
			}
		}

		template <typename... t_arg>
		FORCE_INLINE constexpr decltype(auto)
		operator()(t_arg&&... arg)
		{
			[this]<auto... i>(std::index_sequence<i...>) {
				static_assert(((not std::is_same_v<decltype(run_impl<i>(FWD(arg)...)), invalid_sys_call>) && ...),
							  "[seq] run_impl<i>(...) returned invalid_sys_call - check that system i is callable with given arguments.");
			}(std::index_sequence_for<t_sys...>{});

			if constexpr (sizeof...(t_sys) == 1)
			{
				return unwrap_tpl(
					std::apply(
						[this](auto&&... l_ref_arg) {
							return tuple_cat_all(std::make_tuple(run_as_tpl<0>(FWD(l_ref_arg)...)));
						},
						make_arg_tpl(FWD(arg)...)));
			}

			return unwrap_tpl([this, args = make_arg_tpl(FWD(arg)...)]<auto... i>(std::index_sequence<i...>) {
				return std::apply(
					[this](auto&&... l_ref_arg) {
						return tuple_cat_all(std::tuple{ run_as_tpl<i>(FWD(l_ref_arg)...)... });
					},
					args);
			}(std::index_sequence_for<t_sys...>{}));
		}
	};
}	 // namespace ecs::system

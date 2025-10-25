#pragma once
#ifndef INCLUDED_FROM_ECS_SYSTEM_HEADER
	#error "Do not include this file directly. Include <__ecs_system.h> instead."
#endif

namespace ecs::system
{
	using namespace detail;

	namespace detail
	{
		template <typename... t_sys>
		consteval bool
		validate_seq()
		{
			constexpr auto valid = sizeof...(t_sys) > 0;
			static_assert(valid, "seq: requires at least one system type");

			return valid;
		}
	}	 // namespace detail

	template <typename... t_sys>
	struct seq
	{
		using t_self			  = seq<t_sys...>;
		using t_not_empty_idx_seq = meta::arr_to_seq_t<not_empty_sys_idx_arr<t_sys...>()>;
		using t_sys_not_empty	  = meta::filtered_variadic_t<meta::is_not_empty, t_sys...>;

		no_unique_addr t_sys_not_empty systems;

		static_assert(validate_seq<t_sys...>(), "seq: invalid seq");

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

		template <std::size_t i, typename t_tpl>
		FORCE_INLINE constexpr decltype(auto)
		run_impl_tpl(t_tpl& tpl)
		{
			return std::apply([this](auto&&... arg) -> decltype(auto) { return run_impl<i>(FWD(arg)...); }, tpl);

			// using t_ret = decltype(std::apply([this](auto&&... arg) { return run_impl<i>(FWD(arg)...); }, tpl));
			// if constexpr (std::is_void_v<t_ret> or std::is_same_v<t_ret, detail::sys_result<>>)
			//{
			//	std::apply([this](auto&&... arg) { return run_impl<i>(FWD(arg)...); }, tpl);
			//	return detail::__result_void{};
			//	// return std::tuple<>{};
			// }
			// else
			//{
			//	return std::apply([this](auto&&... arg) -> decltype(auto) { return run_impl<i>(FWD(arg)...); }, tpl);
			//	// return std::tuple{ std::apply([this](auto&&... arg) -> decltype(auto) { return run_impl<i>(FWD(arg)...); }, tpl) };
			// }
		}

		template <typename t_arg_tpl, std::size_t... sys_idx>
		FORCE_INLINE constexpr decltype(auto)
		run_impl_seq(t_arg_tpl& arg_tpl, std::index_sequence<sys_idx...>)
		{
			return (run_impl_tpl<sys_idx>(arg_tpl), ...);
		}

		template <typename t_arg_tpl, std::size_t... sys_idx>
		FORCE_INLINE constexpr decltype(auto)
		run_helper(t_arg_tpl& arg_tpl, std::index_sequence<sys_idx...>)
		{
			using t_sys_res_not_void =
				meta::filtered_index_sequence_t<
					detail::is_result_not_void,
					decltype(run_impl_tpl<sys_idx>(arg_tpl))...>;


			static_assert(meta::index_sequence_empty_v<t_sys_res_not_void> is_false);

			if constexpr (constexpr auto has_trailing_voids =
							  (detail::is_result_not_void<
								  decltype(run_impl_tpl<variadic_auto_back_v<sys_idx...>>(arg_tpl))>::value is_false))
			{
				constexpr auto idx_head			= meta::variadic_auto_front_v<sys_idx...>;
				constexpr auto idx_res_not_void = meta::index_sequence_front_v<t_sys_res_not_void>;
				constexpr auto idx_tail			= meta::variadic_auto_back_v<sys_idx...>;

				using t_res = decltype(run_impl_tpl<idx_res_not_void>(arg_tpl));

				decltype(auto) res = run_impl_seq(arg_tpl, meta::offset_sequence<idx_head, idx_res_not_void - idx_head + 1>{});
				run_impl_seq(arg_tpl, meta::offset_sequence<idx_res_not_void, idx_tail - idx_res_not_void>{});

				if constexpr (std::is_rvalue_reference_v<t_res>)
				{
					return std::move(res);
				}
				else
				{
					return res;
				}
			}
			else
			{
				return run_impl_seq(arg_tpl, std::index_sequence<sys_idx...>{});
			}
		}

		template <typename t_arg_tpl, typename... t_idx_seq>
		FORCE_INLINE constexpr decltype(auto)
		run_helper(t_arg_tpl& arg_tpl, meta::type_pack<t_idx_seq...>)
		{
			return sys_result{ std::tuple{ run_helper(arg_tpl, t_idx_seq{})... } };
		}

		template <typename... t_arg>
		FORCE_INLINE constexpr decltype(auto)
		operator()(t_arg&&... arg)
		{
			[this]<auto... i>(std::index_sequence<i...>) {
				static_assert(((not std::is_same_v<decltype(run_impl<i>(FWD(arg)...)), invalid_sys_call>) && ...),
							  "[seq] run_impl<i>(...) returned invalid_sys_call - check that system i is callable with given arguments.");
			}(std::index_sequence_for<t_sys...>{});

			return [this, arg_tpl = make_arg_tpl(FWD(arg)...)]<auto... i>(std::index_sequence<i...>) mutable {
				using t_sys_res_not_void =
					meta::filtered_index_sequence_t<
						detail::is_result_not_void,
						decltype(run_impl_tpl<i>(arg_tpl))...>;
				if constexpr (meta::index_sequence_empty_v<t_sys_res_not_void>)
				{
					// 1. void , void, ..., void
					// 2. empty

					return (run_impl_tpl<i>(arg_tpl), ..., sys_result{});
				}
				else
				{
					return run_helper(
						arg_tpl,
						detail::index_ranges_seq_t<
							sizeof...(t_sys) - 1,
							t_sys_res_not_void>{});
				}

				// return sys_result{
				//	meta::make_filtered_tuple_from_tuple<detail::is_result_not_void>(
				//		std::tuple<decltype(run_impl_tpl<i>(arg_tpl))...>{ run_impl_tpl<i>(arg_tpl)... })
				// };
			}(std::index_sequence_for<t_sys...>{});
		}
	};
}	 // namespace ecs::system

#pragma once
#ifndef INCLUDED_FROM_ECS_SYSTEM_HEADER
	#error "Do not include this file directly. Include <__ecs_system.h> instead."
#endif

namespace ecs::system::ctx
{
	using namespace ecs::system::detail;

	template <typename t>
	struct is_pipe : std::false_type
	{
	};

	template <typename t1, typename t2>
	struct is_pipe<ecs::system::pipe<t1, t2>> : std::true_type
	{
	};

	template <typename t>
	constexpr bool
	is_constexpr_default_constructible()
	{
		return []() consteval {
			t{};
			return true;
		}();
	}

	template <typename t_exec, typename t_sys, typename... t_arg>
	FORCE_INLINE constexpr decltype(auto)
	__run_sys(t_exec&& exec, t_sys&& sys, t_arg&&... arg) noexcept
	{
		return FWD(sys)(FWD(exec), FWD(arg)...);
	}

	template <typename t>
	inline constexpr auto is_compile_time_constructible_v = is_constexpr_default_constructible<t>();

	struct executor_basic
	{
		template <typename t_sys, typename... t_arg>
		FORCE_INLINE constexpr decltype(auto)
		operator()(t_sys&& sys, t_arg&&... arg) noexcept
		{
			return detail::run_sys(FWD(sys), FWD(arg)...);
		}
	};

	template <typename t_sys>
	struct __sys_wrapper
	{
		no_unique_addr t_sys sys;

		constexpr FORCE_INLINE
		__sys_wrapper(auto&& sys) noexcept : sys(FWD(sys)) { };

		constexpr __sys_wrapper() noexcept requires(is_compile_time_constructible_v<t_sys> or std::is_empty_v<t_sys>)
		= default;

		template <typename t_exec, typename... t_arg>
		FORCE_INLINE constexpr decltype(auto)
		operator()(t_exec&& exec, t_arg&&... arg) noexcept
		{
			if constexpr (is_compile_time_constructible_v<t_sys> and std::is_empty_v<t_sys>)
			{
				return exec(t_sys{}, FWD(arg)...);
			}
			else
			{
				return exec(FWD(sys), FWD(arg)...);
			}
		}
	};

	template <typename... t_sys>
	__sys_wrapper(t_sys&&...) -> __sys_wrapper<meta::value_or_ref_t<t_sys&&>...>;

	template <typename t_executor, typename... t_sys>
	struct __systems
	{
		using t_not_empty_idx_seq = meta::arr_to_seq_t<not_empty_sys_idx_arr<t_sys...>()>;
		using t_sys_not_empty	  = meta::filtered_variadic_t<meta::is_not_empty, t_sys...>;

		no_unique_addr t_executor	   exec;
		no_unique_addr t_sys_not_empty systems;

		constexpr __systems(t_executor&& exec, t_sys&&... sys) noexcept
			: exec(FWD(exec)),
			  systems(meta::make_filtered_tuple<meta::is_not_empty, t_sys...>(FWD(sys)...)) { };

		constexpr __systems() noexcept requires(std::is_empty_v<t_executor> and (std::is_empty_v<t_sys> and ...))
		= default;

		template <std::size_t i, typename... t_arg>
		FORCE_INLINE constexpr decltype(auto)
		__run_impl(t_arg&&... arg) noexcept
		{
			using t_sys_now = meta::variadic_at_t<i, t_sys...>;
			if constexpr (std::is_empty_v<t_sys_now>)
			{
				return __run_sys(exec, t_sys_now{}, FWD(arg)...);
			}
			else
			{
				return __run_sys(exec, std::get<meta::index_sequence_at_v<i, t_not_empty_idx_seq>>(systems), FWD(arg)...);
			}
		}

		template <std::size_t i, typename t_tpl>
		FORCE_INLINE constexpr decltype(auto)
		__run_impl_tpl(t_tpl&& tpl) noexcept
		{
			return meta::tuple_unpack([this](auto&&... arg) noexcept -> decltype(auto) { return __run_impl<i>(FWD(arg)...); }, FWD(tpl));
		}

		template <typename t_arg_tpl, std::size_t... sys_idx>
		FORCE_INLINE constexpr decltype(auto)
		__run_impl_seq(t_arg_tpl&& arg_tpl, std::index_sequence<sys_idx...>) noexcept
		{
			return (__run_impl_tpl<sys_idx>(FWD(arg_tpl)), ...);
		}

		template <typename t_arg_tpl, std::size_t... sys_idx>
		FORCE_INLINE constexpr decltype(auto)
		__run_helper_3(t_arg_tpl&& arg_tpl, std::index_sequence<sys_idx...>) noexcept
		{
			using t_sys_res_not_void =
				meta::filtered_index_sequence_t<
					meta::is_not_void,
					decltype(__run_impl_tpl<sys_idx>(arg_tpl))...>;

			static_assert(meta::index_sequence_empty_v<t_sys_res_not_void> is_false);

			if constexpr (constexpr auto has_trailing_voids =
							  (meta::is_not_void_v<
								  decltype(__run_impl_tpl<variadic_auto_back_v<sys_idx...>>(arg_tpl))>
								   is_false))
			{
				constexpr auto idx_head			= meta::variadic_auto_front_v<sys_idx...>;
				constexpr auto idx_res_not_void = meta::variadic_auto_at_v<meta::index_sequence_front_v<t_sys_res_not_void>, sys_idx...>;
				constexpr auto idx_tail			= meta::variadic_auto_back_v<sys_idx...>;

				auto after_return = detail::scope_guard{ [this, &arg_tpl]() noexcept { __run_impl_seq(arg_tpl, meta::offset_sequence<idx_res_not_void + 1, idx_tail - idx_res_not_void>{}); } };
				return __run_impl_seq(arg_tpl, meta::offset_sequence<idx_head, idx_res_not_void - idx_head + 1>{});
			}
			else
			{
				return __run_impl_seq(arg_tpl, std::index_sequence<sys_idx...>{});
			}
		}

		template <typename t_arg_tpl, typename... t_idx_seq>
		FORCE_INLINE constexpr decltype(auto)
		__run_helper_2(t_arg_tpl&& arg_tpl, meta::type_pack<t_idx_seq...>) noexcept
		{
			return std::tuple{ __run_helper_3(arg_tpl, t_idx_seq{})... };
		}

		template <typename t_arg_tpl, std::size_t... i>
		FORCE_INLINE constexpr decltype(auto)
		__run_helper_1(t_arg_tpl&& arg_tpl, std::index_sequence<i...>) noexcept
		{
			using t_sys_res_not_void =
				meta::filtered_index_sequence_t<
					meta::is_not_void,
					decltype(__run_impl_tpl<i>(arg_tpl))...>;
			if constexpr (meta::index_sequence_empty_v<t_sys_res_not_void>)
			{
				return (__run_impl_tpl<i>(arg_tpl), ...);
			}
			else
			{
				return __run_helper_2(
					FWD(arg_tpl),
					detail::index_ranges_seq_t<
						sizeof...(t_sys) - 1,
						t_sys_res_not_void>{});
			}
		}

		template <std::size_t i>
		FORCE_INLINE constexpr decltype(auto)
		get() noexcept
		{
			using t_sys_now = meta::variadic_at_t<i, t_sys...>;
			if constexpr (std::is_empty_v<t_sys_now>)
			{
				return t_sys_now{};
			}
			else
			{
				return std::get<meta::index_sequence_at_v<i, t_not_empty_idx_seq>>(systems);
			}
		}

		template <typename... t_arg>
		FORCE_INLINE constexpr decltype(auto)
		operator()(t_arg&&... arg) noexcept
		{
			return __run_helper_1(make_arg_tpl(FWD(arg)...), std::index_sequence_for<t_sys...>{});
		}
	};

	template <typename... t_sys>
	__systems(t_sys&&...) -> __systems<meta::value_or_ref_t<t_sys&&>...>;

	template <typename t_executor, typename... t_sys>
	struct on_ctx
	{
		using t_self			  = on_ctx<t_sys...>;
		using t_not_empty_idx_seq = meta::arr_to_seq_t<not_empty_sys_idx_arr<t_sys...>()>;
		using t_sys_not_empty	  = meta::filtered_variadic_t<meta::is_not_empty, t_sys...>;

		no_unique_addr __systems<t_executor&&, __sys_wrapper<t_sys>...> systems;

		constexpr on_ctx(auto&& exec, auto&&... sys) noexcept : systems{ FWD(exec), __sys_wrapper{ FWD(sys) }... } { };

		constexpr on_ctx() noexcept requires(std::is_empty_v<t_sys> and ...)
		= default;

		template <typename... t_sys, typename... t_arg>
		static consteval bool
		validate(meta::type_pack<t_arg...>)
		{
			{
				constexpr auto valid = sizeof...(t_sys) > 0;
				static_assert(valid, "[on_ctx]: requires at least one system type");
			}
			{
				constexpr auto valid = meta::is_not_same_v<decltype(systems(std::declval<t_arg&&>()...)), invalid_sys_call>;

				static_assert(valid, "[on_ctx] systems(...) returned invalid_sys_call - check that system i is callable with given arguments.");
			}

			return true;
		}

		template <typename... t_arg>
		FORCE_INLINE constexpr decltype(auto)
		operator()(t_arg&&... arg) noexcept
		{
			static_assert(validate<t_sys...>(meta::type_pack<t_arg&&...>{}), "[on_ctx]: invalid on_ctx");

			return systems(FWD(arg)...);
		}
	};

	template <typename... t_sys>
	on_ctx(t_sys&&...) -> on_ctx<meta::value_or_ref_t<t_sys&&>...>;
}	 // namespace ecs::system::ctx
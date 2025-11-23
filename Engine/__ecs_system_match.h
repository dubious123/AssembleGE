#pragma once

#ifndef INCLUDED_FROM_ECS_SYSTEM_HEADER
	#error "Do not include this file directly. Include <__ecs_system.h> instead."
#endif

#define MAX_CASE_COUNT 512

namespace ecs::system::detail
{
	template <auto n, typename t_sys>
	struct sys_case
	{
		no_unique_addr t_sys sys;

		static constexpr decltype(n) case_value = n;

		constexpr sys_case(t_sys&& sys) noexcept : sys(FWD(sys)) { }

		constexpr sys_case() = default;

		template <typename... t_arg>
		FORCE_INLINE constexpr decltype(auto)
		operator()(t_arg&&... arg) noexcept
		{
			return run_sys(sys, FWD(arg)...);
		}
	};

	template <auto n>
	struct sys_case<n, void>
	{
		static constexpr decltype(n) case_value = n;

		template <typename t_sys>
		constexpr decltype(auto)
		operator=(t_sys&& sys) const noexcept
		{
			return sys_case<n, t_sys>{ FWD(sys) };
		}
	};

	template <typename t_sys>
	struct sys_default
	{
		no_unique_addr t_sys sys;

		constexpr sys_default(t_sys&& sys) noexcept : sys(FWD(sys)) { }

		constexpr sys_default() { }

		template <typename... t_arg>
		FORCE_INLINE constexpr decltype(auto)
		operator()(t_arg&&... arg) noexcept
		{
			return run_sys(sys, FWD(arg)...);
		}
	};

	template <>
	struct sys_default<void>
	{
		template <typename t_sys>
		constexpr decltype(auto)
		operator=(t_sys&& sys) const noexcept
		{
			return sys_default<t_sys>{ FWD(sys) };
		}
	};

	template <typename t>
	constexpr inline bool is_sys_default_v = false;

	template <typename t_sys>
	constexpr bool is_sys_default_v<sys_default<t_sys>> = true;

	template <typename t>
	constexpr inline bool is_sys_case_v = false;

	template <auto n, typename t_sys>
	constexpr inline bool is_sys_case_v<sys_case<n, t_sys>> = not std::is_void_v<t_sys>;

	template <typename... t_sys_case>
	FORCE_INLINE constexpr bool
	has_sys_default()
	{
		return (is_sys_default_v<t_sys_case> || ...);
	}
}	 // namespace ecs::system::detail

namespace ecs::system
{
	using namespace detail;

	template <auto n>
	inline constexpr sys_case<n, void> on = sys_case<n, void>{};

	inline constexpr sys_default<void> default_to = sys_default<void>{};

	template <typename... t_sys_case>
	FORCE_INLINE constexpr bool
	validate_match()
	{
		constexpr auto default_to_count = ((is_sys_default_v<t_sys_case> ? 1 : 0) + ...);
		constexpr auto case_count		= sizeof...(t_sys_case) - default_to_count;

		static_assert(((is_sys_default_v<t_sys_case> or is_sys_case_v<t_sys_case>) and ...),
					  "match: all arguments must be either on<n> = system or default_to = system");

		static_assert(default_to_count <= 1, "match: only one default_to is allowed");

		if constexpr (default_to_count == 1)
		{
			static_assert(is_sys_default_v<meta::variadic_back_t<t_sys_case...>>, "match: default_to must be the last entry");
		}

		static_assert(case_count <= MAX_CASE_COUNT,
					  "match: too many cases for switch generation. Increase MAX_CASE_COUNT and expand __X_REPEAT_LIST_512 macro accordingly.");

		constexpr auto all_cases_unique = []<auto... idx>(std::index_sequence<idx...>) {
			return meta::variadic_auto_unique<meta::variadic_at_t<idx, t_sys_case...>::case_value...>;
		}(std::make_index_sequence<case_count>());

		static_assert(all_cases_unique, "match: duplicate on<n> detected. Each on<n> must have a unique value.");

		return true;
	}

	template <typename t_sys_selector, typename... t_sys_case>
	struct match
	{
		static_assert(validate_match<t_sys_case...>(), "match: invalid template arguements");

		using t_not_empty_idx_seq = meta::arr_to_seq_t<not_empty_sys_idx_arr<t_sys_case...>()>;
		using t_sys_not_empty	  = meta::filtered_variadic_t<meta::is_not_empty, t_sys_case...>;

		no_unique_addr t_sys_selector  sys_selector;
		no_unique_addr t_sys_not_empty sys_cases;

		constexpr match(t_sys_selector&& sys_selector, t_sys_case&&... sys_case) noexcept
			: sys_selector(FWD(sys_selector)),
			  sys_cases(meta::make_filtered_tuple<meta::is_not_empty, t_sys_case...>(FWD(sys_case)...)) { };

		constexpr match() requires(std::is_empty_v<t_sys_selector> and ... and std::is_empty_v<t_sys_case>)
		= default;

		FORCE_INLINE static consteval decltype(auto)
		case_id_arr()
		{
			constexpr auto sys_case_count = sizeof...(t_sys_case) - ((is_sys_default_v<t_sys_case> ? 1 : 0) + ...);
			auto		   arr			  = meta::seq_to_arr(std::make_index_sequence<MAX_CASE_COUNT>());

			[&arr]<auto... idx>(std::index_sequence<idx...>) {
				(
					([&arr] {
						constexpr auto case_id = meta::variadic_at_t<idx, t_sys_case...>::case_value;
						arr[idx]			   = case_id;
						if constexpr (case_id >= 0 and case_id < MAX_CASE_COUNT)
						{
							arr[case_id] = idx;
						}
					}()),
					...);
			}(std::make_index_sequence<sys_case_count>());

			return arr;
		}

		template <std::size_t i>
		FORCE_INLINE static consteval auto
		case_id()
		{
			// constexpr auto arr = case_id_arr();
			return case_id_arr()[i];
		}

		template <std::size_t i, typename... t_arg>
		FORCE_INLINE constexpr decltype(auto)
		run_impl(t_arg&&... arg) noexcept
		{
			using t_sys_case_now = meta::variadic_at_t<i, t_sys_case...>;
			if constexpr (std::is_empty_v<t_sys_case_now>)
			{
				return run_sys(t_sys_case_now{}, FWD(arg)...);
			}
			else
			{
				return run_sys(std::get<meta::index_sequence_at_v<i, t_not_empty_idx_seq>>(sys_cases), FWD(arg)...);
			}
		}

#define __SYS_CASE_IMPL(N)                   \
	case case_id<N>():                       \
	{                                        \
		if constexpr (N < sys_case_count)    \
		{                                    \
			return run_impl<N>(FWD(arg)...); \
		}                                    \
	}

		template <typename... t_arg>
		FORCE_INLINE constexpr decltype(auto)
		operator()(t_arg&&... arg)
		{
			constexpr auto default_to_exists = (is_sys_default_v<t_sys_case> | ...);
			constexpr auto sys_case_count	 = sizeof...(t_sys_case) - (default_to_exists ? 0 : 1);

			if constexpr (default_to_exists)
			{
				[this]<auto... idx>(std::index_sequence<idx...>) {
					using t_ret_default = decltype(run_impl<sizeof...(t_sys_case) - 1>(FWD(arg)...));
					static_assert((std::is_same_v<t_ret_default, decltype(run_impl<idx>(FWD(arg)...))> and ...),
								  "match: when a default_to is provided, all cases and the default_to must return the same type");
				}(std::make_index_sequence<sys_case_count>());
			}
			else
			{
				[this]<auto... idx>(std::index_sequence<idx...>) {
					static_assert((std::is_same_v<void, decltype(run_impl<idx>(FWD(arg)...))> and ...),
								  "match: when no default_to is provided, all cases must return void");
				}(std::make_index_sequence<sys_case_count>());
			}

			auto args = make_arg_tpl(FWD(arg)...);

			return meta::tuple_unpack(
				[this](auto&&... arg) noexcept {
					auto key = run_sys(sys_selector, FWD(arg)...);

					switch (key)
					{
#define X(N) __SYS_CASE_IMPL(N)
						__X_REPEAT_LIST_512
#undef X
					default:
					{
						if constexpr (default_to_exists)
						{
							return run_impl<sizeof...(t_sys_case) - 1>(FWD(arg)...);
						}
						break;
					}
					}
				},
				args);
		}

#undef __SYS_CASE_IMPL
#undef MAX_CASE_COUNT
	};
}	 // namespace ecs::system

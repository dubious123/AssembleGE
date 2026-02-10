#pragma once

namespace age::ecs::system::detail
{
	template <auto n, typename t_sys>
	struct sys_case
	{
		using t_ctx_tag = ctx_tag<tag_adaptor, tag_ctx_bound>;

		no_unique_addr t_sys sys;

		static inline constexpr decltype(n) case_value = n;

		FORCE_INLINE constexpr sys_case(auto&& arg) noexcept : sys{ FWD(arg) }
		{
		}

		FORCE_INLINE constexpr decltype(auto)
		operator()(this auto&& self, cx_ctx auto&& ctx, auto&&... arg) noexcept
		{
			return run_sys(FWD(ctx), FWD(self).sys, FWD(arg)...);
		}
	};

	template <auto n>
	struct sys_case<n, void>
	{
		static constexpr decltype(n) case_value = n;

		template <typename t_sys>
		FORCE_INLINE constexpr decltype(auto)
		operator=(t_sys&& sys) const noexcept
		{
			return sys_case<n, t_sys>{ FWD(sys) };
		}
	};

	template <typename t_sys>
	struct sys_default
	{
		using t_ctx_tag = ctx_tag<tag_adaptor, tag_ctx_bound>;
		no_unique_addr t_sys sys;

		FORCE_INLINE constexpr sys_default(auto&& arg) noexcept : sys{ FWD(arg) }
		{
		}

		FORCE_INLINE constexpr decltype(auto)
		operator()(this auto&& self, cx_ctx auto&& ctx, auto&&... arg) noexcept
		{
			return run_sys(FWD(ctx), FWD(self).sys, FWD(arg)...);
		}
	};

	template <>
	struct sys_default<void>
	{
		template <typename t_sys>
		FORCE_INLINE constexpr decltype(auto)
		operator=(t_sys&& sys) const noexcept
		{
			return sys_default<t_sys>{ FWD(sys) };
		}
	};

	template <typename t>
	constexpr inline bool is_sys_default_v = false;

	template <typename t_sys>
	constexpr inline bool is_sys_default_v<sys_default<t_sys>> = true;

	template <typename t>
	constexpr inline bool is_sys_case_v = false;

	template <auto n, typename t_sys>
	constexpr inline bool is_sys_case_v<sys_case<n, t_sys>> = not std::is_void_v<t_sys>;

	template <typename... t_sys_case>
	consteval bool
	has_sys_default()
	{
		return (is_sys_default_v<t_sys_case> || ...);
	}
}	 // namespace age::ecs::system::detail

namespace age::ecs::system
{
#define MAX_CASE_COUNT 512

	template <auto n>
	inline constexpr detail::sys_case<n, void> on = detail::sys_case<n, void>{};

	inline constexpr detail::sys_default<void> default_to = detail::sys_default<void>{};

	template <typename... t_sys_case>
	FORCE_INLINE constexpr bool
	validate_match()
	{
		constexpr auto default_to_count = ((detail::is_sys_default_v<t_sys_case> ? 1 : 0) + ...);
		constexpr auto case_count		= ((detail::is_sys_case_v<t_sys_case> ? 1 : 0) + ...);

		static_assert(default_to_count + case_count == sizeof...(t_sys_case),
					  "[match] all arguments must be either on<n> = system or default_to = system");

		static_assert(default_to_count <= 1, "[match] only one default_to is allowed");

		if constexpr (default_to_count == 1)
		{
			static_assert(detail::is_sys_default_v<age::meta::variadic_back_t<t_sys_case...>>, "[match] default_to must be the last entry");
		}

		static_assert(case_count <= MAX_CASE_COUNT,
					  "[match] too many cases for switch generation. Increase MAX_CASE_COUNT and expand __X_REPEAT_LIST_512 macro accordingly.");


		constexpr auto all_cases_unique = []<std::size_t... idx>(std::index_sequence<idx...>) {
			return age::meta::variadic_auto_unique<age::meta::variadic_at_t<idx, t_sys_case...>::case_value...>;
		}(std::make_index_sequence<case_count>());

		static_assert(all_cases_unique, "[match] duplicate on<n> detected. Each on<n> must have a unique value.");

		return true;
	}

	template <typename t_sys_selector, typename... t_sys_case>
	struct match
	{
		static_assert(validate_match<t_sys_case...>(), "[match] invalid template arguements");

		no_unique_addr t_sys_selector sys_selector;

		no_unique_addr age::meta::compressed_pack<t_sys_case...> sys_cases;

		FORCE_INLINE constexpr match(auto&& sys_selector_arg, auto&&... sys_case_arg) noexcept
			: sys_selector{ FWD(sys_selector_arg) },
			  sys_cases{ FWD(sys_case_arg)... } {};

		static consteval decltype(auto)
		case_id_arr()
		{
			constexpr auto sys_case_count = sizeof...(t_sys_case) - ((detail::is_sys_default_v<t_sys_case> ? 1 : 0) + ...);
			auto		   arr			  = age::meta::seq_to_arr(std::make_index_sequence<MAX_CASE_COUNT>());

			[&arr]<auto... idx>(std::index_sequence<idx...>) {
				(
					([&arr] {
						constexpr auto case_id = age::meta::variadic_at_t<idx, t_sys_case...>::case_value;
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
		static consteval auto
		case_id()
		{
			// constexpr auto arr = case_id_arr();
			return case_id_arr()[i];
		}

#define __SYS_CASE_IMPL(N)                                             \
	case case_id<N>():                                                 \
	{                                                                  \
		if constexpr (N < sys_case_count)                              \
		{                                                              \
			return run_sys(ctx, FWD(self).sys_cases.get<N>(), arg...); \
		}                                                              \
		else                                                           \
		{                                                              \
			[[fallthrough]];                                           \
		}                                                              \
	}

		FORCE_INLINE constexpr decltype(auto)
		operator()(this auto&& self, cx_ctx auto&& ctx, auto&&... arg)
		{
			constexpr auto default_to_exists = (detail::is_sys_default_v<t_sys_case> | ...);
			constexpr auto sys_case_count	 = sizeof...(t_sys_case) - (default_to_exists ? 0 : 1);

			// if constexpr (default_to_exists)
			//{
			//	[this]<auto... idx>(std::index_sequence<idx...>) {
			//		using t_ret_default = decltype(run_impl<sizeof...(t_sys_case) - 1>(FWD(arg)...));
			//		static_assert((std::is_same_v<t_ret_default, decltype(run_impl<idx>(FWD(arg)...))> and ...),
			//					  "[match] when a default_to is provided, all cases and the default_to must return the same type");
			//	}(std::make_index_sequence<sys_case_count>());
			// }
			// else
			//{
			//	[this]<auto... idx>(std::index_sequence<idx...>) {
			//		static_assert((std::is_same_v<void, decltype(run_impl<idx>(FWD(arg)...))> and ...),
			//					  "[match] when no default_to is provided, all cases must return void");
			//	}(std::make_index_sequence<sys_case_count>());
			// }

			switch (run_sys(ctx, FWD(self).sys_selector, arg...))
			{
#define X(N) __SYS_CASE_IMPL(N)
				__X_REPEAT_LIST_512
#undef X
			default:
			{
				if constexpr (default_to_exists)
				{
					return run_sys(FWD(ctx), FWD(self).sys_cases.get<sizeof...(t_sys_case) - 1>(), FWD(arg)...);
				}
				else
				{
					break;
				}
			}
			}
		}

#undef __SYS_CASE_IMPL
#undef MAX_CASE_COUNT
	};

	template <typename t_sys_selector, typename... t_sys_case>
	match(t_sys_selector&&, t_sys_case&&...) -> match<t_sys_selector, t_sys_case...>;
}	 // namespace age::ecs::system
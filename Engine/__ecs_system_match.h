#pragma once

namespace ecs
{
	template <typename t_sys_cond, typename t_sys_then>
	struct system_case
	{
		no_unique_addr t_sys_cond sys_cond;
		no_unique_addr t_sys_then sys_then;

		constexpr system_case(t_sys_cond&& sys_cond, t_sys_then&& sys_then)
			: sys_cond(std::forward<t_sys_cond>(sys_cond)),
			  sys_then(std::forward<t_sys_then>(sys_then)) { }

		constexpr system_case() requires(std::is_empty_v<t_sys_cond> && std::is_empty_v<t_sys_then>)
		= default;

		template <typename... t_key>
		bool matches(t_key&&... key)
		{
			return _run_sys(sys_cond, std::forward<t_key>(key)...);
		}
	};

	template <typename t_sys>
	struct system_case_default
	{
		no_unique_addr t_sys sys;

		constexpr system_case_default(t_sys&& sys)
			: sys(std::forward<t_sys>(sys)) { }

		constexpr system_case_default() requires(std::is_empty_v<t_sys>)
		= default;

		template <typename... t_data>
		void run(t_data&&... data)
		{
			_run_sys(sys, std::forward<t_data>(data)...);
		}
	};

	template <typename t>
	struct is_system_case : std::false_type
	{
	};

	template <typename t_cond, typename t_then>
	struct is_system_case<system_case<t_cond, t_then>> : std::true_type
	{
	};

	template <typename t>
	inline constexpr bool is_system_case_v = is_system_case<t>::value;

	template <typename t>
	struct is_system_case_default : std::false_type
	{
	};

	template <typename t_sys>
	struct is_system_case_default<system_case_default<t_sys>> : std::true_type
	{
	};

	template <typename t>
	inline constexpr bool is_system_case_default_v = is_system_case_default<t>::value;

	template <typename... t>
	constexpr bool all_cases_valid = (... && (is_system_case_v<t> || is_system_case_default_v<t>));

	template <typename... t>
	constexpr bool default_is_last = [] {
		if constexpr ((sizeof...(t) == 0) || (sizeof...(t) == 1))
		{
			return true;
		}
		else
		{
			return false;
			// return !std::ranges::any_of(std::array { is_system_case_default_v<t>... } | std::views::take(sizeof...(t) - 1), std::identity {});
		}
	}();

	template <typename t_sys_selector, typename... t_sys_case>
	// requires all_cases_valid<t_sys_case...> && default_is_last<t_sys_case...>
	struct system_match
	{
		static const constexpr std::array<std::size_t, sizeof...(t_sys_case)> not_empty_sys_idx_arr = make_not_empty_sys_idx_arr<t_sys_case...>();

		using t_all_sys_tpl		  = std::tuple<t_sys_case...>;
		using t_sys_not_empty_tpl = meta::filtered_tuple_t<meta::is_not_empty, t_sys_case...>;

		no_unique_addr t_sys_selector	   sys_selector;
		no_unique_addr t_sys_not_empty_tpl sys_cases;

		template <typename... t_sys_case_not_empty>
		constexpr system_match(t_sys_selector&& sys_selector, std::tuple<t_sys_case_not_empty...>&& sys_cases)
			: sys_selector(std::forward<t_sys_selector>(sys_selector)),
			  sys_cases(std::forward<std::tuple<t_sys_case_not_empty...>>(sys_cases))
		{
		}

		template <typename... t_sys_case_not_empty>
		constexpr system_match(t_sys_selector&& sys_selector, t_sys_case_not_empty&&... sys_case)
			: sys_selector(std::forward<t_sys_selector>(sys_selector)),
			  sys_cases(std::forward<t_sys_case_not_empty>(sys_case)...)
		{
		}

		constexpr system_match() requires(std::is_empty_v<t_sys_case> && ...)
		= default;

		template <typename... t_data>
		void run(t_data&&... data)
		{
			run_impl(_run_sys(sys_selector, std::forward<t_data>(data)...), std::forward<t_data>(data)...);
		}

	  private:
		template <std::size_t i = 0, typename t_key, typename... t_data>
		void run_impl(t_key&& key, t_data&&... data)
		{
			if constexpr (i < std::tuple_size_v<t_all_sys_tpl>)
			{
				using t_sys_case_now = std::tuple_element_t<i, t_all_sys_tpl>;

				if constexpr (not std::is_empty_v<t_sys_case_now>)
				{
					auto& current = std::get<not_empty_sys_idx_arr[i]>(sys_cases);
					if constexpr (is_system_case_v<std::decay_t<t_sys_case_now>>)
					{
						if (current.matches(std::forward<t_key>(key)))
						{
							_run_sys(current.sys_then, std::forward<t_data>(data)...);
						}
						else
						{
							run_impl<i + 1>(std::forward<t_key>(key), std::forward<t_data>(data)...);
						}
					}
					else
					{
						_run_sys(current.sys, std::forward<t_data>(data)...);
					}
				}
				else
				{
					if constexpr (is_system_case_v<std::decay_t<t_sys_case_now>>)
					{
						if (t_sys_case_now {}.matches(std::forward<t_key>(key)))
						{
							_run_sys(decltype(t_sys_case_now {}.sys_then) {}, std::forward<t_data>(data)...);
						}
						else
						{
							run_impl<i + 1>(std::forward<t_key>(key), std::forward<t_data>(data)...);
						}
					}
					else
					{
						_run_sys(decltype(t_sys_case_now {}.sys) {}, std::forward<t_data>(data)...);
					}
				}
			}
		}
	};

	template <typename t_sys_selector, typename... t_sys_case>
	constexpr decltype(auto) match(t_sys_selector&& sys_selector, t_sys_case&&... sys_case)
	{
		return system_match<t_sys_selector, t_sys_case...>(
			std::forward<t_sys_selector>(sys_selector),
			meta::make_filtered_tuple<meta::is_not_empty>(std::forward<t_sys_case>(sys_case)...));
	}

	template <typename t_sys_cond, typename t_sys_then>
	constexpr decltype(auto) on(t_sys_cond&& sys_cond, t_sys_then&& sys_then)
	{
		return system_case<t_sys_cond, t_sys_then>(
			std::forward<t_sys_cond>(sys_cond),
			std::forward<t_sys_then>(sys_then));
	}

	template <auto integral, typename t_sys_then>
	constexpr decltype(auto) on(t_sys_then&& sys_then)
	{
		using t_sys_cond = decltype([](auto _) { return integral == _; });
		return system_case<t_sys_cond, t_sys_then>(
			t_sys_cond {},
			std::forward<t_sys_then>(sys_then));
	}

	template <typename t_sys>
	constexpr decltype(auto) default_to(t_sys&& sys)
	{
		return system_case_default<t_sys>(std::forward<t_sys>(sys));
	}
}	 // namespace ecs
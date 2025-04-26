#pragma once

namespace ecs::system::detail
{
	struct __parallel_executor_base
	{
	};

	template <typename t>
	concept has_par_exec_member = requires(t v) {
		requires std::is_base_of_v<__parallel_executor_base, std::decay_t<decltype(v.__parallel_executor)>>;
	};

	template <typename... t>
	struct first_par_exec;

	template <typename t_head, typename... t_tail>
	struct first_par_exec<t_head, t_tail...>
	{
		using type = std::conditional_t<has_par_exec_member<t_head>, t_head, typename first_par_exec<t_tail...>::type>;
	};

	template <>
	struct first_par_exec<>
	{
		using type = void;
	};

	template <typename... t>
	using first_par_exec_t = typename first_par_exec<t...>::type;

	template <typename... t>
	static constexpr bool par_exec_found_v = not std::is_same_v<first_par_exec_t<t...>, void>;

	template <typename... t_data>
	constexpr decltype(auto) extract_par_exec(t_data&&... data)
	{
		using t_holder = first_par_exec_t<t_data...>;
		using t_res	   = decltype(std::get<t_holder>(std::forward_as_tuple(data...)).__parallel_executor);
		return std::forward<t_res>(std::get<t_holder>(std::forward_as_tuple(data...)).__parallel_executor);
	}

	template <typename... t_sys>
	struct system_par
	{
		static const constexpr std::array<std::size_t, sizeof...(t_sys)> not_empty_sys_idx_arr = make_not_empty_sys_idx_arr<t_sys...>();

		using t_all_sys_tpl		  = std::tuple<t_sys...>;
		using t_sys_not_empty_tpl = meta::filtered_tuple_t<meta::is_not_empty, t_sys...>;

		no_unique_addr t_sys_not_empty_tpl systems;

		// std::tuple<t_sys...> systems;

		template <typename... t_sys_not_empty>
		constexpr system_par(t_sys_not_empty&&... sys)
			: systems(std::forward<t_sys_not_empty>(sys)...)
		{
		}

		template <typename... t_sys_not_empty>
		constexpr system_par(std::tuple<t_sys_not_empty...>&& tpl)
			: systems(std::forward<std::tuple<t_sys_not_empty...>>(tpl))
		{
		}

		constexpr system_par() requires(std::is_empty_v<t_sys> && ...)
		= default;

		template <typename... t_data>
		decltype(auto) run(t_data&&... data)
		{
			if constexpr (sizeof...(t_data) == 0)
			{
				if constexpr (par_exec_found_v<std::tuple_element_t<0, decltype(_run_sys(std::get<0>(systems)))>>)
				{
					return run_with_par_exec(
						extract_par_exec(std::get<0>(systems)),
						meta::offset_sequence<1, std::tuple_size_v<decltype(systems)> - 1> {},
						_run_sys(std::get<0>(systems)));
				}
				else
				{
					return run_with_default(meta::offset_sequence<1, std::tuple_size_v<decltype(systems)> - 1> {}, _run_sys(std::get<0>(systems)));
				}
			}
			else
			{
				if constexpr (par_exec_found_v<t_data...>)
				{
					return run_with_par_exec(
						extract_par_exec(std::forward<t_data>(data)...),
						std::index_sequence_for<t_sys...> {},
						std::forward<t_data>(data)...);
				}
				else
				{
					return run_with_default(std::index_sequence_for<t_sys...> {}, std::forward<t_data>(data)...);
				}
			}
		}

	  private:
		template <std::size_t... i, typename... t_data>
		decltype(auto) run_with_default(std::index_sequence<i...>, t_data&&... data)
		{
			auto futures = std::make_tuple(
				std::async(std::launch::async, [&] {
					using t_sys_now = std::tuple_element_t<i, t_all_sys_tpl>;
					if constexpr (not std::is_empty_v<t_sys_now>)
					{
						_run_sys(std::get<not_empty_sys_idx_arr[i]>(systems), std::forward<t_data>(data)...);
					}
					else
					{
						_run_sys(t_sys_now {}, std::forward<t_data>(data)...);
					}
				})...);
			(..., (std::get<i>(futures).wait()));
		}

		template <typename t_par_exec, std::size_t... i, typename... t_data>
		decltype(auto) run_with_par_exec(t_par_exec&& par_exec, std::index_sequence<i...>, t_data&&... data)
		{
			return par_exec.run_par(
				([&] {
					using t_sys_now = std::tuple_element_t<i, t_all_sys_tpl>;
					if constexpr (not std::is_empty_v<t_sys_now>)
					{
						_run_sys(std::get<not_empty_sys_idx_arr[i]>(systems), std::forward<t_data>(data)...);
					}
					else
					{
						_run_sys(t_sys_now {}, std::forward<t_data>(data)...);
					}
				})...);
			// par_exec.run_par(systems, std::forward<t_data>(data)...);
			//(..., par_exec.run_par(std::get<i>(systems), std::forward<t_data>(data)...));
		}
	};
}	 // namespace ecs::system::detail

namespace ecs::system::op
{
	using namespace ecs::system::detail;

	template <typename t_left, typename t_right>
	decltype(auto) operator^(t_left&& left, t_right&& right)
	{
		static_assert(not is_break_or_continue<t_left> and not is_break_or_continue<t_right>,
					  "❌ break_if / continue_if cannot be used with operator^");

		if constexpr (std::is_empty_v<t_left>)
		{
			if constexpr (std::is_empty_v<t_right>)
			{
				return system_par<t_left, t_right>();
			}
			else
			{
				return system_par<t_left, t_right>(std::forward<t_right>(right));
			}
		}
		else
		{
			if constexpr (std::is_empty_v<t_right>)
			{
				return system_par<t_left, t_right>(std::forward<t_left>(left));
			}
			else
			{
				return system_par<t_left, t_right>(std::forward<t_left>(left), std::forward<t_right>(right));
			}
		}
	}

	template <typename... t_sys, template <typename...> typename t_sys_group, typename t_right>
	requires std::same_as<system_par<t_sys...>, t_sys_group<t_sys...>>
	decltype(auto) operator^(t_sys_group<t_sys...>&& left, t_right&& right)
	{
		static_assert(not is_break_or_continue<t_right>,
					  "❌ break_if / continue_if cannot be used with operator^");
		if constexpr (std::is_empty_v<t_right>)
		{
			return system_par<t_sys..., t_right>(std::forward<decltype(left.systems)>(left.systems));
		}
		else
		{
			return system_par<t_sys..., t_right>(std::tuple_cat(std::forward<decltype(left.systems)>(left.systems), std::forward_as_tuple(std::forward<t_right>(right))));
		}
	}
}	 // namespace ecs::system::op
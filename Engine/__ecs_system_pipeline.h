#pragma once

namespace ecs::system::detail
{
	template <typename... t_sys>
	struct system_pipeline
	{
		static const constexpr std::array<std::size_t, sizeof...(t_sys)> not_empty_sys_idx_arr = make_not_empty_sys_idx_arr<t_sys...>();

		using t_all_sys_tpl		  = std::tuple<t_sys...>;
		using t_sys_not_empty_tpl = meta::filtered_tuple_t<meta::is_not_empty, t_sys...>;

		no_unique_addr t_sys_not_empty_tpl systems;

		template <typename... t_sys_not_empty>
		constexpr system_pipeline(t_sys_not_empty&&... sys)
			: systems(std::forward<t_sys_not_empty>(sys)...)
		{
		}

		template <typename... t_sys_not_empty>
		constexpr system_pipeline(std::tuple<t_sys_not_empty...>&& tpl)
			: systems(std::forward<std::tuple<t_sys_not_empty...>>(tpl))
		{
		}

		constexpr system_pipeline() requires(std::is_empty_v<t_sys> && ...)
		= default;

		template <typename... t_data>
		decltype(auto) run(t_data&&... data)
		{
			return run_impl<0>(std::forward<t_data>(data)...);
			//_run_sys(sys_right, _run_sys(sys_left, std::forward<decltype(data)>(data)...));
		}

	  private:
		template <std::size_t i, typename... t_data>
		decltype(auto) run_impl(t_data&&... data)
		{
			if constexpr (i == std::tuple_size_v<t_all_sys_tpl>)
			{
				if constexpr (sizeof...(data) == 1)
				{
					return _run_sys(std::forward<t_data>(data)...);
				}
				else
				{
					// todo
					return;
				}
			}
			else
			{
				using t_sys_now = std::tuple_element_t<i, t_all_sys_tpl>;

				if constexpr (not std::is_empty_v<t_sys_now>)
				{
					if constexpr (not std::is_same_v<decltype(_run_sys(std::get<not_empty_sys_idx_arr[i]>(systems), std::forward<t_data>(data)...)), void>)
					{
						return run_impl<i + 1>(_run_sys(std::get<not_empty_sys_idx_arr[i]>(systems), std::forward<t_data>(data)...));
					}
					else
					{
						_run_sys(std::get<not_empty_sys_idx_arr[i]>(systems), std::forward<t_data>(data)...);
						return run_impl<i + 1>();
					}
				}
				else
				{
					if constexpr (not std::is_same_v<decltype(_run_sys(t_sys_now {}, std::forward<t_data>(data)...)), void>)
					{
						return run_impl<i + 1>(_run_sys(t_sys_now {}, std::forward<t_data>(data)...));
					}
					else
					{
						_run_sys(t_sys_now {}, std::forward<t_data>(data)...);
						return run_impl<i + 1>();
					}
				}
			}
		}
	};
}	 // namespace ecs::system::detail

namespace ecs::system::op
{
	using namespace ecs::system::detail;

	template <typename t_left, typename t_right>
	decltype(auto) operator|(t_left&& left, t_right&& sys)
	{
		static_assert(not is_break_or_continue<t_left> and not is_break_or_continue<t_right>,
					  "❌ break_if / continue_if cannot be used with operator|");

		if constexpr (std::is_empty_v<t_left>)
		{
			if constexpr (std::is_empty_v<t_right>)
			{
				return system_pipeline<t_left, t_right>();
			}
			else
			{
				return system_pipeline<t_left, t_right>(std::forward<t_right>(sys));
			}
		}
		else
		{
			if constexpr (std::is_empty_v<t_right>)
			{
				return system_pipeline<t_left, t_right>(std::forward<t_left>(left));
			}
			else
			{
				return system_pipeline<t_left, t_right>(std::forward<t_left>(left), std::forward<t_right>(sys));
			}
		}
	}

	template <typename... t_sys, template <typename...> typename t_sys_group, typename t_right>
	requires std::same_as<system_pipeline<t_sys...>, t_sys_group<t_sys...>>
	decltype(auto) operator|(t_sys_group<t_sys...>&& left, t_right&& right)
	{
		static_assert(not is_break_or_continue<t_right>,
					  "❌ break_if / continue_if cannot be used with operator|");

		if constexpr (std::is_empty_v<t_right>)
		{
			return system_pipeline<t_sys..., t_right>(std::forward<decltype(left.systems)>(left.systems));
		}
		else
		{
			return system_pipeline<t_sys..., t_right>(std::tuple_cat(std::forward<decltype(left.systems)>(left.systems), std::forward_as_tuple(std::forward<t_right>(right))));
		}
	}

	template <typename... t_sys, template <typename...> typename t_sys_group, typename t_left>
	requires std::same_as<system_pipeline<t_sys...>, t_sys_group<t_sys...>>
	decltype(auto) operator|(t_left&& left, t_sys_group<t_sys...>&& right)
	{
		static_assert(not is_break_or_continue<t_left>,
					  "❌ break_if / continue_if cannot be used with operator|");

		if constexpr (std::is_empty_v<t_left>)
		{
			return system_pipeline<t_left, t_sys...>(std::forward<decltype(right.systems)>(right.systems));
		}
		else
		{
			return system_pipeline<t_left, t_sys...>(std::tuple_cat(std::forward_as_tuple(std::forward<t_left>(left)), std::forward<decltype(right.systems)>(right.systems)));
		}
	}

	template <typename... t_sys_l, template <typename...> typename t_sys_group_l, typename... t_sys_r, template <typename...> typename t_sys_group_r>
	requires std::same_as<system_pipeline<t_sys_l...>, t_sys_group_l<t_sys_l...>> && std::same_as<system_pipeline<t_sys_r...>, t_sys_group_r<t_sys_r...>>
	decltype(auto) operator|(t_sys_group_l<t_sys_l...>&& left, t_sys_group_r<t_sys_r...>&& right)
	{
		return system_pipeline<t_sys_l..., t_sys_r...>(std::tuple_cat(std::forward<decltype(left.systems)>(left.systems), std::forward<decltype(right.systems)>(right.systems)));
	}
}	 // namespace ecs::system::op
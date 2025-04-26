#pragma once

namespace ecs
{
	template <typename... t_sys>
	struct system_seq
	{
		static const constexpr std::array<std::size_t, sizeof...(t_sys)> not_empty_sys_idx_arr = make_not_empty_sys_idx_arr<t_sys...>();

		using t_all_sys_tpl		  = std::tuple<t_sys...>;
		using t_sys_not_empty_tpl = meta::filtered_tuple_t<meta::is_not_empty, t_sys...>;

		no_unique_addr t_sys_not_empty_tpl systems;

		template <typename... t_sys_not_empty>
		constexpr system_seq(t_sys_not_empty&&... sys)
			: systems(std::forward<t_sys_not_empty>(sys)...)
		{
		}

		template <typename... t_sys_not_empty>
		constexpr system_seq(std::tuple<t_sys_not_empty...>&& tpl)
			: systems(std::forward<std::tuple<t_sys_not_empty...>>(tpl))
		{
		}

		constexpr system_seq() requires(std::is_empty_v<t_sys> && ...)
		= default;

		// template <typename... t_sys_not_empty, typename t_sys_r>
		// constexpr system_seq(std::tuple<t_sys_not_empty...>&& tpl, t_sys_r&& sys)
		//	: systems(std::tuple_cat(std::forward<std::tuple<t_sys_not_empty...>>(tpl), std::make_tuple(std::forward<t_sys_r>(sys))))
		//{
		// }

		template <typename... t_data>
		decltype(auto) run(t_data&&... data)
		{
			if constexpr (sizeof...(t_data) == 0 and not std::is_same_v<decltype(run_one_impl<0>()), void>)
			{
				return run_impl(meta::offset_sequence<1, std::tuple_size_v<t_all_sys_tpl> - 1> {}, run_one_impl<0>());
			}
			else
			{
				return run_impl(std::index_sequence_for<t_sys...> {}, std::forward<t_data>(data)...);
			}
		}

	  private:
		template <std::size_t... i, typename... t_data>
		decltype(auto) run_impl(std::index_sequence<i...>, t_data&&... data)
		{
			(run_one_impl<i>(std::forward<t_data>(data)...), ...);
		}

		template <std::size_t i, typename... t_data>
		decltype(auto) run_one_impl(t_data&&... data)
		{
			using t_sys_now = std::tuple_element_t<i, t_all_sys_tpl>;
			if constexpr (std::is_empty_v<t_sys_now>)
			{
				return _run_sys(t_sys_now {}, std::forward<t_data>(data)...);
			}
			else
			{
				return _run_sys(std::get<not_empty_sys_idx_arr[i]>(systems), std::forward<t_data>(data)...);
			}
		}
	};
}	 // namespace ecs

namespace ecs::system::op
{
	template <typename t_left, typename t_right>
	decltype(auto) operator+(t_left&& left, t_right&& right)
	{
		static_assert(not is_break_or_continue<t_left> and not is_break_or_continue<t_right>,
					  "❌ break_if / continue_if cannot be used with operator+");

		if constexpr (std::is_empty_v<t_left>)
		{
			if constexpr (std::is_empty_v<t_right>)
			{
				return system_seq<t_left, t_right>();
			}
			else
			{
				return system_seq<t_left, t_right>(std::forward<t_right>(right));
			}
		}
		else
		{
			if constexpr (std::is_empty_v<t_right>)
			{
				return system_seq<t_left, t_right>(std::forward<t_left>(left));
			}
			else
			{
				return system_seq<t_left, t_right>(std::forward<t_left>(left), std::forward<t_right>(right));
			}
		}
	}

	template <typename... t_sys, template <typename...> typename t_sys_group, typename t_right>
	requires std::same_as<system_seq<t_sys...>, t_sys_group<t_sys...>>
	decltype(auto) operator+(t_sys_group<t_sys...>&& left, t_right&& right)
	{
		static_assert(not is_break_or_continue<t_right>,
					  "❌ break_if / continue_if cannot be used with operator+");
		if constexpr (std::is_empty_v<t_right>)
		{
			return system_seq<t_sys..., t_right>(std::forward<decltype(left.systems)>(left.systems));
		}
		else if constexpr (std::is_lvalue_reference_v<t_right>)
		{
			return system_seq<t_sys..., t_right>(std::tuple_cat(std::forward<decltype(left.systems)>(left.systems), std::forward_as_tuple(std::forward<t_right>(right))));
		}
		else
		{
			return system_seq<t_sys..., t_right>(std::tuple_cat(std::forward<decltype(left.systems)>(left.systems), std::make_tuple(std::forward<t_right>(right))));
		}
	}
}	 // namespace ecs::system::op
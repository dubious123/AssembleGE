#pragma once

namespace ecs::system
{
	namespace detail
	{
		template <typename t_sys_cond>
		struct system_break_if
		{
			no_unique_addr t_sys_cond sys_cond;

			constexpr system_break_if(t_sys_cond&& sys_cond) : sys_cond(std::forward<t_sys_cond>(sys_cond)) { }

			constexpr system_break_if() = default;

			template <typename... t_data>
			constexpr bool operator()(t_data&&... data)
			{
				return _run_sys(sys_cond, std::forward<t_data>(data)...);
			}
		};

		template <typename t_sys_cond>
		struct system_continue_if
		{
			no_unique_addr t_sys_cond sys_cond;

			constexpr system_continue_if(t_sys_cond&& sys_cond) : sys_cond(std::forward<t_sys_cond>(sys_cond)) { }

			constexpr system_continue_if() = default;

			template <typename... t_data>
			constexpr bool operator()(t_data&&... data)
			{
				return _run_sys(sys_cond, std::forward<t_data>(data)...);
			}
		};

		template <typename t>
		inline constexpr bool is_break_if = false;

		template <typename t_sys_cond>
		inline constexpr bool is_break_if<system_break_if<t_sys_cond>> = true;

		template <typename t>
		inline constexpr bool is_continue_if = false;

		template <typename t_sys_cond>
		inline constexpr bool is_continue_if<system_continue_if<t_sys_cond>> = true;

		template <typename t>
		constexpr bool is_break_or_continue = is_break_if<std::decay_t<t>> || is_continue_if<std::decay_t<t>>;

#define __SYS_LOOP_IMPL(N)                                                                                \
	if constexpr (N < std::tuple_size_v<t_all_sys_tpl>)                                                   \
	{                                                                                                     \
		using t_sys_now = std::tuple_element_t<N, t_all_sys_tpl>;                                         \
		if constexpr (is_break_if<std::decay_t<t_sys_now>>)                                               \
		{                                                                                                 \
			if constexpr (not std::is_empty_v<t_sys_now>)                                                 \
			{                                                                                             \
				if (_run_sys(std::get<not_empty_sys_idx_arr[N]>(systems), std::forward<t_data>(data)...)) \
					break;                                                                                \
			}                                                                                             \
			else                                                                                          \
			{                                                                                             \
				if (_run_sys(t_sys_now {}, std::forward<t_data>(data)...))                                \
					break;                                                                                \
			}                                                                                             \
		}                                                                                                 \
		else if constexpr (is_continue_if<std::decay_t<t_sys_now>>)                                       \
		{                                                                                                 \
			if constexpr (not std::is_empty_v<t_sys_now>)                                                 \
			{                                                                                             \
				if (_run_sys(std::get<not_empty_sys_idx_arr[N]>(systems), std::forward<t_data>(data)...)) \
					continue;                                                                             \
			}                                                                                             \
			else                                                                                          \
			{                                                                                             \
				if (_run_sys(t_sys_now {}, std::forward<t_data>(data)...))                                \
					continue;                                                                             \
			}                                                                                             \
		}                                                                                                 \
		else                                                                                              \
		{                                                                                                 \
			if constexpr (not std::is_empty_v<t_sys_now>)                                                 \
			{                                                                                             \
				_run_sys(std::get<not_empty_sys_idx_arr[N]>(systems), std::forward<t_data>(data)...);     \
			}                                                                                             \
			else                                                                                          \
			{                                                                                             \
				_run_sys(t_sys_now {}, std::forward<t_data>(data)...);                                    \
			}                                                                                             \
		}                                                                                                 \
	}

		template <typename t_sys_cond, typename... t_sys>
		struct system_loop
		{
			static const constexpr std::array<std::size_t, sizeof...(t_sys)> not_empty_sys_idx_arr = make_not_empty_sys_idx_arr<t_sys...>();

			using t_all_sys_tpl		  = std::tuple<t_sys...>;
			using t_sys_not_empty_tpl = meta::filtered_tuple_t<meta::is_not_empty, t_sys...>;

			no_unique_addr t_sys_cond sys_cond;

			no_unique_addr t_sys_not_empty_tpl systems;

			template <typename... t_sys_not_empty>
			constexpr system_loop(t_sys_cond&& sys_cond, std::tuple<t_sys_not_empty...>&& systems)
				: sys_cond(std::forward<t_sys_cond>(sys_cond)),
				  systems(std::forward<std::tuple<t_sys_not_empty...>>(systems))
			{
			}

			template <typename... t_sys_not_empty>
			constexpr system_loop(t_sys_cond&& sys_cond, t_sys_not_empty&&... sys)
				: sys_cond(std::forward<t_sys_cond>(sys_cond)),
				  systems(std::forward<t_sys>(sys)...)
			{
			}

			constexpr system_loop() = default;

			template <typename... t_data>
			void run(t_data&&... data)
			{
				run_impl(std::index_sequence_for<t_sys...> {}, std::forward<t_data>(data)...);
			}

		  private:
			template <std::size_t... i, typename... t_data>
			inline void run_impl(std::index_sequence<i...>, t_data&&... data)
			{
				while (_run_sys(sys_cond, std::forward<t_data>(data)...))
				{
#define X(N) __SYS_LOOP_IMPL(N)
					__X_REPEAT_LIST_512
#undef X
				}
			}
		};
	}	 // namespace detail

	using namespace ecs::system::detail;

	template <typename t_sys_cond>
	constexpr decltype(auto) break_if(t_sys_cond&& sys_cond)
	{
		return system_break_if<t_sys_cond>(std::forward<t_sys_cond>(sys_cond));
	}

	template <typename t_sys_cond>
	constexpr decltype(auto) continue_if(t_sys_cond&& sys_cond)
	{
		return system_continue_if<t_sys_cond>(std::forward<t_sys_cond>(sys_cond));
	}

	template <typename t_sys_cond, typename... t_sys>
	constexpr decltype(auto) loop(t_sys_cond&& sys_cond, t_sys&&... sys)
	{
		return system_loop<t_sys_cond, t_sys...>(
			std::forward<t_sys_cond>(sys_cond),
			meta::make_filtered_tuple<meta::is_not_empty>(std::forward<t_sys>(sys)...));
	}
}	 // namespace ecs::system
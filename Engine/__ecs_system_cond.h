#pragma once

namespace ecs::system
{
	namespace detail
	{
		template <typename t_sys_cond, typename t_sys_then, typename t_sys_else = void>
		struct system_cond;

		template <typename t_sys_cond, typename t_sys_then>
		struct system_cond<t_sys_cond, t_sys_then, void>
		{
			no_unique_addr t_sys_cond sys_cond;
			no_unique_addr t_sys_then sys_then;

			constexpr system_cond(t_sys_cond&& sys_cond, t_sys_then&& sys_then)
				: sys_cond(std::forward<t_sys_cond>(sys_cond)), sys_then(std::forward<t_sys_then>(sys_then)) { }

			constexpr system_cond() requires(std::is_empty_v<t_sys_cond> && std::is_empty_v<t_sys_then>)
			= default;

			template <typename... t_data>
			void run(t_data&&... data)
			{
				if (_run_sys(sys_cond, std::forward<t_data>(data)...))
				{
					_run_sys(sys_then, std::forward<t_data>(data)...);
				}
			}
		};

		template <typename t_sys_cond, typename t_sys_then, typename t_sys_else>
		struct system_cond
		{
			no_unique_addr t_sys_cond sys_cond;
			no_unique_addr t_sys_then sys_then;
			no_unique_addr t_sys_else sys_else;

			constexpr system_cond(t_sys_cond&& sys_cond, t_sys_then&& sys_then, t_sys_else&& sys_else)
				: sys_cond(std::forward<t_sys_cond>(sys_cond)),
				  sys_then(std::forward<t_sys_then>(sys_then)),
				  sys_else(std::forward<t_sys_else>(sys_else)) { }

			constexpr system_cond() requires(std::is_empty_v<t_sys_cond> && std::is_empty_v<t_sys_then> && std::is_empty_v<t_sys_else>)
			= default;

			template <typename... t_data>
			void run(t_data&&... data)
			{
				if (_run_sys(sys_cond, std::forward<t_data>(data)...))
				{
					_run_sys(sys_then, std::forward<t_data>(data)...);
				}
				else
				{
					_run_sys(sys_else, std::forward<t_data>(data)...);
				}
			}
		};
	}	 // namespace detail

	using namespace ecs::system::detail;

	template <typename t_sys_cond, typename t_sys_then>
	decltype(auto) cond(t_sys_cond&& sys_cond, t_sys_then&& sys_then)
	{
		return system_cond<t_sys_cond, t_sys_then>(
			std::forward<t_sys_cond>(sys_cond),
			std::forward<t_sys_then>(sys_then));
	}

	template <typename t_sys_cond, typename t_sys_then, typename t_sys_else>
	decltype(auto) cond(t_sys_cond&& sys_cond, t_sys_then&& sys_then, t_sys_else&& sys_else)
	{
		return system_cond<t_sys_cond, t_sys_then, t_sys_else>(
			std::forward<t_sys_cond>(sys_cond),
			std::forward<t_sys_then>(sys_then),
			std::forward<t_sys_else>(sys_else));
	}
}	 // namespace ecs::system
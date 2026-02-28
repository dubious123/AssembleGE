#pragma once

namespace age::ecs::system
{
	template <typename t_sys>
	struct with_ctx
	{
		using t_ctx_tag = ctx_tag<tag_ctx_bound, tag_adaptor>;

		no_unique_addr t_sys sys;

		FORCE_INLINE constexpr with_ctx(auto&&... arg) noexcept(std::is_nothrow_constructible_v<t_sys, decltype(arg)...>)
			: sys{ FWD(arg)... }
		{
		}

		template <typename... t_arg>
		static consteval bool
		validate(age::meta::type_pack<t_sys, t_arg...>)
		{
			{
				constexpr auto valid = std::invocable<t_sys, t_arg...>;

				static_assert(valid, "[with_ctx] systems(...) returned invalid_sys_call - check that system is callable with given arguments.");
			}

			return true;
		}

		FORCE_INLINE constexpr decltype(auto)
		operator()(this auto&& self, cx_ctx auto&& ctx, auto&&... arg) noexcept
		{
			static_assert(validate(age::meta::type_pack<t_sys, decltype(ctx), decltype(arg)...>{}), "[with_ctx]: invalid with_ctx");

			return ctx.get_executor().execute(FWD(ctx), FWD(self).sys, FWD(ctx), FWD(arg)...);
		}
	};

	template <typename t_sys>
	with_ctx(t_sys&&) -> with_ctx<t_sys>;
}	 // namespace age::ecs::system

namespace age::ecs::system
{
	template <typename t_data>
	struct identity
	{
		using t_ctx_tag = age::ecs::system::ctx_tag<age::ecs::system::tag_adaptor>;

		no_unique_addr t_data data;

		FORCE_INLINE constexpr identity(auto&& arg) noexcept : data{ FWD(arg) } {};

		FORCE_INLINE constexpr decltype(auto)
		operator()(this auto&& self) noexcept
		{
			return std::move(FWD(self).data);
		}
	};

	template <typename t_arg>
	identity(t_arg&&) -> identity<t_arg>;
}	 // namespace age::ecs::system

namespace age::ecs::system
{
	struct sys_move
	{
		using t_ctx_tag = age::ecs::system::ctx_tag<age::ecs::system::tag_adaptor>;

		FORCE_INLINE constexpr decltype(auto)
		operator()(this auto&& self, auto&& arg) noexcept
		{
			return std::move(FWD(arg));
		}
	};
}	 // namespace age::ecs::system

namespace age::ecs::system
{
	template <typename t_data>
	struct sys_move_data
	{
		using t_ctx_tag = age::ecs::system::ctx_tag<age::ecs::system::tag_adaptor>;

		no_unique_addr t_data data;

		FORCE_INLINE constexpr sys_move_data(auto&& arg) noexcept : data{ FWD(arg) } {};

		FORCE_INLINE constexpr decltype(auto)
		operator()(auto&& arg) noexcept
		{
			return std::move(FWD(arg));
		}
	};

	template <typename t_arg>
	sys_move_data(t_arg&&) -> sys_move_data<t_arg>;
}	 // namespace age::ecs::system

namespace age::ecs::system
{
	template <typename t_sys>
	struct when
	{
		using t_ctx_tag = ctx_tag<tag_adaptor>;

		no_unique_addr t_sys sys_cond;

		FORCE_INLINE constexpr when(auto&& arg) noexcept : sys_cond{ FWD(arg) } {};

		FORCE_INLINE constexpr decltype(auto)
		operator()(auto&&... arg) noexcept
		{
			return run_sys(sys_cond, FWD(arg)...);
		}
	};

	template <typename t_arg>
	when(t_arg&&) -> when<t_arg>;

	template <typename t_sys>
	struct tap
	{
		using t_ctx_tag = ctx_tag<tag_adaptor>;

		no_unique_addr t_sys sys;

		FORCE_INLINE constexpr tap(auto&& arg) noexcept : sys{ FWD(arg) } {};

		FORCE_INLINE constexpr decltype(auto)
		operator()(auto&&... arg) noexcept
		{
			run_sys(sys, arg...);
			if constexpr (sizeof...(arg) == 0)
			{
				return (FWD(arg), ...);
			}
			else
			{
				return std::forward_as_tuple(FWD(arg)...);
			}
		}
	};

	template <typename t_arg>
	tap(t_arg&&) -> tap<t_arg>;
}	 // namespace age::ecs::system
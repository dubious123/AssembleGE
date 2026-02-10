#pragma once

namespace age::ecs::system
{
	template <typename... t_sys>
	struct pipe
	{
		using t_ctx_tag = ctx_tag<tag_adaptor, tag_ctx_bound>;

		no_unique_addr age::meta::compressed_pack<t_sys...> systems;

		FORCE_INLINE constexpr pipe(auto&&... sys) noexcept : systems{ FWD(sys)... } {};

	  private:
		template <std::size_t sys_idx>
		FORCE_INLINE constexpr decltype(auto)
		__run_impl(cx_ctx auto&& ctx, auto&&... arg)
		{
			static_assert(
				requires { {run_sys(FWD(ctx), systems.get<sys_idx>(), FWD(arg)...)} -> detail::cx_valid_sys_call; },
				"[pipe] invalid_sys_call - check that system left is callable with given arguments.");

			if constexpr (sys_idx + 1 == sizeof...(t_sys))
			{
				return run_sys(FWD(ctx), systems.get<sys_idx>(), FWD(arg)...);
			}
			else
			{
				using t_ret_l = decltype(run_sys(FWD(ctx), systems.get<sys_idx>(), FWD(arg)...));

				if constexpr (std::is_void_v<t_ret_l>)
				{
					run_sys(FWD(ctx), systems.get<sys_idx>(), FWD(arg)...);
					return __run_impl<sys_idx + 1>(FWD(ctx));
				}
				else
				{
					return __run_impl<sys_idx + 1>(FWD(ctx), run_sys(FWD(ctx), systems.get<sys_idx>(), FWD(arg)...));
				}
			}
		}

		template <std::size_t sys_idx>
		FORCE_INLINE constexpr decltype(auto)
		__run_impl(auto&&... arg)
		{
			static_assert(
				requires { {run_sys(systems.get<sys_idx>(), FWD(arg)...)}->detail::cx_valid_sys_call; },
				"[pipe] invalid_sys_call - check that system left is callable with given arguments.");

			if constexpr (sys_idx + 1 == sizeof...(t_sys))
			{
				return run_sys(systems.get<sys_idx>(), FWD(arg)...);
			}
			else
			{
				using t_ret_l = decltype(run_sys(systems.get<sys_idx>(), FWD(arg)...));

				if constexpr (std::is_void_v<t_ret_l>)
				{
					run_sys(systems.get<sys_idx>(), FWD(arg)...);
					return __run_impl<sys_idx + 1>();
				}
				else
				{
					return __run_impl<sys_idx + 1>(run_sys(systems.get<sys_idx>(), FWD(arg)...));
				}
			}
		}

	  public:
		FORCE_INLINE constexpr decltype(auto)
		operator()(cx_ctx auto&& ctx, auto&&... arg) noexcept
		{
			return __run_impl<0>(FWD(ctx), FWD(arg)...);
		}

		FORCE_INLINE constexpr decltype(auto)
		operator()(auto&&... arg) noexcept
		{
			return __run_impl<0>(FWD(arg)...);
		}
	};

	template <typename... t_sys>
	pipe(t_sys&&...) -> pipe<t_sys...>;

	FORCE_INLINE constexpr decltype(auto)
	operator|(auto&& sys_l, auto&& sys_r) noexcept
	{
		return pipe{ FWD(sys_l), FWD(sys_r) };
	}

	template <typename... t_sys_l, typename t_sys_r>
	FORCE_INLINE constexpr decltype(auto)
	operator|(pipe<t_sys_l...>&& sys_l, t_sys_r&& sys_r) noexcept
	{
		return []<std::size_t... i> INLINE_LAMBDA_FRONT(std::index_sequence<i...>, auto&& sys_l, auto&& sys_r) noexcept INLINE_LAMBDA_BACK -> decltype(auto) {
			return pipe{ FWD(sys_l).systems.template get<i>()..., FWD(sys_r) };
		}(std::index_sequence_for<t_sys_l...>{}, FWD(sys_l), FWD(sys_r));
	}

	template <typename t_sys_l, typename... t_sys_r>
	FORCE_INLINE constexpr decltype(auto)
	operator|(t_sys_l&& sys_l, pipe<t_sys_r...>&& sys_r) noexcept
	{
		return []<std::size_t... i> INLINE_LAMBDA_FRONT(std::index_sequence<i...>, auto&& sys_l, auto&& sys_r) noexcept INLINE_LAMBDA_BACK -> decltype(auto) {
			return pipe{ FWD(sys_l), FWD(sys_r).systems.template get<i>()... };
		}(std::index_sequence_for<t_sys_r...>{}, FWD(sys_l), FWD(sys_r));
	}

	template <typename... t_sys_l, typename... t_sys_r>
	FORCE_INLINE constexpr decltype(auto)
	operator|(pipe<t_sys_l...>&& sys_l, pipe<t_sys_r...>&& sys_r) noexcept
	{
		return []<std::size_t... i_l, std::size_t... i_r> INLINE_LAMBDA_FRONT(std::index_sequence<i_l...>, std::index_sequence<i_r...>, auto&& sys_l, auto&& sys_r) noexcept INLINE_LAMBDA_BACK -> decltype(auto) {
			return pipe{ FWD(sys_l).systems.template get<i_l>()..., FWD(sys_r).systems.template get<i_r>()... };
		}(std::index_sequence_for<t_sys_l...>{}, std::index_sequence_for<t_sys_r...>{}, FWD(sys_l), FWD(sys_r));
	}
}	 // namespace age::ecs::system
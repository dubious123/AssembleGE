#pragma once
#ifndef INCLUDED_FROM_ECS_SYSTEM_HEADER
	#error "Do not include this file directly. Include <__ecs_system.h> instead."
#endif

namespace ecs::system::ctx
{
	using namespace detail;

	template <typename... t_sys>
	struct pipe : make_unique_bases<t_sys...>, adaptor_base, sys_ctx_bound
	{
		using t_unique_bases = make_unique_bases<t_sys...>;
		using t_unique_bases::t_unique_bases;

	  private:
		template <std::size_t nth>
		FORCE_INLINE constexpr decltype(auto)
		__run_impl(auto&& ctx, auto&&... arg)
		{
			// meta::print_type<decltype(get_nth_base<nth>())>();
			//  static_assert(
			//	requires { FWD(ctx).execute(get_nth_base<nth>(), FWD(arg)...); },
			//	"[pipe] invalid_sys_call - check that system left is callable with given arguments.");

			// if constexpr (nth + 1 == sizeof...(t_sys))
			//{
			//	return FWD(ctx).execute(get_nth_base<nth>(), FWD(arg)...);
			// }
			// else
			//{
			//	using t_ret_l = decltype(FWD(ctx).execute(get_nth_base<nth>(), FWD(arg)...));

			//	if constexpr (std::is_void_v<t_ret_l>)
			//	{
			//		FWD(ctx).execute(get_nth_base<nth>(), FWD(arg)...);
			//		return __run_impl<nth + 1>(FWD(ctx));
			//	}
			//	else if constexpr (meta::tuple_like<t_ret_l>)
			//	{
			//		if constexpr (requires { meta::tuple_unpack([this, &ctx] INLINE_LAMBDA_FRONT(auto&&... l_ref_arg) mutable noexcept -> decltype(auto) INLINE_LAMBDA_BACK { return __run_impl<nth + 1>(FWD(ctx), FWD(l_ref_arg)...); }, std::declval<t_ret_l>()); })
			//		{
			//			return [this]<auto... i> INLINE_LAMBDA_FRONT(std::index_sequence<i...>, auto&& ctx, auto&& arg_tpl) mutable noexcept(noexcept(run_sys<nth + 1>(FWD(ctx), std::get<i>(arg_tpl)...))) INLINE_LAMBDA_BACK -> decltype(auto) {
			//				return __run_impl<nth + 1>(FWD(ctx), std::get<i>(arg_tpl)...);
			//			}(std::make_index_sequence<std::tuple_size_v<std::remove_cvref_t<t_ret_l>>>{}, FWD(ctx), FWD(ctx).execute(get_nth_base<nth>(), FWD(arg)...));
			//		}
			//		else
			//		{
			//			return __run_impl<nth + 1>(FWD(ctx), FWD(ctx).execute(get_nth_base<nth>(), FWD(arg)...));
			//		}
			//	}
			//	else
			//	{
			//		return __run_impl<nth + 1>(FWD(ctx), FWD(ctx).execute(get_nth_base<nth>(), FWD(arg)...));
			//	}
			//}
		}

	  public:
		FORCE_INLINE constexpr decltype(auto)
		operator()(cx_ctx auto&& ctx, auto&&... arg) noexcept
		{
			return __run_impl<0>(FWD(ctx), FWD(arg)...);
		}
	};

	template <typename... t_sys>
	pipe(t_sys&&...)
		-> pipe<t_sys...>;

	template <typename t_sys_l, typename t_sys_r>
	FORCE_INLINE constexpr decltype(auto)
	operator|(t_sys_l&& sys_l, t_sys_r&& sys_r) noexcept
	{
		return pipe{ FWD(sys_l), FWD(sys_r) };
	}

	template <typename... t_sys_l, typename t_sys_r>
	FORCE_INLINE constexpr decltype(auto)
	operator|(pipe<t_sys_l...>&& sys_l, t_sys_r&& sys_r) noexcept
	{
		return []<std::size_t... i>(std::index_sequence<i...>, auto&& sys_l, auto&& sys_r) {
			return pipe{ FWD(sys_l).template get_nth_base<i>().value..., FWD(sys_r) };
		}(std::index_sequence_for<t_sys_l...>{}, FWD(sys_l), FWD(sys_r));
	}

	template <typename t_sys_l, typename... t_sys_r>
	FORCE_INLINE constexpr decltype(auto)
	operator|(t_sys_l&& sys_l, pipe<t_sys_r...>&& sys_r) noexcept
	{
		return []<std::size_t... i>(std::index_sequence<i...>, auto&& sys_l, auto&& sys_r) {
			return pipe{ FWD(sys_l), FWD(sys_r).template get_nth_base<i>().value... };
		}(std::index_sequence_for<t_sys_r...>{}, FWD(sys_l), FWD(sys_r));
	}

	template <typename... t_sys_l, typename... t_sys_r>
	FORCE_INLINE constexpr decltype(auto)
	operator|(pipe<t_sys_l...>&& sys_l, pipe<t_sys_r...>&& sys_r) noexcept
	{
		return []<std::size_t... i_l, std::size_t... i_r>(std::index_sequence<i_l...>, std::index_sequence<i_r...>, auto&& sys_l, auto&& sys_r) {
			return pipe{ FWD(sys_l).template get_nth_base<i_l>().value..., FWD(sys_r).template get_nth_base<i_r>().value... };
		}(std::index_sequence_for<t_sys_l...>{}, std::index_sequence_for<t_sys_r...>{}, FWD(sys_l), FWD(sys_r));
	}
}	 // namespace ecs::system::ctx
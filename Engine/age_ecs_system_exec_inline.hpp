#pragma once

namespace age::ecs::system
{
	namespace detail
	{
		// return (a, a+1, ..., b-1, b)
		template <std::size_t a, std::size_t b>
		using index_range_t = decltype([]<auto... i>(std::index_sequence<i...>) {
			return std::index_sequence<(a + i)...>{};
		}(std::make_index_sequence<b - a + 1>{}));

		template <std::size_t arr_size, typename t_filtered_idx_seq>
		struct index_ranges_seq;

		template <std::size_t arr_size>
		struct index_ranges_seq<arr_size, std::index_sequence<>>
		{
			using type = age::meta::type_pack<index_range_t<0, arr_size>>;
		};

		template <std::size_t arr_size, std::size_t idx_head>
		struct index_ranges_seq<arr_size, std::index_sequence<idx_head>>
		{
			using type = age::meta::type_pack<>;
		};

		template <std::size_t arr_size, std::size_t idx_head, std::size_t idx_next, std::size_t... idx_tail>
		struct index_ranges_seq<arr_size, std::index_sequence<idx_head, idx_next, idx_tail...>>
		{
			using type = age::meta::type_pack_cat_t<
				age::meta::type_pack<index_range_t<idx_head, idx_next>>,
				typename index_ranges_seq<arr_size, std::index_sequence<idx_next + 1, idx_tail...>>::type>;
		};

		template <std::size_t arr_size, typename t_filtered_idx_seq>
		using index_ranges_seq_t = index_ranges_seq<
			arr_size,
			age::meta::index_sequence_cat_t<
				std::index_sequence<0>,
				decltype([] {
					if constexpr (age::meta::index_sequence_size_v<t_filtered_idx_seq> == 0)
					{
						return std::index_sequence<>{};
					}
					else
					{
						return age::meta::pop_back_seq_t<t_filtered_idx_seq>{};
					}
				}()),
				std::index_sequence<arr_size>>>::type;
	}	 // namespace detail

	struct exec_inline
	{
		using t_ctx_tag = ctx_tag<tag_executor>;

	  private:
		template <std::size_t... sys_idx>
		FORCE_INLINE static constexpr decltype(auto)
		__run_sys_impl_seq(auto&& ctx, std::index_sequence<sys_idx...>, auto&&... arg) noexcept
		{
			return (run_sys(FWD(ctx), FWD(ctx).template get_sys<sys_idx>(), FWD(arg)...), ...);
		}

		template <std::size_t... sys_idx>
		FORCE_INLINE static constexpr decltype(auto)
		__run_all_impl_2(auto&& ctx, std::index_sequence<sys_idx...>, auto&&... arg) noexcept
		{
			using t_sys_res_not_void =
				age::meta::filtered_index_sequence_t<
					age::meta::is_not_void,
					decltype(run_sys(FWD(ctx), FWD(ctx).template get_sys<sys_idx>(), FWD(arg)...))...>;

			static_assert(age::meta::index_sequence_empty_v<t_sys_res_not_void> is_false);

			if constexpr (constexpr auto has_trailing_voids =
							  std::is_void_v<
								  decltype(run_sys(FWD(ctx), FWD(ctx).template get_sys<age::meta::variadic_auto_back_v<sys_idx...>>(), FWD(arg)...))>)
			{
				constexpr auto idx_head			= age::meta::variadic_auto_front_v<sys_idx...>;
				constexpr auto idx_res_not_void = age::meta::variadic_auto_at_v<age::meta::index_sequence_front_v<t_sys_res_not_void>, sys_idx...>;
				constexpr auto idx_tail			= age::meta::variadic_auto_back_v<sys_idx...>;

				auto after_return = age::util::scope_guard{ [&ctx, &arg...] INLINE_LAMBDA_FRONT mutable noexcept INLINE_LAMBDA_BACK {
					__run_sys_impl_seq(ctx, age::meta::offset_sequence<idx_res_not_void + 1, idx_tail - idx_res_not_void>{}, FWD(arg)...);
				} };

				return __run_sys_impl_seq(ctx, age::meta::offset_sequence<idx_head, idx_res_not_void - idx_head + 1>{}, FWD(arg)...);
			}
			else
			{
				return __run_sys_impl_seq(ctx, std::index_sequence<sys_idx...>{}, FWD(arg)...);
			}
		}

		template <typename... t_sys_idx_seq>
		FORCE_INLINE static constexpr decltype(auto)
		__run_all_impl_1(auto&& ctx, age::meta::type_pack<t_sys_idx_seq...>, auto&&... arg) noexcept
		{
			return std::tuple<decltype(__run_all_impl_2(FWD(ctx), t_sys_idx_seq{}, arg...))...>{
				__run_all_impl_2(FWD(ctx), t_sys_idx_seq{}, arg...)...
			};
		}

	  public:
		FORCE_INLINE constexpr decltype(auto)
		execute(this auto&& self, cx_ctx auto&& ctx, auto&& sys, auto&&... arg) noexcept
		{
			return detail::sys_invoke(FWD(sys), FWD(ctx), FWD(arg)...);
		}

		template <std::size_t... sys_idx>
		FORCE_INLINE constexpr decltype(auto)
		run_all(this auto&& self, cx_ctx auto&& ctx, std::index_sequence<sys_idx...>, auto&&... arg) noexcept
		{
			using t_sys_res_not_void =
				age::meta::filtered_index_sequence_t<
					age::meta::is_not_void,
					decltype(run_sys(FWD(ctx), FWD(ctx).template get_sys<sys_idx>(), arg...))...>;

			if constexpr (sizeof...(sys_idx) <= 1)
			{
				return (run_sys(FWD(ctx), FWD(ctx).template get_sys<sys_idx>(), FWD(arg)...), ...);
			}
			else if constexpr (age::meta::index_sequence_empty_v<t_sys_res_not_void>)
			{
				(run_sys(FWD(ctx), FWD(ctx).template get_sys<sys_idx>(), arg...), ...);
			}
			else
			{
				return __run_all_impl_1(FWD(ctx), detail::template index_ranges_seq_t<sizeof...(sys_idx) - 1, t_sys_res_not_void>{}, FWD(arg)...);
			}
		}
	};
}	 // namespace age::ecs::system
#pragma once

namespace age::ecs::system
{
	template <typename... t>
	struct on_ctx
	{
		using t_ctx_tag = ctx_tag<tag_ctx>;

		template <typename t_other>
		using sys_pred = std::bool_constant<not cx_executor<t_other>>;

		static inline constexpr auto executor_idx = age::meta::variadic_index_v<cx_executor_pred, t...>;
		static inline constexpr auto sys_idx_seq  = age::meta::make_filtered_index_sequence<sys_pred, t...>();
		static inline constexpr auto sys_count	  = age::meta::index_sequence_size_v<decltype(sys_idx_seq)>;

		no_unique_addr age::meta::compressed_pack<t...> storage;

		FORCE_INLINE constexpr on_ctx(auto&&... arg) noexcept : storage{ FWD(arg)... } {};

		FORCE_INLINE constexpr decltype(auto)
		get_executor(this auto&& self) noexcept
		{
			return FWD(self).storage.get<executor_idx>();
		}

		template <std::size_t nth>
		FORCE_INLINE constexpr decltype(auto)
		get_sys(this auto&& self) noexcept
		{
			static_assert(nth < meta::index_sequence_size_v<decltype(sys_idx_seq)>);

			return FWD(self).storage.get<meta::index_sequence_at_v<nth, decltype(sys_idx_seq)>>();
		}

		FORCE_INLINE constexpr decltype(auto)
		operator()(this auto&& self, auto&&... arg) noexcept
		{
			return FWD(self).storage.get<executor_idx>().run_all(FWD(self), std::make_index_sequence<sys_count>{}, FWD(arg)...);
		}
	};

	template <typename... t>
	on_ctx(t&&...) -> on_ctx<t...>;
}	 // namespace age::ecs::system
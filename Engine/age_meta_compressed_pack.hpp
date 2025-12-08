#pragma once

namespace age::meta
{
	template <typename... t>
	struct compressed_pack;

	template <>
	struct compressed_pack<>
	{
	};

	template <typename t_head>
	struct compressed_pack<t_head>
	{
		no_unique_addr t_head head;

		constexpr compressed_pack(auto&& h_arg)
			: head{ FWD(h_arg) }
		{
		}

		template <std::size_t nth>
		FORCE_INLINE constexpr decltype(auto)
		get(this auto&& self) noexcept
		{
			if constexpr (nth == 0)
			{
				return static_cast<meta::copy_cv_ref_t<decltype(self), t_head>>(FWD(self).head);
			}
			else
			{
				static_assert(false, "index out of range");
			}
		}
	};

	template <typename t_head, typename... t_tail>
	struct compressed_pack<t_head, t_tail...>
	{
		no_unique_addr t_head head;
		no_unique_addr compressed_pack<t_tail...> tail;

		constexpr compressed_pack() = default;

		constexpr compressed_pack(auto&& h_arg, auto&&... t_arg)
			: head{ FWD(h_arg) }, tail{ FWD(t_arg)... }
		{
		}

		template <std::size_t nth>
		FORCE_INLINE constexpr decltype(auto)
		get(this auto&& self) noexcept
		{
			if constexpr (nth == 0)
			{
				return static_cast<meta::copy_cv_ref_t<decltype(self), t_head>>(FWD(self).head);
			}
			else
			{
				return FWD(self).tail.template get<nth - 1>();
			}
		}
	};

	template <typename... t>
	compressed_pack(t&&... arg) -> compressed_pack<t...>;
}	 // namespace age::meta
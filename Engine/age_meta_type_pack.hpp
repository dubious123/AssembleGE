#pragma once
#if defined(__INTELLISENSE__)
	#include "age_meta.hpp"
#endif

namespace age::meta
{
	template <typename... t>
	struct type_pack
	{
		template <std::size_t i>
		using t_nth = variadic_at_t<i, t...>;

		using t_tpl_ref = std::tuple<std::add_lvalue_reference_t<t>...>;

		static constexpr auto size = sizeof...(t);

		template <typename t_r>
		static consteval bool
		contains() noexcept
		{
			return meta::variadic_contains_v<t_r, t...>;
		}
	};

	consteval auto
	cat(auto v)
	{
		return v;
	}

	template <template <typename...> typename t_container, typename... t_l, typename... t_r>
	consteval auto
	cat(t_container<t_l...>, t_container<t_r...>)
	{
		return t_container<t_l..., t_r...>{};
	}

	consteval auto
	cat(auto h0, auto h1, auto... tail)
		requires(sizeof...(tail) > 0)
	{
		return cat(cat(h0, h1), tail...);
	}

	consteval auto
	combine(auto v)
	{
		return v;
	}

	template <template <typename...> typename t_container, typename... t_l, typename... t_r>
	consteval auto
	combine(t_container<t_l...>, t_container<t_r...>)
	{
		constexpr auto l = []<typename t>() constexpr {
			if constexpr (meta::variadic_contains_v<t, t_r...>)
			{
				return t_container<>{};
			}
			else
			{
				return t_container<t>{};
			}
		};

		if constexpr (sizeof...(t_r) == 0)
		{
			return t_container<t_l...>{};
		}
		else
		{
			return cat(l.template operator()<t_l>()..., t_container<t_r...>{});
		}
	}

	consteval auto
	combine(auto h0, auto h1, auto... tail)
		requires(sizeof...(tail) > 0)
	{
		return combine(combine(h0, h1), tail...);
	}

	consteval auto
	intersect(auto v)
	{
		return v;
	}

	template <template <typename...> typename t_container, typename... t_l, typename... t_r>
	consteval auto
	intersect(t_container<t_l...>, t_container<t_r...>)
	{
		constexpr auto l = []<typename t>() constexpr {
			if constexpr (meta::variadic_contains_v<t, t_r...>)
			{
				return t_container<t>{};
			}
			else
			{
				return t_container<>{};
			}
		};

		if constexpr (sizeof...(t_r) == 0)
		{
			return t_container<>{};
		}
		else
		{
			return cat(l.template operator()<t_l>()...);
		}
	}

	consteval auto
	intersect(auto h0, auto h1, auto... tail)
		requires(sizeof...(tail) > 0)
	{
		return intersect(intersect(h0, h1), tail...);
	}

	consteval auto
	sub(auto v)
	{
		return v;
	}

	template <template <typename...> typename t_container, typename... t, typename... t_rem>
	consteval auto
	sub(t_container<t...>, t_container<t_rem...>)
	{
		constexpr auto l = []<typename t>() constexpr {
			if constexpr (meta::variadic_contains_v<t, t_rem...>)
			{
				return t_container<>{};
			}
			else
			{
				return t_container<t>{};
			}
		};

		if constexpr (sizeof...(t_rem) == 0)
		{
			return t_container<t...>{};
		}
		else
		{
			return cat(l.template operator()<t, t_rem...>()...);
		}
	}

	consteval auto
	sub(auto h0, auto rem0, auto... rem1)
		requires(sizeof...(rem1) > 0)
	{
		return sub(sub(h0, rem0), rem1...);
	}

	template <template <typename> typename t_filter, template <typename...> typename t_container, typename... t>
	consteval auto
	filter(t_container<t...>)
	{
		constexpr auto l = []<typename t>() constexpr {
			if constexpr (t_filter<t>::value)
			{
				return t_container<t>{};
			}
			else
			{
				return t_container<>{};
			}
		};

		if constexpr (sizeof...(t) == 0)
		{
			return t_container{};
		}
		else
		{
			return cat(l.template operator()<t>()...);
		}
	}

	template <template <typename> typename t_filter, template <typename...> typename t_container, typename... t>
	consteval auto
	filter_not(t_container<t...>)
	{
		constexpr auto l = []<typename t>() constexpr {
			if constexpr (t_filter<t>::value)
			{
				return t_container<>{};
			}
			else
			{
				return t_container<t>{};
			}
		};

		if constexpr (sizeof...(t) == 0)
		{
			return t_container{};
		}
		else
		{
			return cat(l.template operator()<t>()...);
		}
	}

	template <template <typename> typename t_transform, template <typename...> typename t_container, typename... t>
	consteval auto
	transform(t_container<t...>)
	{
		if constexpr (sizeof...(t) == 0)
		{
			return t_container{};
		}
		else
		{
			return t_container<typename t_transform<t>::type...>{};
		}
	}

	template <template <typename...> typename t_container, typename... t>
	consteval auto
	make_unique(t_container<t...>)
	{
		if constexpr (sizeof...(t) == 0)
		{
			return t_container{};
		}
		else
		{
			return combine(t_container<t>{}...);
		}
	}

	template <typename... t>
	using cat_t = decltype(cat(t{}...));

	template <typename... t>
	using combine_t = decltype(combine(t{}...));

	template <typename... t>
	using sub_t = decltype(sub(t{}...));

	template <typename h, typename... t>
	using intersect_t = decltype(intersect(h{}, t{}...));

	template <template <typename> typename t_filter, typename t>
	using filter_t = decltype(filter<t_filter>(t{}));

	template <template <typename> typename t_filter, typename t>
	using filter_not_t = decltype(filter_not<t_filter>(t{}));

	template <template <typename> typename t_transform, typename t>
	using transform_t = decltype(transform<t_transform>(t{}));

	template <typename t>
	using make_unique_t = decltype(make_unique(t{}));

	template <template <typename...> typename t_tpl, typename... t>
	auto tpl_to_type_pack_helper(t_tpl<t...>) -> type_pack<t...>;

	template <meta::tuple_like t_tpl_like>
	using tpl_to_type_pack = decltype(tpl_to_type_pack_helper(std::declval<t_tpl_like>()));
}	 // namespace age::meta
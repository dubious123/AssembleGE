#pragma once

namespace age::meta
{
	template <typename t, typename u>
	struct copy_cv_ref
	{
	  private:
		using r	 = std::remove_reference_t<t>;
		using u1 = std::conditional_t<std::is_const_v<r>, std::add_const_t<u>, u>;
		using u2 = std::conditional_t<std::is_volatile_v<r>, std::add_volatile_t<u1>, u1>;
		using u3 = std::conditional_t<std::is_lvalue_reference_v<t>, std::add_lvalue_reference_t<u2>, u2>;
		using u4 = std::conditional_t<std::is_rvalue_reference_v<t>, std::add_rvalue_reference_t<u3>, u3>;

	  public:
		using type = u4;
	};

	template <typename t_from, typename t_to>
	using copy_cv_ref_t = copy_cv_ref<t_from, t_to>::type;

	template <auto v>
	struct auto_wrapper
	{
		constexpr static inline const auto value = v;
	};

	template <typename t>
	struct type_wrapper
	{
		using type = t;
	};

	template <template <typename...> typename t>
	struct template_wrapper
	{
		template <typename... ts>
		using type = t<ts...>;
	};

	template <typename t, typename u>
	concept not_same_as = (not std::same_as<std::remove_cvref_t<t>, std::remove_cvref_t<u>>);

	// not standard yet
	template <typename t>
	inline constexpr bool is_tuple_like_v = false;

	template <typename... t>
	inline constexpr bool is_tuple_like_v<std::tuple<t...>> = true;

	template <typename t1, typename t2>
	inline constexpr bool is_tuple_like_v<std::pair<t1, t2>> = true;

	template <typename t, size_t n>
	inline constexpr bool is_tuple_like_v<std::array<t, n>> = true;

	template <typename t_it, typename t_sent, std::ranges::subrange_kind t_kind>
	inline constexpr bool is_tuple_like_v<std::ranges::subrange<t_it, t_sent, t_kind>> = true;

	template <typename t>
	concept tuple_like = is_tuple_like_v<std::remove_cvref_t<t>>;

	template <typename t>
	concept pair_like = tuple_like<t> and std::tuple_size_v<std::remove_cvref_t<t>> == 2;

	template <typename t>
	struct is_not_void : std::bool_constant<not std::is_void_v<t>>
	{
	};

	template <typename t>
	inline constexpr bool is_not_void_v = meta::is_not_void<t>::value;

	template <typename t, typename u>
	inline constexpr bool is_not_same_v = not std::is_same_v<t, u>;

	template <std::size_t i>
	FORCE_INLINE constexpr decltype(auto)
	variadic_get(auto&& h, auto&&... t) noexcept
	{
		static_assert(i < 1 + sizeof...(t), "index out of range");

		if constexpr (i == 0)
		{
			return FWD(h);
		}
		else
		{
			return variadic_get<i - 1>(FWD(t)...);
		}
	}

	template <typename t, typename h, typename... ts>
	consteval bool
	variadic_contains()
	{
		if constexpr (std::is_same_v<t, h>)
		{
			return true;
		}
		else if constexpr (sizeof...(ts) == 0)
		{
			return false;
		}
		else
		{
			return variadic_contains<t, ts...>();
		}
	};

	template <auto v, auto h, auto... ts>
	consteval bool
	variadic_contains()
	{

		if constexpr (std::is_same_v<auto_wrapper<v>, auto_wrapper<h>>)
		{
			return true;
		}
		else if constexpr (sizeof...(ts) == 0)
		{
			return false;
		}
		else
		{
			return variadic_contains<v, ts...>();
		}
	};

	template <auto v>
	consteval bool
	variadic_contains()
	{
		return false;
	};

	template <typename t, typename h, typename... ts>
	constexpr inline auto variadic_constains_v = variadic_contains<t, h, ts...>();

	template <auto t, auto h, auto... ts>
	constexpr inline auto variadic_auto_constains_v = variadic_contains<t, h, ts...>();

	template <typename t, typename ret, typename... args>
	consteval bool
	param_constains(ret (*func)(args...))
	{
		return meta::variadic_constains_v<t, args...>;
	};

	template <typename t, auto f>
	constexpr inline auto param_constains_v = param_constains<t>(f);

	template <unsigned int idx, typename... tails>
	struct variadic_at;

	template <unsigned int idx, typename head, typename... tails>
	struct variadic_at<idx, head, tails...>
	{
		using type = variadic_at<idx - 1, tails...>::type;
	};

	template <typename head, typename... tails>
	struct variadic_at<0u, head, tails...>
	{
		using type = head;
	};

	// c++26 msvc! do your job!
	// template <unsigned int idx, typename... args>
	// struct variadic_at
	//{
	//	using type = args...[idx];
	//};

	template <unsigned int idx, typename... args>
	using variadic_at_t = variadic_at<idx, args...>::type;

	template <typename... t>
	using variadic_front_t = variadic_at_t<0, t...>;

	template <typename... t>
	using variadic_back_t = variadic_at_t<sizeof...(t) - 1, t...>;

	template <unsigned int idx, auto head, auto... tails>
	struct variadic_auto_at
	{
		static inline constexpr const auto value = variadic_auto_at<idx - 1, tails...>::value;
	};

	template <auto head, auto... tails>
	struct variadic_auto_at<0u, head, tails...>
	{
		static inline constexpr const auto value = head;
	};

	template <unsigned int idx, auto... tails>
	inline constexpr auto variadic_auto_at_v = variadic_auto_at<idx, tails...>::value;

	template <auto... i>
	inline constexpr auto variadic_auto_back_v = variadic_auto_at<sizeof...(i) - 1, i...>::value;

	template <auto... i>
	inline constexpr auto variadic_auto_front_v = variadic_auto_at<0, i...>::value;

	template <template <typename...> typename pred, typename t_default, typename... t>
	struct variadic_find
	{
		using type = t_default;
	};

	template <template <typename...> typename pred, typename t_default, typename t_head, typename... t_tail>
	struct variadic_find<pred, t_default, t_head, t_tail...>
	{
		using type = std::conditional_t<
			pred<t_head>::value,
			t_head,
			typename variadic_find<pred, t_default, t_tail...>::type>;
	};

	template <template <typename...> typename pred, typename t_default, typename... t>
	using variadic_find_t = variadic_find<pred, t_default, t...>::type;

	template <typename t_target>
	struct pred
	{
		template <typename t_other>
		using is_same = std::is_same<t_target, t_other>;
	};

	template <typename t>
	struct index_sequence_size;

	template <std::size_t... i>
	struct index_sequence_size<std::index_sequence<i...>>
	{
		static constexpr std::size_t value = sizeof...(i);
	};

	template <typename t_seq>
	inline constexpr auto index_sequence_size_v = index_sequence_size<std::remove_cvref_t<t_seq>>::value;

	template <typename t_seq>
	inline constexpr bool index_sequence_empty_v = index_sequence_size_v<std::remove_cvref_t<t_seq>> == 0;

	template <std::size_t i, typename seq>
	struct index_sequence_at;

	template <std::size_t i, std::size_t... idx>
	struct index_sequence_at<i, std::index_sequence<idx...>>
	{
		static constexpr std::size_t value = variadic_auto_at_v<i, idx...>;
	};

	template <std::size_t i, typename t_seq>
	inline constexpr auto index_sequence_at_v = index_sequence_at<i, std::remove_cvref_t<t_seq>>::value;

	template <typename t_seq>
	inline constexpr auto index_sequence_back_v = index_sequence_at_v<index_sequence_size_v<std::remove_cvref_t<t_seq>> - 1, std::remove_cvref_t<t_seq>>;

	template <typename t_seq>
	inline constexpr auto index_sequence_front_v = index_sequence_at_v<0, std::remove_cvref_t<t_seq>>;

	template <std::size_t idx, auto func>
	struct param_at;

	// template <std::size_t i, typename ret, typename... args, ret(func)(args...)>
	// struct param_at<i, func>
	//{
	//	using type = variadic_at_t<i, args...>;	   // std::tuple_element_t<i, std::tuple<args...>>;
	// };

	template <std::size_t i, typename ret, typename... args, ret (*func)(args...)>
	struct param_at<i, func>
	{
		using type = variadic_at_t<i, args...>;	   // std::tuple_element_t<i, std::tuple<args...>>;
	};

	template <std::size_t i, typename ret, typename c, typename... args, ret (c::*func)(args...)>
	struct param_at<i, func>
	{
		using type = variadic_at_t<i, args...>;	   // std::tuple_element_t<i, std::tuple<args...>>;
	};

	template <std::size_t i, typename ret, typename c, typename... args, ret (c::*func)(args...) const>
	struct param_at<i, func>
	{
		using type = variadic_at_t<i, args...>;	   // std::tuple_element_t<i, std::tuple<args...>>;
	};

	template <auto func>
	struct function_traits;

	// Specialization for global or static function.
	// template <typename ret, typename... args, ret(func)(args...)>
	// struct function_traits<func>
	//{
	//	using return_type				   = ret;
	//	using argument_types			   = std::tuple<args...>;
	//	static constexpr std::size_t arity = sizeof...(args);

	//	// template <std::size_t i>
	//	// using param_at = std::conditional_t<sizeof...(args) == 0, void, variadic_at_t<i, args...>>;
	//};

	// Specialization for function pointers.
	template <typename ret, typename... args, ret (*func)(args...)>
	struct function_traits<func>
	{
		using return_type				   = ret;
		using argument_types			   = std::tuple<args...>;
		static constexpr std::size_t arity = sizeof...(args);

		// template <std::size_t i>
		// using param_at = std::conditional_t<sizeof...(args) == 0, void, variadic_at_t<i, args...>>;
	};

	// Specialization for const member function pointers.
	template <typename ret, typename c, typename... args, ret (c::*func)(args...) const>
	struct function_traits<func>
	{
		using return_type				   = ret;
		using class_type				   = c;
		using argument_types			   = std::tuple<args...>;
		static constexpr std::size_t arity = sizeof...(args);

		// template <std::size_t i>
		// using param_at = std::conditional_t<sizeof...(args) == 0, void, variadic_at_t<i, args...>>;
	};

	// Specialization for non-const member function pointers.
	template <typename ret, typename c, typename... args, ret (c::*func)(args...)>
	struct function_traits<func>
	{
		using return_type				   = ret;
		using class_type				   = c;
		using argument_types			   = std::tuple<args...>;
		static constexpr std::size_t arity = sizeof...(args);

		// template <std::size_t i>
		// using param_at = std::conditional_t<sizeof...(args) == 0, void, variadic_at_t<i, args...>>;
	};

	template <auto callable>
	struct callable_traits : function_traits<&decltype(callable)::operator()>
	{
	};

	// template <typename t_ret, typename t_callable, typename... t_arg>
	// struct callable_traits<t_ret (t_callable::*)(t_arg...)>
	//{
	//	using return_type	 = t_ret;
	//	using callable_type	 = t_callable;
	//	using argument_types = std::tuple<t_arg...>;

	//	template <std::size_t i>
	//	using arg_type = typename std::tuple_element<i, argument_types>::type;
	//};

	// template <typename t_ret, typename t_callable, typename... t_arg>
	// struct callable_traits<t_ret (t_callable::*)(t_arg...) const>
	//{
	//	using return_type	 = t_ret;
	//	using callable_type	 = t_callable;
	//	using argument_types = std::tuple<t_arg...>;

	//	template <std::size_t i>
	//	using arg_type = typename std::tuple_element<i, argument_types>::type;
	//};

	// template <std::size_t i, typename ret, typename c, typename... args>
	// struct param_at<i, ret(c::*)(args...)>
	//{
	//	using type = variadic_at_t<i, args...>;	   // std::tuple_element_t<i, std::tuple<args...>>;
	// };

	// template <std::size_t i, typename ret, typename... args, ret (*func)(args..., ...)>
	// struct param_at<i, func>
	//{
	//	using type = variadic_at_t<i, args...>;	   // std::tuple_element_t<i, std::tuple<args...>>;
	// };

	// template <std::size_t idx, auto func>
	// struct param_count;

	// template <std::size_t i, typename ret, typename... args, ret (*func)(args...)>
	// struct param_count<i, func>
	//{
	//	static const constinit auto value = sizeof...(args);
	// };

	// template <std::size_t idx, auto func>
	// inline constexpr const auto param_count = param_at<idx, func>::value;

	template <typename t, typename head, typename... tails>
	consteval std::size_t
	get_variadic_index()
	{
		if constexpr (std::is_same_v<t, head>)
		{
			return 0;
		}
		else
		{
			static_assert(sizeof...(tails) > 0 and "target is not found in <head, tails...>");
			return 1 + get_variadic_index<t, tails...>();
		}
	}

	template <template <typename> typename t_pred, typename head, typename... tails>
	consteval std::size_t
	get_variadic_index()
	{
		if constexpr (t_pred<head>::value)
		{
			return 0;
		}
		else
		{
			static_assert(sizeof...(tails) > 0 and "target is not found in <head, tails...>");
			return 1 + get_variadic_index<t_pred, tails...>();
		}
	}

	// template <typename t, typename h, typename... ts>
	// inline constexpr const auto variadic_index_v = get_variadic_index<pred<t>::template is_same, h, ts...>();

	template <template <typename> typename t_pred, typename h, typename... ts>
	inline constexpr const auto variadic_index_v = get_variadic_index<t_pred, h, ts...>();

	template <auto t, auto head, auto... tails>
	consteval std::size_t
	get_variadic_auto_index()
	{
		return get_variadic_index<auto_wrapper<t>, auto_wrapper<head>, auto_wrapper<tails>...>();
		// return variadic_index<auto_wrapper<t>, auto_wrapper<head>, auto_wrapper<tails>...>; ... compile error... why?
	}

	template <auto t, auto h, auto... ts>
	inline constexpr unsigned int variadic_auto_index_v = get_variadic_auto_index<t, h, ts...>();

	template <typename t, typename tuple>
	struct tuple_index;

	template <typename t, typename... ts>
	struct tuple_index<t, std::tuple<ts...>>
	{
		static constexpr std::size_t value = get_variadic_index<t, ts...>();
	};

	template <typename t, typename tuple>
	inline constexpr std::size_t tuple_index_v = tuple_index<t, tuple>::value;

	template <typename... ts>
	using tuple_cat_t = decltype(std::tuple_cat(std::declval<ts>()...));

	template <typename tpl, typename... ts>
	using tuple_append_t = tuple_cat_t<tpl, std::tuple<ts...>>;

	template <typename t, typename... ts>
	using tuple_remove_t = tuple_cat_t<
		std::conditional_t<
			std::is_same_v<t, ts>,
			std::tuple<>,
			std::tuple<ts>>...>;

	template <unsigned n, typename... ts>
	using tuple_remove_at_t = tuple_remove_t<variadic_at_t<n, ts...>, ts...>;

	template <template <typename, typename> typename comparator, typename... t>
	struct tuple_sort;

	template <template <typename, typename> typename comparator, typename h, typename... t>
	struct tuple_sort<comparator, h, t...>
	{
		using subset_l = tuple_cat_t<std::conditional_t<comparator<h, t>::value, std::tuple<t>, std::tuple<>>...>;
		using subset_r = tuple_cat_t<std::conditional_t<comparator<h, t>::value, std::tuple<>, std::tuple<t>>...>;
		using type	   = tuple_cat_t<typename tuple_sort<comparator, subset_l>::type, std::tuple<h>, typename tuple_sort<comparator, subset_r>::type>;
	};

	template <template <typename, typename> typename comparator, template <typename...> typename tuple, typename... t>
	struct tuple_sort<comparator, tuple<t...>> : tuple_sort<comparator, t...>
	{
	};

	template <template <typename, typename> typename comparator>
	struct tuple_sort<comparator>
	{
		using type = std::tuple<>;
	};

	template <template <typename, typename> typename comparator, template <typename...> typename tuple>
	struct tuple_sort<comparator, tuple<>>
	{
		using type = std::tuple<>;
	};

	template <template <typename, typename> typename comparator, template <typename...> typename tuple, typename h>
	struct tuple_sort<comparator, tuple<h>>
	{
		using type = std::tuple<h>;
	};

	// If 'comparator' models strict weak ordering, the result is always stable.
	// Otherwise, the result is not stable.
	//
	// pivot is always the head (first element).
	// partition_l : comparator<head, t> is true  // t < head (left partition)
	// partition_r : comparator<head, t> is false // t >= head (right partition)
	//             : == comp<t, head> (t > head) + (t equivalent to head)
	//
	// Note: comparator must be strict weak ordering.
	//       (i.e., comparator(a, b) == false && comparator(b, a) == false means a, b are equivalent.)
	//       Violating this may result in undefined behavior.
	//
	// Usage:
	//     using result = tuple_sort<my_comp, t1, t2, t3, ...>;
	template <template <typename, typename> typename comparator, typename... t>
	using tuple_sort_t = tuple_sort<comparator, t...>::type;

	template <template <typename, typename> typename comparator, typename... t>
	struct tuple_sort_stable;

	template <template <typename, typename> typename comparator, typename h, typename... t>
	struct tuple_sort_stable<comparator, h, t...>
	{
		template <typename t_l, typename t_r>
		static constexpr int comparator_v = comparator<t_l, t_r>::value;

		using subset_l = tuple_cat_t<std::conditional_t<(comparator_v<h, t> < 0), std::tuple<t>, std::tuple<>>...>;
		using subset_m = tuple_cat_t<std::conditional_t<(comparator_v<h, t> == 0), std::tuple<t>, std::tuple<>>...>;
		using subset_r = tuple_cat_t<std::conditional_t<(comparator_v<h, t> > 0), std::tuple<t>, std::tuple<>>...>;
		using type	   = tuple_cat_t<typename tuple_sort_stable<comparator, subset_l>::type, tuple_cat_t<std::tuple<h>, subset_m>, typename tuple_sort_stable<comparator, subset_r>::type>;
	};

	template <template <typename, typename> typename comparator, template <typename...> typename tuple, typename... t>
	struct tuple_sort_stable<comparator, tuple<t...>> : tuple_sort_stable<comparator, t...>
	{
	};

	template <template <typename, typename> typename comparator>
	struct tuple_sort_stable<comparator>
	{
		using type = std::tuple<>;
	};

	template <template <typename, typename> typename comparator, template <typename...> typename tuple>
	struct tuple_sort_stable<comparator, tuple<>>
	{
		using type = std::tuple<>;
	};

	template <template <typename, typename> typename comparator, template <typename...> typename tuple, typename h>
	struct tuple_sort_stable<comparator, tuple<h>>
	{
		using type = std::tuple<h>;
	};

	template <template <typename, typename> typename comparator, typename... t>
	using tuple_sort_stable_t = tuple_sort_stable<comparator, t...>::type;

	template <template <typename> typename pred, typename... ts>
	struct find_index_impl;

	template <template <typename> typename pred, typename first, typename... rest>
	struct find_index_impl<pred, first, rest...>
	{
		static constexpr std::size_t value = []() {
			if constexpr (pred<first>::value)
			{
				return 0;
			}
			else
			{
				return 1 + find_index_impl<pred, rest...>::value;
			}
		}();
	};

	template <template <typename> typename pred>
	struct find_index_impl<pred>
	{
		static_assert(false, "No matching type found in type list");
	};

	template <template <typename> typename pred, typename tuple>
	struct find_index_from_tuple;

	template <template <typename> typename pred, template <typename...> typename tuple_t, typename... ts>
	struct find_index_from_tuple<pred, tuple_t<ts...>>
	{
		static constexpr std::size_t value = find_index_impl<pred, ts...>::value;
	};

	template <template <typename> typename pred, typename t_tpl>
	struct any_of_tuple;

	template <template <typename> typename pred, template <typename...> typename t_tpl, typename... t>
	struct any_of_tuple<pred, t_tpl<t...>>
	{
		static constexpr bool value = (pred<t>::value || ...);
	};

	template <template <typename> typename pred, typename t_tpl>
	inline constexpr bool any_of_tuple_v = any_of_tuple<pred, t_tpl>::value;

	template <template <typename> typename pred, typename tuple>
	inline constexpr std::size_t find_index_tuple_v = find_index_from_tuple<pred, tuple>::value;

	template <template <typename> typename pred, typename... t>
	inline constexpr std::size_t find_index_v = find_index_impl<pred, t...>::value;

	template <typename tpl>
	struct pop_back;

	template <typename... t>
	struct pop_back<std::tuple<t...>>
	{
		static_assert(sizeof...(t) > 0);
		using type = decltype([]<auto... i>(std::index_sequence<i...>) {
			return std::tuple<std::tuple_element_t<i, std::tuple<t...>>...>{};
		}(std::make_index_sequence<sizeof...(t) - 1>{}));
		// private:
		// template <std::size_t... i>
		// static auto helper(std::index_sequence<i...>) -> std::tuple<std::tuple_element_t<i, std::tuple<t...>>...>;

		// public:
		// using type = decltype(helper(std::make_index_sequence<sizeof...(t) - 1>{}));
	};

	template <std::size_t... i>
	struct pop_back<std::index_sequence<i...>>
	{
		static_assert(sizeof...(i) > 0);
		using type = decltype([]<auto... n>(std::index_sequence<n...>) {
			return std::index_sequence<(meta::variadic_auto_at_v<n, i...>)...>{};
		}(std::make_index_sequence<sizeof...(i) - 1>{}));
	};

	template <typename... t>
	using pop_back_tpl_t = typename pop_back<std::tuple<t...>>::type;

	template <typename t_seq>
	using pop_back_seq_t = typename pop_back<t_seq>::type;

	template <typename t>
	struct arr_size;

	template <typename t, std::size_t n>
	struct arr_size<std::array<t, n>>
	{
		static constexpr std::size_t value = n;
	};

	template <typename t>
	constexpr inline std::size_t arr_size_v = arr_size<t>::value;

	// template <typename... t>
	// struct type_list
	//{
	//	template <typename t_head>
	//	using prepend = type_list<t_head, t...>;
	// };

	// template <template <typename> typename pred, typename... t>
	// struct filter_list;

	// template <template <typename> typename pres>
	// struct filter_list<pres>
	//{
	//	using type = type_list<>;
	// };

	// template <template <typename> typename pred, typename t_head, typename... t_tail>
	// struct filter_list<pred, t_head, t_tail...>
	//{
	//	using tail_filtered = typename filter_list<pred, t_tail...>::type;

	//	using type = std::conditional_t<
	//		pred<t_head>::value,
	//		typename tail_filtered::template prepend<t_head>,
	//		tail_filtered>;
	//};

	// template <typename t>
	// struct type_list_to_tuple;

	// template <typename... t>
	// struct type_list_to_tuple<type_list<t...>>
	//{
	//	using type = std::tuple<t...>;
	// };

	// template <template <typename> typename pred, typename... t>
	// using filter_to_tuple_t = typename type_list_to_tuple<typename filter_list<pred, t...>::type>::type;

	template <std::size_t offset, std::size_t... i>
	constexpr decltype(auto)
	make_offset_sequence(std::index_sequence<i...>)
	{
		return std::index_sequence<offset + i...>{};
	}

	template <std::size_t offset, std::size_t n>
	using offset_sequence = decltype(make_offset_sequence<offset>(std::make_index_sequence<n>{}));

	template <auto... i>
	constexpr auto
	seq_to_arr(std::index_sequence<i...>)
	{
		return std::array<std::size_t, sizeof...(i)>{ i... };
	}

	template <std::size_t i, std::size_t count>
	constexpr auto
	make_iota_array()
	{
		return seq_to_arr(offset_sequence<i, count>());
	}

	template <auto arr>
	constexpr auto
	arr_to_seq()
	{
		return []<std::size_t... i>(std::index_sequence<i...>) {
			return std::index_sequence<arr[i]...>{};
		}(std::make_index_sequence<std::size(arr)>{});
	}

	template <auto arr>
	using arr_to_seq_t = decltype(arr_to_seq<arr>());

	template <typename... t_seq>
	struct index_sequence_cat;

	template <std::size_t... i1, std::size_t... i2, typename... t_seq_tail>
	struct index_sequence_cat<std::index_sequence<i1...>, std::index_sequence<i2...>, t_seq_tail...>
	{
		using type = typename index_sequence_cat<std::index_sequence<i1..., i2...>, t_seq_tail...>::type;
	};

	template <std::size_t... i>
	struct index_sequence_cat<std::index_sequence<i...>>
	{
		using type = std::index_sequence<i...>;
	};

	template <typename... t_seq>
	using index_sequence_cat_t = typename index_sequence_cat<t_seq...>::type;

	template <typename t>
	using value_or_ref_t = std::conditional_t<
		std::is_lvalue_reference_v<t>,
		t,
		std::remove_reference_t<t>>;

	template <typename T>
	FORCE_INLINE constexpr decltype(auto)
	as_value_or_ref(T&& value)
	{
		if constexpr (std::is_lvalue_reference_v<T>)
			return value;
		else
			return std::move(value);
	}

	template <template <typename> typename pred, typename... t>
	consteval auto
	filter_count()
	{
		const std::array<bool, sizeof...(t)> flags = { pred<t>::value... };
		std::size_t							 count = 0;
		for (auto i = 0; i < sizeof...(t); ++i)
		{
			if (flags[i])
			{
				++count;
			}
		}

		return count;
	}

	template <template <typename> typename pred, typename... t>
	consteval auto
	filter_indices()
	{
		const std::array<bool, sizeof...(t)> flags	 = { pred<t>::value... };
		auto								 indices = std::array<std::size_t, filter_count<pred, t...>()>();

		std::size_t idx = 0;
		for (auto i = 0; i < sizeof...(t); ++i)
		{
			if (flags[i])
			{
				indices[idx++] = i;
			}
		}

		return indices;
	}

	template <template <typename> typename pred, typename... t>
	consteval auto
	make_filtered_index_sequence()
	{
		constexpr auto arr = filter_indices<pred, t...>();

		return [&]<std::size_t... i>(std::index_sequence<i...>) {
			return std::index_sequence<arr[i]...>{};
		}(std::make_index_sequence<arr.size()>{});
	}

	template <template <typename> typename pred, typename... t>
	using filtered_index_sequence_t = decltype(make_filtered_index_sequence<pred, t...>());

	template <template <typename> typename pred, typename... t>
	struct filtered_variadic
	{
	  private:
		template <std::size_t... i>
		static auto helper(std::index_sequence<i...>) -> std::tuple<std::tuple_element_t<i, std::tuple<t...>>...>;

	  public:
		using type = decltype(helper(make_filtered_index_sequence<pred, t...>()));
	};

	template <template <typename> typename pred, typename... t>
	using filtered_variadic_t = filtered_variadic<pred, t...>::type;

	template <typename t>
	struct index_sequence_inclusive_scan;

	template <std::size_t... i>
	struct index_sequence_inclusive_scan<std::index_sequence<i...>>
	{
		static consteval auto
		get_arr()
		{
			auto arr	= seq_to_arr(std::index_sequence<i...>{});
			auto offset = 0;
			for (auto& elem : arr)
			{
				auto temp  = elem;
				elem	  += offset;
				offset	  += temp;
			}

			return arr;
		}

		using type = arr_to_seq_t<get_arr()>;
	};

	template <typename t_idx_seq>
	using index_sequence_inclusive_scan_t = typename index_sequence_inclusive_scan<t_idx_seq>::type;

	template <typename t>
	struct index_sequence_exclusive_scan;

	template <std::size_t... i>
	struct index_sequence_exclusive_scan<std::index_sequence<i...>>
	{
		static consteval auto
		get_arr()
		{
			auto arr = seq_to_arr(std::index_sequence<i...>{});
			auto sum = 0;
			for (auto& elem : arr)
			{
				auto temp  = elem;
				elem	   = sum;
				sum		  += temp;
			}

			return arr;
		}

		using type = arr_to_seq_t<get_arr()>;
	};

	template <typename t_idx_seq>
	using index_sequence_exclusive_scan_t = typename index_sequence_exclusive_scan<t_idx_seq>::type;

	template <typename t_tpl>
	struct is_tuple_empty : std::bool_constant<std::tuple_size_v<t_tpl> == 0>
	{
	};

	template <typename t_tpl>
	struct is_tuple_not_empty : std::bool_constant<std::tuple_size_v<t_tpl> != 0>
	{
	};

	template <typename... t>
	inline static constexpr auto tuple_empty_v = is_tuple_empty<t...>::value;

	template <typename... t>
	inline static constexpr auto tuple_not_empty_v = is_tuple_empty<t...>::value;

	template <template <typename> typename pred, typename... t>
	struct filtered_tuple;

	//{
	//  private:
	//	template <std::size_t... i>
	//	static auto helper(std::index_sequence<i...>) -> std::tuple<std::tuple_element_t<i, std::tuple<t...>>...>;

	//  public:
	//	using type = decltype(helper(make_filtered_index_sequence<pred, t...>()));
	//};

	template <template <typename> typename pred, typename... t>
	struct filtered_tuple<pred, std::tuple<t...>>
	{
		using type = typename filtered_variadic_t<pred, t...>;
	};

	template <template <typename> typename pred, typename t>
	using filtered_tuple_t = filtered_tuple<pred, t>::type;

	template <template <typename> typename pred, typename... t>
	FORCE_INLINE constexpr decltype(auto)
	make_filtered_tuple_from_tuple(std::tuple<t...>&& tpl)
	{
		return [&]<std::size_t... i>(std::index_sequence<i...>) {
			// return std::tuple<decltype(std::move(std::get<i>(tpl)))...>(std::move(std::get<i>(tpl))...);
			return filtered_variadic_t<pred, t...>(std::move(std::get<i>(tpl))...);
		}(make_filtered_index_sequence<pred, t...>());
	}

	template <template <typename> typename pred, typename... t>
	FORCE_INLINE constexpr decltype(auto)
	make_filtered_tuple(t&&... args)
	{
		return [&]<std::size_t... i>(std::index_sequence<i...>, auto&& args_tpl) {
			return filtered_variadic_t<pred, t...>(std::forward<std::tuple_element_t<i, std::tuple<t&&...>>>(std::get<i>(args_tpl))...);
		}(make_filtered_index_sequence<pred, t...>(), std::tuple(std::forward<t>(args)...));
	}

	template <typename t, typename... ts>
	consteval size_t
	variadic_count()
	{
		size_t res = 0;
		([&res]() {
			if (std::is_same_v<t, ts>)
			{
				++res;
			}
		}(),
		 ...);

		return res;
	}

	template <auto v, auto... vs>
	consteval size_t
	variadic_count()
	{
		return variadic_count<auto_wrapper<v>, auto_wrapper<vs>...>();
	}

	template <typename t, typename... ts>
	inline static constexpr bool variadic_unique_v = [] {
		if constexpr (sizeof...(ts) == 0)
		{
			return true;
		}
		else if constexpr (sizeof...(ts) == 1)
		{
			return std::is_same_v<t, variadic_at_t<0, ts...>> == false;
		}
		else
		{
			return variadic_constains_v<t, ts...> == false;
		}
	}();

	template <auto... v>
	concept variadic_auto_unique = variadic_unique_v<auto_wrapper<v>...>;

	template <typename... t>
	struct type_pack
	{
		template <std::size_t i>
		using t_nth = variadic_at_t<i, t...>;

		static constexpr auto size = sizeof...(t);
	};

	template <typename... t_pack>
	struct type_pack_cat;

	template <typename... t>
	struct type_pack_cat<type_pack<t...>>
	{
		using type = type_pack<t...>;
	};

	template <typename... t1, typename... t2, typename... t_pack>
	struct type_pack_cat<type_pack<t1...>, type_pack<t2...>, t_pack...>
	{
		using type = typename type_pack_cat<type_pack<t1..., t2...>, t_pack...>::type;
	};

	template <typename... t>
	using type_pack_cat_t = type_pack_cat<t...>::type;

	template <template <typename...> typename t_tpl, typename... t>
	auto tpl_to_type_pack_helper(t_tpl<t...>) -> type_pack<t...>;

	template <meta::tuple_like t_tpl_like>
	using tpl_to_type_pack = decltype(tpl_to_type_pack_helper(std::declval<t_tpl_like>()));

	template <unsigned int n>
	struct string_wrapper
	{
		char value[n];

		constexpr string_wrapper(const char (&str)[n])
		{
			for (auto i = 0; i < n; ++i)
			{
				value[i] = str[i];
			}
		}
	};

	template <int... n>
	struct __seq
	{
	};

	// int_seq<4> => int_seq<3,3> => int_seq<2,2,3> => int_seq<1,1,2,3> => int_seq<0,0,1,2,3> => type : __seq<0,1,2,3>
	template <int h, int... t>
	struct int_seq : int_seq<h - 1, h - 1, t...>
	{
	};

	template <int... t>
	struct int_seq<0, t...>
	{
		using type = __seq<t...>;
	};

	template <typename fn, typename t, int... s>
	void
	__call(fn&& f, t&& tpl, __seq<s...>)
	{
		f(std::get<s>(std::forward<t>(tpl))...);
	}

	template <typename fn, typename... ts>
	void
	call_w_tpl_args(fn&& f, std::tuple<ts...>& tpl)
	{
		__call(std::forward<fn>(f), tpl, int_seq<sizeof...(ts)>::type());
	}

	template <typename fn, typename... ts>
	void
	call_w_tpl_args(fn&& f, std::tuple<ts...>&& tpl)
	{
		__call(std::forward<fn>(f), tpl, int_seq<sizeof...(ts)>::type());
	}

	// use case : meta::func_args_t<decltype(func)>() ...
	template <typename f>
	struct func_args;

	template <typename res, typename... args>
	struct func_args<res(args...)>
	{
		using type = std::tuple<args...>;
	};

	template <typename class_t, typename res, typename... args>
	struct func_args<res (class_t::*)(args...)>
	{
		using type = std::tuple<args...>;
	};

	template <typename t>
	using func_args_t = func_args<t>::type;

	// template <typename res, typename... args>
	// using func_args_t<res(args...)> = func_args<res(args...)>::type;

	// template <typename class_t, typename res, typename... args>
	// using func_args_t<res (class_t::*)(args...)> = func_args<res (class_t::*)(args...)>::type;

	template <typename t>
	struct func_ret;

	template <typename res, typename... args>
	struct func_ret<res(args...)>
	{
		using type = res;
	};

	template <typename class_t, typename res, typename... args>
	struct func_ret<res (class_t::*)(args...)>
	{
		using type = res;
	};

	template <typename t>
	using func_ret_t = func_ret<t>::type;

	template <template <typename...> typename, template <typename...> typename>
	struct is_same_template : std::false_type
	{
	};

	template <template <typename...> typename t>
	struct is_same_template<t, t> : std::true_type
	{
	};

	template <template <typename...> typename tl, template <typename...> typename tr>
	inline constexpr auto is_same_template_v = is_same_template<tr, tr>::value;

	template <typename t1, template <typename...> typename t2>
	struct is_specialization_of : std::false_type
	{
	};

	template <template <typename...> class t, typename... args>
	struct is_specialization_of<t<args...>, t> : std::true_type
	{
	};

	template <typename t>
	struct is_not_empty : std::bool_constant<!std::is_empty_v<t>>
	{
	};

	// template <template <typename> typename t_concept>
	// struct make_pred
	//{
	//	template <typename t>
	//	using type = std::bool_constant<t_concept<t>::value>;
	// };

	// template <template <typename> typename t_concept>
	// using make_pred_t = make_pred<t_concept>::type;

	template <typename t_target>
	struct pred_is_same
	{
		template <typename t>
		struct type : std::bool_constant<std::is_same_v<t_target, t>>
		{
		};
	};

	// template <typename t_target>
	// using pred_is_same_t = template pred_is_same<t_target>::type;

	template <typename t1, template <typename...> typename t2>
	inline constexpr auto is_specialization_of_v = is_specialization_of<t1, t2>::value;

	inline constexpr auto deref_view = std::views::transform([](auto ptr) -> decltype(*ptr) { return *ptr; });

	template <std::size_t i, typename t>
	struct _tpl_leaf
	{
		t val;
	};

	template <typename seq, typename... t>
	struct _tpl_helper;

	template <std::size_t... i, typename... t>
	struct _tpl_helper<std::index_sequence<i...>, t...> : public _tpl_leaf<i, t>...
	{
	};

	template <typename... t>
	struct _tpl : public _tpl_helper<std::index_sequence_for<t...>, t...>
	{
	};

	template <std::size_t i, typename... t>
	constexpr auto&
	_get(_tpl<t...>& tpl)
	{
		return static_cast<_tpl_leaf<i, typename std::tuple_element<i, std::tuple<t...>>::type>&>(tpl).val;
	}

	template <std::size_t i, typename... t>
	constexpr const auto&
	_get(const _tpl<t...>& tpl)
	{
		return static_cast<const _tpl_leaf<i, typename std::tuple_element<i, std::tuple<t...>>::type>&>(tpl).val;
	}

	template <std::size_t i, typename... t>
	constexpr auto&&
	_get(_tpl<t...>&& tpl)
	{
		return static_cast<_tpl_leaf<i, typename std::tuple_element<i, std::tuple<t...>>::type>&&>(tpl).val;
	}

	template <typename t>
	struct _tpl_size;

	template <typename... t>
	struct _tpl_size<_tpl<t...>> : std::integral_constant<std::size_t, sizeof...(t)>
	{
	};

	template <typename t>
	inline constexpr std::size_t _tpl_size_v = _tpl_size<t>::value;

	template <typename f, typename tpl_t, std::size_t... i>
	constexpr decltype(auto)
	_apply_impl(f&& func, tpl_t&& tpl, std::index_sequence<i...>)
	{
		return std::forward<f>(func)(_get<i>(std::forward<tpl_t>(tpl))...);
	}

	template <typename f, typename tpl_t>
	constexpr decltype(auto)
	_apply(f&& func, tpl_t&& tpl)
	{
		return _apply_impl(
			std::forward<f>(func),
			std::forward<tpl_t>(tpl),
			std::make_index_sequence<_tpl_size_v<std::decay_t<tpl_t>>>{});
	}

	template <std::size_t n>
	using smallest_unsigned_t = std::conditional_t<
		n <= std::numeric_limits<uint8>::max(), uint8,
		std::conditional_t<
			n <= std::numeric_limits<uint16>::max(), uint16,
			std::conditional_t<
				n <= std::numeric_limits<uint32>::max(), uint32,
				uint64>>>;

	template <typename t_func, typename t_tpl>
	requires meta::tuple_like<t_tpl>
	FORCE_INLINE constexpr decltype(auto)
	tuple_unpack(t_func&& func, t_tpl&& tpl, auto&&... arg) noexcept(noexcept(std::apply(FWD(func), std::tuple_cat(FWD(tpl), std::tuple{ FWD(arg)... }))))
	{
		return []<auto... i> INLINE_LAMBDA_FRONT(std::index_sequence<i...>, auto&& func, auto&& tpl, auto&&... arg) noexcept(noexcept(func(std::get<i>(tpl)..., FWD(arg)...))) INLINE_LAMBDA_BACK -> decltype(auto) {
			return func(std::get<i>(tpl)..., FWD(arg)...);
		}(std::make_index_sequence<std::tuple_size_v<std::remove_cvref_t<t_tpl>>>{}, FWD(func), FWD(tpl), FWD(arg)...);
	}

	template <typename... t>
	constexpr void
	print_type()
	{
		static_assert([] { return false; }(), "Type info");
	}

	template <auto... n>
	constexpr void
	print_type()
	{
		static_assert([] { return false; }(), "Type info");
	}
}	 // namespace age::meta

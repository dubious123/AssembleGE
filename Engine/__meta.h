#pragma once
#include <type_traits>
#include <array>
#include <tuple>
#include <ranges>
#include "__macro_foreach.h"
#include "__common.h"
#define STR_HASH(x) (meta::MM<sizeof(x) - 1>::crc32(x))

namespace meta
{
	// https://stackoverflow.com/questions/2111667/compile-time-string-hashing
	static constexpr unsigned int crc_table[256] = {
		0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f,
		0xe963a535, 0x9e6495a3, 0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
		0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91, 0x1db71064, 0x6ab020f2,
		0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
		0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9,
		0xfa0f3d63, 0x8d080df5, 0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
		0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b, 0x35b5a8fa, 0x42b2986c,
		0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
		0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423,
		0xcfba9599, 0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
		0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d, 0x76dc4190, 0x01db7106,
		0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
		0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d,
		0x91646c97, 0xe6635c01, 0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
		0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950,
		0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
		0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7,
		0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
		0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa,
		0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
		0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81,
		0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
		0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683, 0xe3630b12, 0x94643b84,
		0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
		0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb,
		0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
		0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5, 0xd6d6a3e8, 0xa1d1937e,
		0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
		0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55,
		0x316e8eef, 0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
		0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe, 0xb2bd0b28,
		0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
		0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f,
		0x72076785, 0x05005713, 0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
		0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242,
		0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
		0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69,
		0x616bffd3, 0x166ccf45, 0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
		0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc,
		0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
		0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693,
		0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
		0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
	};

	template <int size, int idx = 0>	//, class dummy = void>
	struct MM
	{
		static constexpr unsigned int
		crc32(const char* str, unsigned int prev_crc = 0xFFFFFFFF)
		{
			return MM<size, idx + 1>::crc32(str, (prev_crc >> 8) ^ crc_table[(prev_crc ^ str[idx]) & 0xFF]);
		}
	};

	template <int size>		 //, class dummy>
	struct MM<size, size>	 //, dummy>
	{
		static constexpr unsigned int
		crc32(const char* str, unsigned int prev_crc = 0xFFFFFFFF)
		{
			return prev_crc ^ 0xFFFFFFFF;
		}
	};

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

	template <typename seq>
	inline constexpr auto index_sequence_size_v = index_sequence_size<seq>::value;

	template <typename t_seq>
	inline constexpr bool index_sequence_empty_v = index_sequence_size_v<t_seq> == 0;

	template <std::size_t i, typename seq>
	struct index_sequence_at;

	template <std::size_t i, std::size_t... idx>
	struct index_sequence_at<i, std::index_sequence<idx...>>
	{
		static constexpr std::size_t value = variadic_auto_at_v<i, idx...>;
	};

	template <std::size_t i, typename seq>
	inline constexpr auto index_sequence_at_v = index_sequence_at<i, seq>::value;

	template <typename seq>
	inline constexpr auto index_sequence_back_v = index_sequence_at_v<index_sequence_size_v<seq> - 1, seq>;

	template <typename seq>
	inline constexpr auto index_sequence_front_v = index_sequence_at_v<0, seq>;

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

	template <template <typename> typename t_concept>
	struct make_pred
	{
		template <typename t>
		using type = std::bool_constant<t_concept<t>::value>;
	};

	template <template <typename> typename t_concept>
	using make_pred_t = make_pred<t_concept>::type;

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
}	 // namespace meta

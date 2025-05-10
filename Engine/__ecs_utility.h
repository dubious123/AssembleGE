#pragma once

namespace ecs::utility
{
	template <typename t>
	constexpr inline auto popcount(t x)
	{
#ifdef _MSC_VER
		if constexpr (sizeof(t) == 16)
		{
			return __popcnt16(x);
		}
		else if constexpr (sizeof(t) == 32)
		{
			return __popcnt(x);
		}
		else
		{
			return __popcnt64(x);
		}
#elif defined(__GNUC__) || defined(__clang__)
		return __builtin_popcount(x);
#else
		return std::popcount(x);
#endif
	}

	template <typename... t>
	consteval std::size_t max_alignof()
	{
		return std::ranges::max({ alignof(t)... });
	}

	// assume align is power of 2
	consteval std::size_t align_up(std::size_t offset, std::size_t align)
	{
		// (offset + align - 1) / align * align
		return (offset + align - 1) & ~(align - 1);
	}

	namespace detail
	{
	}

	template <typename new_tag, std::size_t n = 1>
	struct element
	{
		using tag_type							= new_tag;
		using type								= new_tag::type;
		static constexpr std::size_t _alignment = alignof(type);
		static constexpr std::size_t _size		= sizeof(type) * n;
	};

	template <typename inner_tag, std::size_t inner_n, std::size_t n>
	struct element<element<inner_tag, inner_n>, n>
	{
		using tag_type							= inner_tag;
		using type								= typename element<inner_tag, inner_n>::type;
		static constexpr std::size_t _alignment = element<inner_tag, inner_n>::_alignment;
		static constexpr std::size_t _size		= element<inner_tag, inner_n>::_size * n;
	};

	template <typename... t_element>
	struct __layout_info
	{
		template <typename t1, typename t2>
		struct align_comparator : std::integral_constant<bool, (t1::_alignment < t2::_alignment)>
		{
		};

		using tpl_sorted = meta::tuple_sort_t<align_comparator, t_element...>;

		template <std::size_t i, std::size_t prev_offset, std::size_t... offset>
		struct offset_sequence_builder
		{
			static constexpr std::size_t curr_offset =
				[]() {
					if constexpr (i < std::tuple_size_v<tpl_sorted>)
					{
						return align_up(prev_offset + std::tuple_element_t<i - 1, tpl_sorted>::_size, std::tuple_element_t<i, tpl_sorted>::_alignment);
					}
					else
					{
						// dummy
						return 0;
					}
				}();

			// not working in msvc
			// static constexpr std::size_t curr_offset = align_up(prev_offset + sizeof(std::tuple_element_t<i - 1, tpl_sorted>), alignof(std::tuple_element_t<i, tpl_sorted>));

			using type = typename offset_sequence_builder<
				i + 1,
				curr_offset,
				offset...,
				curr_offset>::type;
		};

		template <std::size_t prev_offset, std::size_t... offset>
		struct offset_sequence_builder<sizeof...(t_element), prev_offset, offset...>
		{
			using type = std::index_sequence<offset...>;
		};

		using offset_sequence = typename offset_sequence_builder<1, 0, 0>::type;

		template <typename t_tag>
		struct match_type
		{
			template <typename t_elem>
			struct pred
			{
				static constexpr bool value = std::is_same_v<t_tag, typename t_elem::tag_type>;
			};
		};

		template <typename t_tag>
		static consteval std::size_t offset_of()
		{
			// return meta::find_index_tuple_v<match_type<t_tag>::template pred, tpl_sorted>;
			return meta::index_sequence_at_v<meta::find_index_tuple_v<match_type<t_tag>::template pred, tpl_sorted>, offset_sequence>;
		}

		static consteval auto max_alignof()
		{
			return ecs::utility::max_alignof<typename t_element::type...>();
		}

		static consteval auto total_size()
		{
			return meta::index_sequence_at_v<sizeof...(t_element) - 1, offset_sequence> + std::tuple_element_t<sizeof...(t_element) - 1, tpl_sorted>::size;
		}
	};

	template <typename... t>
	struct aligned_layout_info
	{
		// private:
		// template <typename new_tag, std::size_t n>
		// struct element;


		// template <typename t1, typename t2>
		// struct align_comparator : std::integral_constant<bool, (alignof(typename t1::type) < alignof(typename t2::type))>
		//{
		// };


		// using tpl_sorted = meta::tuple_sort_t<align_comparator, t...>;

		// template <std::size_t i, std::size_t prev_offset, std::size_t... offset>
		// struct offset_sequence_builder
		//{
		//	static constexpr std::size_t curr_offset =
		//		[]() {
		//			if constexpr (i < std::tuple_size_v<tpl_sorted>)
		//			{
		//				return align_up(prev_offset + sizeof(typename std::tuple_element_t<i - 1, tpl_sorted>::type), alignof(typename std::tuple_element_t<i, tpl_sorted>::type));
		//			}
		//			else
		//			{
		//				// dummy
		//				return 0;
		//			}
		//		}();

		//	// not working in msvc
		//	// static constexpr std::size_t curr_offset = align_up(prev_offset + sizeof(std::tuple_element_t<i - 1, tpl_sorted>), alignof(std::tuple_element_t<i, tpl_sorted>));

		//	using type = typename offset_sequence_builder<
		//		i + 1,
		//		curr_offset,
		//		offset...,
		//		curr_offset>::type;
		//};

		// template <std::size_t prev_offset, std::size_t... offset>
		// struct offset_sequence_builder<sizeof...(t), prev_offset, offset...>
		//{
		//	using type = std::index_sequence<offset...>;
		// };

		// using offset_sequence = typename __layout_info<element<t>...>::offset_sequence;

		// struct runtime_offset
		//{
		//	std::size_t offset;
		// };


	  public:
		using __detail = __layout_info<element<t, 1>...>;

		template <typename new_tag, std::size_t n>
		using with = aligned_layout_info<element<new_tag, n>, t...>;

		template <typename t_tag>
		static consteval std::size_t offset_of()
		{
			return __detail::template offset_of<t_tag>();
			// return 0;
		}

		static consteval auto max_alignof()
		{
			__detail::max_alignof();
		}

		static consteval auto total_size()
		{
			__detail::total_size();
		}

		static void print()
		{
			[]<std::size_t... i>(std::index_sequence<i...> _) {
				((
					 []() {
						 using t_elem = std::tuple_element_t<i, __detail::tpl_sorted>;
						 std::println("{}_nth, size : {}, align : {} offset : {}", i, t_elem::_size, t_elem::_alignment, offset_of<t_elem::tag_type>());
					 }()

						 ),
				 ...);
			}(std::index_sequence_for<t...> {});
		}
	};

	template <typename... t_cmp>
	struct archetype_traits
	{
		static_assert(sizeof...(t_cmp) <= 64, "Too many components");

		using t_archetype = std::conditional_t<
			(sizeof...(t_cmp) <= 8), uint8,
			std::conditional_t<
				(sizeof...(t_cmp) <= 16), uint16,
				std::conditional_t<
					(sizeof...(t_cmp) <= 32), uint32, uint64>>>;

		using t_storage_cmp_idx = uint8;

		using t_local_cmp_idx = t_storage_cmp_idx;

		template <typename... t>
		static inline consteval t_archetype calc_archetype()
		{
			t_archetype archetype = 0;
			([&archetype] {
				archetype |= 1 << meta::variadic_index_v<t, t_cmp...>;
			}(),
			 ...);
			return archetype;
		};

		static inline consteval auto cmp_count()
		{
			return sizeof...(t_cmp);
		}

		template <typename t>
		static consteval t_storage_cmp_idx calc_storage_cmp_idx()
		{
			return meta::variadic_index_v<t, t_cmp...>;
		}

		template <typename t>
		static inline constexpr t_local_cmp_idx calc_local_cmp_idx(t_archetype local_archetype)
		{
			return ecs::utility::popcount(((1 << calc_storage_cmp_idx<t>()) - 1) & local_archetype);
		}

		static inline t_local_cmp_idx calc_local_cmp_idx(t_archetype local_archetype, t_storage_cmp_idx storage_cmp_idx)
		{
			return ecs::utility::popcount(((1 << storage_cmp_idx) - 1) & local_archetype);
		}
	};
}	 // namespace ecs::utility

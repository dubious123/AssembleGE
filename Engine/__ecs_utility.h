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
		template <typename new_tag, std::size_t n = 1>
		struct layout_element
		{
			using tag_type						   = new_tag;
			using type							   = new_tag::type;
			static constexpr std::size_t alignment = alignof(type);
			static constexpr std::size_t size	   = sizeof(type) * n;
		};

		template <typename inner_tag, std::size_t inner_n, std::size_t n>
		struct layout_element<layout_element<inner_tag, inner_n>, n>
		{
			using tag_type						   = inner_tag;
			using type							   = typename layout_element<inner_tag, inner_n>::type;
			static constexpr std::size_t alignment = layout_element<inner_tag, inner_n>::alignment;
			static constexpr std::size_t size	   = layout_element<inner_tag, inner_n>::size * n;
		};

		template <std::size_t max_size, typename... t_element>
		struct layout_info_impl
		{
			template <typename t1, typename t2>
			struct align_comparator : std::integral_constant<bool, (t1::alignment < t2::alignment)>
			{
			};

			using sorted_element_tpl = meta::tuple_sort_t<align_comparator, t_element...>;

			template <std::size_t i, std::size_t prev_offset, std::size_t... offset>
			struct offset_sequence_builder
			{
				static constexpr std::size_t curr_offset =
					[]() {
						if constexpr (i < std::tuple_size_v<sorted_element_tpl>)
						{
							return align_up(prev_offset + std::tuple_element_t<i - 1, sorted_element_tpl>::size, std::tuple_element_t<i, sorted_element_tpl>::alignment);
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
				return meta::index_sequence_at_v<meta::find_index_tuple_v<match_type<t_tag>::template pred, sorted_element_tpl>, offset_sequence>;
			}

			static consteval auto max_alignof()
			{
				return ecs::utility::max_alignof<typename t_element::type...>();
			}

			static consteval auto total_size()
			{
				return meta::index_sequence_at_v<sizeof...(t_element) - 1, offset_sequence> + std::tuple_element_t<sizeof...(t_element) - 1, sorted_element_tpl>::size;
			}

			static auto print()
			{
				[]<std::size_t... i>(std::index_sequence<i...> _) {
					((
						 []() {
							 using t_elem = std::tuple_element_t<i, sorted_element_tpl>;
							 std::println("{}_nth, size : {}, align : {} offset : {}", i, t_elem::size, t_elem::alignment, offset_of<t_elem::tag_type>());
						 }()

							 ),
					 ...);
				}(std::index_sequence_for<t_element...> {});
			}

			template <typename t_tag, std::size_t n>
			using with = layout_info_impl<max_size, layout_element<t_tag, n>, t_element...>;

			template <std::size_t low, std::size_t high, typename... t_tag>
			struct soa_finder
			{
				static constexpr std::size_t mid = (low + high + 1) / 2;

				using test_layout = layout_info_impl<max_size, layout_element<t_tag, mid>..., t_element...>;

				static constexpr bool fits = test_layout::total_size() <= max_size;

				using type = std::conditional_t<
					(low >= high),
					test_layout,
					std::conditional_t<
						fits,
						typename soa_finder<mid, high, t_tag...>::type,
						typename soa_finder<low, mid - 1, t_tag...>::type>>;

				// using type = std::conditional_t<
				//	(low >= high),
				//	test_layout,
				//	soa_finder<mid, high, t_tag...>::type>;
			};

			template <typename... t_tag>
			using with_soa = typename soa_finder<1, max_size, t_tag...>::type;
		};

		template <std::size_t max_size, typename... t_tag>
		using impl = layout_info_impl<max_size, layout_element<t_tag, 1>...>;
	}	 // namespace detail

	template <std::size_t max_size, typename... t>
	struct aligned_layout_info
	{
	  public:
		using detail_impl = detail::impl<max_size, t...>;

		template <typename new_tag, std::size_t n>
		using with = detail_impl::template with<new_tag, n>;

		template <typename... new_tag>
		using with_soa = detail_impl::template with_soa<new_tag...>;

		static_assert(max_size >= detail_impl::total_size(), "not enough memory");

		template <typename t_tag>
		static consteval std::size_t offset_of()
		{
			return detail_impl::template offset_of<t_tag>();
			// return 0;
		}

		static consteval auto max_alignof()
		{
			return detail_impl::max_alignof();
		}

		static consteval auto total_size()
		{
			return detail_impl::total_size();
		}

		static void print()
		{
			detail_impl::print();
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

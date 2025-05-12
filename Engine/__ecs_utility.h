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
	constexpr std::size_t align_up(std::size_t offset, std::size_t align)
	{
		// (offset + align - 1) / align * align
		return (offset + align - 1) & ~(align - 1);
	}

	namespace detail
	{
		template <typename new_tag, std::size_t n = 1>
		struct layout_unit
		{
			using tag_type						   = new_tag;
			using type							   = new_tag::type;
			static constexpr std::size_t alignment = alignof(type);
			static constexpr std::size_t size	   = sizeof(type) * n;
			static constexpr std::size_t count	   = n;
			static constexpr bool		 flex_fill = false;
		};

		template <typename inner_tag, std::size_t inner_n, std::size_t n>
		struct layout_unit<layout_unit<inner_tag, inner_n>, n>
		{
			using tag_type						   = inner_tag;
			using type							   = typename layout_unit<inner_tag, inner_n>::type;
			static constexpr std::size_t alignment = layout_unit<inner_tag, inner_n>::alignment;
			static constexpr std::size_t size	   = layout_unit<inner_tag, inner_n>::size * n;
			static constexpr std::size_t count	   = n;
			static constexpr bool		 flex_fill = false;
		};

		template <typename new_tag>
		struct layout_unit_flex : layout_unit<new_tag>
		{
			static constexpr bool flex_fill = true;
		};

		template <typename tpl_with, typename tpl_soa>
		struct layout_group;

		template <typename... t_unit_with, typename... t_unit_flex>
		struct layout_group<std::tuple<t_unit_with...>, std::tuple<t_unit_flex...>>
		{
			template <typename... t_unit>
			using _with = layout_group<std::tuple<t_unit_with..., t_unit...>, std::tuple<t_unit_flex...>>;

			template <typename... t_unit>
			using _with_flex = layout_group<std::tuple<t_unit_with...>, std::tuple<t_unit_flex..., t_unit...>>;
		};

		template <std::size_t mem_offset, std::size_t mem_size, typename layout_group>
		struct layout_group_info;

		template <std::size_t mem_offset, std::size_t mem_size, typename... t_unit_with, typename... t_unit_flex>
		struct layout_group_info<mem_offset, mem_size, layout_group<std::tuple<t_unit_with...>, std::tuple<t_unit_flex...>>>
		{
			template <typename t1, typename t2>
			struct align_comparator : std::integral_constant<bool, (t1::alignment < t2::alignment)>
			{
			};

			template <typename t_tag>
			struct match_type
			{
				template <typename t_elem>
				struct pred : std::integral_constant<bool, std::is_same_v<t_tag, typename t_elem::tag_type>>
				{
					// static constexpr bool value = std::is_same_v<t_tag, typename t_elem::tag_type>;
				};
			};

			using sorted_element_tpl = meta::tuple_sort_t<align_comparator, t_unit_with..., t_unit_flex...>;

			static constexpr auto calc_offset_arr(std::size_t soa_count)
			{
				return [soa_count]<std::size_t... i>(std::index_sequence<i...> _) {
					std::array<std::size_t, std::tuple_size_v<sorted_element_tpl> + 1> offsets { mem_offset };
					([&offsets, soa_count] {
						using t_elem = std::tuple_element_t<i, sorted_element_tpl>;

						offsets[i] = align_up(offsets[i], t_elem::alignment);
						if constexpr (t_elem::flex_fill)
						{
							offsets[i + 1] = offsets[i] + t_elem::size * soa_count;
						}
						else
						{
							offsets[i + 1] = offsets[i] + t_elem::size;
						}
					}(),
					 ...);

					return offsets;
				}(std::index_sequence_for<t_unit_with..., t_unit_flex...> {});
			}

			static constexpr auto calc_soa_count()
			{
				// Assumes:
				// - No padding between elements
				constexpr auto static_overhead = (mem_offset + ... + t_unit_with::size);
				constexpr auto soa_unit_size   = (0 + ... + t_unit_flex::size);
				constexpr auto upper_bound	   = (static_overhead > mem_size || soa_unit_size == 0)
												   ? 0
												   : (mem_size - static_overhead) / soa_unit_size;


				for (uint64 low = 0, high = upper_bound;;)
				{
					if (low >= high)
					{
						return low;
					}

					auto mid = (low + high + 1) / 2;

					auto fit = calc_offset_arr(mid).back() <= mem_size;
					if (fit)
					{
						low = mid;
					}
					else
					{
						high = mid - 1;
					}
				}

				std::unreachable();
			}

			static constexpr std::size_t alignment = std::tuple_element_t<0, sorted_element_tpl>::alignment;

			static constexpr int soa_count = calc_soa_count();

			using offset_sequence = meta::arr_to_seq_t<calc_offset_arr(soa_count)>;

			// includes padding
			static constexpr std::size_t size = meta::index_sequence_back_v<offset_sequence> - mem_offset;

			static constexpr std::size_t offset = meta::index_sequence_front_v<offset_sequence>;

			static constexpr std::size_t end_offset = meta::index_sequence_back_v<offset_sequence>;

			static_assert(mem_size >= meta::index_sequence_back_v<offset_sequence>, "not enough mem_size");

			static constexpr std::size_t available_size = mem_size - meta::index_sequence_back_v<offset_sequence>;

			template <typename t_tag>
			static consteval auto contains()
			{
				return meta::any_of_tuple_v<match_type<t_tag>::template pred, sorted_element_tpl>;
			}

			template <typename t_tag>
			static consteval std::size_t offset_of()
			{
				static_assert(contains<t_tag>(), "tag not included");
				return meta::index_sequence_at_v<meta::find_index_tuple_v<match_type<t_tag>::template pred, sorted_element_tpl>, offset_sequence>;
			}

			template <typename t_tag>
			static consteval std::size_t count_of()
			{
				static_assert(contains<t_tag>(), "tag not included");
				if constexpr (meta::variadic_constains_v<t_tag, typename t_unit_flex::tag_type...>)
				{
					return soa_count;
				}
				else
				{
					return meta::find_index_tuple_v<match_type<t_tag>::template pred, sorted_element_tpl>::count;
				}
			}

			static consteval auto max_alignof()
			{
				return alignment;
			}

			static consteval auto total_size()
			{
				return size;
			}

			static auto print()
			{
				[]<std::size_t... i>(std::index_sequence<i...> _) {
					(([]() {
						 using t_elem = std::tuple_element_t<i, sorted_element_tpl>;
						 std::println("{}_nth, size : {}, align : {} offset : {}", i, t_elem::size, t_elem::alignment, offset_of<t_elem::tag_type>());
					 }()),
					 ...);
					std::println("total_size : {}, offset : {} end_offset : {}", size, offset, end_offset);
				}(std::index_sequence_for<t_unit_with..., t_unit_flex...> {});
			}
		};

		template <std::size_t mem_offset, std::size_t mem_size, typename... t_layout_group>
		struct layout_info
		{
			template <typename t_tag>
			struct match_type
			{
				template <typename t_layout_info_complete>
				struct pred : std::integral_constant<bool, t_layout_info_complete::template contains<t_tag>()>
				{
				};
			};

			template <std::size_t offset, typename... t>
			struct make_layout_info_complete_tpl;

			template <std::size_t offset>
			struct make_layout_info_complete_tpl<offset>
			{
				using type = std::tuple<>;
			};

			template <std::size_t offset, typename t_builder_elem_head, typename... t_layout_group_tail>
			struct make_layout_info_complete_tpl<offset, t_builder_elem_head, t_layout_group_tail...>
			{
				using head_info_complete				 = layout_group_info<offset, mem_size, t_builder_elem_head>;
				static constexpr std::size_t next_offset = offset + head_info_complete::size;

				using tail_tpl = typename make_layout_info_complete_tpl<next_offset, t_layout_group_tail...>::type;
				using type	   = decltype(std::tuple_cat(std::tuple<head_info_complete> {}, tail_tpl {}));
			};

			using layout_info_complete_tpl = make_layout_info_complete_tpl<mem_offset, t_layout_group...>::type;

			static constexpr auto calc_offset_arr()
			{
				return []<std::size_t... i>(std::index_sequence<i...>) {
					return std::array<std::size_t, sizeof...(t_layout_group) + 1> {
						(std::tuple_element_t<i, layout_info_complete_tpl>::offset)...,
						std::tuple_element_t<sizeof...(t_layout_group) - 1, layout_info_complete_tpl>::end_offset
					};
				}(std::make_index_sequence<sizeof...(t_layout_group)> {});
			}

			using offset_sequence = meta::arr_to_seq_t<calc_offset_arr()>;

			static constexpr std::size_t alignment = std::tuple_element_t<0, layout_info_complete_tpl>::offset;

			static constexpr std::size_t size = meta::index_sequence_back_v<offset_sequence>;

			template <typename t_tag>
			static consteval std::size_t offset_of()
			{
				return std::tuple_element_t<meta::find_index_tuple_v<match_type<t_tag>::template pred, layout_info_complete_tpl>, layout_info_complete_tpl>::template offset_of<t_tag>();
			}

			template <typename t_tag>
			static consteval auto count_of()
			{
				return std::tuple_element_t<meta::find_index_tuple_v<match_type<t_tag>::template pred, layout_info_complete_tpl>, layout_info_complete_tpl>::template count_of<t_tag>();
			}

			static consteval auto max_alignof()
			{
				return alignment;
			}

			static consteval auto total_size()
			{
				return size;
			}

			static auto print()
			{
				[]<std::size_t... i>(std::index_sequence<i...>) {
					((std::tuple_element_t<i, layout_info_complete_tpl>::print()), ...);
				}(std::make_index_sequence<std::tuple_size_v<layout_info_complete_tpl>> {});
			}
		};

		template <typename... t_layout_group>
		struct layout_builder_impl
		{
			template <std::size_t begin_offset, std::size_t mem_size>
			static constexpr decltype(auto) __build()
			{
				return layout_info<begin_offset, mem_size, t_layout_group...> {};
			}

			template <typename... t_unit>
			static constexpr decltype(auto) __with()
			{
				if constexpr (sizeof...(t_layout_group) == 0)
				{
					return layout_builder_impl<layout_group<std::tuple<t_unit...>, std::tuple<>>> {};
				}
				else
				{
					return []<std::size_t... i>(std::index_sequence<i...> _) {
						using tpl	 = meta::pop_back_t<t_layout_group...>;
						using t_last = meta::variadic_at_t<sizeof...(t_layout_group) - 1, t_layout_group...>;
						// print_type<t_last>();
						return layout_builder_impl<std::tuple_element_t<i, tpl>..., typename t_last::template _with<t_unit...>> {};
						// return layout_info_impl_builder<std::tuple_element_t<i, tpl>..., t_last> {};
					}(std::make_index_sequence<sizeof...(t_layout_group) - 1> {});
				}
			}

			template <typename... t_unit_flex>
			static constexpr decltype(auto) __with_flex()
			{
				if constexpr (sizeof...(t_layout_group) == 0)
				{
					return layout_builder_impl<layout_group<std::tuple<t_unit_flex...>, std::tuple<>>> {};
				}
				else
				{

					return []<std::size_t... i>(std::index_sequence<i...> _) {
						using tpl	 = meta::pop_back_t<t_layout_group...>;
						using t_last = meta::variadic_at_t<sizeof...(t_layout_group) - 1, t_layout_group...>;

						return layout_builder_impl<std::tuple_element_t<i, tpl>..., typename t_last::template _with_flex<t_unit_flex...>> {};
					}(std::make_index_sequence<sizeof...(t_layout_group) - 1> {});
				}
			}

			template <typename... t_unit>
			static constexpr decltype(auto) __after_with()
			{
				return layout_builder_impl<t_layout_group..., layout_group<std::tuple<t_unit...>, std::tuple<>>> {};
			}

			template <typename... t_unit_flex>
			static constexpr decltype(auto) __after_with_flex()
			{
				return layout_builder_impl<t_layout_group..., layout_group<std::tuple<>, std::tuple<t_unit_flex...>>> {};
			}

			template <typename... t_tag>
			using with = decltype(__with<layout_unit<t_tag>...>());

			template <typename t_tag, std::size_t n>
			using with_n = decltype(__with<layout_unit<t_tag, n>>());

			template <typename... t_tag>
			using with_flex = decltype(__with_flex<layout_unit_flex<t_tag>...>());

			template <typename... t_tag>
			using after_with = decltype(__after_with<layout_unit<t_tag>...>());

			template <typename t_tag, std::size_t n>
			using after_with_n = decltype(__after_with<layout_unit<t_tag, n>>());

			template <typename... t_tag>
			using after_with_flex = decltype(__after_with_flex<layout_unit_flex<t_tag>...>());

			template <std::size_t begin_offset, std::size_t mem_size>
			using build = decltype(__build<begin_offset, mem_size>());
		};
	}	 // namespace detail

	template <typename... t_tag_with>
	using layout_builder = detail::layout_builder_impl<detail::layout_group<std::tuple<detail::layout_unit<t_tag_with>...>, std::tuple<>>>;

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

		using t_storage_cmp_idx = meta::smallest_unsigned_t<std::bit_width(std::numeric_limits<t_archetype>::max())>;

		using t_local_cmp_idx = t_storage_cmp_idx;

		static_assert(std::is_same_v<t_storage_cmp_idx, uint8>);

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

		static inline constexpr auto cmp_count(t_archetype archetype)
		{
			return popcount(archetype);
		}

		template <t_storage_cmp_idx storage_cmp_idx>
		static inline consteval std::size_t cmp_size()
		{
			if constexpr (storage_cmp_idx < sizeof...(t_cmp))
			{
				return sizeof(meta::variadic_at_t<storage_cmp_idx, t_cmp...>);
			}
			else
			{
				return std::size_t { 0 };
			}
		}

		static inline constexpr std::size_t cmp_size(t_storage_cmp_idx storage_cmp_idx)
		{
			switch (storage_cmp_idx)
			{
#define __CMP_SIZE_IMPL(N)    \
	case N:                   \
	{                         \
		return cmp_size<N>(); \
	}
#define X(N) __CMP_SIZE_IMPL(N)
				__X_REPEAT_LIST_512
#undef X
#undef __CMP_SIZE_IMPL
			default:
			{
				return std::size_t { 0 };
			}
			}
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

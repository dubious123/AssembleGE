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
		struct layout_element
		{
			using tag_type						   = new_tag;
			using type							   = new_tag::type;
			static constexpr std::size_t alignment = alignof(type);
			static constexpr std::size_t size	   = sizeof(type) * n;
			static constexpr bool		 flex_fill = false;
		};

		template <typename inner_tag, std::size_t inner_n, std::size_t n>
		struct layout_element<layout_element<inner_tag, inner_n>, n>
		{
			using tag_type						   = inner_tag;
			using type							   = typename layout_element<inner_tag, inner_n>::type;
			static constexpr std::size_t alignment = layout_element<inner_tag, inner_n>::alignment;
			static constexpr std::size_t size	   = layout_element<inner_tag, inner_n>::size * n;
			static constexpr bool		 flex_fill = false;
		};

		template <typename new_tag>
		struct layout_element_flex_fill : layout_element<new_tag>
		{
			static constexpr bool flex_fill = true;
		};

		// template <std::size_t max_size, typename... t_element>
		// struct layout_info_impl
		//{
		//	template <typename t1, typename t2>
		//	struct align_comparator : std::integral_constant<bool, (t1::alignment < t2::alignment)>
		//	{
		//	};

		//	using sorted_element_tpl = meta::tuple_sort_t<align_comparator, t_element...>;

		//	template <std::size_t i, std::size_t prev_offset, std::size_t... offset>
		//	struct offset_sequence_builder
		//	{
		//		static constexpr std::size_t curr_offset =
		//			[]() {
		//				if constexpr (i < std::tuple_size_v<sorted_element_tpl>)
		//				{
		//					return align_up(prev_offset + std::tuple_element_t<i - 1, sorted_element_tpl>::size, std::tuple_element_t<i, sorted_element_tpl>::alignment);
		//				}
		//				else
		//				{
		//					// dummy
		//					return 0;
		//				}
		//			}();

		//		// not working in msvc
		//		// static constexpr std::size_t curr_offset = align_up(prev_offset + sizeof(std::tuple_element_t<i - 1, tpl_sorted>), alignof(std::tuple_element_t<i, tpl_sorted>));

		//		using type = typename offset_sequence_builder<
		//			i + 1,
		//			curr_offset,
		//			offset...,
		//			curr_offset>::type;
		//	};

		//	template <std::size_t prev_offset, std::size_t... offset>
		//	struct offset_sequence_builder<sizeof...(t_element), prev_offset, offset...>
		//	{
		//		using type = std::index_sequence<offset...>;
		//	};

		//	using offset_sequence = typename offset_sequence_builder<1, 0, 0>::type;

		//	template <typename t_tag>
		//	struct match_type
		//	{
		//		template <typename t_elem>
		//		struct pred
		//		{
		//			static constexpr bool value = std::is_same_v<t_tag, typename t_elem::tag_type>;
		//		};
		//	};

		//	template <typename t_tag>
		//	static consteval std::size_t offset_of()
		//	{
		//		return meta::index_sequence_at_v<meta::find_index_tuple_v<match_type<t_tag>::template pred, sorted_element_tpl>, offset_sequence>;
		//	}

		//	static consteval auto max_alignof()
		//	{
		//		return ecs::utility::max_alignof<typename t_element::type...>();
		//	}

		//	static consteval auto total_size()
		//	{
		//		return meta::index_sequence_at_v<sizeof...(t_element) - 1, offset_sequence> + std::tuple_element_t<sizeof...(t_element) - 1, sorted_element_tpl>::size;
		//	}

		//	static auto print()
		//	{
		//		[]<std::size_t... i>(std::index_sequence<i...> _) {
		//			(([]() {
		//				 using t_elem = std::tuple_element_t<i, sorted_element_tpl>;
		//				 std::println("{}_nth, size : {}, align : {} offset : {}", i, t_elem::size, t_elem::alignment, offset_of<t_elem::tag_type>());
		//			 }()),
		//			 ...);
		//		}(std::index_sequence_for<t_element...> {});
		//	}

		//	template <typename t_tag, std::size_t n>
		//	using with = layout_info_impl<max_size, layout_element<t_tag, n>, t_element...>;

		//	template <std::size_t low, std::size_t high, typename... t_tag>
		//	struct soa_finder
		//	{
		//		static constexpr auto binary_search()
		//		{
		//			constexpr auto mid = (low + high + 1) / 2;
		//			using test_layout  = layout_info_impl<max_size, layout_element<t_tag, mid>..., t_element...>;

		//			if constexpr (low >= high)
		//			{
		//				return std::type_identity<test_layout> {};
		//			}
		//			else if constexpr (test_layout::total_size() <= max_size)
		//			{
		//				return soa_finder<mid, high, t_tag...>::binary_search();
		//			}
		//			else
		//			{
		//				return soa_finder<low, mid - 1, t_tag...>::binary_search();
		//			}
		//		}

		//		using type = typename decltype(binary_search())::type;
		//	};

		//	template <typename... t_tag>
		//	static constexpr auto _upper_bound = [] {
		//		constexpr auto static_overhead = total_size();
		//		constexpr auto one_entity_size = layout_info_impl<max_size, layout_element<t_tag, 1>..., t_element...>::total_size();
		//		return (static_overhead > max_size || one_entity_size == 0 || one_entity_size > max_size) ? 0 : (max_size - static_overhead) / one_entity_size;
		//	}();

		//	template <typename... t_tag>
		//	using with_soa = typename soa_finder<1, _upper_bound<t_tag...>, t_tag...>::type;

		//	template <typename t_tag, std::size_t n>
		//	using after_with = layout_info_impl<max_size, t_element..., layout_element<t_tag, n>>;
		//};

		template <typename tpl_with, typename tpl_soa>
		struct builder_elem;

		template <typename... t_elem_with, typename... t_elem_soa>
		struct builder_elem<std::tuple<t_elem_with...>, std::tuple<t_elem_soa...>>
		{
			template <typename... t_element>
			using _with = builder_elem<std::tuple<t_elem_with..., t_element...>, std::tuple<t_elem_soa...>>;

			template <typename... t_element>
			using _with_soa = builder_elem<std::tuple<t_elem_with...>, std::tuple<t_elem_soa..., t_element...>>;
		};

		template <std::size_t mem_offset, std::size_t mem_size, typename builder_elem>
		struct layout_info_complete;

		template <std::size_t mem_offset, std::size_t mem_size, typename... t_elem_with, typename... t_elem_soa>
		struct layout_info_complete<mem_offset, mem_size, builder_elem<std::tuple<t_elem_with...>, std::tuple<t_elem_soa...>>>
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

			using sorted_element_tpl = meta::tuple_sort_t<align_comparator, t_elem_with..., t_elem_soa...>;

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
				}(std::index_sequence_for<t_elem_with..., t_elem_soa...> {});
			}

			static constexpr auto calc_soa_count()
			{
				// Assumes:
				// - No padding between elements
				constexpr auto static_overhead = (mem_offset + ... + t_elem_with::size);
				constexpr auto soa_unit_size   = (0 + ... + t_elem_soa::size);
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
				}(std::index_sequence_for<t_elem_with..., t_elem_soa...> {});
			}
		};

		template <std::size_t mem_offset, std::size_t mem_size, typename... t_builder_elem>
		struct layout_info_impl
		{
			template <typename t_tag>
			struct match_type
			{
				template <typename t_layout_info_complete>
				struct pred : std::integral_constant<bool, t_layout_info_complete::template contains<t_tag>()>
				{
					// static constexpr bool value = std::is_same_v<t_tag, typename t_elem::tag_type>;
				};
			};

			template <std::size_t offset, typename... t_builder_element>
			struct make_layout_info_complete_tpl;

			template <std::size_t offset>
			struct make_layout_info_complete_tpl<offset>
			{
				using type = std::tuple<>;
			};

			template <std::size_t offset, typename t_builder_elem_head, typename... t_builder_elem_tail>
			struct make_layout_info_complete_tpl<offset, t_builder_elem_head, t_builder_elem_tail...>
			{
				using head_info_complete				 = layout_info_complete<offset, mem_size, t_builder_elem_head>;
				static constexpr std::size_t next_offset = offset + head_info_complete::size;

				using tail_tpl = typename make_layout_info_complete_tpl<next_offset, t_builder_elem_tail...>::type;
				using type	   = decltype(std::tuple_cat(std::tuple<head_info_complete> {}, tail_tpl {}));
			};

			using layout_info_complete_tpl = make_layout_info_complete_tpl<mem_offset, t_builder_elem...>::type;

			static constexpr auto calc_offset_arr()
			{
				return []<std::size_t... i>(std::index_sequence<i...>) {
					return std::array<std::size_t, sizeof...(t_builder_elem) + 1> {
						(std::tuple_element_t<i, layout_info_complete_tpl>::offset)...,
						std::tuple_element_t<sizeof...(t_builder_elem) - 1, layout_info_complete_tpl>::end_offset
					};
				}(std::make_index_sequence<sizeof...(t_builder_elem)> {});
			}

			using offset_sequence = meta::arr_to_seq_t<calc_offset_arr()>;

			static constexpr std::size_t alignment = std::tuple_element_t<0, layout_info_complete_tpl>::offset;

			static constexpr std::size_t size = meta::index_sequence_back_v<offset_sequence>;

			template <typename t_tag>
			static consteval std::size_t offset_of()
			{
				return std::tuple_element_t<meta::find_index_tuple_v<match_type<t_tag>::template pred, layout_info_complete_tpl>, layout_info_complete_tpl>::template offset_of<t_tag>();
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

		template <typename... t_builder_elem>
		struct layout_info_impl_builder
		{
			template <std::size_t begin_offset, std::size_t mem_size>
			static constexpr decltype(auto) __build()
			{
				return layout_info_impl<begin_offset, mem_size, t_builder_elem...> {};
			}

			template <typename... t_element>
			static constexpr decltype(auto) __with()
			{
				if constexpr (sizeof...(t_builder_elem) == 0)
				{
					return layout_info_impl_builder<builder_elem<std::tuple<t_element...>, std::tuple<>>> {};
				}
				else
				{
					return []<std::size_t... i>(std::index_sequence<i...> _) {
						using tpl	 = meta::pop_back_t<t_builder_elem...>;
						using t_last = meta::variadic_at_t<sizeof...(t_builder_elem) - 1, t_builder_elem...>;
						// print_type<t_last>();
						return layout_info_impl_builder<std::tuple_element_t<i, tpl>..., typename t_last::template _with<t_element...>> {};
						// return layout_info_impl_builder<std::tuple_element_t<i, tpl>..., t_last> {};
					}(std::make_index_sequence<sizeof...(t_builder_elem) - 1> {});
				}
			}

			template <typename... t_element>
			static constexpr decltype(auto) __with_soa()
			{
				if constexpr (sizeof...(t_builder_elem) == 0)
				{
					return layout_info_impl_builder<builder_elem<std::tuple<t_element...>, std::tuple<>>> {};
				}
				else
				{

					return []<std::size_t... i>(std::index_sequence<i...> _) {
						using tpl	 = meta::pop_back_t<t_builder_elem...>;
						using t_last = meta::variadic_at_t<sizeof...(t_builder_elem) - 1, t_builder_elem...>;

						return layout_info_impl_builder<std::tuple_element_t<i, tpl>..., typename t_last::template _with_soa<t_element...>> {};
					}(std::make_index_sequence<sizeof...(t_builder_elem) - 1> {});
				}
			}

			template <typename... t_element>
			static constexpr decltype(auto) __after_with()
			{
				return layout_info_impl_builder<t_builder_elem..., builder_elem<std::tuple<t_element...>, std::tuple<>>> {};
			}

			template <typename... t_element>
			static constexpr decltype(auto) __after_with_soa()
			{
				return layout_info_impl_builder<t_builder_elem..., builder_elem<std::tuple<>, std::tuple<t_element...>>> {};
			}

			template <typename... t_tag>
			using with = decltype(__with<layout_element<t_tag>...>());

			template <typename t_tag, std::size_t n>
			using with_n = decltype(__with<layout_element<t_tag, n>>());

			template <typename... t_tag>
			using with_soa = decltype(__with_soa<layout_element_flex_fill<t_tag>...>());

			template <typename... t_tag>
			using after_with = decltype(__after_with<layout_element<t_tag>...>());

			template <typename t_tag, std::size_t n>
			using after_with_n = decltype(__after_with<layout_element<t_tag, n>>());

			template <typename... t_tag>
			using after_with_soa = decltype(__after_with_soa<layout_element_flex_fill<t_tag>...>());

			template <std::size_t begin_offset, std::size_t mem_size>
			using build = decltype(__build<begin_offset, mem_size>());
		};
	}	 // namespace detail

	template <typename... t_tag_with>
	using aligned_layout_info_builder = detail::layout_info_impl_builder<detail::builder_elem<std::tuple<detail::layout_element<t_tag_with>...>, std::tuple<>>>;

	// template <std::size_t mem_size, typename t_tpl_tag_with, typename t_tpl_tag_soa>
	// struct aligned_layout_info;

	// template <std::size_t mem_size, typename... t_tag_with>
	// struct aligned_layout_info<mem_size, std::tuple<t_tag_with...>, std::tuple<>>
	//{
	//   public:
	//	using default_builder = detail::layout_info_impl_builder<detail::builder_elem<std::tuple<detail::layout_element<t_tag_with>...>, std::tuple<>>>;

	//	template <typename new_tag, std::size_t n>
	//	using with_n = default_builder::template with<detail::layout_element<new_tag, n>>;

	//	template <typename... new_tag>
	//	using with = default_builder::template with<detail::layout_element<new_tag>...>;

	//	template <typename new_tag, std::size_t n>
	//	using after_with_n = default_builder::template after_with<detail::layout_element<new_tag, n>>;

	//	template <typename... new_tag>
	//	using after_with = default_builder::template after_with<detail::layout_element<new_tag>...>;

	//	template <typename... new_tag>
	//	using with_soa = default_builder::template with_soa<new_tag...>;

	//	static_assert(max_size >= detail_impl::total_size(), "not enough memory");

	//	template <typename t_tag>
	//	static consteval std::size_t offset_of()
	//	{
	//		return detail_impl::template offset_of<t_tag>();
	//		// return 0;
	//	}

	//	static consteval auto max_alignof()
	//	{
	//		return detail_impl::max_alignof();
	//	}

	//	static consteval auto total_size()
	//	{
	//		return detail_impl::total_size();
	//	}

	//	static void print()
	//	{
	//		detail_impl::print();
	//	}
	//};

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

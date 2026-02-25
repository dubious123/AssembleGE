#pragma once

namespace age::util
{
	template <typename t>
	concept cx_has_type = requires {
		typename t::type;
	};
}	 // namespace age::util

// compile time
namespace age::util
{
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

			using sorted_element_tpl = age::meta::tuple_sort_t<align_comparator, t_unit_with..., t_unit_flex...>;

			static constexpr auto
			calc_offset_arr(std::size_t soa_count)
			{
				return [soa_count]<std::size_t... i>(std::index_sequence<i...> _) {
					std::array<std::size_t, std::tuple_size_v<sorted_element_tpl> + 1> offsets{ mem_offset };
					([&offsets, soa_count] {
						using t_unit = std::tuple_element_t<i, sorted_element_tpl>;

						offsets[i] = align_up(offsets[i], t_unit::alignment);

						if constexpr (t_unit::flex_fill)
						{
							offsets[i + 1] = offsets[i] + t_unit::size * soa_count;
						}
						else
						{
							offsets[i + 1] = offsets[i] + t_unit::size;
						}
					}(),
					 ...);

					return offsets;
				}(std::index_sequence_for<t_unit_with..., t_unit_flex...>{});
			}

			static constexpr auto
			calc_soa_count()
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

			static constexpr std::size_t soa_count = calc_soa_count();

			using offset_sequence = age::meta::arr_to_seq_t<calc_offset_arr(soa_count)>;

			// includes padding
			static constexpr std::size_t size = age::meta::index_sequence_back_v<offset_sequence> - mem_offset;

			static constexpr std::size_t offset = age::meta::index_sequence_front_v<offset_sequence>;

			static constexpr std::size_t end_offset = age::meta::index_sequence_back_v<offset_sequence>;

			static_assert(mem_size >= age::meta::index_sequence_back_v<offset_sequence>, "not enough mem_size");

			static constexpr std::size_t available_size = mem_size - age::meta::index_sequence_back_v<offset_sequence>;

			template <typename t_tag>
			static consteval auto
			contains()
			{
				return age::meta::any_of_tuple_v<match_type<t_tag>::template pred, sorted_element_tpl>;
			}

			template <typename t_tag>
			static consteval std::size_t
			offset_of()
			{
				static_assert(contains<t_tag>(), "tag not included");
				return age::meta::index_sequence_at_v<age::meta::find_index_tuple_v<match_type<t_tag>::template pred, sorted_element_tpl>, offset_sequence>;
			}

			template <typename t_tag>
			static consteval std::size_t
			count_of()
			{
				static_assert(contains<t_tag>(), "tag not included");
				if constexpr (age::meta::variadic_contains_v<t_tag, typename t_unit_flex::tag_type...>)
				{
					return soa_count;
				}
				else
				{
					return std::tuple_element_t<age::meta::find_index_tuple_v<match_type<t_tag>::template pred, sorted_element_tpl>, sorted_element_tpl>::count;
				}
			}

			static consteval auto
			max_alignof()
			{
				return alignment;
			}

			static consteval auto
			total_size()
			{
				return size;
			}

			static auto
			print()
			{
				[]<std::size_t... i>(std::index_sequence<i...> _) {
					(([]() {
						 using t_elem = std::tuple_element_t<i, sorted_element_tpl>;
						 std::println("{}_nth, size : {}, align : {} offset : {}", i, t_elem::size, t_elem::alignment, offset_of<t_elem::tag_type>());
					 }()),
					 ...);
					std::println("total_size : {}, offset : {} end_offset : {}", size, offset, end_offset);
				}(std::index_sequence_for<t_unit_with..., t_unit_flex...>{});
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
			struct make_layout_group_info;

			template <std::size_t offset>
			struct make_layout_group_info<offset>
			{
				using type = std::tuple<>;
			};

			template <std::size_t offset, typename t_builder_elem_head, typename... t_layout_group_tail>
			struct make_layout_group_info<offset, t_builder_elem_head, t_layout_group_tail...>
			{
				using t_layout_group_head				 = layout_group_info<offset, mem_size, t_builder_elem_head>;
				static constexpr std::size_t next_offset = offset + t_layout_group_head::size;

				using tail_tpl = typename make_layout_group_info<next_offset, t_layout_group_tail...>::type;
				using type	   = decltype(std::tuple_cat(std::tuple<t_layout_group_head>{}, tail_tpl{}));
			};

			using layout_group_info_tpl = make_layout_group_info<mem_offset, t_layout_group...>::type;

			static constexpr auto
			calc_offset_arr()
			{
				return []<std::size_t... i>(std::index_sequence<i...>) {
					return std::array<std::size_t, sizeof...(t_layout_group) + 1>{
						(std::tuple_element_t<i, layout_group_info_tpl>::offset)...,
						std::tuple_element_t<sizeof...(t_layout_group) - 1, layout_group_info_tpl>::end_offset
					};
				}(std::make_index_sequence<sizeof...(t_layout_group)>{});
			}

			using offset_sequence = age::meta::arr_to_seq_t<calc_offset_arr()>;

			static constexpr std::size_t alignment = std::tuple_element_t<0, layout_group_info_tpl>::offset;

			static constexpr std::size_t size = age::meta::index_sequence_back_v<offset_sequence>;

			template <typename t_tag>
			static consteval std::size_t
			offset_of()
			{
				return std::tuple_element_t<age::meta::find_index_tuple_v<match_type<t_tag>::template pred, layout_group_info_tpl>, layout_group_info_tpl>::template offset_of<t_tag>();
			}

			template <typename t_tag>
			static consteval auto
			count_of()
			{
				return std::tuple_element_t<age::meta::find_index_tuple_v<match_type<t_tag>::template pred, layout_group_info_tpl>, layout_group_info_tpl>::template count_of<t_tag>();
			}

			static consteval auto
			max_alignof()
			{
				return alignment;
			}

			static consteval auto
			total_size()
			{
				return size;
			}

			static auto
			print()
			{
				[]<std::size_t... i>(std::index_sequence<i...>) {
					((std::tuple_element_t<i, layout_group_info_tpl>::print()), ...);
				}(std::make_index_sequence<std::tuple_size_v<layout_group_info_tpl>>{});
			}
		};

		template <typename... t_layout_group>
		struct layout_builder_impl
		{
			template <std::size_t begin_offset, std::size_t mem_size>
			static constexpr decltype(auto)
			__build()
			{
				return layout_info<begin_offset, mem_size, t_layout_group...>{};
			}

			template <typename... t_unit>
			static constexpr decltype(auto)
			__with()
			{
				if constexpr (sizeof...(t_layout_group) == 0)
				{
					return layout_builder_impl<layout_group<std::tuple<t_unit...>, std::tuple<>>>{};
				}
				else
				{
					return []<std::size_t... i>(std::index_sequence<i...>) {
						using tpl	 = age::meta::pop_back_tpl_t<t_layout_group...>;
						using t_last = age::meta::variadic_at_t<sizeof...(t_layout_group) - 1, t_layout_group...>;
						return layout_builder_impl<std::tuple_element_t<i, tpl>..., typename t_last::template _with<t_unit...>>{};
					}(std::make_index_sequence<sizeof...(t_layout_group) - 1>{});
				}
			}

			template <typename... t_unit_flex>
			static constexpr decltype(auto)
			__with_flex()
			{
				if constexpr (sizeof...(t_layout_group) == 0)
				{
					return layout_builder_impl<layout_group<std::tuple<t_unit_flex...>, std::tuple<>>>{};
				}
				else
				{
					return []<std::size_t... i>(std::index_sequence<i...> _) {
						using tpl	 = age::meta::pop_back_tpl_t<t_layout_group...>;
						using t_last = age::meta::variadic_at_t<sizeof...(t_layout_group) - 1, t_layout_group...>;

						return layout_builder_impl<std::tuple_element_t<i, tpl>..., typename t_last::template _with_flex<t_unit_flex...>>{};
					}(std::make_index_sequence<sizeof...(t_layout_group) - 1>{});
				}
			}

			template <typename... t_unit>
			static constexpr decltype(auto)
			__after_with()
			{
				return layout_builder_impl<t_layout_group..., layout_group<std::tuple<t_unit...>, std::tuple<>>>{};
			}

			template <typename... t_unit_flex>
			static constexpr decltype(auto)
			__after_with_flex()
			{
				return layout_builder_impl<t_layout_group..., layout_group<std::tuple<>, std::tuple<t_unit_flex...>>>{};
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

	template <cx_has_type... t_tag_with>
	using layout_builder = detail::layout_builder_impl<detail::layout_group<std::tuple<detail::layout_unit<t_tag_with>...>, std::tuple<>>>;
}	 // namespace age::util

// runtime
namespace age::util
{
	struct type_layout_info
	{
		const std::size_t size;
		const std::size_t alignment;
	};

	namespace detail
	{
		struct with_n_runtime
		{
			static constexpr bool __is_with_n = true;

			const std::size_t size;
			const std::size_t alignment;
			const uint32	  count;
		};

		inline constexpr uint32 dynamic = static_cast<uint32>(-1);

		template <typename t_tag = void, uint32 count = dynamic>
		struct with_n_hybrid;

		template <typename t_tag>
		struct with_n_hybrid<t_tag, dynamic>
		{
			static constexpr bool __is_with_n = true;

			using tag_type = t_tag;
			using type	   = t_tag::type;

			static constexpr std::size_t size	   = sizeof(type);
			static constexpr std::size_t alignment = alignof(type);
			const uint32				 count;
		};

		template <uint32 count>
		requires(count != dynamic)
		struct with_n_hybrid<void, count>
		{
			static constexpr bool __is_with_n = true;

			const std::size_t		size;
			const std::size_t		alignment;
			static constexpr uint32 count = count;
		};

		template <typename t_tag, uint32 count>
		struct with_n_compile
		{
			using tag_type = t_tag;
			using type	   = t_tag::type;

			static constexpr bool __is_with_n = true;

			static constexpr std::size_t size	   = sizeof(type);
			static constexpr std::size_t alignment = alignof(type);
			static constexpr uint32		 count	   = count;
		};

		struct with_flex_runtime
		{
			static constexpr bool __is_flex = true;

			const std::size_t size;
			const std::size_t alignment;
		};

		template <typename t_tag>
		struct with_flex_compile
		{
			static constexpr bool __is_flex = true;

			using tag_type = t_tag;
			using type	   = t_tag::type;

			static constexpr std::size_t size	   = sizeof(type);
			static constexpr std::size_t alignment = alignof(type);
		};
	}	 // namespace detail

	namespace detail
	{
		template <typename t>
		struct is_with_n_compile : std::false_type
		{
		};

		template <typename t_tag, uint32 count>
		struct is_with_n_compile<with_n_compile<t_tag, count>> : std::true_type
		{
		};

		template <typename t>
		concept has_count_compile = requires {
			{
				[]<auto v = t::count>() { }
			};
		};

		template <typename t>
		concept has_count_runtime = requires(t elem) {
			{
				elem.count
			};
		};

		template <typename t>
		concept is_with_n = requires {
			{
				t::__is_with_n
			};
		};

		template <typename t>
		concept is_flex = requires {
			{
				t::__is_flex
			};
		};

		template <typename t>
		concept has_range = requires(t elem) {
			{
				elem.range
			};
		};
	}	 // namespace detail

	template <cx_has_type t_tag>
	FORCE_INLINE decltype(auto)
	with()
	{
		return detail::with_n_compile<t_tag, 1>{};
	}

	FORCE_INLINE decltype(auto)
	with(const type_layout_info& info)
	{
		return detail::with_n_hybrid<void, 1>{ info.size, info.alignment };
	}

	FORCE_INLINE decltype(auto)
	with_n(const type_layout_info& info, const uint32 count)
	{
		return detail::with_n_runtime{ info.size, info.alignment, count };
	}

	template <cx_has_type t_tag>
	FORCE_INLINE decltype(auto)
	with_n(uint32 count)
	{
		return detail::with_n_hybrid<t_tag, detail::dynamic>{ count };
	}

	template <uint32 count>
	FORCE_INLINE decltype(auto)
	with_n(const type_layout_info& info)
	{
		return detail::with_n_hybrid<void, count>{ info.size, info.alignment };
	}

	template <cx_has_type t_tag, std::size_t n>
	FORCE_INLINE decltype(auto)
	with_n()
	{
		return detail::with_n_compile<t_tag, n>{};
	}

	template <cx_has_type t_tag>
	FORCE_INLINE decltype(auto)
	with_flex()
	{
		return detail::with_flex_compile<t_tag>{};
	}

	FORCE_INLINE decltype(auto)
	with_flex(const type_layout_info& info)
	{
		return detail::with_flex_runtime{ info.size, info.alignment };
	}

	// [runtime_layout_builder: Key Steps and Constraints]
	//
	// - with_n_buffer and with_flex_buffer are separated to match the compile-time builder logic.
	// - The number of inserted elements must never exceed buffer capacity (undefined behavior if exceeded).
	// - No sorting occurs before build(); build() must be called only once.
	//   Calling offset_of/count_of before build(), or multiple build() calls, is undefined behavior.
	//
	// - Each slot is associated with a key, which is computed based on its type or runtime index.
	//   This key is mapped via idx_lut, and is used to access result_arr[key], where the final offset and count for each slot are stored.
	//
	// Execution flow:
	//   1) Store elements in with_n_buffer / with_flex_buffer with their computed keys.
	//   2) On build(), concatenate buffers and indirectly sort by alignment (idx_arr).
	//   3) Find flex_count using binary search on sorted indices.
	//   4) For each slot in sorted order, compute and store offset/count in result_arr using its key.
	template <typename... t_element>
	struct layout_builder_runtime
	{
		template <typename t>
		struct known_type : std::integral_constant<bool, cx_has_type<t>>
		{
		};

		template <typename t>
		struct pred_is_with_n : std::integral_constant<bool, detail::is_with_n<t>>
		{
		};

		template <typename t_tag>
		struct match
		{
			template <typename t>
			struct pred : std::integral_constant<bool, std::is_same_v<t_tag, typename t::tag_type>>
			{
			};
		};

		using known_type_tpl = age::meta::filtered_variadic_t<known_type, t_element...>;

		template <typename t_tag>
		using t_known_element = std::tuple_element_t<0, age::meta::filtered_tuple_t</*typename*/ match<t_tag>::template pred, known_type_tpl>>;

		using t_known_type_idx = age::meta::smallest_unsigned_t<std::tuple_size_v<known_type_tpl>>;

		template <typename t_tag>
		static constexpr t_known_type_idx known_type_idx = age::meta::index_sequence_front_v<age::meta::filtered_index_sequence_t</*typename*/ match<t_tag>::template pred, t_element...>>;

		static constexpr t_known_type_idx known_type_count = std::tuple_size_v<known_type_tpl>;

		static constexpr uint32 with_n_count	= age::meta::filter_count<pred_is_with_n, t_element...>();
		static constexpr uint32 with_flex_count = sizeof...(t_element) - with_n_count;

		struct slot_info
		{
			uint32		key;
			uint32		count;
			std::size_t alignment;
			std::size_t size;
			std::size_t offset;
		};

		std::array<slot_info, 10> with_n_buffer;
		std::array<slot_info, 10> with_flex_buffer;

		std::array<uint32, age::meta::arr_size_v<decltype(with_n_buffer)> + age::meta::arr_size_v<decltype(with_flex_buffer)>>
			key_lut;

		std::array<std::pair<std::size_t, uint32>, age::meta::arr_size_v<decltype(with_n_buffer)> + age::meta::arr_size_v<decltype(with_flex_buffer)>>
			result_arr;

		uint32 runtime_key	 = known_type_count;
		uint32 with_n_idx	 = 0;
		uint32 with_flex_idx = 0;

		layout_builder_runtime(t_element&&... elem)
		{
			// fill with_n_buffer and flex_n_buffer
			[this, tpl = std::forward_as_tuple(std::forward<t_element>(elem)...)]<std::size_t... i>(std::index_sequence<i...>) {
				auto runtime_key = known_type_count;
				([this, &tpl, &runtime_key] {
					using t_elem = age::meta::variadic_at_t<i, t_element...>;
					if constexpr (detail::is_with_n<t_elem>)
					{
						if constexpr (known_type<t_elem>::value)
						{
							with_n_buffer[with_n_idx].alignment = t_elem::alignment;
							with_n_buffer[with_n_idx].size		= t_elem::size;
							with_n_buffer[with_n_idx].key		= age::meta::tuple_index_v<t_elem, known_type_tpl>;
						}
						else
						{
							auto& elem_get						= std::get<i>(tpl);
							with_n_buffer[with_n_idx].alignment = elem_get.alignment;
							with_n_buffer[with_n_idx].size		= elem_get.size;
							with_n_buffer[with_n_idx].key		= runtime_key++;
						}

						if constexpr (detail::has_count_runtime<t_elem>)
						{
							with_n_buffer[with_n_idx].count	 = std::get<i>(tpl).count;
							with_n_buffer[with_n_idx].size	*= std::get<i>(tpl).count;
						}
						else
						{
							with_n_buffer[with_n_idx].count	 = t_elem::count;
							with_n_buffer[with_n_idx].size	*= t_elem::count;
						}

						++with_n_idx;
					}
					else if constexpr (detail::is_flex<t_elem>)
					{
						if constexpr (known_type<t_elem>::value)
						{
							with_flex_buffer[with_flex_idx].alignment = t_elem::alignment;
							with_flex_buffer[with_flex_idx].size	  = t_elem::size;
							with_flex_buffer[with_flex_idx].key		  = age::meta::tuple_index_v<t_elem, known_type_tpl>;
						}
						else
						{
							auto& elem_get							  = std::get<i>(tpl);
							with_flex_buffer[with_flex_idx].alignment = elem_get.alignment;
							with_flex_buffer[with_flex_idx].size	  = elem_get.size;
							with_flex_buffer[with_flex_idx].key		  = runtime_key++;
						}

						++with_flex_idx;
					}
					else
					{
						static_assert(false, "invalid_type");
					}
				}(),
				 ...);
			}(std::make_index_sequence<sizeof...(t_element)>{});
		}

		template <typename t_tag>
		static consteval uint32
		index_of()
		{
			return known_type_idx<t_tag>;
		}

		FORCE_INLINE static constexpr uint32
		index_of(uint32 i)
		{
			return i + known_type_count;
		}

		void
		add_with_n(const type_layout_info& info, const uint32 count)
		{
			with_n_buffer[with_n_idx].alignment = info.alignment;
			with_n_buffer[with_n_idx].count		= count;
			with_n_buffer[with_n_idx].size		= info.size * count;
			with_n_buffer[with_n_idx].key		= runtime_key++;
			++with_n_idx;
		}

		void
		add_flex(const type_layout_info& info)
		{
			with_flex_buffer[with_flex_idx].alignment = info.alignment;
			with_flex_buffer[with_flex_idx].size	  = info.size;
			with_flex_buffer[with_flex_idx].key		  = runtime_key++;
			++with_flex_idx;
		}

		template <typename t_tag>
		FORCE_INLINE std::size_t
		offset_of()
		{
			return result_arr[key_lut[index_of<t_tag>()]].first;
		}

		FORCE_INLINE std::size_t
		offset_of(uint32 runtime_idx)
		{
			return result_arr[key_lut[index_of(runtime_idx)]].first;
		}

		template <typename t_tag>
		FORCE_INLINE uint32
		count_of()
		{
			return result_arr[key_lut[index_of<t_tag>()]].second;
		}

		FORCE_INLINE uint32
		count_of(uint32 runtime_idx)
		{
			return result_arr[key_lut[index_of(runtime_idx)]].second;
		}

		void
		print()
		{
			[this]<auto... i>(std::index_sequence<i...>) {
				([this] {
					using t_tag = std::tuple_element_t<i, known_type_tpl>::tag_type;
					auto idx	= index_of<t_tag>();
					auto key	= key_lut[idx];
					std::println("known type | idx : {}, key : {}, offset : {}, size : {},  count : {}", idx, key, result_arr[key].first, sizeof(t_tag::type), result_arr[key].second);
				}(),
				 ...);
			}(std::make_index_sequence<known_type_count>{});

			for (auto i : std::views::iota(0u, with_n_idx + with_flex_idx - known_type_count))
			{
				auto idx = index_of(static_cast<uint32>(i));
				auto key = key_lut[idx];
				std::println("runtime type | idx : {}, key : {}, offset : {}, count : {}", idx, key, result_arr[key].first, result_arr[key].second);
			}
		}

		void
		build(std::size_t mem_offset, std::size_t mem_size)
		{
			using namespace std::ranges::views;
			constexpr int buffer_size = age::meta::arr_size_v<decltype(result_arr)>;
			assert(age::meta::arr_size_v<decltype(result_arr)> >= with_n_idx + with_flex_idx);

			auto slot_buffer = std::array<slot_info, buffer_size>{};
			//
			auto idx_arr = age::meta::make_iota_array<0, buffer_size>();

			std::ranges::copy(with_n_buffer | take(with_n_idx), slot_buffer.begin());
			std::ranges::copy(with_flex_buffer | take(with_flex_idx), (slot_buffer | drop(with_n_idx)).begin());

			std::ranges::stable_sort(idx_arr | take(with_n_idx + with_flex_idx), [&slot_buffer](auto l, auto r) { return slot_buffer[l].alignment > slot_buffer[r].alignment; });

			// find flex_count
			auto flex_count = [this, &slot_buffer, &idx_arr, mem_offset, mem_size]() {
				slot_buffer[idx_arr[0]].offset = mem_size;

				auto upper_bound = [this, mem_offset, mem_size] {
					auto static_overhead = (std::ranges::fold_left(with_n_buffer | take(with_n_idx), mem_offset, [](auto left, const auto& slot) { return left + slot.size; }));
					auto soa_unit_size	 = (std::ranges::fold_left(with_flex_buffer | take(with_flex_idx), 0, [](auto left, const auto& slot) { return left + slot.size; }));
					auto res			 = (static_overhead > mem_size || soa_unit_size == 0)
											 ? 0
											 : (mem_size - static_overhead) / soa_unit_size;

					assert(std::numeric_limits<uint32>::max() > res and "memsize too big");
					return static_cast<uint32>(res);
				}();

				for (uint32 low = 0ul, high = upper_bound;;)
				{
					if (low >= high)
					{
						return low;
					}

					auto mid = (low + high + 1) / 2;

					auto offset = mem_offset;
					for (auto& slot_idx : idx_arr | take(with_n_idx + with_flex_idx))
					{
						auto& slot	= slot_buffer[slot_idx];
						offset		= align_up(offset, slot.alignment);
						slot.offset = offset;

						if (slot_idx < with_n_idx)
						{
							// slot is with_n
							offset += slot.size;
						}
						else
						{
							// slot is with_flex
							offset += slot.size * mid;
						}
					}

					auto fit = offset <= mem_size;
					if (fit)
					{
						low = mid;
					}
					else
					{
						high = mid - 1;
					}
				}
			}();

			for (auto [res_arr_key, slot_idx] : idx_arr | take(with_n_idx + with_flex_idx) | enumerate)
			{
				auto& slot		  = slot_buffer[slot_idx];
				key_lut[slot.key] = static_cast<uint32>(res_arr_key);

				result_arr[res_arr_key].first = slot.offset;
				if (slot_idx < with_n_idx)
				{
					// with_n
					result_arr[res_arr_key].second = slot.count;
				}
				else
				{
					// with_flex
					result_arr[res_arr_key].second = flex_count;
				}
			}
		}
	};
}	 // namespace age::util
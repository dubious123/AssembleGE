#pragma once
#ifndef __ECS_UTILITY_H_INCLUDED
	#error "__ecs_utility_layout.h should not be included directly. Include __ecs_utility.h instead."
#endif

namespace ecs::utility
{
	template <typename t>
	concept has_type_ = requires {
		typename t::type;
	};
}	 // namespace ecs::utility

// compile time
namespace ecs::utility
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

			using sorted_element_tpl = meta::tuple_sort_t<align_comparator, t_unit_with..., t_unit_flex...>;

			static constexpr auto calc_offset_arr(std::size_t soa_count)
			{
				return [soa_count]<std::size_t... i>(std::index_sequence<i...> _) {
					std::array<std::size_t, std::tuple_size_v<sorted_element_tpl> + 1> offsets { mem_offset };
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
			struct make_layout_group_info;

			template <std::size_t offset>
			struct make_layout_group_info<offset>
			{
				using type = std::tuple<>;
			};

			template <std::size_t offset, typename t_builder_elem_head, typename... t_layout_group_tail>
			struct make_layout_group_info<offset, t_builder_elem_head, t_layout_group_tail...>
			{
				using head_info_complete				 = layout_group_info<offset, mem_size, t_builder_elem_head>;
				static constexpr std::size_t next_offset = offset + head_info_complete::size;

				using tail_tpl = typename make_layout_group_info<next_offset, t_layout_group_tail...>::type;
				using type	   = decltype(std::tuple_cat(std::tuple<head_info_complete> {}, tail_tpl {}));
			};

			using layout_group_info_tpl = make_layout_group_info<mem_offset, t_layout_group...>::type;

			static constexpr auto calc_offset_arr()
			{
				return []<std::size_t... i>(std::index_sequence<i...>) {
					return std::array<std::size_t, sizeof...(t_layout_group) + 1> {
						(std::tuple_element_t<i, layout_group_info_tpl>::offset)...,
						std::tuple_element_t<sizeof...(t_layout_group) - 1, layout_group_info_tpl>::end_offset
					};
				}(std::make_index_sequence<sizeof...(t_layout_group)> {});
			}

			using offset_sequence = meta::arr_to_seq_t<calc_offset_arr()>;

			static constexpr std::size_t alignment = std::tuple_element_t<0, layout_group_info_tpl>::offset;

			static constexpr std::size_t size = meta::index_sequence_back_v<offset_sequence>;

			template <typename t_tag>
			static consteval std::size_t offset_of()
			{
				return std::tuple_element_t<meta::find_index_tuple_v<match_type<t_tag>::template pred, layout_group_info_tpl>, layout_group_info_tpl>::template offset_of<t_tag>();
			}

			template <typename t_tag>
			static consteval auto count_of()
			{
				return std::tuple_element_t<meta::find_index_tuple_v<match_type<t_tag>::template pred, layout_group_info_tpl>, layout_group_info_tpl>::template count_of<t_tag>();
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
					((std::tuple_element_t<i, layout_group_info_tpl>::print()), ...);
				}(std::make_index_sequence<std::tuple_size_v<layout_group_info_tpl>> {});
			}
		};

		template <typename... t_layout_group>
		struct layout_builder_impl
		{
			template <std::size_t begin_offset, std::size_t mem_size>
			static constexpr decltype(auto) __build()
			{
				meta::print_type<std::tuple<t_layout_group...>>();
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
						return layout_builder_impl<std::tuple_element_t<i, tpl>..., typename t_last::template _with<t_unit...>> {};
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

	template <has_type_... t_tag_with>
	using layout_builder = detail::layout_builder_impl<detail::layout_group<std::tuple<detail::layout_unit<t_tag_with>...>, std::tuple<>>>;
}	 // namespace ecs::utility

// runtime
namespace ecs::utility
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
			const std::size_t size;
			const std::size_t alignment;
			const std::size_t count;
		};

		inline constexpr std::size_t dynamic = static_cast<std::size_t>(-1);

		template <typename t_tag = void, std::size_t count = dynamic>
		struct with_n_hybrid;

		template <typename t_tag>
		struct with_n_hybrid<t_tag, dynamic>
		{
			using tag_type = t_tag;
			using type	   = t_tag::type;

			static constexpr std::size_t size	   = sizeof(type);
			static constexpr std::size_t alignment = alignof(type);
			const std::size_t			 count;
		};

		template <std::size_t count>
		requires(count != dynamic)
		struct with_n_hybrid<void, count>
		{
			const std::size_t			 size;
			const std::size_t			 alignment;
			static constexpr std::size_t count = count;
		};

		template <typename t_tag, std::size_t count>
		struct with_n_compile
		{
			using tag_type = t_tag;
			using type	   = t_tag::type;

			static constexpr std::size_t size	   = sizeof(type);
			static constexpr std::size_t alignment = alignof(type);
			static constexpr std::size_t count	   = count;
		};

		struct with_flex_runtime
		{
			static constexpr bool __is_flex = true;

			const std::size_t size;
			const std::size_t alignment;
		};

		template <std::ranges::input_range r>
		requires std::same_as<std::ranges::range_value_t<r>, type_layout_info>
		struct with_flex_runtime_view
		{
			static constexpr bool __is_flex = true;

			r range;
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

		template <typename t_tag, std::size_t count>
		struct is_with_n_compile<with_n_compile<t_tag, count>> : std::true_type
		{
		};

		template <typename t>
		concept has_count = requires {
			{
				[]<auto v = t::count>() {}
			};
		};

		template <typename t>
		concept has_count_runtime = requires(t elem) {
			{
				elem.count
			};
		};

		template <typename t>
		concept is_flex = requires {
			{
				t::__is_flex
			};
		};
	}	 // namespace detail

	template <has_type_ t_tag>
	decltype(auto) with()
	{
		return detail::with_n_compile<t_tag, 1> {};
	}

	decltype(auto) with(const type_layout_info& info)
	{
		return detail::with_n_hybrid<void, 1> { info.size, info.alignment };
	}

	decltype(auto) with_n(const type_layout_info& info, const std::size_t count)
	{
		return detail::with_n_runtime { info.size, info.alignment, count };
	}

	template <has_type_ t_tag>
	decltype(auto) with_n(std::size_t count)
	{
		return detail::with_n_hybrid<t_tag> { count };
	}

	template <std::size_t count>
	decltype(auto) with_n(const type_layout_info& info)
	{
		return detail::with_n_hybrid<count> { info.size, info.alignment };
	}

	template <has_type_ t_tag, std::size_t n>
	decltype(auto) with_n()
	{
		return detail::with_n_compile<t_tag, n> {};
	}

	template <has_type_ t_tag>
	decltype(auto) with_flex()
	{
		return detail::with_flex_compile<t_tag> {};
	}

	decltype(auto) with_flex(const type_layout_info& info)
	{
		return detail::with_flex_runtime { info.size, info.alignment };
	}

	template <std::ranges::input_range r>
	requires std::same_as<std::ranges::range_value_t<r>, type_layout_info>
	decltype(auto) with_flex(r&& range)
	{
		return detail::with_flex_runtime_view<std::decay_t<r>> { std::forward<r>(range) };
	}

	template <typename... t_element>
	struct layout_builder_runtime
	{
		template <typename t>
		struct known_type : std::integral_constant<bool, has_type_<t>>
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

		using known_type_tpl = meta::filtered_tuple_t<known_type, t_element...>;

		template <typename t_tag>
		using t_known_element = std::tuple_element_t<0, meta::filtered_tuple_t<typename match<t_tag>::template pred, known_type_tpl>>;

		layout_builder_runtime(t_element&&... elem)
		{
			std::println("{}", std::tuple_size_v<known_type_tpl>);
			// auto compile_time_tag_count = 0;
			// auto runtime_tag_count		= 0;

			//([]() {
			//	if constexpr ()
			//	{
			//	}
			//	else if constexpr ()
			//	{
			//	}
			//}(),
			// ...);
		}

		template <typename t_tag>
		std::size_t count_of()
		{
			using t_elem = t_known_element<t_tag>;

			if constexpr (detail::has_count<t_elem>)
			{
				// with_n_compile
				return t_elem::count;
			}
			else if constexpr (detail::has_count_runtime<t_elem>)
			{
				return 1;
			}
			else if constexpr (detail::is_flex<t_elem>)
			{
				return 2;
			}

			return 0;
		}

		template <typename t_tag>
		std::size_t offset_of()
		{
			using t_elem = t_known_element<t_tag>;
			return 0;
		}

		decltype(auto) build(std::size_t offset, std::size_t mem_size) { return *this; };
	};
}	 // namespace ecs::utility
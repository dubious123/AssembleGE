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

	template <typename... t>
	struct aligned_layout_info
	{
		// private:
		template <typename t1, typename t2>
		struct align_comparator : std::integral_constant<bool, (alignof(typename t1::type) < alignof(typename t2::type))>
		{
		};

		using tpl_sorted = meta::tuple_sort_t<align_comparator, t...>;

		template <std::size_t i, std::size_t prev_offset, std::size_t... offset>
		struct offset_sequence_builder
		{
			static constexpr std::size_t curr_offset =
				[]() {
					if constexpr (i < std::tuple_size_v<tpl_sorted>)
					{
						return align_up(prev_offset + sizeof(typename std::tuple_element_t<i - 1, tpl_sorted>::type), alignof(typename std::tuple_element_t<i, tpl_sorted>::type));
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
		struct offset_sequence_builder<sizeof...(t), prev_offset, offset...>
		{
			using type = std::index_sequence<offset...>;
		};

		// index=1, prev_offset=0, offset=0
		using offset_sequence = typename offset_sequence_builder<1, 0, 0>::type;

		struct runtime_offset
		{
			std::size_t offset;
		};

	  public:
		template <typename k>
		static consteval std::size_t offset_of()
		{
			return meta::index_sequence_at_v<meta::tuple_index_v<k, tpl_sorted>, offset_sequence>;
		}

		static consteval auto max_alignof()
		{
			return ecs::utility::max_alignof<typename t::type...>();
		}

		static consteval auto total_size()
		{
			return meta::index_sequence_at_v<sizeof...(t) - 1, offset_sequence> + sizeof(std::tuple_element_t<sizeof...(t) - 1, tpl_sorted>);
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

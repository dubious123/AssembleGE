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

		template <t_storage_cmp_idx storage_cmp_idx>
		static inline consteval std::size_t cmp_alignment()
		{
			if constexpr (storage_cmp_idx < sizeof...(t_cmp))
			{
				return alignof(meta::variadic_at_t<storage_cmp_idx, t_cmp...>);
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

		static inline constexpr std::size_t cmp_alignment(t_storage_cmp_idx storage_cmp_idx)
		{
			switch (storage_cmp_idx)
			{
#define __CMP_ALIGNMENT_IMPL(N)    \
	case N:                        \
	{                              \
		return cmp_alignment<N>(); \
	}
#define X(N) __CMP_ALIGNMENT_IMPL(N)
				__X_REPEAT_LIST_512
#undef X
#undef __CMP_ALIGNMENT_IMPL
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

#define __ECS_UTILITY_H_INCLUDED
#include "__ecs_utility_layout.h"
#undef __ECS_UTILITY_H_INCLUDED

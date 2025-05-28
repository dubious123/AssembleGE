#pragma once

namespace ecs::utility
{
#if defined(_MSC_VER)
	#define FORCE_INLINE __forceinline
#elif defined(__GNUC__) || defined(__clang__)
	#define FORCE_INLINE inline __attribute__((always_inline))
#else
	#define FORCE_INLINE inline
#endif

	template <typename t>
	FORCE_INLINE constexpr auto
	popcount(t x)
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
	FORCE_INLINE consteval std::size_t
	max_alignof()
	{
		return std::ranges::max({ alignof(t)... });
	}

	// assume align is power of 2
	FORCE_INLINE constexpr std::size_t
	align_up(std::size_t offset, std::size_t align)
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

		using t_component_size = meta::smallest_unsigned_t<std::ranges::max({ sizeof(t_cmp)... })>;

		using t_component_count = meta::smallest_unsigned_t<sizeof...(t_cmp)>;

		// using t_storage_cmp_idx = meta::smallest_unsigned_t<std::bit_width(std::numeric_limits<t_archetype>::max())>;
		using t_storage_cmp_idx = meta::smallest_unsigned_t<sizeof...(t_cmp) - 1>;

		using t_local_cmp_idx = t_storage_cmp_idx;

		static_assert(std::is_same_v<t_storage_cmp_idx, uint8>);

		template <typename... t>
		static consteval t_archetype
		calc_archetype()
		{
			t_archetype archetype = 0;
			([&archetype] {
				archetype |= 1 << meta::variadic_index_v<t, t_cmp...>;
			}(),
			 ...);
			return archetype;
		};

		static consteval t_component_count
		cmp_count()
		{
			return sizeof...(t_cmp);
		}

		FORCE_INLINE static constexpr t_component_count
		cmp_count(t_archetype archetype)
		{
			return static_cast<t_component_count>(popcount(archetype));
		}

		template <std::size_t storage_cmp_idx>
		static consteval t_component_size
		cmp_size()
		{
			if constexpr (storage_cmp_idx < sizeof...(t_cmp))
			{
				return sizeof(meta::variadic_at_t<storage_cmp_idx, t_cmp...>);
			}
			else
			{
				return t_component_size{ 0 };
			}
		}

		template <std::size_t storage_cmp_idx>
		static consteval std::size_t
		cmp_alignment()
		{
			if constexpr (storage_cmp_idx < sizeof...(t_cmp))
			{
				return alignof(meta::variadic_at_t<storage_cmp_idx, t_cmp...>);
			}
			else
			{
				return std::size_t{ 0 };
			}
		}

		FORCE_INLINE static constexpr t_component_size
		cmp_size(t_storage_cmp_idx storage_cmp_idx)
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
				return std::size_t{ 0 };
			}
			}
		}

		FORCE_INLINE static constexpr std::size_t
		cmp_alignment(t_storage_cmp_idx storage_cmp_idx)
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
				return std::size_t{ 0 };
			}
			}
		}

		template <typename t>
		static consteval t_storage_cmp_idx
		calc_storage_cmp_idx()
		{
			return meta::variadic_index_v<std::decay_t<t>, std::decay_t<t_cmp>...>;
		}

		template <typename t>
		FORCE_INLINE static constexpr t_local_cmp_idx
		calc_local_cmp_idx(t_archetype local_archetype)
		{
			return static_cast<t_local_cmp_idx>(ecs::utility::popcount(((1 << calc_storage_cmp_idx<t>()) - 1) & local_archetype));
		}

		FORCE_INLINE static t_local_cmp_idx
		calc_local_cmp_idx(t_archetype local_archetype, t_storage_cmp_idx storage_cmp_idx)
		{
			return static_cast<t_local_cmp_idx>(ecs::utility::popcount(((1 << storage_cmp_idx) - 1) & local_archetype));
		}
	};
}	 // namespace ecs::utility

#define __ECS_UTILITY_H_INCLUDED
#include "__ecs_utility_layout.h"
#undef __ECS_UTILITY_H_INCLUDED

#undef FORCE_INLINE

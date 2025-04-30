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

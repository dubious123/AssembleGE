#pragma once

namespace ecs::utility
{
	template <typename... t_cmp>
	struct archetype_traits
	{
		static_assert(sizeof...(t_cmp) <= 64, "Too many components");

		using t_archetype = std::conditional_t<
			(sizeof...(t_cmp) <= 8), std::uint8_t,
			std::conditional_t<
				(sizeof...(t_cmp) <= 16), std::uint16_t,
				std::conditional_t<
					(sizeof...(t_cmp) <= 32), std::uint32_t,
					std::uint64_t>>>;

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
	};
}	 // namespace ecs::utility

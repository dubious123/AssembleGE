#pragma once

namespace age::ecs
{
	template <typename t_cmp>
	concept cx_component = requires {
		requires std::is_trivially_copyable_v<std::decay_t<t_cmp>>;
		requires std::is_trivially_destructible_v<std::decay_t<t_cmp>>;
		requires std::is_standard_layout_v<std::decay_t<t_cmp>>;
	};
}	 // namespace age::ecs

namespace age::ecs
{
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

		using t_component_size = age::meta::smallest_unsigned_t<std::ranges::max({ sizeof(t_cmp)... })>;

		using t_component_count = age::meta::smallest_unsigned_t<sizeof...(t_cmp)>;

		// using t_storage_cmp_idx = age::meta::smallest_unsigned_t<std::bit_width(std::numeric_limits<t_archetype>::max())>;
		using t_storage_cmp_idx = age::meta::smallest_unsigned_t<sizeof...(t_cmp) - 1>;

		using t_local_cmp_idx = t_storage_cmp_idx;

		static_assert(std::is_same_v<t_storage_cmp_idx, uint8>);

		template <typename... t>
		static consteval t_archetype
		calc_archetype()
		{
			t_archetype archetype = 0;
			([&archetype] {
				archetype |= 1 << age::meta::get_variadic_index<t, t_cmp...>();
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
			return static_cast<t_component_count>(age::util::popcount(archetype));
		}

		template <std::size_t storage_cmp_idx>
		static consteval t_component_size
		cmp_size()
		{
			if constexpr (storage_cmp_idx < sizeof...(t_cmp))
			{
				return sizeof(age::meta::variadic_at_t<storage_cmp_idx, t_cmp...>);
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
				return alignof(age::meta::variadic_at_t<storage_cmp_idx, t_cmp...>);
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
			return static_cast<t_storage_cmp_idx>(age::meta::get_variadic_index<std::decay_t<t>, std::decay_t<t_cmp>...>());
		}

		template <typename t>
		FORCE_INLINE static constexpr t_local_cmp_idx
		calc_local_cmp_idx(t_archetype local_archetype)
		{
			return static_cast<t_local_cmp_idx>(age::util::popcount(((1 << calc_storage_cmp_idx<t>()) - 1) & local_archetype));
		}

		FORCE_INLINE static t_local_cmp_idx
		calc_local_cmp_idx(t_archetype local_archetype, t_storage_cmp_idx storage_cmp_idx)
		{
			return static_cast<t_local_cmp_idx>(age::util::popcount(((1 << storage_cmp_idx) - 1) & local_archetype));
		}

	  private:
		template <template <typename...> typename t_clause, typename... t>
		static consteval t_archetype
		calc_mask_impl(t_clause<t...> clause)
		{
			return calc_archetype<t...>();
		}

	  public:
		template <typename t_clause>
		static consteval t_archetype
		calc_mask()
		{
			return calc_mask_impl(t_clause());
		}

		// FORCE_INLINE static bool
		// matches()
		//{
		// }
	};
}	 // namespace age::ecs
#pragma once

namespace ecs::entity_group
{
	template <std::size_t mem_size, typename t_entity_id, typename t_local_entity_idx, ecs::component_type... t_cmp>
	struct basic
	{
		using t_archetype_traits = ecs::utility::archetype_traits<t_cmp...>;

		using t_archetype		 = t_archetype_traits::t_archetype;
		using t_entity_count	 = meta::smallest_unsigned_t<mem_size / sizeof(t_entity_id)>;
		using t_capacity		 = t_entity_count;
		using t_local_cmp_idx	 = t_archetype_traits::t_local_cmp_idx;
		using t_component_count	 = meta::smallest_unsigned_t<sizeof...(t_cmp)>;
		using t_component_size	 = meta::smallest_unsigned_t<std::ranges::max({ sizeof(t_cmp)... })>;
		using t_component_offset = decltype(mem_size);

		using align_info = ecs::utility::aligned_layout_info<t_archetype, t_entity_count, t_capacity, t_local_cmp_idx, t_component_count, t_component_size, t_component_offset>;

	  private:
		// alignas(std::max(align_info::max_alignof(), ecs::utility::max_alignof<t_cmp...>()))
		alignas(ecs::utility::max_alignof<t_cmp...>())
			std::byte storage[mem_size];

		// template <typename t>
		// consteval t& access_as()
		//{
		//	constexpr auto	offset = align_info::template offset_of<t>();
		//	constexpr auto* p_mem  = &storage[offset];

		//	static_assert((alignof(decltype(storage)) + offset) % alignof(t) == 0);

		//	return *reinterpret_cast<t*>(p_mem);
		//}

		template <typename t>
		inline t& access_as()
		{
			constexpr auto	offset = align_info::template offset_of<t>();
			constexpr auto* p_mem  = &storage[offset];

			assert((alignof(decltype(storage)) + offset) % alignof(t) == 0);

			return *reinterpret_cast<t*>(p_mem);
		}

		inline t_entity_count& entity_count() const
		{
			return access_as<t_entity_count>();
		}

		inline t_capacity& capacity() const
		{
			return access_as<t_capacity, sizeof(t_entity_count)>();
		}

	  public:
		template <typename... t>
		void init()
		{
		}

		void init(t_archetype archetype)
		{
		}

		t_local_entity_idx& entity_count()
		{
			static auto _ = t_local_entity_idx {};
			return _;
		}

		t_entity_id& entity_id(t_local_entity_idx local_ent_idx)
		{
			static auto _ = t_entity_id {};
			return _;
		}

		template <typename... t>
		t_local_entity_idx new_entity(t_entity_id entity_id)
		{
			return t_local_entity_idx {};
		}

		template <typename... t>
		t_local_entity_idx new_entity(t_entity_id entity_id, t&&... arg)
		{
			return t_local_entity_idx {};
		}

		t_local_entity_idx remove_entity(t_local_entity_idx local_ent_idx)
		{
			return t_local_entity_idx {};
		}

		void evict_component(const t_local_entity_idx local_ent_idx, const t_local_cmp_idx local_cmp_idx, void* p_dest)
		{
		}

		void evict_component(const t_local_entity_idx local_ent_idx, const t_local_cmp_idx local_cmp_idx)
		{
		}

		void* get_component_write_ptr(const t_local_cmp_idx local_cmp_idx)
		{
			access_as<t_capacity>() = 0;
			return nullptr;
		}

		template <typename... t>
		decltype(auto) get_component(const t_local_entity_idx local_ent_idx)
		{
			using ret_t = std::conditional_t<
				std::is_const_v<decltype(*this)>,
				std::conditional_t<
					(sizeof...(t) == 1),
					const meta::variadic_at_t<0, t&...>,
					const std::tuple<t&...>>,
				std::conditional_t<
					(sizeof...(t) == 1),
					meta::variadic_at_t<0, t&...>,
					std::tuple<t&...>>>;
			// static ret_t _;

			return std::tuple<t...> {};
		}

		bool is_full() const
		{
			return false;
		}
	};
}	 // namespace ecs::entity_group
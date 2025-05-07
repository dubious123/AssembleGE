#pragma once

namespace ecs::entity_group
{
	template <std::size_t mem_size, typename t_entity_id, typename t_local_entity_idx, ecs::component_type... t_cmp>
	struct basic
	{
		using t_archetype_traits = ecs::utility::archetype_traits<t_cmp...>;

		struct archetype_tag
		{
			using type = t_archetype_traits::t_archetype;
		};

		struct entity_id_tag
		{
			using type = t_entity_id;
		};

		struct entity_count_tag
		{
			using type = meta::smallest_unsigned_t<mem_size / sizeof(t_entity_id)>;
		};

		struct capacity_tag
		{
			using type = entity_count_tag::type;
		};

		struct local_cmp_idx_tag
		{
			using type = t_archetype_traits::t_local_cmp_idx;
		};

		struct component_count_tag
		{
			using type = meta::smallest_unsigned_t<sizeof...(t_cmp)>;
		};

		struct component_size_tag
		{
			using type = meta::smallest_unsigned_t<std::ranges::max({ sizeof(t_cmp)... })>;
		};

		struct component_offset_tag
		{
			using type = decltype(mem_size);
		};

		using t_archetype		 = archetype_tag::type;
		using t_entity_count	 = entity_count_tag::type;
		using t_capacity		 = capacity_tag::type;
		using t_local_cmp_idx	 = local_cmp_idx_tag::type;
		using t_component_count	 = component_count_tag::type;
		using t_component_size	 = component_size_tag::type;
		using t_component_offset = component_offset_tag::type;

		using align_info = ecs::utility::aligned_layout_info<archetype_tag, entity_count_tag, capacity_tag, component_count_tag>;

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

		template <typename t_tag>
		inline t_tag::type& access_as()
		{
			constexpr auto offset = align_info::template offset_of<t_tag>();
			auto*		   p_mem  = &storage[offset];

			assert((alignof(decltype(storage)) + offset) % alignof(typename t_tag::type) == 0);

			// not UB since c++20
			return *reinterpret_cast<t_tag::type*>(p_mem);
		}


	  public:
		inline t_entity_count& entity_count()
		{
			return access_as<entity_count_tag>();
		}

		inline t_capacity& capacity()
		{
			return access_as<capacity_tag>();
		}

		inline t_component_count& component_count()
		{
			return access_as<component_count_tag>();
		}

		inline t_archetype& local_archetype()
		{
			return access_as<archetype_tag>();
		}

		inline t_entity_id& entity_id(t_local_entity_idx local_ent_idx)
		{
			static auto _ = t_entity_id {};
			return _;
		}

		template <typename... t>
		void init()
		{
			std::println(
				"offset of entity_count {}\n"
				"offset of capacity {}\n"
				"offset of component_count {}\n"
				"offset of component_size {}\n"
				"offset of component_offset {}\n",
				align_info::template offset_of<entity_count_tag>(),
				align_info::template offset_of<capacity_tag>(),
				align_info::template offset_of<component_count_tag>(),
				align_info::template offset_of<component_size_tag>(),
				align_info::template offset_of<component_offset_tag>());

			capacity() = mem_size - align_info::total_size() -
		}

		void init(t_archetype archetype)
		{
			entity_count() = 0;
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
			capacity() = 0;
			return nullptr;
		}

		template <typename... t>
		decltype(auto) get_component(const t_local_entity_idx local_ent_idx)
		{
			// using ret_t = std::conditional_t<
			//	std::is_const_v<decltype(*this)>,
			//	std::conditional_t<
			//		(sizeof...(t) == 1),
			//		const meta::variadic_at_t<0, t&...>,
			//		const std::tuple<t&...>>,
			//	std::conditional_t<
			//		(sizeof...(t) == 1),
			//		meta::variadic_at_t<0, t&...>,
			//		std::tuple<t&...>>>;
			//  static ret_t _;

			return 1;
		}

		bool is_full() const
		{
			return false;
		}
	};
}	 // namespace ecs::entity_group
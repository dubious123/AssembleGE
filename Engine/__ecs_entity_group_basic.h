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
			using type = meta::smallest_unsigned_t<mem_size>;
		};

		struct cmp_offset_arr_base_tag
		{
			using type = meta::smallest_unsigned_t<mem_size>;
		};

		struct cmp_size_arr_base_tag
		{
			using type = meta::smallest_unsigned_t<mem_size>;
		};

		struct entity_id_arr_base_tag
		{
			using type = meta::smallest_unsigned_t<mem_size>;
		};

		template <typename t>
		struct component_tag
		{
			using type = t;
		};

		using t_archetype			= archetype_tag::type;
		using t_entity_count		= entity_count_tag::type;
		using t_capacity			= capacity_tag::type;
		using t_local_cmp_idx		= local_cmp_idx_tag::type;
		using t_component_count		= component_count_tag::type;
		using t_component_size		= component_size_tag::type;
		using t_component_offset	= component_offset_tag::type;
		using t_cmp_offset_arr_base = cmp_offset_arr_base_tag::type;
		using t_cmp_size_arr_base	= cmp_size_arr_base_tag::type;
		using t_entity_id_arr_base	= entity_id_arr_base_tag::type;

		using align_info = ecs::utility::aligned_layout_info<
			mem_size,
			archetype_tag,
			entity_count_tag,
			capacity_tag,
			component_count_tag,
			cmp_offset_arr_base_tag,
			cmp_size_arr_base_tag,
			entity_id_arr_base_tag>;
		// if alignment of A is 4
		// writing offset 20 => 20/4 = 5 instead of 20
		// reading offset of A => 5 * 4 = 20 instead of 5

		// alignment : 1, 4, 8, 16, 32, 64
		// shift : 1, 2, 3, 4, 5, 6
		// offset : uint8, uint16, uint32, uint64
	  private:
		// alignas(std::max(align_info::max_alignof(), ecs::utility::max_alignof<t_cmp...>()))
		alignas(ecs::utility::max_alignof<t_cmp...>())
			std::byte storage[mem_size];

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
			static t_entity_count _;
			return _;
			// return access_as<entity_count_tag>();
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

		inline cmp_size_arr_base_tag::type& component_size_arr_base()
		{
			return access_as<cmp_size_arr_base_tag>();
		}

		inline cmp_offset_arr_base_tag::type& component_offset_arr_base()
		{
			return access_as<cmp_size_arr_base_tag>();
		}

		inline entity_id_arr_base_tag::type& entity_id_arr_base()
		{
			return access_as<entity_id_arr_base_tag>();
		}

		inline t_entity_id& entity_id(t_local_entity_idx ent_idx)
		{
			return *(reinterpret_cast<t_entity_id*>(storage[entity_id_arr_base()]) + ent_idx);
		}

		template <cmp_size_arr_base_tag::type offset, t_local_cmp_idx cmp_idx>
		inline const t_component_size& cmp_size()
		{
			return *reinterpret_cast<t_component_size*>(&storage[offset + cmp_idx]);
		}

		inline const t_component_size& cmp_size(t_local_cmp_idx cmp_idx)
		{
			return *(reinterpret_cast<t_component_size*>(&storage[component_size_arr_base()]) + cmp_idx);
		}

		template <cmp_offset_arr_base_tag::type offset, t_local_cmp_idx cmp_idx>
		inline const t_component_offset& cmp_offset()
		{
			return *(reinterpret_cast<t_component_offset*>(&storage[offset]) + cmp_idx);
		}

		inline const t_component_offset& cmp_offset(t_local_cmp_idx cmp_idx)
		{
			return *(reinterpret_cast<t_component_offset*>(&storage[component_offset_arr_base()]) + cmp_idx);
		}

		template <typename... t>
		void init()
		{
			using header_align_info = align_info::template with<component_offset_tag, sizeof...(t)>::template with<component_size_tag, sizeof...(t)>::template with_soa<entity_id_tag, component_tag<t>...>;
			header_align_info::print();
			std::println(
				"component_offset_tag size : {} align : {}\n"
				"component_size_tag size : {} align : {}\n"
				"archetype_tag size : {} align : {}\n"
				"entity_count_tag size : {} align : {}\n"
				"capacity_tag size : {} align : {}\n"
				"component_count_tag size : {} align : {}\n"
				"cmp_offset_arr_base_tag size : {} align : {}\n"
				"cmp_size_arr_base_tag size : {} align : {}\n"
				"entity_id_arr_base_tag size : {} align : {}\n",

				sizeof(typename component_offset_tag::type), alignof(typename component_offset_tag::type),
				sizeof(typename component_size_tag::type), alignof(typename component_size_tag::type),
				sizeof(typename archetype_tag::type), alignof(typename archetype_tag::type),
				sizeof(typename entity_count_tag::type), alignof(typename entity_count_tag::type),
				sizeof(typename capacity_tag::type), alignof(typename capacity_tag::type),
				sizeof(typename component_count_tag::type), alignof(typename component_count_tag::type),
				sizeof(typename cmp_offset_arr_base_tag::type), alignof(typename cmp_offset_arr_base_tag::type),
				sizeof(typename cmp_size_arr_base_tag::type), alignof(typename cmp_size_arr_base_tag::type),
				sizeof(typename entity_id_arr_base_tag::type), alignof(typename entity_id_arr_base_tag::type));

			int a = 1;
			// sizeof t_component_offset > sizeof t_component_size


			// constexpr auto cmp_size_arr_base   = total_align_info::template offset_of<cmp_size_arr_base_tag>();
			// constexpr auto cmp_offset_arr_base = total_align_info::template offset_of<cmp_offset_arr_base_tag>();
			// constexpr auto ent_id_arr_base	   = total_align_info::template offset_of<entity_id_tag>();

			// capacity()					= total_align_info::count_of<entity_id_tag>();
			// component_count()			= sizeof...(t);
			// local_archetype()			= archetype_traits::calc_archetype<t...>();
			// entity_id_arr_base()		= ent_id_arr_base;
			// component_size_arr_base()	= cmp_size_arr_base;
			// component_offset_arr_base() = cmp_offset_arr_base;

			//[]<std::size_t... i>(std::index_sequence<i...> _) {
			//	using cmp_curr						 = meta::variadic_at_t<i, t...>;
			//	cmp_size<cmp_size_arr_base, i>()	 = sizeof(cmp_curr);
			//	cmp_offset<cmp_offset_arr_base, i>() = total_align_info::offset_of<component_tag<cmp_curr>>();
			//}(std::index_sequence_for<t...> {});
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
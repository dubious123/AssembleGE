#pragma once

namespace ecs::entity_group
{
	template <std::size_t mem_size, typename t_entity_id, typename t_local_entity_idx, ecs::component_type... t_cmp>
	struct basic
	{
		using t_archetype_traits = ecs::utility::archetype_traits<t_cmp...>;
		using t_storage_cmp_idx	 = t_archetype_traits::t_storage_cmp_idx;
		using t_self			 = basic<mem_size, t_entity_id, t_local_entity_idx, t_cmp...>;

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

		using align_info_builder = ecs::utility::layout_builder<
			archetype_tag,
			entity_count_tag,
			capacity_tag,
			component_count_tag,
			cmp_offset_arr_base_tag,
			cmp_size_arr_base_tag,
			entity_id_arr_base_tag>;

		using align_info = align_info_builder::template build<0, mem_size>;

		static constexpr std::size_t alignment = std::max(ecs::utility::max_alignof<t_component_offset, t_component_size, t_entity_id, t_cmp...>(), align_info::max_alignof());

		alignas(alignment)
			std::byte storage[mem_size];

		template <typename t_tag>
		inline t_tag::type& access_as()
		{
			constexpr auto offset = align_info::template offset_of<t_tag>();

			static_assert((alignment + offset) % alignof(typename t_tag::type) == 0);

			// not UB since c++20
			return *reinterpret_cast<t_tag::type*>(&storage[offset]);
		}

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
			return *(reinterpret_cast<t_entity_id*>(&storage[entity_id_arr_base()]) + ent_idx);
		}

		template <typename t>
		inline t_local_cmp_idx calc_cmp_idx()
		{
			return t_archetype_traits::template calc_local_cmp_idx<t>(local_archetype());
		}

		inline t_component_size& cmp_size(t_local_cmp_idx cmp_idx)
		{
			return *(reinterpret_cast<t_component_size*>(&storage[component_size_arr_base()]) + cmp_idx);
		}

		inline t_component_offset& cmp_offset(t_local_cmp_idx cmp_idx)
		{
			return *(reinterpret_cast<t_component_offset*>(&storage[component_offset_arr_base()]) + cmp_idx);
		}

		template <typename t>
		inline t_component_offset& cmp_offset()
		{
			return *(reinterpret_cast<t_component_offset*>(&storage[component_offset_arr_base()]) + calc_cmp_idx<t>());
		}

		template <typename t>
		inline t* get_cmp_ptr(t_local_entity_idx local_ent_idx)
		{
			return reinterpret_cast<t*>(&storage[cmp_offset<t>()] + local_ent_idx);
		}

		template <typename... t>
		void init()
		{
			// clang-format off
			using total_align_info = align_info_builder
				::template after_with_n<component_offset_tag, sizeof...(t)>
				::template with_n<component_size_tag, sizeof...(t)>
				::template with_flex<entity_id_tag, std::type_identity<t>...>
				::template build<0, mem_size>;
			// clang-format on

			total_align_info::print();
			std::println(
				"component_offset_tag size : {} align : {} , offset : {}\n"
				"component_size_tag size : {} align : {} , offset : {}\n"
				"archetype_tag size : {} align : {} , offset : {}\n"
				"entity_count_tag size : {} align : {} , offset : {}\n"
				"capacity_tag size : {} align : {} , offset : {}\n"
				"component_count_tag size : {} align : {} , offset : {}\n"
				"cmp_offset_arr_base_tag size : {} align : {} , offset : {}\n"
				"cmp_size_arr_base_tag size : {} align : {} , offset : {}\n"
				"entity_id_arr_base_tag size : {} align : {} , offset : {}\n",

				sizeof(typename component_offset_tag::type), alignof(typename component_offset_tag::type), total_align_info::template offset_of<component_offset_tag>(),
				sizeof(typename component_size_tag::type), alignof(typename component_size_tag::type), total_align_info::template offset_of<component_size_tag>(),
				sizeof(typename archetype_tag::type), alignof(typename archetype_tag::type), total_align_info::template offset_of<archetype_tag>(),
				sizeof(typename entity_count_tag::type), alignof(typename entity_count_tag::type), total_align_info::template offset_of<entity_count_tag>(),
				sizeof(typename capacity_tag::type), alignof(typename capacity_tag::type), total_align_info::template offset_of<capacity_tag>(),
				sizeof(typename component_count_tag::type), alignof(typename component_count_tag::type), total_align_info::template offset_of<component_count_tag>(),
				sizeof(typename cmp_offset_arr_base_tag::type), alignof(typename cmp_offset_arr_base_tag::type), total_align_info::template offset_of<cmp_offset_arr_base_tag>(),
				sizeof(typename cmp_size_arr_base_tag::type), alignof(typename cmp_size_arr_base_tag::type), total_align_info::template offset_of<cmp_size_arr_base_tag>(),
				sizeof(typename entity_id_arr_base_tag::type), alignof(typename entity_id_arr_base_tag::type), total_align_info::template offset_of<entity_id_arr_base_tag>());

			capacity()					= total_align_info::template count_of<entity_id_tag>();
			component_count()			= sizeof...(t);
			local_archetype()			= t_archetype_traits::template calc_archetype<t...>();
			entity_id_arr_base()		= total_align_info::template offset_of<entity_id_tag>();
			component_size_arr_base()	= total_align_info::template offset_of<component_size_tag>();
			component_offset_arr_base() = total_align_info::template offset_of<component_offset_tag>();

			[this]<std::size_t... local_cmp_idx>(std::index_sequence<local_cmp_idx...> _) {
				([this] {
					using cmp_curr																													 = meta::variadic_at_t<local_cmp_idx, t...>;
					*(reinterpret_cast<t_component_size*>(&storage[total_align_info::template offset_of<component_size_tag>()]) + local_cmp_idx)	 = sizeof(cmp_curr);
					*(reinterpret_cast<t_component_offset*>(&storage[total_align_info::template offset_of<component_offset_tag>()]) + local_cmp_idx) = total_align_info::template offset_of<std::type_identity<cmp_curr>>();
				}(),
				 ...);
			}(std::index_sequence_for<t...> {});
		}

		void init(t_archetype archetype)
		{
			using namespace std::ranges::views;

			auto cmp_count		  = t_archetype_traits::cmp_count(archetype);
			auto total_align_info = ecs::utility::layout_builder_runtime(
				ecs::utility::with_n<component_offset_tag>(cmp_count),
				ecs::utility::with_n<component_size_tag>(cmp_count),
				ecs::utility::with_flex<entity_id_tag>());

			for (auto storage_cmp_idx : std::ranges::views::iota(0, std::bit_width(archetype)) | std::ranges::views::filter([archetype](auto idx) { return (archetype >> idx) & 1; }))
			{
				total_align_info.add_flex(ecs::utility::type_layout_info { t_archetype_traits::cmp_size(storage_cmp_idx), t_archetype_traits::cmp_alignment(storage_cmp_idx) });
			}

			total_align_info.build(align_info::total_size(), mem_size);

			total_align_info.print();

			capacity()					= total_align_info.count_of<entity_id_tag>();
			component_count()			= cmp_count;
			local_archetype()			= archetype;
			entity_id_arr_base()		= total_align_info.offset_of<entity_id_tag>();
			component_size_arr_base()	= total_align_info.offset_of<component_size_tag>();
			component_offset_arr_base() = total_align_info.offset_of<component_offset_tag>();


			for (auto [local_cmp_idx, storage_cmp_idx] : iota(0, std::bit_width(archetype))
															 | filter([archetype](auto idx) { return (archetype >> idx) & 1; })
															 | enumerate)
			{
				cmp_offset(local_cmp_idx) = static_cast<t_component_offset>(total_align_info.offset_of(local_cmp_idx));
				cmp_size(local_cmp_idx)	  = static_cast<t_component_size>(t_archetype_traits::cmp_size(storage_cmp_idx));
			}
		}

		void init(t_self& other)
		{
			std::memcpy(other.storage, storage, align_info::total_size());

			auto cmp_count	 = t_archetype_traits::cmp_count(local_archetype());
			auto offset_base = component_offset_arr_base();
			auto size_base	 = component_size_arr_base();

			std::memcpy(&other.storage[offset_base], &storage[offset_base], sizeof(t_component_offset) * cmp_count);
			std::memcpy(&other.storage[size_base], &storage[size_base], sizeof(t_component_size) * cmp_count);
		}

		template <typename... t>
		t_local_entity_idx new_entity(t_entity_id ent_id)
		{
			assert(not is_full());

			auto local_ent_idx		 = static_cast<t_local_entity_idx>(entity_count());
			entity_id(local_ent_idx) = ent_id;

			(std::construct_at<t>(get_cmp_ptr<t>(local_ent_idx)), ...);
			return local_ent_idx;
		}

		template <typename... t>
		t_local_entity_idx new_entity(t_entity_id ent_id, t&&... arg)
		{
			assert(not is_full());

			auto local_ent_idx		 = static_cast<t_local_entity_idx>(entity_count());
			entity_id(local_ent_idx) = ent_id;

			(std::construct_at<t>(get_cmp_ptr<t>(local_ent_idx, std::forward<t>(arg))), ...);
			return local_ent_idx;
		}

		t_local_entity_idx remove_entity(t_local_entity_idx local_ent_idx)
		{
			assert(not is_empty());

			t_local_cmp_idx local_ent_idx_back = =

				for (t_local_cmp_idx local_cmp_idx : std::iota(0, component_count()))
			{
			}

			return local_ent_idx;
		}

		void evict_component(const t_local_entity_idx local_ent_idx, const t_local_cmp_idx local_cmp_idx, void* p_dest)
		{
		}

		void evict_component(const t_local_entity_idx local_ent_idx, const t_local_cmp_idx local_cmp_idx)
		{
		}

		void* get_component_write_ptr(const t_local_cmp_idx local_cmp_idx)
		{
			return nullptr;
		}

		template <ecs::component_type... t>
		decltype(auto) get_component(const t_local_entity_idx local_ent_idx)
		{
			static_assert((true && ... && !std::is_reference_v<t>), "no reference type component");

			if constexpr (sizeof...(t) == 1)
			{
				using t_ret = meta::variadic_at_t<0, t...>;
				return *reinterpret_cast<t_ret*>(&storage[cmp_offset<t_ret>()] + local_ent_idx);
			}
			else
			{
				const auto arch = local_archetype();

				return std::tuple<t&...> { (*reinterpret_cast<t*>(&storage[component_offset_arr_base(t_archetype_traits::template calc_local_cmp_idx<t>(arch))] + local_ent_idx))... };
			}
		}

		bool is_full() const
		{
			return capacity() == entity_count();
		}

		bool is_empty() const
		{
			return entity_count() == 0;
		}
	};
}	 // namespace ecs::entity_group
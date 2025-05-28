#pragma once

namespace ecs::entity_group
{
	template <std::size_t mem_size, typename t_entity_id, typename t_entity_group_idx, ecs::component_type... t_cmp>
	struct basic
	{
		using t_self			 = basic<mem_size, t_entity_id, t_entity_group_idx, t_cmp...>;
		using t_archetype_traits = ecs::utility::archetype_traits<t_cmp...>;
		using t_storage_cmp_idx	 = t_archetype_traits::t_storage_cmp_idx;
		using t_local_entity_idx = meta::smallest_unsigned_t<mem_size / sizeof(t_entity_id)>;

		// group can't be more than entity counts ?
		struct entity_group_idx_tag
		{
			using type = t_entity_group_idx;
		};

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
			using type = t_local_entity_idx;
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
			using type = t_archetype_traits::t_component_count;
		};

		struct component_size_tag
		{
			using type = t_archetype_traits::t_component_size;
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
			entity_group_idx_tag,
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

		inline t_entity_group_idx& entity_group_idx()
		{
			return access_as<entity_group_idx_tag>();
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
			return access_as<cmp_offset_arr_base_tag>();
		}

		inline entity_id_arr_base_tag::type& entity_id_arr_base()
		{
			return access_as<entity_id_arr_base_tag>();
		}

		inline t_entity_id& ent_id(t_local_entity_idx ent_idx)
		{
			return *(reinterpret_cast<t_entity_id*>(&storage[entity_id_arr_base()]) + ent_idx);
		}

		template <typename t>
		inline t_local_cmp_idx calc_cmp_idx()
		{
			return t_archetype_traits::template calc_local_cmp_idx<t>(local_archetype());
		}

		template <typename t>
		inline t_component_size& cmp_size()
		{
			return *(reinterpret_cast<t_component_size*>(&storage[component_size_arr_base()]) + calc_cmp_idx<t>());
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
		inline t* cmp_ptr(t_local_entity_idx local_ent_idx)
		{
			return reinterpret_cast<t*>(&storage[cmp_offset<t>()] + local_ent_idx);
		}

		inline std::byte* cmp_ptr(t_local_cmp_idx local_cmp_idx, t_local_entity_idx local_ent_idx)
		{
			return &storage[cmp_offset(local_cmp_idx)] + local_ent_idx;
		}

		template <typename... t>
		void init(t_entity_group_idx group_idx)
		{
			// clang-format off
			using total_align_info = align_info_builder
				::template after_with_n<component_offset_tag, sizeof...(t)>
				::template with_n<component_size_tag, sizeof...(t)>
				::template with_flex<entity_id_tag, std::type_identity<t>...>
				::template build<0, mem_size>;
			// clang-format on

			// total_align_info::print();
			// std::println(
			//	"component_offset_tag size : {} align : {} , offset : {}\n"
			//	"component_size_tag size : {} align : {} , offset : {}\n"
			//	"archetype_tag size : {} align : {} , offset : {}\n"
			//	"entity_count_tag size : {} align : {} , offset : {}\n"
			//	"capacity_tag size : {} align : {} , offset : {}\n"
			//	"component_count_tag size : {} align : {} , offset : {}\n"
			//	"cmp_offset_arr_base_tag size : {} align : {} , offset : {}\n"
			//	"cmp_size_arr_base_tag size : {} align : {} , offset : {}\n"
			//	"entity_id_arr_base_tag size : {} align : {} , offset : {}\n",

			//	sizeof(typename component_offset_tag::type), alignof(typename component_offset_tag::type), total_align_info::template offset_of<component_offset_tag>(),
			//	sizeof(typename component_size_tag::type), alignof(typename component_size_tag::type), total_align_info::template offset_of<component_size_tag>(),
			//	sizeof(typename archetype_tag::type), alignof(typename archetype_tag::type), total_align_info::template offset_of<archetype_tag>(),
			//	sizeof(typename entity_count_tag::type), alignof(typename entity_count_tag::type), total_align_info::template offset_of<entity_count_tag>(),
			//	sizeof(typename capacity_tag::type), alignof(typename capacity_tag::type), total_align_info::template offset_of<capacity_tag>(),
			//	sizeof(typename component_count_tag::type), alignof(typename component_count_tag::type), total_align_info::template offset_of<component_count_tag>(),
			//	sizeof(typename cmp_offset_arr_base_tag::type), alignof(typename cmp_offset_arr_base_tag::type), total_align_info::template offset_of<cmp_offset_arr_base_tag>(),
			//	sizeof(typename cmp_size_arr_base_tag::type), alignof(typename cmp_size_arr_base_tag::type), total_align_info::template offset_of<cmp_size_arr_base_tag>(),
			//	sizeof(typename entity_id_arr_base_tag::type), alignof(typename entity_id_arr_base_tag::type), total_align_info::template offset_of<entity_id_arr_base_tag>());

			entity_group_idx()			= group_idx;
			capacity()					= total_align_info::template count_of<entity_id_tag>();
			entity_count()				= 0;
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
			}(std::index_sequence_for<t...>{});
		}

		void init(t_archetype archetype, t_entity_group_idx group_idx)
		{
			using namespace std::ranges::views;

			auto cmp_count		  = t_archetype_traits::cmp_count(archetype);
			auto total_align_info = ecs::utility::layout_builder_runtime(
				ecs::utility::with_n<component_offset_tag>(cmp_count),
				ecs::utility::with_n<component_size_tag>(cmp_count),
				ecs::utility::with_flex<entity_id_tag>());

			for (auto storage_cmp_idx : std::ranges::views::iota(0, std::bit_width(archetype)) | std::ranges::views::filter([archetype](auto idx) { return (archetype >> idx) & 1; }))
			{
				total_align_info.add_flex(ecs::utility::type_layout_info{ t_archetype_traits::cmp_size(storage_cmp_idx), t_archetype_traits::cmp_alignment(storage_cmp_idx) });
			}

			total_align_info.build(align_info::total_size(), mem_size);

			// total_align_info.print();

			entity_group_idx()			= group_idx;
			capacity()					= static_cast<t_capacity>(total_align_info.count_of<entity_id_tag>());
			entity_count()				= 0;
			component_count()			= cmp_count;
			local_archetype()			= archetype;
			entity_id_arr_base()		= static_cast<t_entity_id_arr_base>(total_align_info.offset_of<entity_id_tag>());
			component_size_arr_base()	= static_cast<t_cmp_size_arr_base>(total_align_info.offset_of<component_size_tag>());
			component_offset_arr_base() = static_cast<t_cmp_offset_arr_base>(total_align_info.offset_of<component_offset_tag>());


			for (auto [idx, storage_cmp_idx] : iota(0, std::bit_width(archetype))
												   | filter([archetype](auto idx) { return (archetype >> idx) & 1; })
												   | enumerate)
			{
				auto local_cmp_idx		  = static_cast<t_local_cmp_idx>(idx);
				cmp_offset(local_cmp_idx) = static_cast<t_component_offset>(total_align_info.offset_of(local_cmp_idx));
				cmp_size(local_cmp_idx)	  = t_archetype_traits::cmp_size(storage_cmp_idx);
			}
		}

		void init(t_self& other, t_entity_group_idx group_idx)
		{
			std::memcpy(storage, other.storage, align_info::total_size());

			auto cmp_count	 = t_archetype_traits::cmp_count(local_archetype());
			auto offset_base = component_offset_arr_base();
			auto size_base	 = component_size_arr_base();

			std::memcpy(&storage[offset_base], &other.storage[offset_base], sizeof(t_component_offset) * cmp_count);
			std::memcpy(&storage[size_base], &other.storage[size_base], sizeof(t_component_size) * cmp_count);

			entity_group_idx() = group_idx;
			entity_count()	   = 0;
		}

		template <typename... t>
		t_local_entity_idx new_entity(t_entity_id entity_id)
		{
			assert(is_full() is_false);

			auto local_ent_idx	  = static_cast<t_local_entity_idx>(entity_count()++);
			ent_id(local_ent_idx) = entity_id;

			(std::construct_at<t>(cmp_ptr<t>(local_ent_idx)), ...);
			return local_ent_idx;
		}

		template <typename... t>
		t_local_entity_idx new_entity(t_entity_id entity_id, t&&... arg)
		{
			assert(is_full() is_false);

			auto local_ent_idx	  = static_cast<t_local_entity_idx>(entity_count()++);
			ent_id(local_ent_idx) = entity_id;

			((std::construct_at<t>(cmp_ptr<t>(local_ent_idx), std::forward<t>(arg))), ...);
			return local_ent_idx;
		}

		// Removes the entity at the given index. Returns the id of the entity that was moved to fill the hole, if any, for handle updates.
		t_entity_id& remove_entity(t_local_entity_idx local_ent_idx)
		{
			assert(is_empty() is_false);

			auto local_ent_idx_back = static_cast<t_local_entity_idx>(--entity_count());

			for (t_local_cmp_idx local_cmp_idx : std::views::iota(0, (int)component_count()))
			{
				std::memcpy(cmp_ptr(local_cmp_idx, local_ent_idx), cmp_ptr(local_cmp_idx, local_ent_idx_back), cmp_size(local_cmp_idx));
			}

			ent_id(local_ent_idx) = ent_id(local_ent_idx_back);

			return ent_id(local_ent_idx);
		}

		void evict_component(const t_local_entity_idx local_ent_idx, const t_local_cmp_idx local_cmp_idx, void* p_dest)
		{
			assert(is_empty() is_false);
			auto		local_ent_idx_back = static_cast<t_local_cmp_idx>(entity_count() - 1);
			const auto& component_size	   = cmp_size(local_cmp_idx);
			std::memcpy(p_dest, cmp_ptr(local_cmp_idx, local_ent_idx), component_size);

			std::memcpy(cmp_ptr(local_cmp_idx, local_ent_idx), cmp_ptr(local_cmp_idx, local_ent_idx_back), component_size);
		}

		void evict_component(const t_local_entity_idx local_ent_idx, const t_local_cmp_idx local_cmp_idx)
		{
			assert(is_empty() is_false);
			auto local_ent_idx_back = static_cast<t_local_cmp_idx>(entity_count() - 1);
			std::memcpy(cmp_ptr(local_cmp_idx, local_ent_idx), cmp_ptr(local_cmp_idx, local_ent_idx_back), cmp_size(local_cmp_idx));
		}

		void* get_component_write_ptr(const t_local_cmp_idx local_cmp_idx)
		{
			assert(is_full() is_false);
			return reinterpret_cast<void*>(cmp_ptr(local_cmp_idx, entity_count()));
		}

		template <ecs::component_type... t>
		decltype(auto) get_component(const t_local_entity_idx local_ent_idx)
		{
			static_assert((true && ... && !std::is_reference_v<t>), "no reference type component");

			if constexpr (sizeof...(t) == 1)
			{
				using t_ret = meta::variadic_at_t<0, t...>;
				return *cmp_ptr<t_ret>(local_ent_idx);
			}
			else
			{
				return std::tuple<t&...>{ (*cmp_ptr<t>(local_ent_idx))... };
			}
		}

		bool is_full()
		{
			return capacity() == entity_count();
		}

		bool is_empty()
		{
			return entity_count() == 0;
		}
	};
}	 // namespace ecs::entity_group
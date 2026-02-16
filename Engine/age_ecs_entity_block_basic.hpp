#pragma once

namespace age::ecs::entity_block
{
	template <std::size_t mem_size, typename t_entity_id, typename t_entity_block_idx, age::ecs::cx_component... t_cmp>
	struct basic
	{
		using t_self			 = basic<mem_size, t_entity_id, t_entity_block_idx, t_cmp...>;
		using t_ent_id			 = t_entity_id;
		using t_ent_block_idx	 = t_entity_block_idx;
		using t_archetype_traits = age::ecs::archetype_traits<t_cmp...>;
		using t_storage_cmp_idx	 = t_archetype_traits::t_storage_cmp_idx;
		using t_local_entity_idx = age::meta::smallest_unsigned_t<mem_size / sizeof(t_ent_id)>;

		// block can't be more than entity counts ?
		struct entity_block_idx_tag
		{
			using type = t_entity_block_idx;
		};

		struct archetype_tag
		{
			using type = t_archetype_traits::t_archetype;
		};

		struct entity_id_tag
		{
			using type = t_ent_id;
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
			using type = age::meta::smallest_unsigned_t<mem_size>;
		};

		struct cmp_offset_arr_base_tag
		{
			using type = age::meta::smallest_unsigned_t<mem_size>;
		};

		struct cmp_size_arr_base_tag
		{
			using type = age::meta::smallest_unsigned_t<mem_size>;
		};

		struct entity_id_arr_base_tag
		{
			using type = age::meta::smallest_unsigned_t<mem_size>;
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

		using align_info_builder = age::util::layout_builder<
			entity_block_idx_tag,
			archetype_tag,
			entity_count_tag,
			capacity_tag,
			component_count_tag,
			cmp_offset_arr_base_tag,
			cmp_size_arr_base_tag,
			entity_id_arr_base_tag>;

		using align_info = align_info_builder::template build<0, mem_size>;

		static constexpr std::size_t alignment = std::max(age::util::max_alignof<t_component_offset, t_component_size, t_ent_id, t_cmp...>(), align_info::max_alignof());

		alignas(alignment)
			std::byte storage[mem_size];

		template <typename t_tag>
		FORCE_INLINE t_tag::type&
		access_as()
		{
			constexpr auto offset = align_info::template offset_of<t_tag>();

			static_assert((alignment + offset) % alignof(typename t_tag::type) == 0);

			return *age::util::start_lifetime_as<typename t_tag::type>(&storage[offset]);
		}

		FORCE_INLINE t_entity_block_idx&
		entity_block_idx()
		{
			return access_as<entity_block_idx_tag>();
		}

		FORCE_INLINE t_entity_count&
		entity_count()
		{
			return access_as<entity_count_tag>();
		}

		FORCE_INLINE t_capacity&
		capacity()
		{
			return access_as<capacity_tag>();
		}

		FORCE_INLINE t_component_count&
		component_count()
		{
			return access_as<component_count_tag>();
		}

		FORCE_INLINE t_archetype&
		local_archetype()
		{
			return access_as<archetype_tag>();
		}

		FORCE_INLINE t_cmp_size_arr_base&
		component_size_arr_base()
		{
			return access_as<cmp_size_arr_base_tag>();
		}

		FORCE_INLINE t_cmp_offset_arr_base&
		component_offset_arr_base()
		{
			return access_as<cmp_offset_arr_base_tag>();
		}

		FORCE_INLINE t_entity_id_arr_base&
		entity_id_arr_base()
		{
			return access_as<entity_id_arr_base_tag>();
		}

		FORCE_INLINE t_ent_id&
		ent_id(t_local_entity_idx ent_idx)
		{
			return *age::util::start_lifetime_as<t_ent_id>(reinterpret_cast<t_entity_id*>(&storage[entity_id_arr_base()]) + ent_idx);
		}

		template <typename t>
		FORCE_INLINE t_local_cmp_idx
		calc_cmp_idx()
		{
			return t_archetype_traits::template calc_local_cmp_idx<t>(local_archetype());
		}

		FORCE_INLINE t_component_size&
		cmp_size(t_local_cmp_idx cmp_idx)
		{
			AGE_ASSERT(*age::util::start_lifetime_as<t_component_size>(reinterpret_cast<t_component_size*>(&storage[component_size_arr_base()]) + cmp_idx) > 0);
			return *age::util::start_lifetime_as<t_component_size>(reinterpret_cast<t_component_size*>(&storage[component_size_arr_base()]) + cmp_idx);
		}

		template <typename t>
		FORCE_INLINE t_component_size
		cmp_size()
		{
			AGE_ASSERT(cmp_size(calc_cmp_idx<t>()) == sizeof(t));
			return static_cast<t_component_size>(sizeof(t));
		}

		FORCE_INLINE t_component_offset&
		cmp_offset(t_local_cmp_idx cmp_idx)
		{
			return *age::util::start_lifetime_as<t_component_offset>(reinterpret_cast<t_component_offset*>(&storage[component_offset_arr_base()]) + cmp_idx);
		}

		template <typename t>
		FORCE_INLINE t_component_offset&
		cmp_offset()
		{
			return *age::util::start_lifetime_as<t_component_offset>(reinterpret_cast<t_component_offset*>(&storage[component_offset_arr_base()]) + calc_cmp_idx<t>());
		}

		template <typename t>
		FORCE_INLINE t*
		cmp_ptr(t_local_entity_idx local_ent_idx)
		{
			return reinterpret_cast<t*>(&storage[cmp_offset<t>()]) + local_ent_idx;
		}

		FORCE_INLINE
		std::byte*
		cmp_ptr(t_local_cmp_idx local_cmp_idx, t_local_entity_idx local_ent_idx)
		{
			return &storage[cmp_offset(local_cmp_idx)] + local_ent_idx * cmp_size(local_cmp_idx);
		}

		template <typename... t>
		void
		init(t_entity_block_idx block_idx)
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

			// static_assert(total_align_info::template count_of<component_size_tag>() == sizeof...(t));

			entity_block_idx()			= block_idx;
			capacity()					= total_align_info::template count_of<entity_id_tag>();
			entity_count()				= 0;
			component_count()			= sizeof...(t);
			local_archetype()			= t_archetype_traits::template calc_archetype<t...>();
			entity_id_arr_base()		= total_align_info::template offset_of<entity_id_tag>();
			component_size_arr_base()	= total_align_info::template offset_of<component_size_tag>();
			component_offset_arr_base() = total_align_info::template offset_of<component_offset_tag>();

			[this]<std::size_t... local_cmp_idx>(std::index_sequence<local_cmp_idx...>) {
				([this] {
					using cmp_curr = age::meta::variadic_at_t<local_cmp_idx, t...>;
					buffer::write_bytes(reinterpret_cast<t_component_size*>(&storage[total_align_info::template offset_of<component_size_tag>()]) + local_cmp_idx,
										static_cast<t_component_size>(sizeof(cmp_curr)));

					buffer::write_bytes(reinterpret_cast<t_component_offset*>(&storage[total_align_info::template offset_of<component_offset_tag>()]) + local_cmp_idx,
										static_cast<t_component_offset>(total_align_info::template offset_of<std::type_identity<cmp_curr>>()));
				}(),
				 ...);
			}(std::index_sequence_for<t...>{});
		}

		void
		init(t_archetype archetype, t_entity_block_idx block_idx)
		{
			using namespace std::ranges::views;

			auto cmp_count		  = t_archetype_traits::cmp_count(archetype);
			auto total_align_info = age::util::layout_builder_runtime(
				age::util::with_n<component_offset_tag>(cmp_count),
				age::util::with_n<component_size_tag>(cmp_count),
				age::util::with_flex<entity_id_tag>());

			for (auto storage_cmp_idx : std::ranges::views::iota(0, std::bit_width(archetype)) | std::ranges::views::filter([archetype](auto idx) { return (archetype >> idx) & 1; }))
			{
				total_align_info.add_flex(age::util::type_layout_info{ t_archetype_traits::cmp_size(storage_cmp_idx), t_archetype_traits::cmp_alignment(storage_cmp_idx) });
			}

			total_align_info.build(align_info::total_size(), mem_size);

			total_align_info.print();

			entity_block_idx()			= block_idx;
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

		void
		init(t_self& other, t_entity_block_idx block_idx)
		{
			std::memcpy(storage, other.storage, align_info::total_size());

			auto cmp_count	 = t_archetype_traits::cmp_count(local_archetype());
			auto offset_base = component_offset_arr_base();
			auto size_base	 = component_size_arr_base();

			std::memcpy(&storage[offset_base], &other.storage[offset_base], sizeof(t_component_offset) * cmp_count);
			std::memcpy(&storage[size_base], &other.storage[size_base], sizeof(t_component_size) * cmp_count);

			entity_block_idx() = block_idx;
			entity_count()	   = 0;
		}

		template <typename... t>
		t_local_entity_idx
		new_entity(t_ent_id entity_id)
		{
			AGE_ASSERT(is_full() is_false);

			auto local_ent_idx	  = static_cast<t_local_entity_idx>(entity_count()++);
			ent_id(local_ent_idx) = entity_id;

			(std::construct_at<t>(cmp_ptr<t>(local_ent_idx)), ...);
			return local_ent_idx;
		}

		template <typename... t>
		t_local_entity_idx
		new_entity(t_ent_id entity_id, t&&... arg)
		{
			AGE_ASSERT(is_full() is_false);

			auto local_ent_idx	  = static_cast<t_local_entity_idx>(entity_count()++);
			ent_id(local_ent_idx) = entity_id;

			((std::construct_at<t>(cmp_ptr<t>(local_ent_idx), std::forward<t>(arg))), ...);
			return local_ent_idx;
		}

		// Removes the entity at the given index. Returns the id of the entity that was moved to fill the hole, if any, for handle updates.
		t_ent_id&
		remove_entity(t_local_entity_idx local_ent_idx)
		{
			AGE_ASSERT(is_empty() is_false);

			auto local_ent_idx_back = static_cast<t_local_entity_idx>(--entity_count());

			for (t_local_cmp_idx local_cmp_idx : std::views::iota(0, (int)component_count()))
			{
				std::memcpy(cmp_ptr(local_cmp_idx, local_ent_idx), cmp_ptr(local_cmp_idx, local_ent_idx_back), cmp_size(local_cmp_idx));
			}

			ent_id(local_ent_idx) = ent_id(local_ent_idx_back);

			return ent_id(local_ent_idx);
		}

		FORCE_INLINE void
		evict_component(const t_local_entity_idx local_ent_idx, const t_local_cmp_idx local_cmp_idx, void* p_dest)
		{
			AGE_ASSERT(is_empty() is_false);
			auto		local_ent_idx_back = static_cast<t_local_cmp_idx>(entity_count() - 1);
			const auto& component_size	   = cmp_size(local_cmp_idx);

			AGE_ASSERT((cmp_ptr(local_cmp_idx, local_ent_idx_back) - cmp_ptr(local_cmp_idx, local_ent_idx)) / component_size == local_ent_idx_back - local_ent_idx);

			std::memcpy(p_dest, cmp_ptr(local_cmp_idx, local_ent_idx), component_size);
			std::memcpy(cmp_ptr(local_cmp_idx, local_ent_idx), cmp_ptr(local_cmp_idx, local_ent_idx_back), component_size);
		}

		FORCE_INLINE void
		evict_component(const t_local_entity_idx local_ent_idx, const t_local_cmp_idx local_cmp_idx)
		{
			AGE_ASSERT(is_empty() is_false);
			auto local_ent_idx_back = static_cast<t_local_cmp_idx>(entity_count() - 1);
			std::memcpy(cmp_ptr(local_cmp_idx, local_ent_idx), cmp_ptr(local_cmp_idx, local_ent_idx_back), cmp_size(local_cmp_idx));
		}

		FORCE_INLINE void*
		get_component_write_ptr(const t_local_cmp_idx local_cmp_idx)
		{
			AGE_ASSERT(is_full() is_false);
			return reinterpret_cast<void*>(cmp_ptr(local_cmp_idx, entity_count()));
		}

		template <age::ecs::cx_component... t>
		FORCE_INLINE decltype(auto)
		get_component(const t_local_entity_idx local_ent_idx)
		{
			static_assert((true && ... && !std::is_rvalue_reference_v<t>), "no rvalue reference type component");

			return std::tuple<t...>{ (*cmp_ptr<std::remove_reference_t<t>>(local_ent_idx))... };
		}

		template <typename t_sys, template <typename...> typename t_cmp_pack, age::ecs::cx_component... t>
		FORCE_INLINE void
		run_sys(t_sys&& sys, t_local_entity_idx local_ent_idx, t_cmp_pack<t...>)
		{
			age::ecs::system::run_sys(FWD(sys), std::get<0>(get_component<t>(local_ent_idx))...);
		}

		template <typename t_sys>
		FORCE_INLINE void
		foreach_entity(t_sys&& sys)
		{
			using t_arg_tpl		 = age::meta::function_traits<&std::decay_t<t_sys>::operator()>::argument_types;
			constexpr auto arity = age::meta::function_traits<&std::decay_t<t_sys>::operator()>::arity;

			for (t_local_entity_idx local_ent_idx : std::views::iota(0) | std::views::take(entity_count()))
			{
				this->run_sys(sys, local_ent_idx, age::meta::tpl_to_type_pack<t_arg_tpl>{});
			}
		}

		FORCE_INLINE bool
		is_full()
		{
			return capacity() == entity_count();
		}

		FORCE_INLINE bool
		is_empty()
		{
			return entity_count() == 0;
		}
	};
}	 // namespace age::ecs::entity_block
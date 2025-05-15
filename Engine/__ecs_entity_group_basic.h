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

	  private:
		template <typename t_tag>
		inline t_tag::type& access_as()
		{
			constexpr auto offset = align_info::template offset_of<t_tag>();

			static_assert((alignment + offset) % alignof(typename t_tag::type) == 0);

			// not UB since c++20
			return *reinterpret_cast<t_tag::type*>(&storage[offset]);
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

		// template <typename t>
		// void quick_sort(t* p_data, std::size_t low, std::size_t high, auto comp)
		//{
		//	auto pivot_idx = low;
		// }

		void init(t_archetype archetype)
		{
			using namespace std::ranges::views;
			auto cmp_count = t_archetype_traits::cmp_count(archetype);

			// auto* cmp_info_arr	= ::new ecs::utility::type_layout_info[cmp_count];
			// auto  cmp_info_view = std::span<const ecs::utility::type_layout_info>(cmp_info_arr, cmp_count);

			auto&& layout_info_view = iota(0, std::bit_width(archetype))
									| filter([archetype](auto idx) { return (archetype >> idx) & 1; })
									| transform([](auto idx) {
										  return ecs::utility::type_layout_info {
											  t_archetype_traits::cmp_size(idx),
											  t_archetype_traits::cmp_alignment(idx)
										  };
									  });
			// auto layout_info_view = iota(0, std::bit_width(archetype))
			//					  | filter([archetype](auto idx) { return (archetype >> idx) & 1; })
			//					  | transform([](auto idx) {
			//							return ecs::utility::type_layout_info {
			//								t_archetype_traits::cmp_size(idx),
			//								t_archetype_traits::cmp_alignment(idx)
			//							};
			//						});

			// for (auto [local_cmp_idx, storage_cmp_idx] : iota(0, std::bit_width(archetype))
			//												 | filter([archetype](auto idx) { return (archetype >> idx) & 1; })
			//												 | enumerate)
			//{
			//	cmp_info_view[local_cmp_idx].first	= t_archetype_traits::cmp_alignment(storage_cmp_idx);
			//	cmp_info_view[local_cmp_idx].second = t_archetype_traits::cmp_size(storage_cmp_idx);
			// }

			using tagg = decltype(ecs::utility::with_flex<entity_id_tag>())::tag_type;
			static_assert(std::is_same_v<tagg, entity_id_tag>);
			std::println("{}", sizeof(std::decay_t<decltype(layout_info_view)>));

			auto total_align_info = ecs::utility::layout_builder_runtime(
										ecs::utility::with_n<component_offset_tag>(cmp_count),
										ecs::utility::with_n<component_size_tag>(cmp_count),
										ecs::utility::with_flex<entity_id_tag>(),
										ecs::utility::with_flex(std::forward<decltype(layout_info_view)>(layout_info_view)))
										// ecs::utility::with_flex(layout_info_view))
										.build(align_info::total_size(), mem_size);

			std::println("{}", total_align_info.count_of<component_offset_tag>());
			std::println("{}", std::ranges::distance(layout_info_view));


			capacity()					= total_align_info.count_of<entity_id_tag>();
			component_count()			= cmp_count;
			local_archetype()			= archetype;
			entity_id_arr_base()		= total_align_info.offset_of<entity_id_tag>();
			component_size_arr_base()	= total_align_info.offset_of<component_size_tag>();
			component_offset_arr_base() = total_align_info.offset_of<component_offset_tag>();

			// size, alignment, count

			// compile_time buffer
			auto compile_typeinfo_buffer = std::array<std::tuple<std::size_t, std::size_t, std::size_t>, 3> {
				std::tuple { sizeof(t_component_offset), alignof(t_component_offset), cmp_count },
				std::tuple { sizeof(t_component_size), alignof(t_component_size), cmp_count },
				std::tuple { sizeof(t_entity_id), alignof(t_entity_id), -1 }
			};

			// 1. with_n, flex
			// 2. sort by alignment (quick_sort)
			// 3. offset_of<_type> ??


			// type -> index -> offset
			auto compile_type_index = std::array<std::size_t, 3> { 0, 1, 2 };
			// local_cmp_idx -> index -> offset
			auto runtime_type_index = std::array<std::size_t, sizeof...(t_cmp)> { 0 };
			auto offset_index		= std::array<std::size_t, sizeof...(t_cmp) + 3> {};

			// runtime buffer
			auto runtime_typeinfo_buffer = std::array<std::tuple<std::size_t, std::size_t, std::size_t>, sizeof...(t_cmp)> {};


			for (auto [local_cmp_idx, storage_cmp_idx] : iota(0, std::bit_width(archetype))
															 | filter([archetype](auto idx) { return (archetype >> idx) & 1; })
															 | enumerate)
			{
				runtime_typeinfo_buffer[local_cmp_idx] = std::make_tuple<std::size_t, std::size_t, std::size_t>(
					t_archetype_traits::cmp_size(storage_cmp_idx),
					t_archetype_traits::cmp_alignment(storage_cmp_idx),
					-1);
			}

			// sort by alignment

			// compile_time_sort
			// std::ranges::sort(compile_typeinfo_buffer, std::greater_equal {}, [](const auto& tpl) { return std::get<1>(tpl); });

			// runtime_sort
			std::ranges::sort(runtime_typeinfo_buffer | take(cmp_count), std::greater_equal {}, [](const auto& tpl) { return std::get<1>(tpl); });


			// entity_count() = 0;
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
			return false;
		}
	};
}	 // namespace ecs::entity_group
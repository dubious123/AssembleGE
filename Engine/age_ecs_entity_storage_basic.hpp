#pragma once

namespace age::ecs::entity_storage
{
	using age::literal::operator"" _KiB;

	template <typename t_entity_id, age::ecs::cx_component... t_cmp>
	struct basic
	{
		using t_ent_id			 = t_entity_id;
		using t_archetype_traits = age::ecs::archetype_traits<t_cmp...>;
		using t_archetype		 = t_archetype_traits::t_archetype;
		using t_storage_cmp_idx	 = t_archetype_traits::t_storage_cmp_idx;
		using t_local_cmp_idx	 = t_archetype_traits::t_local_cmp_idx;
		using t_entity_block_idx = t_ent_id;
		using t_entity_block	 = age::ecs::entity_block::basic<4_KiB, t_ent_id, t_entity_block_idx, t_cmp...>;
		using t_local_entity_idx = t_entity_block::t_local_entity_idx;

		struct entity_info
		{
			t_archetype archetype;

			// Index of the entity within its current entity_block.
			// Used to access component data in SoA blocks.
			// Must be updated when entities move between blocks.
			t_local_entity_idx local_idx;

			// nullptr if the entity is invalid (not part of any block)
			t_entity_block* p_block;
		};

		struct entity_block_collection
		{
			//[ full ... | free ... ]
			age::data_structure::vector<t_entity_block*> ent_block_vec;
			std::size_t									 free_block_count = 0;

			FORCE_INLINE void
			deinit() noexcept
			{
				std::ranges::for_each(ent_block_vec, [](auto* p_block) { delete p_block; });
			}

			FORCE_INLINE std::size_t
			free_block_idx() noexcept
			{
				return static_cast<std::size_t>(ent_block_vec.size() - free_block_count);
			}

			FORCE_INLINE t_entity_block&
			free_block(t_archetype archetype) noexcept
			{
				if (free_block_count > 0)
				{
					return *ent_block_vec[free_block_idx()];
				}

				++free_block_count;
				if (ent_block_vec.empty())
				{
					auto p_entity_block = new t_entity_block();
					p_entity_block->init(archetype, static_cast<t_entity_block_idx>(ent_block_vec.size()));
					ent_block_vec.emplace_back(p_entity_block);
					return *p_entity_block;
				}
				else
				{
					auto p_entity_block = new t_entity_block();
					p_entity_block->init(*ent_block_vec.back(), static_cast<t_entity_block_idx>(ent_block_vec.size()));
					ent_block_vec.emplace_back(p_entity_block);
					return *p_entity_block;
				}
			}

			template <typename... t>
			FORCE_INLINE t_entity_block&
			free_block() noexcept
			{
				if (free_block_count > 0)
				{
					return *ent_block_vec[free_block_idx()];
				}

				++free_block_count;

				auto p_entity_block = new t_entity_block();
				p_entity_block->template init<t...>(static_cast<t_entity_block_idx>(ent_block_vec.size()));
				ent_block_vec.emplace_back(p_entity_block);
				return *p_entity_block;
			}

			void
			update_free(std::size_t block_idx) noexcept
			{
				assert(ent_block_vec[block_idx]->is_full() is_false);
				assert(block_idx < free_block_idx());
				++free_block_count;

				// ent_block_vec[swap_idx] is full
				auto swap_idx = free_block_idx();
				std::swap(ent_block_vec[block_idx], ent_block_vec[swap_idx]);
				std::swap(ent_block_vec[block_idx]->entity_block_idx(), ent_block_vec[swap_idx]->entity_block_idx());
			}

			void
			update_full(std::size_t block_idx) noexcept
			{
				assert(ent_block_vec[block_idx]->is_full());
				assert(block_idx >= free_block_idx());

				// the first free_block
				auto swap_idx = free_block_idx();
				std::swap(ent_block_vec[block_idx], ent_block_vec[swap_idx]);
				std::swap(ent_block_vec[block_idx]->entity_block_idx(), ent_block_vec[swap_idx]->entity_block_idx());

				--free_block_count;
			}
		};

		age::data_structure::sparse_vector<entity_info>				   entity_info_vec;
		age::data_structure::map<t_archetype, entity_block_collection> entity_blocks_map;

	  private:
		template <cx_component... t>
		static consteval decltype(auto)
		get_sorted_arg_index_sequence()
		{
			constexpr auto arr = []<auto... i, auto... j>(std::index_sequence<i...>, std::index_sequence<j...>) constexpr {
				auto arr = std::array<std::pair<std::size_t, std::size_t>, sizeof...(i)>{ std::pair{ i, j }... };

				std::ranges::sort(arr, std::less<>{}, &std::pair<std::size_t, std::size_t>::first);

				return std::array<std::size_t, sizeof...(j)>{ arr[j].second... };
			}(std::index_sequence<age::meta::variadic_index_v<age::meta::pred_is_same<std::remove_cv_t<t>>::template type, t_cmp...>...>{}, std::index_sequence_for<t...>{});

			return age::meta::arr_to_seq<arr>();
		}

		template <cx_component... t, typename... t_arg>
		t_ent_id
		new_entity_impl(t_arg&&... arg) noexcept
		{
			constexpr auto archetype = t_archetype_traits::template calc_archetype<t...>();

			auto& ent_block_collection = entity_blocks_map[archetype];
			auto& entity_block		   = ent_block_collection.free_block<t...>();
			auto  entity_id			   = static_cast<t_ent_id>(entity_info_vec.emplace_back(entity_info{ archetype, 0, &entity_block }));

			entity_info_vec[entity_id].local_idx = entity_block.new_entity<t...>(entity_id, std::forward<t_arg>(arg)...);

			if (entity_block.is_full())
			{
				ent_block_collection.update_full(entity_block.entity_block_idx());
			}

			return entity_id;
		}

		template <typename... t, typename... t_arg>
		void
		add_component_impl(const t_ent_id id, t_arg&&... arg) noexcept
		{
			using namespace std::ranges::views;

			auto& ent_info					= entity_info_vec[id];
			auto  new_archetype				= static_cast<t_archetype>(ent_info.archetype | t_archetype_traits::template calc_archetype<t...>());
			auto& src_ent_block_collection	= entity_blocks_map[ent_info.archetype];
			auto& dest_ent_block_collection = entity_blocks_map[new_archetype];
			auto& src_block					= *ent_info.p_block;
			auto& dst_block					= dest_ent_block_collection.free_block(new_archetype);
			auto  need_src_update			= src_block.is_full();

			for (auto [idx, storage_cmp_idx] : age::views::each_set_bit_idx(ent_info.archetype) | enumerate)
			{
				auto src_local_cmp_idx	= static_cast<t_local_cmp_idx>(idx);
				auto dest_local_cmp_idx = t_archetype_traits::calc_local_cmp_idx(new_archetype, storage_cmp_idx);
				src_block.evict_component(ent_info.local_idx, src_local_cmp_idx, dst_block.get_component_write_ptr(dest_local_cmp_idx));
			}

			if constexpr (sizeof...(t_arg) == 0)
			{
				(std::construct_at(reinterpret_cast<t*>(dst_block.get_component_write_ptr(t_archetype_traits::template calc_local_cmp_idx<t>(new_archetype)))), ...);
			}
			else if constexpr (sizeof...(t_arg) == sizeof...(t))
			{
				(std::construct_at(reinterpret_cast<t*>(dst_block.get_component_write_ptr(t_archetype_traits::template calc_local_cmp_idx<t>(new_archetype))), std::forward<t_arg>(arg)), ...);
			}
			else
			{
				static_assert(false, "invalid template parameter");
			}

			src_block.ent_id(ent_info.local_idx)	   = src_block.ent_id(--src_block.entity_count());
			dst_block.ent_id(dst_block.entity_count()) = id;

			ent_info.archetype = new_archetype;
			ent_info.p_block   = &dst_block;
			ent_info.local_idx = dst_block.entity_count()++;

			if (need_src_update)
			{
				src_ent_block_collection.update_free(src_block.entity_block_idx());
			}
			if (dst_block.is_full())
			{
				dest_ent_block_collection.update_full(dst_block.entity_block_idx());
			}
		}

		template <typename... t>
		void
		remove_component_impl(const t_ent_id id) noexcept
		{
			using namespace std::ranges::views;

			auto& ent_info					= entity_info_vec[id];
			auto  new_archetype				= static_cast<t_archetype>(ent_info.archetype ^ t_archetype_traits::template calc_archetype<t...>());
			auto& src_ent_block_collection	= entity_blocks_map[ent_info.archetype];
			auto& dest_ent_block_collection = entity_blocks_map[new_archetype];
			auto& src_block					= *ent_info.p_block;
			auto& dst_block					= dest_ent_block_collection.free_block(new_archetype);
			auto  need_src_update			= src_block.is_full();

			for (auto [idx, storage_cmp_idx] : age::views::each_set_bit_idx(new_archetype) | enumerate)
			{
				auto dest_local_cmp_idx = static_cast<t_local_cmp_idx>(idx);
				auto src_local_cmp_idx	= t_archetype_traits::calc_local_cmp_idx(ent_info.archetype, storage_cmp_idx);
				src_block.evict_component(ent_info.local_idx, src_local_cmp_idx, dst_block.get_component_write_ptr(dest_local_cmp_idx));
			}

			(src_block.evict_component(ent_info.local_idx, t_archetype_traits::template calc_local_cmp_idx<t>(ent_info.archetype)), ...);

			src_block.ent_id(ent_info.local_idx)	   = src_block.ent_id(--src_block.entity_count());
			dst_block.ent_id(dst_block.entity_count()) = id;

			ent_info.archetype = new_archetype;
			ent_info.p_block   = &dst_block;
			ent_info.local_idx = dst_block.entity_count()++;

			if (need_src_update)
			{
				src_ent_block_collection.update_free(src_block.entity_block_idx());
			}
			if (dst_block.is_full())
			{
				dest_ent_block_collection.update_full(dst_block.entity_block_idx());
			}
		}

	  public:
		FORCE_INLINE
		std::size_t
		entity_count() const noexcept
		{
			return entity_info_vec.size();
		}

		FORCE_INLINE bool
		is_valid(t_ent_id id) const noexcept
		{
			return entity_info_vec.capacity() > id and entity_info_vec[id].p_block == nullptr;
		}

		template <cx_component... t, typename... t_arg>
		t_ent_id
		new_entity(t_arg&&... arg) noexcept
		{
			if constexpr (sizeof...(t_arg) == 0)
			{
				return [this]<auto... i> INLINE_LAMBDA_FRONT(std::index_sequence<i...>) noexcept INLINE_LAMBDA_BACK -> decltype(auto) {
					return this->new_entity_impl<meta::variadic_at_t<i, t...>...>();
				}(get_sorted_arg_index_sequence<t...>());
			}
			else
			{
				static_assert((sizeof...(t) == sizeof...(t_arg)), "invalid template parameter");
				static_assert((std::is_constructible_v<std::remove_cvref<t>, t_arg> and ...), "invalid template parameter");
				static_assert((age::meta::variadic_contains_v<std::remove_cv_t<t>, t_cmp...> and ...), "invalid component type, reference type is not allowed");

				return [this]<auto... i> INLINE_LAMBDA_FRONT(std::index_sequence<i...>, auto&&... arg) noexcept INLINE_LAMBDA_BACK -> decltype(auto) {
					return this->new_entity_impl<meta::variadic_at_t<i, t...>...>(age::meta::variadic_get<i>(FWD(arg)...)...);
				}(get_sorted_arg_index_sequence<t...>(), FWD(arg)...);
			}
		}

		void
		remove_entity(const t_ent_id id) noexcept
		{
			auto& ent_info = entity_info_vec[id];

			auto need_update = ent_info.p_block->is_full();

			auto& entity_id_last					  = ent_info.p_block->remove_entity(ent_info.local_idx);
			entity_info_vec[entity_id_last].local_idx = ent_info.local_idx;

			if (need_update)
			{
				entity_blocks_map[ent_info.archetype].update_free(ent_info.p_block->entity_block_idx());
			}

			entity_info_vec[id].p_block = nullptr;
			entity_info_vec.remove(id);
		}

		// if dup cmp => UB
		template <typename... t, typename... t_arg>
		void
		add_component(const t_ent_id id, t_arg&&... arg) noexcept
		{
			if constexpr (sizeof...(t_arg) == 0)
			{
				return [this]<auto... i> INLINE_LAMBDA_FRONT(std::index_sequence<i...>, const t_ent_id id) noexcept INLINE_LAMBDA_BACK -> decltype(auto) {
					return this->add_component_impl<meta::variadic_at_t<i, t...>...>(id);
				}(get_sorted_arg_index_sequence<t...>(), id);
			}
			else
			{
				static_assert((sizeof...(t) == sizeof...(t_arg)), "invalid template parameter");
				static_assert((std::is_constructible_v<std::remove_cvref<t>, t_arg> and ...), "invalid template parameter");
				static_assert((age::meta::variadic_contains_v<std::remove_cv_t<t>, t_cmp...> and ...), "invalid component type, reference type is not allowed");

				return [this]<auto... i> INLINE_LAMBDA_FRONT(std::index_sequence<i...>, const t_ent_id id, auto&&... arg) noexcept INLINE_LAMBDA_BACK -> decltype(auto) {
					return this->add_component_impl<meta::variadic_at_t<i, t...>...>(age::meta::variadic_get<i>(id, FWD(arg)...)...);
				}(get_sorted_arg_index_sequence<t...>(), id, FWD(arg)...);
			}
		}

		template <typename... t>
		void
		remove_component(const t_ent_id id) noexcept
		{

			static_assert((age::meta::variadic_contains_v<std::remove_cv_t<t>, t_cmp...> and ...), "invalid component type, reference type is not allowed");

			return [this]<auto... i> INLINE_LAMBDA_FRONT(std::index_sequence<i...>, const t_ent_id id) noexcept INLINE_LAMBDA_BACK -> decltype(auto) {
				return this->remove_component_impl<meta::variadic_at_t<i, t...>...>(id);
			}(get_sorted_arg_index_sequence<t...>(), id);
		}

		template <typename... t>
		FORCE_INLINE decltype(auto)
		get_component(const t_ent_id id) noexcept
		{
			auto& ent_info = entity_info_vec[id];
			return ent_info.p_block->template get_component<t...>(ent_info.local_idx);
		}

		template <typename... t>
		FORCE_INLINE bool
		has_component(const t_ent_id id) noexcept
		{
			constexpr auto archetype = t_archetype_traits::template calc_archetype<t...>();
			return (entity_info_vec[id].archetype & archetype) == archetype;
		}

		void
		load_from_file(const char* path) noexcept
		{
		}

		void
		save_to_file(const char* path) const noexcept
		{
		}

		void
		load_from_memory(const void* data, std::size_t size) noexcept
		{
		}

		// template <typename t_query>
		// bool
		// matches(t_archetype arch);

		template <typename t_query>
		FORCE_INLINE bool
		matches(t_query, t_archetype arch) noexcept
		{
			constexpr auto with_mask	= t_archetype_traits::template calc_mask<typename t_query::with>();
			constexpr auto without_mask = t_archetype_traits::template calc_mask<typename t_query::without>();

			static_assert((with_mask ^ without_mask) == (with_mask | without_mask), "invalid query");


			//  without_mask , arch  (1, 1) -> false, (1, 0) -> true
			//
			// without_mask & arch == 0
			//
			//
			//  with_mask , arch          (1, 1) -> true, (1, 0) -> false  // (0, ?)
			//
			// with_mask & arch == with_mask
			//
			//
			return ((arch & with_mask) | (arch & without_mask)) == with_mask;
		}

		template <typename t_query, typename t_sys>
		FORCE_INLINE void
		foreach_block(t_query&& block_query, t_sys&& sys) noexcept
		{
			for (auto& [arch, blocks] : entity_blocks_map)
			{
				if (matches(block_query, arch) is_false)
				{
					continue;
				}

				for (auto& block : blocks.ent_block_vec
									   | age::meta::deref_view
									   | std::views::filter([](auto& block) { return block.is_empty() is_false; }))
				{
					age::ecs::system::run_sys(FWD(sys), block);
				}
			}
		}

		template <typename t_query, typename t_sys>
		FORCE_INLINE void
		foreach_entity(t_query&& block_query, t_sys&& sys) noexcept
		{
			for (auto& [arch, blocks] : entity_blocks_map)
			{
				if (matches(block_query, arch) is_false)
				{
					continue;
				}

				for (auto& block : blocks.ent_block_vec
									   | age::meta::deref_view
									   | std::views::filter([](auto& block) { return block.is_empty() is_false; }))
				{
					block.foreach_entity(FWD(sys));
				}
			}
		}

		void
		init() noexcept
		{
		}

		void
		deinit() noexcept
		{
			std::ranges::for_each(entity_blocks_map | std::views::values /*| std::views::join*/, [](auto& ent_blocks) { ent_blocks.deinit(); });

			if constexpr (age::config::debug_mode)
			{
				entity_info_vec.debug_validate();
			}
		}
	};
}	 // namespace age::ecs::entity_storage
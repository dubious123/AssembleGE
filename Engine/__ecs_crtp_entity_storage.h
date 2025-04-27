#pragma once

namespace ecs::crtp
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

		consteval auto cmp_count()
		{
			return sizeof...(t_cmp);
		}
	};

	template <typename derived_storage, typename _t_entity_id>
	struct entity_storage;

	template <typename... t_cmp, template <typename...> typename derived_storage, typename _t_entity_id>
	struct entity_storage<derived_storage<t_cmp...>, _t_entity_id>
	{
		using t_archetype_traits = archetype_traits<t_cmp...>;
		using t_entity_id		 = _t_entity_id;
		using t_archetype		 = t_archetype_traits::t_archetype;

		template <typename... t_cmp>
		t_entity_id add_entity(t_cmp&&... cmp)
		{
			return static_cast<derived_storage*>(this)->add_entity(std::forward<t_cmp>(cmp)...);
		}

		template <typename... t_cmp>
		t_entity_id add_entity()
		{
			return static_cast<derived_storage*>(this)->template add_entity<t_cmp...>();
		}

		void remove_entity(t_entity_id id)
		{
			static_cast<derived_storage*>(this)->remove_entity(id);
		}

		bool is_valid(t_entity_id id) const
		{
			return static_cast<const derived_storage*>(this)->is_valid(id);
		}

		template <typename... t_cmp>
		void add_component(t_entity_id id, t_cmp&&... cmp)
		{
			static_cast<derived_storage*>(this)->add_component(id, std::forward<t_cmp>(cmp));
		}

		template <typename... t_cmp>
		void remove_component(t_entity_id id)
		{
			static_cast<derived_storage*>(this)->template remove_component<t_cmp...>(id);
		}

		template <typename t_cmp>
		t_cmp& get_component(t_entity_id id)
		{
			return static_cast<derived_storage*>(this)->template get_component<t_cmp>(id);
		}

		template <typename... t_cmp>
		std::tuple<t_cmp&...> get_component(t_entity_id id)
		{
			return static_cast<derived_storage*>(this)->template get_component<t_cmp...>(id);
		}

		template <typename t_cmp>
		const t_cmp& get_component(t_entity_id id) const
		{
			return static_cast<const derived_storage*>(this)->template get_component<t_cmp>(id);
		}

		template <typename... t_cmp>
		std::tuple<const t_cmp&...> get_component(t_entity_id id) const
		{
			return static_cast<const derived_storage*>(this)->template get_component<t_cmp...>(id);
		}

		template <typename... t_cmp>
		bool has_component(t_entity_id id) const
		{
			return static_cast<const derived_storage*>(this)->template has_component<t_cmp...>(id);
		}

		void load_from_file(const char* path)
		{
			static_cast<derived_storage*>(this)->load_from_file(path);
		}

		void save_to_file(const char* path) const
		{
			static_cast<const derived_storage*>(this)->save_to_file(path);
		}

		void load_from_memory(const void* data, std::size_t size)
		{
			static_cast<derived_storage*>(this)->load_from_memory(data, size);
		}

		template <typename... t_cmp, typename lambda>
		void each_entity(lambda&& fn)
		{
			static_cast<derived_storage*>(this)->template each_entity<t_cmp...>(std::forward<lambda>(fn));
		}

		template <typename... t_cmp, typename lambda>
		void each_group(lambda&& fn)
		{
			static_cast<derived_storage*>(this)->template each_group<t_cmp...>(std::forward<lambda>(fn));
		}
	};
}	 // namespace ecs::crtp

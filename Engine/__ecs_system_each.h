#pragma once
#include "__ecs_common.h"
#ifndef INCLUDED_FROM_ECS_SYSTEM_HEADER
	#error "Do not include this file directly. Include <__ecs_system.h> instead."
#endif

namespace ecs::system
{
	template <component_type... t>
	struct with_clause
	{
		using types = std::tuple<t...>;

		constexpr with_clause() = default;
	};

	template <component_type... t>
	struct without_clause
	{
		using types = std::tuple<t...>;

		constexpr without_clause() = default;
	};

	template <component_type... t>
	static constexpr inline auto with = with_clause<t...>{};

	template <component_type... t>
	static constexpr inline auto without = without_clause<t...>{};

	namespace detail
	{
		template <typename t>
		struct is_with_clause : std::false_type
		{
		};

		template <typename... t>
		struct is_with_clause<with_clause<t...>> : std::true_type
		{
		};

		template <typename t>
		struct is_without_clause : std::false_type
		{
		};

		template <typename... t>
		struct is_without_clause<without_clause<t...>> : std::true_type
		{
		};

		template <typename t_query>
		consteval bool
		validate_query()
		{
			using t_with	= t_query::with;
			using t_without = t_query::without;
			{
				constexpr auto duplicated =
					[]<auto... i>(std::index_sequence<i...>) {
						return ((meta::tuple_empty_v<
									meta::filtered_tuple_t<
										meta::pred<std::tuple_element_t<i, typename t_with::types>>::template is_same,
										t_without::types>>)
								and ...);
					}(std::make_index_sequence<std::tuple_size_v<typename t_with::types>>{});

				static_assert(duplicated, "query: same component(s) in with<> and without<>");
			}

			return true;
		}
	}	 // namespace detail

	template <typename... t_clause>
	struct query
	{
		using t_self  = query<t_clause...>;
		using with	  = meta::variadic_find_t<detail::is_with_clause, with_clause<>, std::decay_t<t_clause>...>;
		using without = meta::variadic_find_t<detail::is_without_clause, without_clause<>, std::decay_t<t_clause>...>;

		static_assert(detail::validate_query<t_self>(), "query: invalid query");

		constexpr query(t_clause&... clause) requires(sizeof...(t_clause) > 0)
		{ };

		constexpr query() = default;
	};

	namespace detail
	{
		template <typename t>
		struct is_query : std::false_type
		{
		};

		template <typename... t_clause>
		struct is_query<query<t_clause...>> : std::true_type
		{
		};

	}	 // namespace detail

	template <typename t>
	concept i_entity_group_like = requires(t&& obj) {
		typename i_entity_group<t>;
		typename std::remove_reference_t<t>::t_ent_id;
		typename std::remove_reference_t<t>::t_ent_group_idx;
		typename std::remove_reference_t<t>::t_local_entity_idx;
		typename std::remove_reference_t<t>::t_storage_cmp_idx;
		typename std::remove_reference_t<t>::t_archetype;
		typename std::remove_reference_t<t>::t_entity_count;
		typename std::remove_reference_t<t>::t_capacity;
		typename std::remove_reference_t<t>::t_local_cmp_idx;
		typename std::remove_reference_t<t>::t_component_count;
		typename std::remove_reference_t<t>::t_component_size;
		typename std::remove_reference_t<t>::t_component_offset;
		typename std::remove_reference_t<t>::t_cmp_offset_arr_base;
		typename std::remove_reference_t<t>::t_cmp_size_arr_base;
		typename std::remove_reference_t<t>::t_entity_id_arr_base;

		{ obj.entity_group_idx() } -> std::same_as<typename std::remove_reference_t<t>::t_ent_group_idx&>;
		{ obj.entity_count() } -> std::same_as<typename std::remove_reference_t<t>::t_entity_count&>;
		{ obj.capacity() } -> std::same_as<typename std::remove_reference_t<t>::t_capacity&>;
		{ obj.component_count() } -> std::same_as<typename std::remove_reference_t<t>::t_component_count&>;
		{ obj.local_archetype() } -> std::same_as<typename std::remove_reference_t<t>::t_archetype&>;
		{ obj.component_size_arr_base() } -> std::same_as<typename std::remove_reference_t<t>::t_cmp_size_arr_base&>;
		{ obj.component_offset_arr_base() } -> std::same_as<typename std::remove_reference_t<t>::t_cmp_offset_arr_base&>;
		{ obj.entity_id_arr_base() } -> std::same_as<typename std::remove_reference_t<t>::t_entity_id_arr_base&>;
		{ obj.ent_id(std::declval<typename std::remove_reference_t<t>::t_local_entity_idx>()) } -> std::same_as<typename std::remove_reference_t<t>::t_ent_id&>;
		{ obj.is_full() } -> std::same_as<bool>;
		{ obj.is_empty() } -> std::same_as<bool>;

		// templates?
		//{
		//	i_entity_group<t>{ FWD(obj) }
		//};
	};

	template <typename t>
	concept i_entity_storage_like = requires(t&& obj) {
		typename i_entity_storage<t>;
		typename std::remove_reference_t<t>::t_ent_id;
		typename std::remove_reference_t<t>::t_archetype_traits;
		typename std::remove_reference_t<t>::t_archetype;
		typename std::remove_reference_t<t>::t_storage_cmp_idx;
		typename std::remove_reference_t<t>::t_local_cmp_idx;
		typename std::remove_reference_t<t>::t_entity_group_idx;
		typename std::remove_reference_t<t>::t_entity_group;
		typename std::remove_reference_t<t>::t_local_entity_idx;

		{ obj.entity_count() } -> std::same_as<std::size_t>;
		{ obj.is_valid(std::declval<const typename std::remove_reference_t<t>::t_ent_id>()) } -> std::same_as<bool>;
		{ obj.remove_entity(std::declval<const typename std::remove_reference_t<t>::t_ent_id>()) } -> std::same_as<void>;
		{ obj.init() } -> std::same_as<void>;
		{ obj.deinit() } -> std::same_as<void>;
	};

	template <typename t_query, typename t_sys>
	struct each_group
	{
		no_unique_addr t_sys sys;

		constexpr each_group(t_query&& query, t_sys&& sys) noexcept : sys(FWD(sys)) { };

		constexpr each_group() = default;

		template <i_entity_storage_like t_ent_storage>
		FORCE_INLINE constexpr decltype(auto)
		operator()(t_ent_storage&& ent_group) noexcept
		{
			// static_assert(
			//	std::tuple_size_v<typename sys_trait::argument_types> == 1
			//		and meta::is_specialization_of_v<
			//			std::tuple_element_t<0, typename sys_trait::argument_types>,
			//			i_entity_group>,
			//	"each_group : The system must be callable with exactly one parameter of type i_entity_group<T>.");

			ent_group.foreach_group(t_query{}, sys);
		}
	};

	namespace detail
	{
		// todo
		template <typename t_query, typename t_sys, typename t_arg>
		consteval bool
		validate_each_entity()
		{
			return true;
		}
	}	 // namespace detail

	template <typename t_query, typename t_sys>
	struct each_entity
	{
		using t_self = each_entity<t_query, t_sys>;

		no_unique_addr t_sys sys;

		constexpr each_entity(t_query&& query, t_sys&& sys) noexcept : sys(FWD(sys)) { };

		constexpr each_entity() = default;

		// template <i_entity_group_like g>
		// FORCE_INLINE constexpr decltype(auto)
		// run(i_entity_group<g> i_group)
		//{
		// }

		// template <i_entity_storage_like s>
		// FORCE_INLINE constexpr decltype(auto)
		// run(i_entity_storage<s> i_storage)
		//{
		// }

		template <typename t_arg>
		FORCE_INLINE constexpr decltype(auto)
		operator()(t_arg&& arg) noexcept
		{
			static_assert(detail::validate_each_entity<t_query, t_sys, t_arg>());

			if constexpr (i_entity_group_like<t_arg>)
			{
				auto i_group = i_entity_group{ FWD(arg) };
				return i_group.foreach_entity(sys);
			}
			else if constexpr (i_entity_storage_like<t_arg>)
			{
				auto storage = i_entity_storage{ FWD(arg) };

				// todo validate sys and query
				return storage.foreach_entity(t_query{}, sys);
			}
			else
			{
				// meta::print_type<t_arg>();
				static_assert(false, "each_entity : invalid arguement");
			}
		}
	};
}	 // namespace ecs::system
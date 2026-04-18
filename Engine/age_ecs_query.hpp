#pragma once
#include "age.hpp"

namespace age::ecs
{
	class sv_entity_id { };

	class sv_block { };

	class sv_local_index { };

	class sv_archetype { };

	template <typename... t>
	class include { };

	template <typename... t>
	class exclude { };

	template <typename... t>
	class any { };
}	 // namespace age::ecs

namespace age::ecs::detail
{
	template <typename t>
	concept cx_query_sv = meta::variadic_contains_v<t, sv_entity_id, sv_block, sv_local_index, sv_archetype>;

	template <typename t>
	struct is_query_sv : std::bool_constant<cx_query_sv<t>>
	{
	};

	template <typename t_select, typename t_include, typename t_exclude, typename t_any>
	requires meta::is_specialization_of_v<t_select, meta::type_pack>
		 and meta::is_specialization_of_v<t_include, meta::type_pack>
		 and meta::is_specialization_of_v<t_exclude, meta::type_pack>
		 and meta::is_specialization_of_v<t_any, meta::type_pack>
	struct query_desc
	{
		using t_select_list	 = t_select;
		using t_include_list = t_include;
		using t_exclude_list = t_exclude;
		using t_any_list	 = t_any;

		// static_assert

		using t_with = meta::make_unique_t<
			meta::combine_t<
				meta::transform_t<std::remove_const, meta::filter_not_t<is_query_sv, t_select>>,
				t_include>>;

		using t_without = meta::make_unique_t<t_exclude>;
	};
}	 // namespace age::ecs::detail

namespace age::ecs
{
	template <typename... t_select>
	consteval auto
	query()
	{
		return detail::query_desc<meta::type_pack<t_select...>,
								  meta::type_pack<>,
								  meta::type_pack<>,
								  meta::type_pack<>>{};
	}

	template <typename t_sel, typename t_inc, typename t_exc, typename t_any, typename... t>
	consteval auto
	operator|(detail::query_desc<t_sel, t_inc, t_exc, t_any>, include<t...>)
	{
		return detail::query_desc<t_sel,
								  meta::combine_t<t_inc, meta::type_pack<t...>>,
								  t_exc,
								  t_any>{};
	}

	template <typename t_sel, typename t_inc, typename t_exc, typename t_any, typename... t>
	consteval auto
	operator|(detail::query_desc<t_sel, t_inc, t_exc, t_any>, exclude<t...>)
	{
		return detail::query_desc<t_sel,
								  t_inc,
								  meta::combine_t<t_exc, meta::type_pack<t...>>,
								  t_any>{};
	}

	template <typename t_sel, typename t_inc, typename t_exc, typename t_any, typename... t>
	consteval auto
	operator|(detail::query_desc<t_sel, t_inc, t_exc, t_any>, any<t...>)
	{
		return detail::query_desc<t_sel,
								  t_inc,
								  t_exc,
								  meta::combine_t<t_any, meta::type_pack<t...>>>{};
	}

	consteval auto
	test()
	{
		// === test components ===
		struct comp_a
		{ };

		struct comp_b
		{ };

		struct comp_c
		{ };

		struct comp_d
		{ };

		// === basic query ===
		{
			constexpr auto q = ecs::query<comp_a, comp_b>();
			using desc		 = decltype(q);

			static_assert(std::is_same_v<desc::t_select_list, meta::type_pack<comp_a, comp_b>>);
			static_assert(std::is_same_v<desc::t_with, meta::type_pack<comp_a, comp_b>>);
			static_assert(std::is_same_v<desc::t_without, meta::type_pack<>>);
			static_assert(std::is_same_v<desc::t_any_list, meta::type_pack<>>);
		}

		// === const select ===
		{
			constexpr auto q = ecs::query<const comp_a, comp_b>();
			using desc		 = decltype(q);

			static_assert(std::is_same_v<desc::t_select_list, meta::type_pack<const comp_a, comp_b>>);
			static_assert(std::is_same_v<desc::t_with, meta::type_pack<comp_a, comp_b>>);
		}

		// === sv filtered from with ===
		{
			constexpr auto q = ecs::query<ecs::sv_entity_id, comp_a>();
			using desc		 = decltype(q);

			static_assert(std::is_same_v<desc::t_select_list, meta::type_pack<ecs::sv_entity_id, comp_a>>);
			static_assert(std::is_same_v<desc::t_with, meta::type_pack<comp_a>>);
		}

		// === include ===
		{
			constexpr auto q = ecs::query<comp_a>() | ecs::include<comp_b, comp_c>();
			using desc		 = decltype(q);

			static_assert(std::is_same_v<desc::t_include_list, meta::type_pack<comp_b, comp_c>>);
			static_assert(std::is_same_v<desc::t_with, meta::type_pack<comp_a, comp_b, comp_c>>);
		}

		// === exclude ===
		{
			constexpr auto q = ecs::query<comp_a>() | ecs::exclude<comp_d>();
			using desc		 = decltype(q);

			static_assert(std::is_same_v<desc::t_without, meta::type_pack<comp_d>>);
		}

		// === any ===
		{
			constexpr auto q = ecs::query<comp_a>() | ecs::any<comp_b, comp_c>();
			using desc		 = decltype(q);

			static_assert(std::is_same_v<desc::t_any_list, meta::type_pack<comp_b, comp_c>>);
		}

		// === combined ===
		{
			constexpr auto q = ecs::query<ecs::sv_entity_id, const comp_a, comp_b>()
							 | ecs::include<comp_c>()
							 | ecs::exclude<comp_d>()
							 | ecs::any<comp_a, comp_b>();
			using desc		 = decltype(q);

			static_assert(std::is_same_v<desc::t_select_list, meta::type_pack<ecs::sv_entity_id, const comp_a, comp_b>>);
			static_assert(std::is_same_v<desc::t_with, meta::type_pack<comp_a, comp_b, comp_c>>);
			static_assert(std::is_same_v<desc::t_without, meta::type_pack<comp_d>>);
			static_assert(std::is_same_v<desc::t_any_list, meta::type_pack<comp_a, comp_b>>);
		}

		// === duplicate removal ===
		{
			constexpr auto q = ecs::query<comp_a>() | ecs::include<comp_a>();
			using desc		 = decltype(q);

			static_assert(std::is_same_v<desc::t_with, meta::type_pack<comp_a>>);
		}

		// === chained include ===
		{
			constexpr auto q = ecs::query<comp_a>() | ecs::include<comp_b>() | ecs::include<comp_c>();
			using desc		 = decltype(q);

			static_assert(std::is_same_v<desc::t_include_list, meta::type_pack<comp_b, comp_c>>);
			static_assert(std::is_same_v<desc::t_with, meta::type_pack<comp_a, comp_b, comp_c>>);
		}
	}
}	 // namespace age::ecs

namespace age::ecs::detail
{
	template <cx_entity_block t_block, typename t_query>
	struct each_entity_range
	{
		using t_local_ent_id = typename t_block::t_local_entity_idx;

		t_block& block;

		struct iterator
		{
			using t_idx = age::meta::smallest_signed_t<std::numeric_limits<typename t_block::t_local_entity_idx>::max()>;

			t_block* p_block;
			t_idx	 idx;

			using iterator_concept = std::random_access_iterator_tag;
			using difference_type  = t_idx;
			using value_type	   = decltype(std::declval<t_block>().query_entity(t_query{}, t_local_ent_id{}));

			FORCE_INLINE decltype(auto)
			operator*() const noexcept
			{
				return p_block->query_entity(t_query{}, static_cast<t_local_ent_id>(idx));
			}

			FORCE_INLINE iterator&
			operator++()
			{
				++idx;
				return *this;
			}

			FORCE_INLINE iterator
			operator++(int)
			{
				auto tmp = *this;
				++idx;
				return tmp;
			}

			FORCE_INLINE iterator&
			operator--()
			{
				--idx;
				return *this;
			}

			FORCE_INLINE iterator
			operator--(int)
			{
				auto tmp = *this;
				--idx;
				return tmp;
			}

			FORCE_INLINE iterator&
			operator+=(difference_type n)
			{
				idx += n;
				return *this;
			}

			FORCE_INLINE iterator&
			operator-=(difference_type n)
			{
				idx -= n;
				return *this;
			}

			FORCE_INLINE iterator
			operator+(difference_type n) const
			{
				return { p_block, idx + n };
			}

			FORCE_INLINE iterator
			operator-(difference_type n) const
			{
				return { p_block, idx - n };
			}

			FORCE_INLINE difference_type
			operator-(const iterator& rhs) const
			{
				return idx - rhs.idx;
			}

			FORCE_INLINE decltype(auto)
			operator[](difference_type n) const
			{
				return *(*this + n);
			}

			FORCE_INLINE auto
			operator<=>(const iterator& rhs) const
			{
				return idx <=> rhs.idx;
			}

			FORCE_INLINE bool
			operator==(const iterator& rhs) const
			{
				return idx == rhs.idx;
			}

			friend FORCE_INLINE iterator
			operator+(difference_type n, const iterator& it)
			{
				return it + n;
			}
		};

		FORCE_INLINE iterator
		begin() const noexcept
		{
			return { &block, 0 };
		}

		FORCE_INLINE iterator
		end() const noexcept
		{
			return { &block, static_cast<iterator::t_idx>(block.entity_count()) };
		}
	};

	template <typename t_block_range, typename t_query>
	struct each_entity_joined_range
	{
		class sentinel { };

		t_block_range block_rng;

		struct iterator
		{
			using t_block_it	   = std::ranges::iterator_t<t_block_range>;
			using t_block_sentinel = std::ranges::sentinel_t<t_block_range>;
			using t_block		   = std::remove_cvref_t<std::ranges::range_reference_t<t_block_range>>;
			using t_local_ent_id   = typename t_block::t_local_entity_idx;
			using t_idx			   = age::meta::smallest_signed_t<std::numeric_limits<t_local_ent_id>::max()>;

			using iterator_concept = std::forward_iterator_tag;
			using difference_type  = std::ptrdiff_t;
			using value_type	   = decltype(std::declval<t_block>().query_entity(t_query{}, t_local_ent_id{}));

			t_block_it		 block_iter;
			t_block_sentinel block_end;
			t_idx			 idx;

			FORCE_INLINE decltype(auto)
			operator*() const
			{
				return (*block_iter).query_entity(t_query{}, static_cast<t_local_ent_id>(idx));
			}

			FORCE_INLINE iterator&
			operator++()
			{
				++idx;
				if (idx >= static_cast<t_idx>((*block_iter).entity_count()))
				{
					++block_iter;
					idx = 0;
				}
				return *this;
			}

			FORCE_INLINE iterator
			operator++(int)
			{
				auto tmp = *this;
				++(*this);
				return tmp;
			}

			FORCE_INLINE bool
			operator==(const iterator& rhs) const
			{
				return block_iter == rhs.block_iter;
			}

			FORCE_INLINE bool
			operator==(sentinel) const
			{
				return block_iter == block_end;
			}
		};

		FORCE_INLINE iterator
		begin()
		{
			return iterator{ std::ranges::begin(block_rng), std::ranges::end(block_rng), 0 };
		}

		FORCE_INLINE sentinel
		end()
		{
			return {};
		}
	};

	template <typename t_query>
	class each_entity_adaptor { };

	template <typename t_query>
	FORCE_INLINE auto
	operator|(auto&& lhs, each_entity_adaptor<t_query>)
	{
		using t_lhs = BARE_OF(lhs);
		if constexpr (cx_entity_storage<t_lhs>)
		{
			// storage | each_entity(query) = storage | each_block(query) | each_entity(query)
			return lhs.each_block(t_query{}) | each_entity_adaptor<t_query>{};
		}
		else if constexpr (cx_entity_block<t_lhs>)
		{
			return each_entity_range<t_lhs, t_query>{ lhs };
		}
		else if constexpr (std::ranges::range<t_lhs> and cx_entity_block<std::remove_cvref_t<std::ranges::range_reference_t<t_lhs>>>)
		{
			return each_entity_joined_range<t_lhs, t_query>{ FWD(lhs) };
		}
		else
		{
			static_assert(false, "invalid type");
		}
	}

	template <typename t_query>
	class each_block_adaptor { };

	template <typename t_query>
	FORCE_INLINE auto
	operator|(cx_entity_storage auto&& storage, each_block_adaptor<t_query>)
	{
		return storage.each_block(t_query{});
	}

	struct each_block_archetype_adaptor
	{
		uint64 archetype;
	};

	FORCE_INLINE auto
	operator|(cx_entity_storage auto&& storage, each_block_archetype_adaptor adaptor)
	{
		using t_storage	  = BARE_OF(storage);
		using t_archetype = typename t_storage::t_archetype;
		return storage.each_block(static_cast<t_archetype>(adaptor.archetype));
	}
}	 // namespace age::ecs::detail

namespace age::ecs
{
	template <typename t_query>
	constexpr decltype(auto)
	each_entity(t_query)
	{
		return detail::each_entity_adaptor<t_query>{};
	}

	template <typename... t_select>
	requires(sizeof...(t_select) > 0)
	constexpr decltype(auto)
	each_entity()
	{
		return each_entity(query<t_select...>());
	}

	template <typename t_query>
	constexpr decltype(auto)
	each_block(t_query)
	{
		return detail::each_block_adaptor<t_query>{};
	}

	constexpr decltype(auto)
	each_block(uint64 archetype)
	{
		return detail::each_block_archetype_adaptor{ archetype };
	}
}	 // namespace age::ecs

namespace age::ecs::detail
{
	template <typename t_cmp, typename t_block>
	FORCE_INLINE decltype(auto)
	resolve_select(t_block& block, typename t_block::t_local_entity_idx local_ent_idx) noexcept
	{
		if constexpr (std::is_same_v<t_cmp, sv_entity_id>)
		{
			return block.ent_id(local_ent_idx);
		}
		else if constexpr (std::is_same_v<t_cmp, sv_block>)
		{
			return std::ref(block);
		}
		else if constexpr (std::is_same_v<t_cmp, sv_local_index>)
		{
			return local_ent_idx;
		}
		else if constexpr (std::is_same_v<t_cmp, sv_archetype>)
		{
			return block.local_archetype();
		}
		else if constexpr (std::is_const_v<t_cmp>)
		{
			return std::as_const(*block.template cmp_ptr<std::remove_const_t<t_cmp>>(local_ent_idx));
		}
		else
		{
			return *block.template cmp_ptr<t_cmp>(local_ent_idx);
		}
	}

	template <typename t_query, typename... t_cmp>
	FORCE_INLINE decltype(auto)
	query_entity_impl(auto&& block, auto local_ent_idx, meta::type_pack<t_cmp...>) noexcept
	{
		return std::forward_as_tuple(resolve_select<t_cmp>(block, local_ent_idx)...);
	}
}	 // namespace age::ecs::detail

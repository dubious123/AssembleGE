#pragma once
#include "age.hpp"

namespace age::ecs::command
{
	namespace detail
	{
		template <typename t_storage, typename... t_cmp>
		FORCE_INLINE void
		fn_new_entity_impl(t_storage& storage, t_cmp&&... cmp) noexcept
		{
			storage.new_entity(FWD(cmp)...);
		}

		template <typename t_storage, typename... t_cmp>
		void
		fn_new_entity_with_arg(t_storage& storage, age::byte_buf& buffer) noexcept
		{
			meta::tuple_unpack_prefix<fn_new_entity_impl<t_storage, t_cmp...>>(buffer.read<t_cmp...>(), storage);
		}

		template <typename t_storage, typename... t_cmp>
		void
		fn_new_entity_no_arg(t_storage& storage, age::byte_buf&) noexcept
		{
			storage.new_entity<t_cmp...>();
		}

		template <typename t_storage, typename t_ent_id, typename... t_cmp>
		FORCE_INLINE void
		fn_add_component_impl(t_storage& storage, t_ent_id&& ent_id, t_cmp&&... cmp) noexcept
		{
			storage.add_component<t_cmp...>(FWD(ent_id), FWD(cmp)...);
		}

		template <typename t_storage, typename t_ent_id, typename... t_cmp>
		void
		fn_add_component_with_arg(t_storage& storage, age::byte_buf& buffer) noexcept
		{
			meta::tuple_unpack_prefix<fn_add_component_impl<t_storage, t_ent_id, t_cmp...>>(buffer.read<t_ent_id, t_cmp...>(), storage);
		}

		template <typename t_storage, typename t_ent_id, typename... t_cmp>
		void
		fn_add_component_no_arg(t_storage& storage, age::byte_buf& buffer) noexcept
		{
			storage.add_component<t_cmp...>(buffer.read<t_ent_id>());
		}

		template <typename t_storage, typename t_ent_id, typename... t_cmp>
		void
		fn_remove_component(t_storage& storage, t_ent_id ent_id) noexcept
		{
			storage.remove_component<t_cmp...>(ent_id);
		}
	}	 // namespace detail

	template <cx_entity_storage t_storage>
	struct command_buffer
	{
		using t_ent_id			 = typename t_storage::t_ent_id;
		using t_archetype_traits = typename t_storage::t_archetype_traits;
		using t_archetype		 = typename t_storage::t_archetype;
		using t_storage_cmp_idx	 = typename t_storage::t_storage_cmp_idx;
		using t_local_cmp_idx	 = typename t_storage::t_local_cmp_idx;
		using t_entity_block_idx = typename t_storage::t_entity_block_idx;
		using t_entity_block	 = typename t_storage::t_entity_block;
		using t_local_entity_idx = typename t_storage::t_local_entity_idx;

		struct new_entity_cmd
		{
			void (*fn)(t_storage&, age::byte_buf&);
		};

		struct add_component_cmd
		{
			void (*fn)(t_storage&, age::byte_buf&);
		};

		struct remove_component_cmd
		{
			t_ent_id id;
			void	 (*fn)(t_storage&, t_ent_id);
		};

		age::vector<new_entity_cmd> new_entity_cmd_vec;
		age::byte_buf				new_entity_cmp_buffer;

		age::vector<t_ent_id> remove_entity_cmd_vec;

		age::vector<add_component_cmd> add_component_cmd_vec;
		age::byte_buf				   add_component_cmp_buffer;

		age::vector<remove_component_cmd> remove_component_cmd_vec;

		template <typename... t_cmp>
		void
		new_entity(auto&&... arg) noexcept
		{
			if constexpr (sizeof...(arg) == 0)
			{
				new_entity_cmd_vec.emplace_back(&detail::fn_new_entity_no_arg<t_storage, t_cmp...>);
			}
			else if constexpr (sizeof...(arg) == sizeof...(t_cmp))
			{
				new_entity_cmp_buffer.write(t_cmp{ FWD(arg) }...);
				new_entity_cmd_vec.emplace_back(&detail::fn_new_entity_with_arg<t_storage, t_cmp...>);
			}
			else
			{
				static_assert(false, "invalid");
			}
		}

		void
		remove_entity(t_ent_id id) noexcept
		{
			remove_entity_cmd_vec.emplace_back(id);
		}

		template <typename... t_cmp>
		void
		add_component(t_ent_id id, auto&&... arg) noexcept
		{
			if constexpr (sizeof...(arg) == 0)
			{
				add_component_cmp_buffer.write(id);
				add_component_cmd_vec.emplace_back(&detail::fn_add_component_no_arg<t_storage, t_ent_id, t_cmp...>);
			}
			else if constexpr (sizeof...(arg) == sizeof...(t_cmp))
			{
				add_component_cmp_buffer.write(id, t_cmp{ FWD(arg) }...);
				add_component_cmd_vec.emplace_back(&detail::fn_add_component_with_arg<t_storage, t_ent_id, t_cmp...>);
			}
			else
			{
				static_assert(false, "invalid");
			}
		}

		template <typename... t_cmp>
		void
		remove_component(t_ent_id id) noexcept
		{
			remove_component_cmd_vec.emplace_back({ id, &detail::fn_remove_component<t_storage, t_ent_id, t_cmp...> });
		}

		void
		flush(t_storage& storage) noexcept
		{
			for (auto ent_id : remove_entity_cmd_vec)
			{
				storage.remove_entity(ent_id);
			}

			for (auto rem_cmd : remove_component_cmd_vec)
			{
				rem_cmd.fn(storage, rem_cmd.id);
			}

			for (auto cmd : new_entity_cmd_vec)
			{
				cmd.fn(storage, new_entity_cmp_buffer);
			}

			AGE_ASSERT(new_entity_cmp_buffer.has_remaining() is_false);

			for (auto cmd : add_component_cmd_vec)
			{
				cmd.fn(storage, add_component_cmp_buffer);
			}

			AGE_ASSERT(add_component_cmp_buffer.has_remaining() is_false);

			new_entity_cmd_vec.clear();
			new_entity_cmp_buffer.clear();
			remove_entity_cmd_vec.clear();
			add_component_cmd_vec.clear();
			add_component_cmp_buffer.clear();
			remove_component_cmd_vec.clear();
		}
	};
}	 // namespace age::ecs::command
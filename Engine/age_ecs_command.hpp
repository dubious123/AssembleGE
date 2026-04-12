#pragma once
#include "age.hpp"

namespace age::ecs::command
{
	struct header
	{
		uint32 payload_size;
	};
}	 // namespace age::ecs::command

// namespace age::ecs::command
//{
//	namespace detail
//	{
//		template <typename t_storage, typename t_ent_id, typename... t_cmp>
//		FORCE_INLINE void
//		fn_add_entity_impl(t_storage& storage, t_ent_id&& ent_id, t_cmp&&... cmp) noexcept
//		{
//			storage.add_entity(FWD(ent_id), FWD(cmp)...);
//		}
//
//		template <typename t_storage, typename t_ent_id, typename... t_cmp>
//		void
//		fn_add_entity(t_storage& storage, void* p_buffer) noexcept
//		{
//			meta::tuple_unpack_prefix<fn_add_entity_impl<t_storage, t_ent_id, t_cmp...>>(age::buffer::read<t_ent_id, t_cmp...>(p_buffer), storage);
//		}
//
//		template <typename t_storage, typename t_ent_id, typename... t_cmp>
//		FORCE_INLINE void
//		fn_add_component_impl(t_storage& storage, t_ent_id&& ent_id, t_cmp&&... cmp) noexcept
//		{
//			storage.add_component<t_cmp...>(FWD(ent_id), FWD(cmp)...);
//		}
//
//		template <typename t_storage, typename t_ent_id, typename... t_cmp>
//		void
//		fn_add_component(t_storage& storage, void* p_buffer) noexcept
//		{
//			meta::tuple_unpack_prefix<fn_add_component_impl<t_storage, t_ent_id, t_cmp...>>(age::buffer::read<t_ent_id, t_cmp...>(p_buffer), storage);
//		}
//
//		template <typename t_storage, typename t_ent_id, typename... t_cmp>
//		void
//		fn_remove_component(t_storage& storage, t_ent_id ent_id) noexcept
//		{
//			storage.remove_component<t_cmp...>(ent_id);
//		}
//	}	 // namespace detail
//
//	template <cx_entity_storage t_storage>
//	struct command_buffer
//	{
//		using t_ent_id			 = typename t_storage::t_ent_id;
//		using t_archetype_traits = typename t_storage::t_archetype_traits;
//		using t_archetype		 = typename t_storage::t_archetype;
//		using t_storage_cmp_idx	 = typename t_storage::t_storage_cmp_idx;
//		using t_local_cmp_idx	 = typename t_storage::t_local_cmp_idx;
//		using t_entity_block_idx = typename t_storage::t_entity_block_idx;
//		using t_entity_block	 = typename t_storage::t_entity_block;
//		using t_local_entity_idx = typename t_storage::t_local_entity_idx;
//
//		struct add_entity_cmd
//		{
//			uint64 arg_byte_offset;
//			void   (*fn)(t_storage&, void* p_arg);
//		};
//
//		struct add_component_cmd
//		{
//			uint64 arg_byte_offset;
//			void   (*fn)(t_storage&, void* p_arg);
//		};
//
//		struct remove_component_cmd
//		{
//			t_ent_id id;
//			void	 (*fn)(t_storage&, t_ent_id);
//		};
//
//		age::vector<add_entity_cmd>		  add_entity_cmd_vec;
//		age::vector<t_end_id>			  remove_entity_cmd_vec;
//		age::vector<add_component_cmd>	  add_component_cmd_vec;
//		age::vector<remove_component_cmd> remove_component_cmd_vec;
//
//		age::vector<std::byte> arg_storage;
//
//		template <typename... t_cmp>
//		void
//		add_entity(auto&&... arg) noexcept
//		{
//
//		}
//
//		void
//		remove_entity(t_ent_id id) noexcept
//		{
//			remove_entity_cmd_vec.emplace_back(id);
//		}
//
//		template <typename... t_cmp>
//		void
//		add_component() noexcept
//		{
//		}
//
//		template <typename... t_cmp>
//		void
//		remove_component(t_ent_id id) noexcept
//		{
//			remove_entity_cmd_vec.emplace_back({ id, &detail::fn_remove_component<t_storage, t_ent_id, t_cmp...> });
//		}
//
//		void
//		flush(t_storage& storage) noexcept
//		{
//			for (auto ent_id : remove_entity_cmd_vec)
//			{
//				storage.remove_entity(ent_id);
//			}
//
//			for (auto rem_cmd : remove_component_cmd_vec)
//			{
//				rem_cmd.fn(storage, rem_cmd.id);
//			}
//		}
//	};
// }	 // namespace age::ecs::command
#pragma once
#include "age.hpp"

namespace age::ecs
{
	namespace detail
	{
		FORCE_INLINE decltype(auto)
		handle_add_renderable(auto& renderer, auto&& cmp) noexcept
		{
			using t_cmp = BARE_OF(cmp);
			if constexpr (cx_renderable<t_cmp>)
			{
				t_cmp::add_renderable(renderer, cmp);
			}

			return FWD(cmp);
		}

		template <typename t_renderer, typename... t_cmp>
		FORCE_INLINE void
		handle_remove_renderable_impl(t_renderer& renderer, t_cmp&... cmp) noexcept
		{
			(BARE_OF(cmp)::remove_renderable(renderer, cmp), ...);
		}

		template <typename t_storage, typename t_renderer, typename... t_cmp>
		FORCE_INLINE void
		handle_remove_renderable(t_storage& storage, t_renderer& renderer, auto ent_id, meta::type_pack<t_cmp...>) noexcept
		{
			if constexpr (sizeof...(t_cmp) > 0)
			{
				meta::tuple_unpack_prefix<handle_remove_renderable_impl<t_renderer, t_cmp...>>(storage.template get_component<t_cmp...>(ent_id), renderer);
			}
		}

		template <typename t_storage, typename t_renderer, typename... t_cmp>
		FORCE_INLINE void
		fn_new_entity_impl(t_storage& storage, t_renderer& renderer, t_cmp&&... cmp) noexcept
		{
			storage.new_entity(handle_add_renderable(renderer, FWD(cmp))...);
		}

		template <typename t_storage, typename t_renderer, typename... t_cmp>
		void
		fn_new_entity_with_arg(t_storage& storage, t_renderer& renderer, age::byte_buf& buffer) noexcept
		{
			meta::tuple_unpack_prefix<fn_new_entity_impl<t_storage, t_renderer, t_cmp...>>(buffer.read<t_cmp...>(), storage, renderer);
		}

		template <typename t_storage, typename t_renderer, typename... t_cmp>
		void
		fn_new_entity_no_arg(t_storage& storage, t_renderer& renderer, age::byte_buf&) noexcept
		{
			storage.new_entity<t_cmp...>(handle_add_renderable(renderer, t_cmp{})...);
		}

		template <typename t_storage, typename t_renderer, typename t_ent_id, typename... t_cmp>
		FORCE_INLINE void
		fn_add_component_impl(t_storage& storage, t_renderer& renderer, t_ent_id&& ent_id, t_cmp&&... cmp) noexcept
		{
			storage.add_component<t_cmp...>(FWD(ent_id), handle_add_renderable(renderer, FWD(cmp))...);
		}

		template <typename t_storage, typename t_renderer, typename t_ent_id, typename... t_cmp>
		void
		fn_add_component_with_arg(t_storage& storage, t_renderer& renderer, age::byte_buf& buffer) noexcept
		{
			meta::tuple_unpack_prefix<fn_add_component_impl<t_storage, t_renderer, t_ent_id, t_cmp...>>(buffer.read<t_ent_id, t_cmp...>(), storage, renderer);
		}

		template <typename t_storage, typename t_renderer, typename t_ent_id, typename... t_cmp>
		void
		fn_add_component_no_arg(t_storage& storage, t_renderer& renderer, age::byte_buf& buffer) noexcept
		{
			storage.add_component<t_cmp...>(buffer.read<t_ent_id>(), handle_add_renderable(renderer, t_cmp{})...);
		}

		template <typename t_storage, typename t_renderer, typename t_ent_id, typename... t_cmp>
		void
		fn_remove_component(t_storage& storage, t_renderer& renderer, t_ent_id ent_id) noexcept
		{
			handle_remove_renderable(storage, renderer, ent_id, meta::filter_t<is_renderable, meta::type_pack<t_cmp...>>{});

			storage.remove_component<t_cmp...>(ent_id);
		}
	}	 // namespace detail

	struct command_buffer
	{
		using t_fn_erased = void (*)();

		struct new_entity_cmd
		{
			t_fn_erased fn_ptr;
		};

		struct add_component_cmd
		{
			t_fn_erased fn_ptr;
		};

		struct remove_component_cmd
		{
			uint64		id;
			t_fn_erased fn_ptr;
		};

		age::vector<new_entity_cmd> new_entity_cmd_vec;
		age::byte_buf				new_entity_cmp_buffer;

		age::byte_buf remove_entity_cmd_vec;

		age::vector<add_component_cmd> add_component_cmd_vec;
		age::byte_buf				   add_component_cmp_buffer;

		age::vector<remove_component_cmd> remove_component_cmd_vec;

		template <typename... t_cmp, typename t_storage, typename t_renderer>
		void
		new_entity(t_storage&, t_renderer&, auto&&... arg) noexcept
		{
			if constexpr (sizeof...(arg) == 0)
			{
				new_entity_cmd_vec.emplace_back(reinterpret_cast<t_fn_erased>(&detail::fn_new_entity_no_arg<t_storage, t_renderer, t_cmp...>));
			}
			else if constexpr (sizeof...(arg) == sizeof...(t_cmp))
			{
				new_entity_cmp_buffer.write(t_cmp{ FWD(arg) }...);
				new_entity_cmd_vec.emplace_back(reinterpret_cast<t_fn_erased>(&detail::fn_new_entity_with_arg<t_storage, t_renderer, t_cmp...>));
			}
			else
			{
				static_assert(false, "invalid");
			}
		}

		template <typename t_storage, typename t_renderer>
		void
		remove_entity(t_storage&, t_renderer&, typename t_storage::t_ent_id id) noexcept
		{
			remove_entity_cmd_vec.write(id);
		}

		template <typename... t_cmp, typename t_storage, typename t_renderer>
		void
		add_component(t_storage&, t_renderer&, typename t_storage::t_ent_id id, auto&&... arg) noexcept
		{
			if constexpr (sizeof...(arg) == 0)
			{
				add_component_cmp_buffer.write(id);
				add_component_cmd_vec.emplace_back(reinterpret_cast<t_fn_erased>(&detail::fn_add_component_no_arg<t_storage, t_renderer, typename t_storage::t_ent_id, t_cmp...>));
			}
			else if constexpr (sizeof...(arg) == sizeof...(t_cmp))
			{
				add_component_cmp_buffer.write(id, t_cmp{ FWD(arg) }...);
				add_component_cmd_vec.emplace_back(reinterpret_cast<t_fn_erased>(&detail::fn_add_component_with_arg<t_storage, t_renderer, typename t_storage::t_ent_id, t_cmp...>));
			}
			else
			{
				static_assert(false, "invalid");
			}
		}

		template <typename... t_cmp, typename t_storage, typename t_renderer>
		void
		remove_component(t_storage&, t_renderer&, typename t_storage::t_ent_id id) noexcept
		{
			remove_component_cmd_vec.emplace_back(static_cast<uint64>(id), reinterpret_cast<t_fn_erased>(&detail::fn_remove_component<t_storage, t_renderer, typename t_storage::t_ent_id, t_cmp...>));
		}

		template <typename t_storage, typename t_renderer>
		void
		flush(t_storage& storage, t_renderer& renderer) noexcept
		{
			for (auto rem_cmd : remove_component_cmd_vec)
			{
				reinterpret_cast<void (*)(t_storage&, t_renderer&, typename t_storage::t_ent_id)>(rem_cmd.fn_ptr)(storage, renderer, static_cast<typename t_storage::t_ent_id>(rem_cmd.id));
			}

			while (remove_entity_cmd_vec.has_remaining())
			{
				storage.remove_entity(remove_entity_cmd_vec.read<typename t_storage::t_ent_id>(), renderer);
			}

			for (auto cmd : new_entity_cmd_vec)
			{
				reinterpret_cast<void (*)(t_storage&, t_renderer&, age::byte_buf&)>(cmd.fn_ptr)(storage, renderer, new_entity_cmp_buffer);
			}

			AGE_ASSERT(new_entity_cmp_buffer.has_remaining() is_false);

			for (auto cmd : add_component_cmd_vec)
			{
				reinterpret_cast<void (*)(t_storage&, t_renderer&, age::byte_buf&)>(cmd.fn_ptr)(storage, renderer, add_component_cmp_buffer);
			}

			AGE_ASSERT(add_component_cmp_buffer.has_remaining() is_false);

			clear();
		}

		void
		clear() noexcept
		{
			new_entity_cmd_vec.clear();
			new_entity_cmp_buffer.clear();
			remove_entity_cmd_vec.clear();
			add_component_cmd_vec.clear();
			add_component_cmp_buffer.clear();
			remove_component_cmd_vec.clear();
		}

		void
		validate() noexcept
		{
			if constexpr (age::config::debug_mode)
			{
				AGE_ASSERT(new_entity_cmd_vec.is_empty());
				AGE_ASSERT(new_entity_cmp_buffer.has_remaining() is_false);
				AGE_ASSERT(remove_entity_cmd_vec.has_remaining() is_false);
				AGE_ASSERT(add_component_cmd_vec.is_empty());
				AGE_ASSERT(add_component_cmp_buffer.has_remaining() is_false);
				AGE_ASSERT(remove_component_cmd_vec.is_empty());
			}
		}
	};
}	 // namespace age::ecs
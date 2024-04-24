#pragma once
#include <thread>
#include <intrin.h>
#include "__common.h"
#include "__meta.h"

#ifdef _DEBUG
	#include <sstream>
	#include <string>
	#include <iostream>
	#define DEBUG_LOG(message) [] {std::stringstream ss; ss << message << std::endl; std::cout<<ss.str(); }();
#else
	#define DEBUG_LOG(message)
#endif

#define MEMORY_BLOCK_SIZE (sizeof(uint8) * 1024 * 16)

// todo
// no runtime calculation from memory_block , do compile time calculation from world
// world => memoryblock_interpretor => take archetype and return void* ptr

namespace ecs
{
	using namespace meta;

	using scene_id	   = uint16;
	using world_idx	   = uint16;
	using entity_idx   = uint64;
	using component_id = uint64;
	using archetype_t  = uint64;

	struct memory_block;

	struct entity
	{
		entity_idx	idx;
		archetype_t archetype;
		// memory_block* p_mem_block;
		uint64 mem_block_idx;
		uint64 memory_idx;
	};

	template <typename c1, typename c2>
	struct component_comparator : std::integral_constant<bool, (c1::id >= c2::id)>
	{
	};

	template <typename... c>
	struct component_wrapper
	{
		using component_tpl = tuple_sort<component_comparator, c...>::type;

		static inline constinit auto sizes = [] {
			auto arr = std::array<size_t, sizeof...(c)>();
			auto i	 = 0;
			([&arr, &i] {
				arr[tuple_index<c, component_tpl>::value]										  = sizeof(c);
				arr[tuple_index<c, typename tuple_sort<component_comparator, c...>::type>::value] = sizeof(c);
				++i;
			}(),
			 ...);
			return arr;
		}();

		template <typename... t>
		static inline consteval archetype_t calc_archetype()
		{
			archetype_t archetype = 0;
			([&archetype] {
				archetype |= 1ui64 << tuple_index<t, component_tpl>::value;
			}(),
			 ...);
			return archetype;
		};
	};

	template <typename t>
	concept is_seq = requires { t::_seq; };

	template <typename t>
	concept is_par = requires { t::_par; };

	template <typename t>
	concept is_cond = requires { t::_cond; };

	template <typename t>
	concept is_node = ecs::is_par<t> || ecs::is_seq<t> || is_cond<t>;

	template <typename system>
	concept has_on_system_begin = requires() { &system::on_system_begin; };

	template <typename system, typename world>
	concept has_on_system_begin_w = requires() { &system::template on_system_begin<world>; };

	template <typename system>
	concept has_on_thread_init = requires() { &system::on_thread_init; };

	template <typename system, typename world>
	concept has_on_thread_init_w = requires() { &system::template on_thread_init<world>; };

	template <typename system>
	concept has_update = requires() { &system::update; };

	template <typename system, typename world>
	concept has_update_w = requires() { &system::template update<world>; };

	template <typename system>
	concept has_on_thread_dispose = requires() { &system::on_thread_dispose; };

	template <typename system, typename world>
	concept has_on_thread_dispose_w = requires() { &system::template on_thread_dispose<world>; };

	template <typename system>
	concept has_on_system_end = requires() { system::on_system_end; };

	template <typename system, typename world>
	concept has_on_system_end_w = requires() { system::template on_system_end<world>; };

	template <typename... s>
	struct par
	{
		constinit static inline const auto _par = true;

		std::tuple<s...> tpl;

		void update(auto&& world)
		{
			DEBUG_LOG("---par start (func)---");

			std::vector<std::thread> threads;
			([&]() {
				auto& node = meta::get_tuple_value<s>(tpl);
				if constexpr (is_node<s>)
				{
					threads.emplace_back([&]() {
						int a = 2;
						node.update(std::forward<decltype(world)>(world));
					});
				}
				else
				{
					threads.emplace_back([&]() {
						world.perform(node);
					});

					// system
					//
				}
			}(),
			 ...);

			std::ranges::for_each(threads, [](auto& th) {
				th.join();
			});

			DEBUG_LOG("---par end (func)---");
		}
	};

	template <typename... s>
	struct seq
	{
		constinit static inline const auto _seq = true;

		std::tuple<s...> tpl;

		void update(auto&& world)
		{
			DEBUG_LOG("---seq start (func)---");
			([&]() {
				auto& node = meta::get_tuple_value<s>(tpl);
				if constexpr (is_node<s>)
				{
					int a = 2;
					node.update(std::forward<decltype(world)>(world));
				}
				else
				{
					world.perform(node);
					// system
					// world.update(node);
				}
			}(),
			 ...);
			DEBUG_LOG("---seq end (func)---");
		}
	};

	template <auto cond_func, typename s_l, typename s_r>
	struct cond
	{
		constinit static inline const auto _cond = true;

		s_l left;
		s_r right;

		void update(auto&& world)
		{
			DEBUG_LOG("---cond start (func)---");
			if (cond_func())
			{
				if constexpr (is_node<s_l>)
				{
					int a = 2;
					left.update(std::forward<decltype(world)>(world));
				}
				else
				{
					world.perform(left);
					// system
					// world.update(node);
				}
			}
			else
			{
				if constexpr (is_node<s_r>)
				{
					int a = 2;
					right.update(std::forward<decltype(world)>(world));
				}
				else
				{
					world.perform(right);
					// system
					// world.update(node);
				}
			}
			DEBUG_LOG("---cond end (func)---");
		}
	};

	template <typename... s>
	struct system_group : seq<s...>
	{
	};

	struct memory_block
	{
		// uint8 memory[MEMORY_BLOCK_SIZE];
		uint8* memory;

		memory_block()
		{
			memory = (uint8*)malloc(MEMORY_BLOCK_SIZE);
		}

		memory_block(memory_block&& other) noexcept : memory(other.memory)
		{
			other.memory = nullptr;
		}

		memory_block& operator=(memory_block&& other) noexcept
		{
			memory		 = other.memory;
			other.memory = nullptr;
		}

		~memory_block()
		{
			free(memory);
		}

		memory_block(const memory_block& other) = delete;

		memory_block& operator=(memory_block& other) = delete;

		// 16kb => n component, (0 < n <= 64)
		//  2 byte => size,
		//  2 byte => capatity
		//  2 byte => component_count
		//  4 * n byte => {component_size, offset}
		//
		//  6 <= header_size <= 6 + 4 * 64 = 6 + 256 = 262
		// write component => know type(new entity) or not(add component) (== copy)
		// read component => know type
		// copy component => don't know type

		// component_idx (c_idx) => n in nth component in archetype
		//  ex) if archetype is a b c d (0b1111) component_idx of component c is 2
		//   ex) if archetype is a c d (0b1101) component_idx of component a is 0
		//    ex) if archetype is a c d (0b1101) component_idx of component c is 1
		//    ex) if archetype is a c d (0b1101) component_idx of component d is 2
		uint16 get_count() const
		{
			return *(uint16*)memory;
		}

		uint16 get_capacity() const
		{
			return *(uint16*)(memory + 2);
		}

		uint16 get_component_count() const
		{
			return *(uint16*)(memory + 4);
		}

		uint16 get_component_size(uint8 c_idx) const
		{
			return (uint16)((*(uint32*)(memory + 6 + c_idx * 4)) >> 16);
		}

		uint16 get_component_offset(uint8 c_idx) const
		{
			return (uint16)((*(uint32*)(memory + 6 + c_idx * 4)));
		}

		uint16 get_header_size() const
		{
			return 6 + get_component_count() * sizeof(uint32);
		}

		entity_idx get_entity_idx(uint16 m_idx) const
		{
			return *(entity_idx*)(memory + get_header_size() + m_idx * sizeof(entity_idx));
		}

		void* get_component_ptr(uint16 m_idx, uint8 c_idx) const
		{
			return (void*)(memory + get_component_offset(c_idx) + m_idx * get_component_size(c_idx));
		}

		uint16 calc_size_per_archetype() const
		{
			auto   c_count = get_component_count();
			uint16 size	   = sizeof(entity_idx);
			for (auto c_idx = 0; c_idx < c_count; ++c_idx)
			{
				size += get_component_size(c_idx);
			}

			return size;
		}

		void write_count(uint16 new_count)
		{
			*(uint16*)memory = new_count;
		}

		void write_capacity(uint16 capacity)
		{
			*(uint16*)(memory + 2) = capacity;
		}

		void write_component_count(uint16 count)
		{
			*(uint16*)(memory + 4) = count;
		}

		void write_component_data(uint8 c_idx, uint16 offset, uint16 size)
		{
			auto component_info_ptr			   = memory + 6 + sizeof(uint32) * c_idx;
			*(uint16*)component_info_ptr	   = offset;	// offset
			*(uint16*)(component_info_ptr + 2) = size;
		}

		void write_component_data(uint8 c_idx, uint32 c_data)
		{
			*(uint32*)(memory + 6 + sizeof(uint32) * c_idx) = c_data;
		}

		void write_entity_idx(uint8 m_idx, entity_idx idx)
		{
			*(entity_idx*)(memory + get_header_size() + m_idx * sizeof(entity_idx)) = idx;
		}

		void remove_entity(uint16 m_idx)
		{
			auto count		= get_count();
			auto last_m_idx = count - 1;

			if (m_idx != last_m_idx)
			{
				auto c_count = get_component_count();

				write_entity_idx(m_idx, get_entity_idx(last_m_idx));
				for (auto c_idx = 0; c_idx < c_count; ++c_idx)
				{
					memcpy(get_component_ptr(m_idx, c_idx), get_component_ptr(last_m_idx, c_idx), get_component_size(c_idx));
				}
			}

			write_count(count - 1);
		}

		bool is_full() const
		{
			return get_count() == get_capacity();
		}

		template <typename... t>
		void new_entity(entity& e)
		{
			using sorted_component_tpl = component_wrapper<t...>::component_tpl;
			assert(__popcnt(e.archetype) == get_component_count());

			auto m_idx = get_count();

			write_entity_idx(m_idx, e.idx);

			([this, &m_idx] {
				new (get_component_ptr(m_idx, meta::tuple_index_v<t, sorted_component_tpl>)) t();
			}(),
			 ...);

			write_count(m_idx + 1);
			e.memory_idx = m_idx;
			// e.p_mem_block = this;
		}

		template <typename... t>
		static void init(memory_block& block)
		{
			using sorted_component_tpl											 = component_wrapper<t...>::component_tpl;
			static const std::array<size_t, sizeof...(t)> sorted_component_sizes = []() {
				std::array<size_t, sizeof...(t)> arr;
				([&arr] {
					arr[meta::tuple_index_v<t, sorted_component_tpl>] = sizeof(t);
				}(),
				 ...);
				return arr;
			}();

			assert(sizeof...(t) <= 64);
			assert(sizeof...(t) > 0);
			const auto size_per_archetype = sizeof(entity_idx) + (sizeof(t) + ... + 0);
			const auto component_count	  = sizeof...(t);
			const auto capacity			  = (MEMORY_BLOCK_SIZE - 6 - 4 * component_count) / size_per_archetype;

			block.write_count(0);
			block.write_capacity(capacity);
			block.write_component_count(component_count);

			uint16 offset = block.get_header_size() + sizeof(entity_idx) * capacity;	// header + entity_idx
			for (auto c_idx = 0; c_idx < sizeof...(t); ++c_idx)
			{
				block.write_component_data(c_idx, offset, sorted_component_sizes[c_idx]);
				offset += capacity * (sorted_component_sizes[c_idx]);
			}

			assert(MEMORY_BLOCK_SIZE > offset);
			assert(MEMORY_BLOCK_SIZE - offset < size_per_archetype);
		}
	};

	struct world_base
	{
		data_structure::vector<entity>										   entities;
		data_structure::map<archetype_t, data_structure::vector<memory_block>> memory_block_vec_map;
		// data_structure::vector<std::pair<archetype_t, data_structure::list<memory_block>>> memory_block_list_vec;

		size_t entity_hole_begin_idx = -1;
		size_t entity_hole_count	 = 0;

		world_base(const world_base&&)		= delete;
		world_base(const world_base&)		= delete;
		world_base& operator=(world_base&&) = delete;
		world_base& operator=(world_base&)	= delete;
		world_base()						= default;
	};

	template <typename... c>
	class world : public world_base
	{
	  private:
		using component_tpl = tuple_sort<component_comparator, c...>::type;
		using world_t		= world<c...>;

		template <typename... t>
		static consteval archetype_t _calc_archetype()
		{
			archetype_t archetype = 0;
			([&archetype] {
				archetype |= 1ui64 << tuple_index<t, component_tpl>::value;
			}(),
			 ...);
			return archetype;
		};

		template <typename sys, typename... t>
		static consteval archetype_t _calc_func_archetype(void (sys::*)(t...))
		{
			return _calc_archetype<std::remove_cvref_t<t>...>();
		}

		template <typename sys, typename... t>
		static consteval archetype_t _calc_func_archetype(void (sys::*)(entity_idx, t...))
		{
			return _calc_archetype<std::remove_cvref_t<t>...>();
		}

		template <typename sys, typename w, typename... t>
		static consteval archetype_t _calc_func_archetype(void (sys::*)(w, t...))
		{
			return _calc_archetype<std::remove_cvref_t<t>...>();
		}

		template <typename sys, typename w, typename... t>
		static consteval archetype_t _calc_func_archetype(void (sys::*)(w, entity_idx, t...))
		{
			return _calc_archetype<std::remove_cvref_t<t>...>();
		}

		template <typename system_t, typename... t>
		void _update_entity(system_t& sys, void (system_t::*func)(t...), memory_block& mem_block, uint16 m_idx)
		{
			static constinit const auto archetype = _calc_archetype<std::remove_cvref_t<t>...>();
			(sys.*func)(*(std::remove_cvref_t<t>*)(mem_block.get_component_ptr(m_idx, _calc_component_idx<std::remove_cvref_t<t>>(archetype)))...);
		}

		template <typename system_t, typename... t>
		void _update_entity(system_t& sys, void (system_t::*func)(entity_idx, t...), memory_block& mem_block, uint16 m_idx)
		{
			static constinit const auto archetype = _calc_archetype<std::remove_cvref_t<t>...>();
			(sys.*func)(mem_block.get_entity_idx(m_idx), *(std::remove_cvref_t<t>*)(mem_block.get_component_ptr(m_idx, _calc_component_idx<std::remove_cvref_t<t>>(archetype)))...);
		}

		template <typename world_t, typename system_t, typename... t>
		void _update_entity(world_t& world, system_t& sys, void (system_t::*func)(world_t&, entity_idx, t...), memory_block& mem_block, uint16 m_idx)
		{
			static constinit const auto archetype = _calc_archetype<std::remove_cvref_t<t>...>();
			(sys.*func)(world, mem_block.get_entity_idx(m_idx), *(std::remove_cvref_t<t>*)(mem_block.get_component_ptr(m_idx, _calc_component_idx<std::remove_cvref_t<t>>(archetype)))...);
		}

		template <typename world_t, typename system_t, typename... t>
		void _update_entity(world_t& world, system_t& sys, void (system_t::*func)(world_t&, t...), memory_block& mem_block, uint16 m_idx)
		{
			static constinit const auto archetype = _calc_archetype<std::remove_cvref_t<t>...>();
			(sys.*func)(world, *(std::remove_cvref_t<t>*)(mem_block.get_component_ptr(m_idx, _calc_component_idx<std::remove_cvref_t<t>>(archetype)))...);
		}

		template <typename sys>
		static consteval archetype_t _calc_sys_archetype()
		{
			if constexpr (has_update<sys>)
			{
				// return 0;
				return _calc_func_archetype(&sys::update);
			}
			else if constexpr (has_update_w<sys, world<c...>>)
			{
				return _calc_func_archetype(&sys::template update<ecs::world<c...>>);
			}
			else
			{
				return 0;
			}
		}

		template <typename t>
		inline uint8 _calc_component_idx(archetype_t a) const
		{
			return __popcnt(((1 << tuple_index_v<t, component_tpl>)-1) & a);
		}

	  public:
		size_t entity_count() const
		{
			return entities.size() - entity_hole_count;
		}

		memory_block* get_p_mem_block(entity& e)
		{
			assert(memory_block_vec_map.contains(e.archetype));
			return &memory_block_vec_map[e.archetype][e.mem_block_idx];
		}

		template <typename... t>
		entity_idx new_entity()
		{
			static constinit const auto size	  = sizeof(entity_idx) + (sizeof(t) + ...);
			static constinit const auto capacity  = MEMORY_BLOCK_SIZE / size;
			static constinit const auto archetype = _calc_archetype<t...>();

			auto&		  block_list	= memory_block_vec_map[archetype];
			memory_block* p_block		= nullptr;
			size_t		  mem_block_idx = -1;

			auto res = std::ranges::find_if(block_list, [&](auto& block) { 
					++mem_block_idx;
					return block.is_full() is_false; });


			if (res == block_list.end())
			{
				auto& block = block_list.emplace_back();
				memory_block::init<t...>(block);
				p_block		  = &block;
				mem_block_idx = 0;
			}
			else
			{
				p_block = &(*res);
			}

			if (entity_hole_count == 0)
			{
				auto& e			= entities.emplace_back();
				e.archetype		= archetype;
				e.idx			= entities.size() - 1;
				e.mem_block_idx = mem_block_idx;
				p_block->new_entity<t...>(e);
				return e.idx;
			}
			else
			{
				auto& e				  = entities[entity_hole_begin_idx];
				auto  next_hole_idx	  = e.idx;
				e.idx				  = entity_hole_begin_idx;
				entity_hole_begin_idx = next_hole_idx;
				--entity_hole_count;
				e.archetype		= archetype;
				e.mem_block_idx = mem_block_idx;
				p_block->new_entity<t...>(e);

				return e.idx;
			}
		}

		void delete_entity(entity_idx idx)
		{
			auto& e = entities[idx];
			get_p_mem_block(e)->remove_entity(e.memory_idx);
			entities[idx].idx	  = entity_hole_begin_idx;
			entity_hole_begin_idx = idx;
			++entity_hole_count;
		}

		template <typename... t>
		bool has_component(entity_idx idx)
		{
			auto archetype = _calc_archetype<t...>();
			return (entities[idx].archetype & archetype) == archetype;
		}

		template <typename t>
		t& get_component(entity_idx idx)
		{
			assert(has_component<t>(idx));
			auto& e = entities[idx];
			return *(t*)(memory_block_vec_map[e.archetype][e.mem_block_idx].get_component_ptr(e.memory_idx, _calc_component_idx<t>(e.archetype)));
		}

		template <typename... t>
		void add_component(entity_idx idx)
		{
			assert(has_component<t...>(idx) == false);
			auto& e				= entities[idx];
			auto  new_archetype = e.archetype | _calc_archetype<t...>();

			const auto c_idx_arr = [&] {
				auto arr = std::array<size_t, sizeof...(t)>();
				auto i	 = 0;
				([&] {
					arr[tuple_index_v<t, component_wrapper<t...>::component_tpl>] = __popcnt(((1 << tuple_index_v<t, component_tpl>)-1) & new_archetype);
				}(),
				 ...);
				return arr;
			}();

			auto&		  block_list		= memory_block_vec_map[new_archetype];
			memory_block* p_new_block		= nullptr;
			size_t		  new_mem_block_idx = -1;

			auto res = std::ranges::find_if(block_list, [&new_mem_block_idx](auto& block) {
				++new_mem_block_idx;
				return block.is_full() == false;
			});

			if (res == block_list.end())
			{
				auto& block		  = block_list.emplace_back();
				new_mem_block_idx = 0;
				p_new_block		  = &block;

				p_new_block->write_count(0);

				auto	   memory			  = p_new_block->memory;
				const auto size_per_archetype = get_p_mem_block(e)->calc_size_per_archetype() + (sizeof(t) + ... + 0);
				const auto component_count	  = get_p_mem_block(e)->get_component_count() + sizeof...(t);
				const auto capacity			  = (MEMORY_BLOCK_SIZE - 6 - sizeof(uint32) * component_count) / size_per_archetype;


				p_new_block->write_count(0);
				p_new_block->write_capacity(capacity);
				p_new_block->write_component_count(component_count);

				auto c_idx	= 0;
				auto offset = p_new_block->get_header_size() + sizeof(entity_idx) * capacity;
				auto i		= 0;
				for (; i < sizeof...(t); ++i)
				{
					auto new_c_idx	= c_idx_arr[i];
					auto new_c_size = component_wrapper<t...>::sizes[i];
					for (; c_idx < new_c_idx; ++c_idx)
					{
						p_new_block->write_component_data(c_idx, offset, get_p_mem_block(e)->get_component_size(c_idx - i));
						offset += capacity + get_p_mem_block(e)->get_component_size(c_idx);
					}

					assert(c_idx == new_c_idx);
					p_new_block->write_component_data(c_idx, offset, new_c_size);
					offset += capacity * new_c_size;
					++c_idx;
				}


				for (; c_idx < component_count; ++c_idx)
				{
					p_new_block->write_component_data(c_idx, offset, get_p_mem_block(e)->get_component_size(c_idx - i));
					offset += capacity + get_p_mem_block(e)->get_component_size(c_idx);
				}
			}
			else
			{
				p_new_block = &(*res);
			}

			const auto new_component_count = __popcnt(new_archetype);
			const auto new_m_idx		   = p_new_block->get_count();
			assert(new_component_count == get_p_mem_block(e)->get_component_count() + sizeof...(t));
			assert(p_new_block->is_full() == false);

			auto i	   = 0;
			auto c_idx = 0;
			([&]() {
				auto new_c_idx = c_idx_arr[i];
				for (; c_idx < new_c_idx; ++c_idx)
				{
					assert(p_new_block->get_component_size(c_idx) == get_p_mem_block(e)->get_component_size(c_idx - i));
					memcpy(
						p_new_block->get_component_ptr(new_m_idx, c_idx),
						get_p_mem_block(e)->get_component_ptr(e.memory_idx, c_idx - i),
						(size_t)(get_p_mem_block(e)->get_component_size(c_idx - i)));
				}

				assert(c_idx == new_c_idx);
				new (p_new_block->get_component_ptr(new_m_idx, _calc_component_idx<t>(new_archetype))) t();
				++c_idx;
				++i;
			}(),
			 ...);

			for (; c_idx < new_component_count; ++c_idx)
			{
				assert(p_new_block->get_component_size(c_idx) == get_p_mem_block(e)->get_component_size(c_idx - i));
				memcpy(
					p_new_block->get_component_ptr(new_m_idx, c_idx),
					get_p_mem_block(e)->get_component_ptr(e.memory_idx, c_idx - i),
					(size_t)(get_p_mem_block(e)->get_component_size(c_idx - i)));
			}

			p_new_block->write_count(p_new_block->get_count() + 1);
			p_new_block->write_entity_idx(new_m_idx, e.idx);

			get_p_mem_block(e)->remove_entity(e.memory_idx);

			e.mem_block_idx = new_mem_block_idx;
			e.memory_idx	= new_m_idx;
			e.archetype		= new_archetype;
		}

		template <typename... t>
		void remove_component(entity_idx idx)
		{
			assert(has_component<t...>(idx));
			auto& e				= entities[idx];
			auto  new_archetype = e.archetype ^ _calc_archetype<t...>();
			// todo
			if (new_archetype == 0)
			{
				get_p_mem_block(e)->remove_entity(e.memory_idx);
				e.archetype = 0;
			}

			const auto c_idx_arr = [&] {
				auto arr = std::array<size_t, sizeof...(t)>();
				auto i	 = 0;
				([&] {
					arr[tuple_index_v<t, component_wrapper<t...>::component_tpl>] = __popcnt(((1 << tuple_index_v<t, component_tpl>)-1) & new_archetype);
				}(),
				 ...);
				return arr;
			}();

			auto&		  block_list		= memory_block_vec_map[new_archetype];
			auto*		  p_prev_block		= get_p_mem_block(e);
			memory_block* p_new_block		= nullptr;
			size_t		  new_mem_block_idx = -1;

			auto res = std::ranges::find_if(block_list, [&](auto& mem_block) { 
				++new_mem_block_idx; 
				return mem_block.is_full() is_false; });

			if (res == block_list.end())
			{
				auto& block = block_list.emplace_back();
				p_new_block = &block;

				p_new_block->write_count(0);

				auto memory = p_new_block->memory;

				const auto size_per_archetype = p_prev_block->calc_size_per_archetype() - (sizeof(t) + ... + 0);
				const auto component_count	  = p_prev_block->get_component_count() - sizeof...(t);
				const auto capacity			  = (MEMORY_BLOCK_SIZE - 6 - sizeof(uint32) * component_count) / size_per_archetype;

				assert(component_count != 0);

				p_new_block->write_count(0);
				p_new_block->write_capacity(capacity);
				p_new_block->write_component_count(component_count);

				auto c_idx	= 0;
				auto offset = p_new_block->get_header_size() + sizeof(entity_idx) * capacity;

				auto i = 0;
				for (; i < sizeof...(t); ++i)
				{
					auto new_c_idx	= c_idx_arr[i];
					auto new_c_size = component_wrapper<t...>::sizes[i];
					for (; c_idx < new_c_idx; ++c_idx)
					{
						p_new_block->write_component_data(c_idx, offset, p_prev_block->get_component_size(c_idx + i));
						offset += capacity + p_prev_block->get_component_size(c_idx);
					}

					assert(c_idx == new_c_idx);
					//++c_idx;
				}

				for (; c_idx < component_count; ++c_idx)
				{
					p_new_block->write_component_data(c_idx, offset, p_prev_block->get_component_size(c_idx + i));
					offset += capacity + p_prev_block->get_component_size(c_idx);
				}
			}
			else
			{
				p_new_block = &(*res);
			}

			const auto new_component_count = __popcnt(new_archetype);
			const auto new_m_idx		   = p_new_block->get_count();

			auto i	   = 0;
			auto c_idx = 0;
			for (; i < sizeof...(t); ++i)
			{
				auto new_c_idx = c_idx_arr[i];
				for (; c_idx < new_c_idx; ++c_idx)
				{
					assert(p_new_block->get_component_size(c_idx) == p_prev_block->get_component_size(c_idx + i));
					memcpy(
						p_new_block->get_component_ptr(new_m_idx, c_idx),
						p_prev_block->get_component_ptr(e.memory_idx, c_idx + i),
						(size_t)(p_prev_block->get_component_size(c_idx + i)));
				}

				assert(c_idx == new_c_idx);
			}

			for (; c_idx < new_component_count; ++c_idx)
			{
				assert(p_new_block->get_component_size(c_idx) == p_prev_block->get_component_size(c_idx + i));
				memcpy(
					p_new_block->get_component_ptr(new_m_idx, c_idx),
					p_prev_block->get_component_ptr(e.memory_idx, c_idx + i),
					(size_t)(p_prev_block->get_component_size(c_idx + i)));
			}


			p_new_block->write_count(p_new_block->get_count() + 1);
			p_new_block->write_entity_idx(new_m_idx, e.idx);

			p_prev_block->remove_entity(e.memory_idx);

			e.memory_idx	= new_m_idx;
			e.archetype		= new_archetype;
			e.mem_block_idx = new_mem_block_idx;
		}

		archetype_t get_archetype(entity_idx idx) const
		{
			return entities[idx].archetype;
		}

		template <typename system_t>
		void perform(system_t& sys)
		{
			DEBUG_LOG("sys perform begin");
			static constinit const auto archetype = _calc_sys_archetype<system_t>();
			if constexpr (has_on_system_begin<system_t>)
			{
				sys.on_system_begin();
			}
			else if constexpr (has_on_system_begin_w<system_t, world_t>)
			{
				sys.on_system_begin(*this);
			}

			auto threads = data_structure::vector<std::thread>();

			std::ranges::for_each(
				memory_block_vec_map | std::views::filter([](auto& pair) { return (pair.first & archetype) != 0; }),
				[&, this](auto& pair) {
					for (auto& mem_block : pair.second)
					{
						threads.emplace_back(
							[&, this]() {
								if constexpr (has_on_thread_init<system_t>)
								{
									sys.on_thread_init();
								}
								else if constexpr (has_on_thread_init_w<system_t, world_t>)
								{
									sys.on_thread_init(*this);
								}

								auto count = mem_block.get_count();
								for (auto m_idx = 0; m_idx < count; ++m_idx)
								{
									if constexpr (has_update<system_t>)
									{
										_update_entity(sys, &system_t::update, mem_block, m_idx);
									}
									else if constexpr (has_update_w<system_t, world_t>)
									{
										_update_entity((*this), sys, &system_t::template update<world_t>, mem_block, m_idx);
									}
								}

								if constexpr (has_on_thread_dispose<system_t>)
								{
									sys.on_thread_dispose();
								}
								else if constexpr (has_on_thread_dispose_w<system_t, world_t>)
								{
									sys.on_thread_dispose(*this);
								}
							});
					}
				});

			std::ranges::for_each(threads, [](auto& th) { th.join(); });

			if constexpr (has_on_system_end<system_t>)
			{
				sys.on_system_end();
			}
			else if constexpr (has_on_system_end_w<system_t, world_t>)
			{
				sys.on_system_end(*this);
			}

			DEBUG_LOG("sys perform end");
		}
	};

	struct scene_base
	{
		world_base* worlds;
	};

	template <typename... W>
	struct scene : scene_base
	{
	  private:
		template <unsigned int N>
		using world_at = variadic_at_t<N, W...>;

	  public:
		scene()
		{
			worlds	   = (world_base*)malloc(sizeof(world_base) * sizeof...(W));
			auto index = 0;
			([&index, this] {
				new (worlds + index) W();
				++index;
			}(),
			 ...);
		}

		~scene()
		{
			for (auto i = 0; i < sizeof...(W); ++i)
			{
				(worlds + i)->~world_base();
			}

			free(worlds);
		}

		scene(const scene&&)	  = delete;
		scene(const scene&)		  = delete;
		scene& operator=(scene&&) = delete;
		scene& operator=(scene&)  = delete;

		template <unsigned int N>
		world_at<N>& get_world()
		{
			return *(world_at<N>*)(void*)(worlds + N);
		}

		// inline void loop();
	};

	struct subworld
	{
	};

	bool is_idx_valid(uint64);
	// bool is_idx_valid(uint32);
	// bool is_idx_valid(uint16);

	bool is_id_valid(uint64);

	// bool is_id_valid(uint32);
	// bool is_id_valid(uint16);
	// EDITOR_API scene_base get_scene(size_t index);
}	 // namespace ecs
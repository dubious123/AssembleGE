#pragma once
#include <thread>
#include <execution>
#include <bit>
#include <intrin.h>
#include "__common.h"
#include "__meta.h"
#include "__ecs_common.h"

#ifdef _DEBUG
	#include <sstream>
	#include <string>
	#include <iostream>
	#define DEBUG_LOG(message) [&] {std::stringstream ss; ss << message << std::endl; std::cout<<ss.str(); }();
#else
	#define DEBUG_LOG(message)
#endif

#define MEMORY_BLOCK_SIZE	 (sizeof(uint8) * 1024 * 16)
#define INVALID_MEMBLOCK_IDX -1

// todo
// no runtime calculation from memory_block , do compile time calculation from world
// world => memoryblock_interpretor => take archetype and return void* ptr

namespace ecs
{
	using namespace meta;

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

		std::array<std::tuple<s...>, 1> tpl_arr { std::tuple<s...>() };

		void update(auto&& world)
		{
			DEBUG_LOG("---par start (func)---");

			auto v = tpl_arr | std::views::elements<0>;
			std::for_each(std::execution::par_unseq, v.begin(), v.end(),
						  [&](auto& node) {
							  if constexpr (is_node<decltype(node)>)
							  {
								  node.update(std::forward<decltype(world)>(world));
							  }
							  else
							  {
								  world.perform(node);
							  }
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
					node.update(std::forward<decltype(world)>(world));
				}
				else
				{
					world.perform(node);
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
					left.update(std::forward<decltype(world)>(world));
				}
				else
				{
					world.perform(left);
				}
			}
			else
			{
				if constexpr (is_node<s_r>)
				{
					right.update(std::forward<decltype(world)>(world));
				}
				else
				{
					world.perform(right);
				}
			}
			DEBUG_LOG("---cond end (func)---");
		}
	};

	template <typename... s>
	struct system_group : seq<s...>
	{
	};

	struct world_base
	{
		data_structure::vector<entity>										   entities;
		data_structure::map<archetype_t, data_structure::vector<memory_block>> memory_block_vec_map;

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
		using component_tpl = tuple_sort<component_comparator, std::tuple<c...>>::type;
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

		template <typename sys, typename... t>
		static consteval archetype_t _calc_func_archetype(void (sys::*)(world_t&, t...))
		{
			return _calc_archetype<std::remove_cvref_t<t>...>();
		}

		template <typename sys, typename... t>
		static consteval archetype_t _calc_func_archetype(void (sys::*)(world_t&, entity_idx, t...))
		{
			return _calc_archetype<std::remove_cvref_t<t>...>();
		}

		template <typename system_t, typename... t>
		static void _update_entity(system_t& sys, void (system_t::*func)(t...), memory_block& mem_block, uint16 m_idx)
		{
			static constinit const auto archetype = _calc_archetype<std::remove_cvref_t<t>...>();
			(sys.*func)(*(std::remove_cvref_t<t>*)(mem_block.get_component_ptr(m_idx, _calc_component_idx<std::remove_cvref_t<t>>(archetype)))...);
		}

		template <typename system_t, typename... t>
		static void _update_entity(system_t& sys, void (system_t::*func)(entity_idx, t...), memory_block& mem_block, uint16 m_idx)
		{
			static constinit const auto archetype = _calc_archetype<std::remove_cvref_t<t>...>();
			(sys.*func)(mem_block.get_entity_idx(m_idx), *(std::remove_cvref_t<t>*)(mem_block.get_component_ptr(m_idx, _calc_component_idx<std::remove_cvref_t<t>>(archetype)))...);
		}

		template <typename world_t, typename system_t, typename... t>
		static void _update_entity(world_t& world, system_t& sys, void (system_t::*func)(world_t&, entity_idx, t...), memory_block& mem_block, uint16 m_idx)
		{
			static constinit const auto archetype = _calc_archetype<std::remove_cvref_t<t>...>();
			(sys.*func)(world, mem_block.get_entity_idx(m_idx), *(std::remove_cvref_t<t>*)(mem_block.get_component_ptr(m_idx, _calc_component_idx<std::remove_cvref_t<t>>(archetype)))...);
		}

		template <typename world_t, typename system_t, typename... t>
		static void _update_entity(world_t& world, system_t& sys, void (system_t::*func)(world_t&, t...), memory_block& mem_block, uint16 m_idx)
		{
			static constinit const auto archetype = _calc_archetype<std::remove_cvref_t<t>...>();
			(sys.*func)(world, *(std::remove_cvref_t<t>*)(mem_block.get_component_ptr(m_idx, _calc_component_idx<std::remove_cvref_t<t>>(archetype)))...);
		}

		template <typename sys>
		static consteval archetype_t _calc_sys_archetype()
		{
			if constexpr (has_update<sys>)
			{
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
		static inline uint8 _calc_component_idx(archetype_t a)
		{
			return __popcnt(((1 << tuple_index_v<t, component_tpl>)-1) & a);
		}

		static inline uint8 _calc_component_idx(archetype_t a, uint32 nth_component)
		{
			assert(nth_component < 64);
			assert(nth_component >= 0);
			return __popcnt64(((1 << nth_component) - 1) & a);
		}

		static void _init_mem_block(archetype_t archetype, size_t size_per_archetype, memory_block* p_block)
		{
			using namespace std::views;

			assert((sizeof...(c) <= 64) and (sizeof...(c) >= 0));
			assert(p_block is_not_nullptr);

			const auto component_count = __popcnt64(archetype);
			const auto capacity		   = (MEMORY_BLOCK_SIZE - (6 + sizeof(uint32) * component_count)) / size_per_archetype;

			p_block->write_component_count(component_count);
			p_block->write_capacity(capacity);
			p_block->write_count(0);


			auto offset = p_block->get_header_size() + sizeof(entity_idx) * capacity;	 // header + entity_idx
			auto c_idx	= 0;

			for (const auto nth_bit : iota(0, std::bit_width(archetype)) | filter([archetype](auto nth_bit) { return (archetype >> nth_bit) & 1; }))
			{
				auto c_size = component_wrapper<c...>::sizes[nth_bit];
				p_block->write_component_data(c_idx, offset, c_size);
				offset += c_size * capacity;
				++c_idx;
			}

			assert(p_block->get_header_size() == 6 + sizeof(uint32) * component_count);
			assert(offset + p_block->calc_unused_mem_size() == MEMORY_BLOCK_SIZE);
		}

		// static void _init_mem_block(archetype_t archetype, memory_block* p_block)
		//{
		//	_init_mem_block(archetype, component_wrapper<c...>::calc_total_size(archetype) + sizeof(entity_idx), p_block);
		// }

		static void _copy_components(archetype_t a_old, archetype_t a_new, memory_block* p_mem_old, memory_block* p_mem_new, uint16 old_m_idx, uint16 new_m_idx)
		{
			using namespace std::views;
			assert(a_old != a_new);
			assert(p_mem_old is_not_nullptr);
			assert(p_mem_new is_not_nullptr);

			auto archetype_to_copy = a_old & a_new;
			for (auto nth_component : iota(0, std::bit_width(archetype_to_copy)) | filter([archetype_to_copy](auto nth_c) { return (archetype_to_copy >> nth_c) & 1; }))
			{
				auto c_idx_old = _calc_component_idx(a_old, nth_component);
				auto c_idx_new = _calc_component_idx(a_new, nth_component);

				memcpy(
					p_mem_new->get_component_ptr(new_m_idx, c_idx_new),
					p_mem_old->get_component_ptr(old_m_idx, c_idx_old),
					(size_t)(p_mem_old->get_component_size(c_idx_old)));

				assert((a_old >> nth_component) & 1);
				assert((a_new >> nth_component) & 1);
				assert(p_mem_new->get_component_size(c_idx_new) == p_mem_old->get_component_size(c_idx_old));
				assert(p_mem_new->get_component_size(c_idx_new) == component_wrapper<c...>::sizes[nth_component]);
				assert(p_mem_old->get_component_size(c_idx_old) == component_wrapper<c...>::sizes[nth_component]);
			}
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
			static constinit const auto archetype = _calc_archetype<t...>();

			auto& block_list	= memory_block_vec_map[archetype];
			auto* p_block		= (memory_block*)nullptr;
			auto  mem_block_idx = 0;
			auto* p_entity		= (entity*)nullptr;

			auto res = std::ranges::find_if(block_list, [&](auto& block) { return block.is_full() is_false; });

			if (res != block_list.end())
			{
				p_block		  = &(*res);
				mem_block_idx = res - block_list.begin();
			}
			else
			{
				p_block		  = &block_list.emplace_back();
				mem_block_idx = 0;

				_init_mem_block(archetype, (sizeof(t) + ... + sizeof(entity_idx)), p_block);
			}

			if (entity_hole_count == 0)
			{
				p_entity	  = &entities.emplace_back();
				p_entity->idx = entities.size() - 1;
			}
			else
			{
				p_entity = &entities[entity_hole_begin_idx];

				auto next_hole_idx	  = p_entity->idx;
				p_entity->idx		  = entity_hole_begin_idx;
				entity_hole_begin_idx = next_hole_idx;
				--entity_hole_count;
			}

			p_entity->archetype		= archetype;
			p_entity->mem_block_idx = mem_block_idx;

			p_block->new_entity<t...>(*p_entity);
			return p_entity->idx;
		}

		void delete_entity(entity_idx idx)
		{
			auto& e										= entities[idx];
			auto  entity_idx_need_update				= get_p_mem_block(e)->remove_entity(e.memory_idx);
			entities[entity_idx_need_update].memory_idx = e.memory_idx;
			entities[idx].idx							= entity_hole_begin_idx;
			entity_hole_begin_idx						= idx;
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
			auto& e					= entities[idx];
			auto  new_archetype		= e.archetype | _calc_archetype<t...>();
			auto& block_list		= memory_block_vec_map[new_archetype];
			auto* p_new_block		= (memory_block*)nullptr;
			auto  new_mem_block_idx = 0;
			auto* p_prev_block		= get_p_mem_block(e);

			auto res = std::ranges::find_if(block_list, [](auto& block) {
				return block.is_full() == false;
			});

			if (res != block_list.end())
			{
				new_mem_block_idx = res - block_list.begin();
				p_new_block		  = &block_list[new_mem_block_idx];
			}
			else
			{
				new_mem_block_idx = 0;
				p_new_block		  = &block_list.emplace_back();

				auto new_size = p_prev_block->calc_size_per_archetype() + (sizeof(t) + ...);
				_init_mem_block(new_archetype, new_size, p_new_block);
			}

			const auto new_m_idx = p_new_block->get_count();

			_copy_components(e.archetype, new_archetype, p_prev_block, p_new_block, e.memory_idx, new_m_idx);
			([=]() {
				new (p_new_block->get_component_ptr(new_m_idx, _calc_component_idx<t>(new_archetype))) t();
			}(),
			 ...);
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
			auto& e					= entities[idx];
			auto  new_archetype		= e.archetype ^ _calc_archetype<t...>();
			auto& block_list		= memory_block_vec_map[new_archetype];
			auto* p_prev_block		= get_p_mem_block(e);
			auto* p_new_block		= (memory_block*)nullptr;
			auto  new_mem_block_idx = 0u;

			auto res = std::ranges::find_if(block_list, [](auto& mem_block) { return mem_block.is_full() is_false; });

			if (res != block_list.end())
			{
				new_mem_block_idx = res - block_list.begin();
				p_new_block		  = &block_list[new_mem_block_idx];
			}
			else
			{
				new_mem_block_idx = 0;
				p_new_block		  = &block_list.emplace_back();

				auto new_size = p_prev_block->calc_size_per_archetype() - (sizeof(t) + ...);
				_init_mem_block(new_archetype, new_size, p_new_block);
			}

			const auto new_m_idx = p_new_block->get_count();

			_copy_components(e.archetype, new_archetype, p_prev_block, p_new_block, e.memory_idx, new_m_idx);

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
			using namespace std::views;

			DEBUG_LOG("sys perform begin");
			static constinit const auto archetype = _calc_sys_archetype<system_t>();
			auto						s		  = archetype;
			if constexpr (has_on_system_begin<system_t>)
			{
				sys.on_system_begin();
			}
			else if constexpr (has_on_system_begin_w<system_t, world_t>)
			{
				sys.on_system_begin(*this);
			}

			auto mem_blocks_view = memory_block_vec_map
								 | filter([](auto& pair) { return (pair.first & archetype) == archetype; })
								 | transform([](auto& pair) -> auto& { return pair.second; })
								 | join;

			std::for_each(
				std::execution::par,
				mem_blocks_view.begin(),
				mem_blocks_view.end(),
				[&, this](memory_block& mem_block) {
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
	};

	template <>
	struct scene<> : scene_base
	{
	};

	struct subworld
	{
	};

	// bool is_idx_valid(uint64);
	// bool is_idx_valid(uint32);
	// bool is_idx_valid(uint16);

	// bool is_id_valid(uint64);

	// bool is_id_valid(uint32);
	// bool is_id_valid(uint16);
	// EDITOR_API scene_base get_scene(size_t index);
}	 // namespace ecs
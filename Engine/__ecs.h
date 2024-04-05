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
				// arr[tuple_index_v<c, tuple_sort<component_comparator, c...>::type>] = sizeof(c);
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

	template <typename system>
	concept has_on_system_begin = requires(system s) { s.on_system_begin; };

	template <typename system>
	concept has_on_system_begin_w = requires(system s) { s.on_system_begin_w; };

	template <typename system>
	concept has_on_thread_init = requires(system s) { s.on_thread_init; };

	template <typename system>
	concept has_on_thread_init_w = requires(system s) { s.on_thread_init_w; };

	template <typename system>
	concept has_update = requires(system s) { s.update; };

	template <typename system>
	concept has_update_w = requires(system s) { s.update_init_w; };

	template <typename system>
	concept has_on_thread_dispose = requires(system s) { s.on_thread_dispose; };


	template <typename system>
	concept has_on_thread_dispose_w = requires(system s) { s.on_thread_dispose_w; };

	template <typename system>
	concept has_on_system_end = requires(system s) { s.on_system_end; };


	template <typename system>
	concept has_on_system_end_w = requires(system s) { s.on_system_end_w; };

	template <typename... ts>
	requires meta::variadic_unique<ts...>
	struct __seq
	{
		constinit static inline const auto _seq = true;
		using tpl_t								= std::tuple<ts...>;
	};

	template <typename... ts>
	requires meta::variadic_unique<ts...>
	struct __par
	{
		constinit static inline const auto _par = true;
		using tpl_t								= std::tuple<ts...>;
	};

	// template <typename... ts>
	// requires meta::variadic_unique<ts...>
	// struct __wt
	//{
	//	constinit static inline const auto _wt = true;
	//	using tpl_t							   = std::tuple<ts...>;
	// };

	template <typename... ts>
	requires meta::variadic_unique<ts...>
	struct __cond
	{
		constinit static inline const auto _cond = true;
		using tpl_t								 = std::tuple<ts...>;
	};

	template <auto... fn>
	auto seq()
	{
		return __seq<meta::auto_wrapper<fn>...>();
	}

	template <typename... pipeline>
	auto seq()
	{
		return __seq<meta::type_wrapper<pipeline>...>();
	}

	template <auto... fn>
	auto par()
	{
		return __par<meta::auto_wrapper<fn>...>();
	}

	template <typename... pipeline>
	auto par()
	{
		return __par<meta::type_wrapper<pipeline>...>();
	}

	// template <auto... fn>
	// auto wt()
	//{
	//	return __wt<meta::auto_wrapper<fn>...>();
	// }

	// template <typename... pipeline>
	// auto wt()
	//{
	//	return __wt<meta::type_wrapper<pipeline>...>();
	// }

	template <auto cond_fn, auto fn1, auto fn2>
	auto cond()
	{
		return __cond<meta::auto_wrapper<cond_fn>, meta::auto_wrapper<fn1>, meta::auto_wrapper<fn2>>();
	}

	template <auto cond_fn, typename p1, typename p2>
	auto cond()
	{
		return __cond<meta::auto_wrapper<cond_fn>, meta::type_wrapper<p1>, meta::type_wrapper<p2>>();
	}

	// todo seq<func, pipe> is not possible, fix (maybe seq<func, pipe1()>)
	// this is because pipe is type and function is value
	// loop node?
	// rename to system
	template <auto... wrapper_fn>
	class pipeline
	{
	  public:
		constinit static inline const auto _pipeline = true;

	  private:
		template <template <typename...> typename node_t, template <auto> typename wrapper, auto... fn>
		void _update(auto& world, node_t<wrapper<fn>...> node)
		{
			if constexpr (ecs::is_seq<decltype(node)>)
			{
				DEBUG_LOG("---seq (func)---");
				(world.update(fn), ...);
			}
			else if constexpr (ecs::is_par<decltype(node)>)
			{
				DEBUG_LOG("---par (func)---");
				std::thread _threads[sizeof...(fn)];
				auto		idx = 0;
				([&, this] {
					_threads[idx++] = std::thread([&world, this]() { world.update(fn); });
				}(),
				 ...);

				for (auto& th : _threads)
				{
					th.join();
				}
			}
			else if constexpr (ecs::is_cond<decltype(node)>)
			{
				DEBUG_LOG("---cond (func)---");
				if (meta::variadic_auto_at_v<0, fn...>)
				{
					world.update(meta::variadic_auto_at_v<1, fn...>);
				}
				else
				{
					world.update(meta::variadic_auto_at_v<2, fn...>);
				}
			}
			else
			{
				assert(false and "invalid node type");
			}
		}

		template <template <typename...> typename node_t, template <typename> typename wrapper, typename... pipelines>
		void _update(auto& world, node_t<wrapper<pipelines>...> node)
		{
			if constexpr (ecs::is_seq<decltype(node)>)
			{
				DEBUG_LOG("---seq (pipe)---");
				(pipelines().update(world), ...);
			}
			else if constexpr (ecs::is_par<decltype(node)>)
			{
				DEBUG_LOG("---par (pipe)---");
				std::thread _threads[sizeof...(pipelines)];
				auto		idx = 0;
				([&, this] {
					_threads[idx++] = std::thread([&world]() { pipelines().update(world); });
				}(),
				 ...);

				for (auto& th : _threads)
				{
					th.join();
				}
			}
			else
			{
				assert(false and "invalid node type");
			}
		}

		template <auto cond_fn, typename... pipelines>
		void _update(auto& world, __cond<auto_wrapper<cond_fn>, type_wrapper<pipelines>...> node)
		{
			if constexpr (ecs::is_cond<decltype(node)>)
			{
				DEBUG_LOG("---cond (func)---");
				// node_t =__cond<meta::auto_wrapper<cond_fn>, meta::type_wrapper<p1>, meta::type_wrapper<p2>>();
				if (cond_fn())
				{
					variadic_at_t<0, pipelines...>().update(world);
				}
				else
				{
					variadic_at_t<1, pipelines...>().update(world);
				}
			}
			else
			{
				assert(false and "invalid node type");
			}
		}

	  public:
		void update(auto& world)
		{
			(_update(world, wrapper_fn()), ...);
		}
	};

	template <typename fn, typename... ts>
	struct _binder
	{
		const std::pair<fn, std::tuple<ts...>> _pair;

		_binder(fn&& f, ts&&... args)
			: _pair(std::forward<fn>(f), std::tuple(std::forward<ts>(args)...)) { }

		void operator()()
		{
			meta::call_w_tpl_args(_pair.first, _pair.second);
		}
	};

	template <typename fn, typename... ts>
	auto bind(fn&& f, ts&&... args)
	{
		return _binder<fn, ts...>(std::forward<fn>(f), std::forward<ts>(args)...);
	}

	template <auto... fn>
	void n_seq()
	{
		([] {
			fn();
		}(),
		 ...);
	}

	template <auto... fn>
	void n_par()
	{
		std::thread threads[sizeof...(fn)];
		auto		idx = 0;
		([&]() {
			threads[idx++] = std::thread([] { fn(); });
		}(),
		 ...);

		for (auto& th : threads)
		{
			th.join();
		}
	}

	template <auto cond, auto f1, auto f2>
	void n_cond()
	{
		if (cond())
		{
			f1();
		}
		else
		{
			f2();
		}
	}

	template <auto... wrapper_fn>
	struct node
	{
		// using fn_tpl = std::tuple<auto_wrapper<wrapper_fn>...>;

		template <typename... ts>
		void operator()(ts&&... args) const
		{
			auto arg_tpl = std::tuple(std::forward<ts>(args)...);
			// auto fn_tpl	 = std::tuple(auto_wrapper<wrapper_fn>...);
			//  std::apply([](auto&&... args) { ((std::cout << args << '\n'), ...); }, ts);
			([&] {
				meta::call_w_tpl_args(wrapper_fn, arg_tpl);
			}(),
			 ...);
		}
	};

	struct memory_block
	{
		uint8 memory[MEMORY_BLOCK_SIZE];

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
			auto c_count = get_component_count();
			auto size	 = sizeof(entity_idx);
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

		template <typename... t>
		void update(void (*func)(entity_idx, t...))
		{
			auto archetype = _calc_archetype<std::remove_const_t<std::remove_reference_t<t>>...>();
			auto threads   = data_structure::vector<std::jthread>();

			std::ranges::for_each(
				memory_block_vec_map
					| std::views::filter([](auto& pair) { return pair.first & archetype != 0; }),
				/*| std::views::transform([=](auto& pair) { return pair.second; })*/
				[func, &threads](auto& pair) {
					auto th = std::jthread(
						[func, &threads, &pair]() {
							for (auto& mem_block : pair.second)
							{
								auto count = mem_block.get_count();
								for (auto m_idx = 0; m_idx < count; ++m_idx)
								{
									// func(
									//  mem_block.get_entity_idx(m_idx),
									//  *(std::remove_const_t<std::remove_reference_t<t>>*)(mem_block.get_component_ptr(m_idx, _calc_component_idx<std::remove_const_t<std::remove_reference_t<t>>>(archetype)))...);

									func(
										mem_block.get_entity_idx(m_idx),
										*(std::remove_reference_t<t>*)(mem_block.get_component_ptr(m_idx, _calc_component_idx<std::remove_const_t<std::remove_reference_t<t>>>(archetype)))...);
								}
							}
						});

					threads.emplace_back(th);
				});

			std::ranges::for_each(threads, [](auto& th) { th.join(); });
		}

		template <typename sys_group>
		void perform(sys_group& group)
		{
			if constexpr (has_on_system_begin<sys_group>)
			{
				group.on_system_begin();
			}
			else if constexpr (has_on_system_begin_w<sys_group>)
			{
				group.on_system_begin_w(std::forward<decltype(world)>(world));
			}

			auto thread_init_func = [this, &group]() {
				if constexpr (has_on_thread_init<sys_group>)
				{
					group.on_thread_init();
				}
				else if constexpr (has_on_thread_init_w<sys_group>)
				{
					group.on_thread_init_w(std::forward<decltype(world)>(world));
				}

				if constexpr (has_on_thread_dispose<sys_group>)
				{
					group.on_thread_dispose();
				}
				else if constexpr (has_on_thread_dispose_w<sys_group>)
				{
					group.on_thread_dispose_w(std::forward<decltype(world)>(world));
				}
			};

			if constexpr (has_update<sys_group>)
			{
				// group.update();
			}
			else if constexpr (has_update_w<sys_group>)
			{
				// group.update_w(std::forward<decltype(world)>(world));
			}


			if constexpr (has_on_system_end<sys_group>)
			{
				group.on_system_end();
			}
			else if constexpr (has_on_system_end_w<sys_group>)
			{
				group.on_system_end_w(std::forward<decltype(world)>(world));
			}
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
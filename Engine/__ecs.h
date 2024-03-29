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
		entity_idx	  idx;
		archetype_t	  archetype;
		memory_block* p_mem_block;
		uint64		  memory_idx;
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
	concept is_wt = requires { t::_wt; };

	template <typename t>
	concept is_cond = requires { t::_cond; };

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

	template <typename... ts>
	requires meta::variadic_unique<ts...>
	struct __wt
	{
		constinit static inline const auto _wt = true;
		using tpl_t							   = std::tuple<ts...>;
	};

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

	template <auto... fn>
	auto wt()
	{
		return __wt<meta::auto_wrapper<fn>...>();
	}

	template <typename... pipeline>
	auto wt()
	{
		return __wt<meta::type_wrapper<pipeline>...>();
	}

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
	template <auto... wrapper_fn>
	class pipeline
	{
	  public:
		constinit static inline const auto _pipeline = true;

	  private:
		using tpl_par_t = meta::tuple_cat_t<std::conditional_t<ecs::is_par<decltype(wrapper_fn())>, typename decltype(wrapper_fn())::tpl_t, std::tuple<>>...>;

		std::thread _threads[std::tuple_size_v<tpl_par_t>];

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
				([&world, this] {
					_threads[meta::tuple_index_v<meta::auto_wrapper<fn>, tpl_par_t>] = std::thread([&world, this]() { world.update(fn); });
				}(),
				 ...);
			}
			else if constexpr (ecs::is_wt<decltype(node)>)
			{
				DEBUG_LOG("---wt (func)---");
				([this]() {
					_threads[meta::tuple_index_v<meta::auto_wrapper<fn>, tpl_par_t>].join();
				}(),
				 ...);
			}
			else if constexpr (ecs::is_cond<decltype(node)>)
			{
				DEBUG_LOG("---cond (func)---");
				// node_t =__cond<meta::auto_wrapper<cond_fn>, meta::auto_wrapper<p1>, meta::auto_wrapper<p2>>();
				if (meta::variadic_at_t<0, wrapper<fn>...>::value())
				{
					int a = 1;
					// world.update(std::get<1>(typename decltype(node)::tpl_t()));
				}
				else
				{
					int b = 2;
					// world.update(std::get<2>(typename decltype(node)::tpl_t()));
					//  world.update(meta::variadic_at_t<2, typename decltype(node)::tpl_t>::value);
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
				([&world, this] {
					_threads[meta::tuple_index_v<meta::type_wrapper<pipelines>, tpl_par_t>] = std::thread([&world]() { pipelines().update(world); });
				}(),
				 ...);
			}
			else if constexpr (ecs::is_wt<decltype(node)>)
			{
				DEBUG_LOG("---wt (pipe)---");
				([this]() {
					_threads[meta::tuple_index_v<meta::type_wrapper<pipelines>, tpl_par_t>].join();
				}(),
				 ...);
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
			e.memory_idx  = m_idx;
			e.p_mem_block = this;
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
		data_structure::vector<entity>													   entities;
		data_structure::vector<std::pair<archetype_t, data_structure::list<memory_block>>> memory_block_list_vec;

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

		template <typename... t>
		entity_idx new_entity()
		{
			static constinit const auto size	  = sizeof(entity_idx) + (sizeof(t) + ...);
			static constinit const auto capacity  = MEMORY_BLOCK_SIZE / size;
			static constinit const auto archetype = _calc_archetype<t...>();

			data_structure::list<memory_block>* p_block_list = nullptr;
			memory_block*						p_block		 = nullptr;

			for (auto i = 0; i < memory_block_list_vec.size(); ++i)
			{
				if (archetype == memory_block_list_vec[i].first)
				{
					p_block_list = &(memory_block_list_vec[i].second);
					break;
				}
			}

			if (p_block_list == nullptr)
			{
				auto& pair	 = memory_block_list_vec.emplace_back(archetype, data_structure::list<memory_block>());
				p_block_list = &pair.second;
			}


			for (auto p_node = p_block_list->front(); p_node != nullptr; p_node = p_node->next)
			{
				if (p_node->value.is_full() == false)
				{
					p_block = &(p_node->value);
					break;
				}
			}

			if (p_block == nullptr)
			{
				auto& block = p_block_list->emplace_back();
				memory_block::init<t...>(block);
				p_block = &block;
			}

			if (entity_hole_count == 0)
			{
				auto& e		= entities.emplace_back();
				e.archetype = archetype;
				e.idx		= entities.size() - 1;
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
				e.archetype = archetype;
				p_block->new_entity<t...>(e);

				return e.idx;
			}
		}

		void delete_entity(entity_idx idx)
		{
			auto& e = entities[idx];
			e.p_mem_block->remove_entity(e.memory_idx);
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
			return *(t*)(e.p_mem_block->get_component_ptr(e.memory_idx, _calc_component_idx<t>(e.archetype)));
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

			data_structure::list<memory_block>* p_block_list = nullptr;
			memory_block*						p_new_block	 = nullptr;

			for (auto i = 0; i < memory_block_list_vec.size(); ++i)
			{
				if (new_archetype == memory_block_list_vec[i].first)
				{
					p_block_list = &(memory_block_list_vec[i].second);
					break;
				}
			}

			if (p_block_list == nullptr)
			{
				auto& pair	 = memory_block_list_vec.emplace_back(new_archetype, data_structure::list<memory_block>());
				p_block_list = &pair.second;
			}


			for (auto p_node = p_block_list->front(); p_node != nullptr; p_node = p_node->next)
			{
				if (p_node->value.is_full() == false)
				{
					p_new_block = &(p_node->value);
					break;
				}
			}

			if (p_new_block == nullptr)
			{
				auto& block = p_block_list->emplace_back();
				p_new_block = &block;

				p_new_block->write_count(0);

				auto	   memory			  = p_new_block->memory;
				const auto size_per_archetype = e.p_mem_block->calc_size_per_archetype() + (sizeof(t) + ... + 0);
				const auto component_count	  = e.p_mem_block->get_component_count() + sizeof...(t);
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
						p_new_block->write_component_data(c_idx, offset, e.p_mem_block->get_component_size(c_idx - i));
						offset += capacity + e.p_mem_block->get_component_size(c_idx);
					}

					assert(c_idx == new_c_idx);
					p_new_block->write_component_data(c_idx, offset, new_c_size);
					offset += capacity * new_c_size;
					++c_idx;
				}


				for (; c_idx < component_count; ++c_idx)
				{
					p_new_block->write_component_data(c_idx, offset, e.p_mem_block->get_component_size(c_idx - i));
					offset += capacity + e.p_mem_block->get_component_size(c_idx);
				}
			}

			const auto new_component_count = __popcnt(new_archetype);
			const auto new_m_idx		   = p_new_block->get_count();
			assert(new_component_count == e.p_mem_block->get_component_count() + sizeof...(t));
			assert(p_new_block->is_full() == false);

			auto i	   = 0;
			auto c_idx = 0;
			([&]() {
				auto new_c_idx = c_idx_arr[i];
				for (; c_idx < new_c_idx; ++c_idx)
				{
					assert(p_new_block->get_component_size(c_idx) == e.p_mem_block->get_component_size(c_idx - i));
					memcpy(
						p_new_block->get_component_ptr(new_m_idx, c_idx),
						e.p_mem_block->get_component_ptr(e.memory_idx, c_idx - i),
						(size_t)(e.p_mem_block->get_component_size(c_idx - i)));
				}

				assert(c_idx == new_c_idx);
				new (p_new_block->get_component_ptr(new_m_idx, _calc_component_idx<t>(new_archetype))) t();
				++c_idx;
				++i;
			}(),
			 ...);

			for (; c_idx < new_component_count; ++c_idx)
			{
				assert(p_new_block->get_component_size(c_idx) == e.p_mem_block->get_component_size(c_idx - i));
				memcpy(
					p_new_block->get_component_ptr(new_m_idx, c_idx),
					e.p_mem_block->get_component_ptr(e.memory_idx, c_idx - i),
					(size_t)(e.p_mem_block->get_component_size(c_idx - i)));
			}

			p_new_block->write_count(p_new_block->get_count() + 1);
			p_new_block->write_entity_idx(new_m_idx, e.idx);

			e.p_mem_block->remove_entity(e.memory_idx);

			e.p_mem_block = p_new_block;
			e.memory_idx  = new_m_idx;
			e.archetype	  = new_archetype;
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
				e.p_mem_block->remove_entity(e.memory_idx);
				e.p_mem_block = nullptr;
				e.archetype	  = 0;
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

			data_structure::list<memory_block>* p_block_list = nullptr;
			memory_block*						p_new_block	 = nullptr;

			for (auto i = 0; i < memory_block_list_vec.size(); ++i)
			{
				if (new_archetype == memory_block_list_vec[i].first)
				{
					p_block_list = &(memory_block_list_vec[i].second);
					break;
				}
			}

			if (p_block_list == nullptr)
			{
				auto& pair	 = memory_block_list_vec.emplace_back(new_archetype, data_structure::list<memory_block>());
				p_block_list = &pair.second;
			}


			for (auto p_node = p_block_list->front(); p_node != nullptr; p_node = p_node->next)
			{
				if (p_node->value.is_full() == false)
				{
					p_new_block = &(p_node->value);
					break;
				}
			}

			if (p_new_block == nullptr)
			{
				auto& block = p_block_list->emplace_back();
				p_new_block = &block;

				p_new_block->write_count(0);

				auto	   memory			  = p_new_block->memory;
				const auto size_per_archetype = e.p_mem_block->calc_size_per_archetype() - (sizeof(t) + ... + 0);
				const auto component_count	  = e.p_mem_block->get_component_count() - sizeof...(t);
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
						p_new_block->write_component_data(c_idx, offset, e.p_mem_block->get_component_size(c_idx + i));
						offset += capacity + e.p_mem_block->get_component_size(c_idx);
					}

					assert(c_idx == new_c_idx);
					//++c_idx;
				}

				for (; c_idx < component_count; ++c_idx)
				{
					p_new_block->write_component_data(c_idx, offset, e.p_mem_block->get_component_size(c_idx + i));
					offset += capacity + e.p_mem_block->get_component_size(c_idx);
				}
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
					assert(p_new_block->get_component_size(c_idx) == e.p_mem_block->get_component_size(c_idx + i));
					memcpy(
						p_new_block->get_component_ptr(new_m_idx, c_idx),
						e.p_mem_block->get_component_ptr(e.memory_idx, c_idx + i),
						(size_t)(e.p_mem_block->get_component_size(c_idx + i)));
				}

				assert(c_idx == new_c_idx);
			}

			for (; c_idx < new_component_count; ++c_idx)
			{
				assert(p_new_block->get_component_size(c_idx) == e.p_mem_block->get_component_size(c_idx + i));
				memcpy(
					p_new_block->get_component_ptr(new_m_idx, c_idx),
					e.p_mem_block->get_component_ptr(e.memory_idx, c_idx + i),
					(size_t)(e.p_mem_block->get_component_size(c_idx + i)));
			}


			p_new_block->write_count(p_new_block->get_count() + 1);
			p_new_block->write_entity_idx(new_m_idx, e.idx);

			e.p_mem_block->remove_entity(e.memory_idx);

			e.p_mem_block = p_new_block;
			e.memory_idx  = new_m_idx;
			e.archetype	  = new_archetype;
		}

		archetype_t get_archetype(entity_idx idx) const
		{
			return entities[idx].archetype;
		}

		template <typename... t>
		void update(void (*func)(entity_idx, t...))
		{
			auto archetype = _calc_archetype<std::remove_const_t<std::remove_reference_t<t>>...>();
			for (auto i = 0; i < memory_block_list_vec.size(); ++i)
			{
				auto  block_archetype = memory_block_list_vec[i].first;
				auto& block_list	  = memory_block_list_vec[i].second;
				if ((archetype & block_archetype) != archetype) continue;

				assert(block_list.empty() == false);
				for (auto p_node = block_list.front(); p_node != nullptr; p_node = p_node->next)
				{
					auto& block = p_node->value;
					auto  count = block.get_count();
					for (auto m_idx = 0; m_idx < count; ++m_idx)
					{
						func(
							block.get_entity_idx(m_idx),																																					   // entity_idx
							*(std::remove_const_t<std::remove_reference_t<t>>*)(block.get_component_ptr(m_idx, _calc_component_idx<std::remove_const_t<std::remove_reference_t<t>>>(block_archetype)))...);	   // read_component<std::remove_const_t<std::remove_reference_t<t>>>(block, k, meta::tuple_index_v<std::remove_const_t<std::remove_reference_t<t>>, component_tpl>))...);
					}
				}
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
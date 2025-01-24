#pragma once

#define MEMORY_BLOCK_SIZE	 (sizeof(uint8) * 1024 * 16)
#define INVALID_MEMBLOCK_IDX -1

namespace ecs
{
	using scene_id	   = uint16;
	using world_idx	   = uint16;
	using entity_idx   = uint64;
	using component_id = uint64;
	using archetype_t  = uint64;

	template <typename c1, typename c2>
	struct component_comparator : std::integral_constant<bool, (c1::id >= c2::id)>
	{
	};

	template <typename... c>
	struct component_wrapper
	{
		using component_tpl = meta::tuple_sort<component_comparator, c...>::type;

		static inline constinit auto sizes = [] {
			auto arr = std::array<size_t, sizeof...(c)>();
			auto i	 = 0;
			([&arr, &i] {
				arr[meta::tuple_index<c, component_tpl>::value] = sizeof(c);
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
				archetype |= 1ui64 << meta::tuple_index<t, component_tpl>::value;
			}(),
			 ...);
			return archetype;
		};

		static inline size_t calc_total_size(archetype_t archetype)
		{
			auto v = std::views::iota(0, std::bit_width(archetype))
				   | std::views::filter([=](auto nth_c) { return (archetype >> nth_c) & 1; })
				   | std::views::transform([=](auto nth_c) { return sizes[nth_c]; });
			auto size_total = std::accumulate(v.begin(), v.end(), 0);
			return size_total;
		}
	};

	struct entity
	{
		entity_idx	idx;
		archetype_t archetype;
		// memory_block* p_mem_block;
		uint64 mem_block_idx;
		uint64 memory_idx;
	};

	struct memory_block
	{
		uint8* _memory;

		memory_block()
		{
			_memory = (uint8*)malloc(MEMORY_BLOCK_SIZE);
		}

		memory_block(void* memory) : _memory((uint8*)memory)
		{
		}

		memory_block(memory_block&& other) noexcept : _memory(other._memory)
		{
			other._memory = nullptr;
		}

		memory_block& operator=(memory_block&& other) noexcept
		{
			_memory		  = other._memory;
			other._memory = nullptr;
		}

		~memory_block()
		{
			free(_memory);
		}

		memory_block(const memory_block& other) = delete;

		memory_block& operator=(memory_block& other) = delete;

		// 16kb => n component, (0 < n <= 64)
		//  2 byte => size,
		//  2 byte => capatity
		//  2 byte => component_count
		//  4 * n byte => {component_size, offset}
		//
		//  sizeof(entity_idx) * capacity => entity_idx ... why?
		//  => system iterate base on mem_idx not entity_idx
		//	=> to get entity_idx inside update function
		//	memory_block must save entity_idx
		//  => maybe make every entity to have component {entity}
		//
		//	=> solves empty entity problem
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

		// for empty entity
		// for not doing if(archetype == 0) every time
		// 6byte memory_block will be added

		// unused_memory_size
		// MEMORY_BLOCK_SIZE - header_size - (sizeof(entity_id) + size_per_archetype) * capacity
		//

		uint16 get_count() const
		{
			return *(uint16*)_memory;
		}

		uint16 get_capacity() const
		{
			return *(uint16*)(_memory + 2);
		}

		uint16 get_component_count() const
		{
			return *(uint16*)(_memory + 4);
		}

		uint16 get_component_size(uint8 c_idx) const
		{
			assert(c_idx < get_component_count());
			return (uint16)((*(uint32*)(_memory + 6 + c_idx * 4)) >> 16);
		}

		uint16 get_component_offset(uint8 c_idx) const
		{
			assert(c_idx < get_component_count());
			return (uint16)((*(uint32*)(_memory + 6 + c_idx * 4)));
		}

		uint16 get_header_size() const
		{
			return 6 + get_component_count() * sizeof(uint32);
		}

		entity_idx get_entity_idx(uint16 m_idx) const
		{
			assert(m_idx < get_count());
			return *(entity_idx*)(_memory + get_header_size() + m_idx * sizeof(entity_idx));
		}

		void* get_component_ptr(uint16 m_idx, uint8 c_idx) const
		{
			assert(c_idx < get_component_count());
			return (void*)(_memory + get_component_offset(c_idx) + m_idx * get_component_size(c_idx));
		}

		// sizeof entity_idx + sizeof all component size
		uint16 calc_size_per_archetype() const
		{
			auto   c_count = get_component_count();
			uint16 size	   = sizeof(entity_idx);
			// todo maybe more optimized way to do this?
			for (auto c_idx = 0; c_idx < c_count; ++c_idx)
			{
				size += get_component_size(c_idx);
			}

			return size;
		}

		uint16 calc_unused_mem_size() const
		{
			assert(MEMORY_BLOCK_SIZE > get_header_size() + calc_size_per_archetype() * get_capacity());
			return MEMORY_BLOCK_SIZE - get_header_size() - calc_size_per_archetype() * get_capacity();
		}

		void write_count(uint16 new_count)
		{
			*(uint16*)_memory = new_count;
		}

		void write_capacity(uint16 capacity)
		{
			*(uint16*)(_memory + 2) = capacity;
		}

		void write_component_count(uint16 count)
		{
			*(uint16*)(_memory + 4) = count;
		}

		void write_component_data(uint8 c_idx, uint16 offset, uint16 size)
		{
			auto component_info_ptr			   = _memory + 6 + sizeof(uint32) * c_idx;
			*(uint16*)component_info_ptr	   = offset;
			*(uint16*)(component_info_ptr + 2) = size;
		}

		void write_component_data(uint8 c_idx, uint32 c_data)
		{
			*(uint32*)(_memory + 6 + sizeof(uint32) * c_idx) = c_data;
		}

		void write_entity_idx(uint8 m_idx, entity_idx idx)
		{
			*(entity_idx*)(_memory + get_header_size() + m_idx * sizeof(entity_idx)) = idx;
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
		}

		template <>
		void new_entity(entity& e)
		{
			assert(0 == get_component_count());

			auto m_idx = get_count();

			write_entity_idx(m_idx, e.idx);

			write_count(m_idx + 1);
			e.memory_idx = m_idx;
		}
	};
}	 // namespace ecs
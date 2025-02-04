#include "pch.h"
#include "editor.h"
#include "game.h"

#include "../Engine/__reflection.h"
#include "../Engine/__ecs_common.h"

namespace
{
	using struct_info	 = reflection::struct_info;
	using scene_info	 = reflection::scene_info;
	using world_info	 = reflection::world_info;
	using component_info = reflection::component_info;
	using entity_info	 = reflection::entity_info;

	struct ecs_scene;
}	 // namespace

// static data
namespace
{
	struct ecs_field_info
	{
		e_primitive_type type;
		uint32			 mem_offset;
	};

	struct ecs_struct_info
	{
		size_t						size;
		std::vector<ecs_field_info> field_info_vec;
		void*						p_default_value;
	};

	ecs_scene*					 scenes;
	uint32						 scene_count;
	editor::game::ecs::scene_idx scene_hole_begin_idx = -1;
	editor::game::ecs::scene_idx scene_hole_count	  = 0;
	std::vector<ecs_struct_info> struct_info_vec;
}	 // namespace

namespace
{
	using ecs_entity = ecs::entity;

	struct ecs_world;

	struct ecs_scene
	{
		// todo data structure
		ecs_world*	   worlds		  = nullptr;
		uint32		   world_count	  = 0ul;
		ecs::world_idx world_capacity = 0ul;

		ecs::world_idx world_hole_begin_idx = -1;
		ecs::world_idx world_hole_count		= 0;

		editor::game::ecs::scene_idx idx;

		ecs_scene(ecs_scene&&)			  = delete;
		ecs_scene(const ecs_scene&)		  = delete;
		ecs_scene& operator=(ecs_scene&&) = delete;
		ecs_scene& operator=(ecs_scene&)  = delete;

		ecs_scene() = default;
	};

	struct ecs_world
	{
		std::vector<ecs_entity>												  entities;
		std::map<ecs::archetype_t, data_structure::vector<ecs::memory_block>> memory_block_vec_map;

		// editor only
		std::map<ecs::archetype_t, size_t> archetype_size_map;
		std::vector<uint64>				   ecs_struct_idx_vec;

		ecs::world_idx idx;

		size_t entity_hole_begin_idx = -1;
		size_t entity_hole_count	 = 0;

		ecs_world(ecs_world&&)			  = delete;
		ecs_world(const ecs_world&)		  = delete;
		ecs_world& operator=(ecs_world&&) = delete;
		ecs_world& operator=(ecs_world&)  = delete;

		ecs_world() = default;

		size_t _calc_archetype_size(ecs::archetype_t archetype)
		{
			if (archetype_size_map.contains(archetype))
			{
				return archetype_size_map.find(archetype)->second;
			}

			archetype_size_map[archetype] = (size_t)std::ranges::fold_left(
				std::views::iota(0, std::bit_width(archetype))
					| std::views::filter([archetype](auto nth_bit) { return (archetype >> nth_bit) & 1; })
					| std::views::transform([this](auto nth_bit) { return struct_info_vec[ecs_struct_idx_vec[nth_bit]].size; }),
				0ull, std::plus {});
			return archetype_size_map[archetype];
		}

		uint32 _calc_nth_component(uint64 ecs_struct_idx)
		{
			return std::distance(ecs_struct_idx_vec.begin(), std::ranges::find(ecs_struct_idx_vec, ecs_struct_idx));
		}

		uint64 _calc_archetype(uint64 ecs_struct_idx)
		{
			return 1ull << _calc_nth_component(ecs_struct_idx);
		}

		static inline uint8 _calc_component_idx(ecs::archetype_t a, uint32 nth_component)
		{
			assert(nth_component < 64);
			assert(nth_component >= 0);
			return __popcnt64(((1 << nth_component) - 1) & a);
		}

		static void _copy_components(ecs::archetype_t a_old, ecs::archetype_t a_new, ecs::memory_block* p_mem_old, ecs::memory_block* p_mem_new, uint16 old_m_idx, uint16 new_m_idx)
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
				// assert(p_mem_new->get_component_size(c_idx_new) == component_wrapper<c...>::sizes[nth_component]);
				// assert(p_mem_old->get_component_size(c_idx_old) == component_wrapper<c...>::sizes[nth_component]);
			}
		}

		size_t entity_count() const
		{
			return entities.size() - entity_hole_count;
		}

		ecs::memory_block* get_p_mem_block(ecs::entity& e)
		{
			assert(memory_block_vec_map.contains(e.archetype));
			return &memory_block_vec_map[e.archetype][e.mem_block_idx];
		}

		void init_mem_block(ecs::archetype_t archetype, size_t size_per_archetype, ecs::memory_block* p_block)
		{
			using namespace std::views;

			assert(p_block is_not_nullptr);

			const auto component_count = __popcnt64(archetype);
			const auto capacity		   = (MEMORY_BLOCK_SIZE - (6 + sizeof(uint32) * component_count)) / size_per_archetype;

			p_block->write_component_count(component_count);
			p_block->write_capacity(capacity);
			p_block->write_count(0);


			auto offset = p_block->get_header_size() + sizeof(ecs::entity_idx) * capacity;	  // header + entity_idx
			auto c_idx	= 0;

			for (const auto nth_bit : std::views::iota(0, std::bit_width(archetype)) | std::views::filter([archetype](auto nth_bit) { return (archetype >> nth_bit) & 1; }))
			{
				auto c_size = struct_info_vec[ecs_struct_idx_vec[nth_bit]].size;
				p_block->write_component_data(c_idx, offset, c_size);
				offset += c_size * capacity;
				++c_idx;
			}

			assert(p_block->get_header_size() == 6 + sizeof(uint32) * component_count);
			assert(offset + p_block->calc_unused_mem_size() == MEMORY_BLOCK_SIZE);
		}

		ecs::entity* new_entity(ecs::archetype_t archetype)
		{
			auto& block_list	= memory_block_vec_map[archetype];
			auto* p_block		= (ecs::memory_block*)nullptr;
			auto  mem_block_idx = 0;
			auto* p_entity		= (ecs::entity*)nullptr;

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

				// for (const auto nth_bit : std::views::iota(0, std::bit_width(archetype)) | std::views::filter([archetype](auto nth_bit) { return (archetype >> nth_bit) & 1; }))
				//{
				//	auto p_c_info = editor::models::reflection::find_struct(component_info_vec[nth_bit]);
				// }
				init_mem_block(archetype, (_calc_archetype_size(archetype) + sizeof(ecs::entity_idx)), p_block);
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

			// p_memblock->new_entity<c...>
			{
				assert(__popcnt(p_entity->archetype) == p_block->get_component_count());

				auto m_idx = p_block->get_count();

				p_block->write_entity_idx(m_idx, p_entity->idx);

				for (const auto nth_bit : std::views::iota(0, std::bit_width(archetype)) | std::views::filter([archetype](auto nth_bit) { return (archetype >> nth_bit) & 1; }))
				{
					auto* p_s_info = &struct_info_vec[ecs_struct_idx_vec[nth_bit]];
					memcpy(p_block->get_component_ptr(m_idx, nth_bit), p_s_info->p_default_value, p_s_info->size);
				}

				p_block->write_count(m_idx + 1);
				p_entity->memory_idx = m_idx;
			}

			return p_entity;
		};

		void delete_entity(ecs::entity_idx ecs_entity_idx)
		{
			auto& e										= entities[ecs_entity_idx];
			auto  entity_idx_need_update				= get_p_mem_block(e)->remove_entity(e.memory_idx);
			entities[entity_idx_need_update].memory_idx = e.memory_idx;
			entities[ecs_entity_idx].idx				= entity_hole_begin_idx;
			entity_hole_begin_idx						= ecs_entity_idx;
			++entity_hole_count;
		}

		bool has_component(ecs::entity_idx e_idx, ecs::archetype_t archetype)
		{
			return (entities[e_idx].archetype & archetype) == archetype;
		}

		void add_component(ecs::entity_idx ecs_entity_idx, uint64 ecs_struct_idx)
		{
			auto archetype_to_add = _calc_archetype(ecs_struct_idx);
			assert(has_component(ecs_entity_idx, archetype_to_add) == false);

			auto& e					= entities[ecs_entity_idx];
			auto  new_archetype		= e.archetype | archetype_to_add;
			auto& block_list		= memory_block_vec_map[new_archetype];
			auto* p_new_block		= (ecs::memory_block*)nullptr;
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

				auto new_size = p_prev_block->calc_size_per_archetype() + _calc_archetype_size(archetype_to_add);
				assert(new_size == sizeof(ecs::entity_idx) + _calc_archetype_size(e.archetype | archetype_to_add));

				init_mem_block(new_archetype, new_size, p_new_block);
			}

			const auto new_m_idx = p_new_block->get_count();

			_copy_components(e.archetype, new_archetype, p_prev_block, p_new_block, e.memory_idx, new_m_idx);


			for (const auto nth_bit : std::views::iota(0, std::bit_width(new_archetype)) | std::views::filter([archetype_to_add](auto nth_bit) { return (archetype_to_add >> nth_bit) & 1; }))
			{
				auto* p_s_info = &struct_info_vec[ecs_struct_idx_vec[nth_bit]];
				memcpy(p_new_block->get_component_ptr(new_m_idx, nth_bit), p_s_info->p_default_value, p_s_info->size);
			}

			p_new_block->write_count(p_new_block->get_count() + 1);
			p_new_block->write_entity_idx(new_m_idx, e.idx);

			get_p_mem_block(e)->remove_entity(e.memory_idx);

			e.mem_block_idx = new_mem_block_idx;
			e.memory_idx	= new_m_idx;
			e.archetype		= new_archetype;
		}

		void remove_component(ecs::entity_idx ecs_entity_idx, uint64 ecs_struct_idx)
		{
			auto archetype_to_remove = _calc_archetype(ecs_struct_idx);
			assert(has_component(ecs_entity_idx, archetype_to_remove));
			auto& e					= entities[ecs_entity_idx];
			auto  new_archetype		= e.archetype ^ archetype_to_remove;
			auto& block_list		= memory_block_vec_map[new_archetype];
			auto* p_prev_block		= get_p_mem_block(e);
			auto* p_new_block		= (ecs::memory_block*)nullptr;
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

				auto new_size = p_prev_block->calc_size_per_archetype() - _calc_archetype_size(archetype_to_remove);
				init_mem_block(new_archetype, new_size, p_new_block);
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

		void* get_component(ecs::entity_idx idx, uint64 ecs_struct_idx)
		{
			assert(has_component(idx, _calc_archetype(ecs_struct_idx)));
			auto& e = entities[idx];
			return memory_block_vec_map[e.archetype][e.mem_block_idx].get_component_ptr(e.memory_idx, _calc_component_idx(e.archetype, _calc_nth_component(ecs_struct_idx)));
		}
	};
}	 // namespace

// dll related
namespace editor::game::ecs
{
	using namespace editor::models;
	using namespace std::views;

	size_t (*_get_registered_struct_count)();
	size_t (*_get_registered_scene_count)();
	size_t (*_get_registered_world_count)();
	size_t (*_get_registered_entity_count)(size_t world_index);
	struct_info* (*_get_struct_info)(size_t index);
	scene_info* (*_get_scene_info)(size_t index);
	world_info* (*_get_world_info)(size_t index);
	component_info (*_get_component_info)(size_t world_idx, size_t entity_idx, size_t component_idx);
	entity_info* (*_get_entity_info)(size_t world_index, size_t entity_index);

	bool init_from_dll(HMODULE proj_dll)
	{
		_get_registered_struct_count = LOAD_FUNC(size_t (*)(), "get_registered_struct_count", proj_dll);
		_get_registered_scene_count	 = LOAD_FUNC(size_t (*)(), "get_registered_scene_count", proj_dll);
		_get_registered_world_count	 = LOAD_FUNC(size_t (*)(), "get_registered_world_count", proj_dll);
		_get_registered_entity_count = LOAD_FUNC(size_t (*)(size_t), "get_registered_entity_count", proj_dll);

		_get_struct_info	= LOAD_FUNC(struct_info * (*)(size_t), "get_struct_info", proj_dll);
		_get_scene_info		= LOAD_FUNC(scene_info * (*)(size_t), "get_scene_info", proj_dll);
		_get_world_info		= LOAD_FUNC(world_info * (*)(size_t), "get_world_info", proj_dll);
		_get_component_info = LOAD_FUNC(component_info(*)(size_t, size_t, size_t), "get_component_info", proj_dll);
		_get_entity_info	= LOAD_FUNC(entity_info * (*)(size_t, size_t), "get_entity_info", proj_dll);

		assert(_get_registered_struct_count
			   && _get_registered_world_count
			   && _get_registered_entity_count
			   && _get_struct_info
			   && _get_scene_info
			   && _get_world_info
			   && _get_component_info
			   && _get_entity_info);

		// for (const auto struct_idx : iota(0ul, _get_registered_struct_count()))
		//{
		//	auto* p_struct_info = _get_struct_info(struct_idx);
		//	auto  s_id			= reflection::create_struct();
		//	auto* p_s			= reflection::find_struct(s_id);
		//	{
		//		p_s->name = p_struct_info->name;
		//		// p_s->p_default_value = p_struct_info->p_default_value;
		//		p_s->hash_id	 = p_struct_info->hash_id;
		//		p_s->field_count = p_struct_info->field_count;
		//	}

		//	for (const auto field_idx : iota(0ul, p_struct_info->field_count))
		//	{
		//		auto* p_field_info = &p_struct_info->fields[field_idx];
		//		auto  f_id		   = reflection::add_field(s_id);
		//		auto* p_f		   = reflection::find_field(f_id);
		//		{
		//			p_f->struct_id = s_id;
		//			p_f->name	   = p_field_info->name;
		//			// p_f->p_value   = (void*)((char*)(p_s->p_default_value) + p_field_info->offset);
		//			p_f->type	= p_field_info->type;
		//			p_f->offset = p_field_info->offset;
		//		}
		//		{
		//			p_s->size += reflection::utils::type_size(p_field_info->type);
		//		}
		//	}
		//}

		// for (const auto scene_idx : iota(0ul, _get_registered_scene_count()))
		//{
		//	auto* p_scene_info = _get_scene_info(scene_idx);
		//	auto  s_id		   = scene::create();
		//	auto  p_scene	   = scene::find(s_id);
		//	{
		//		p_scene->name = p_scene_info->name;
		//	}

		//	for (const auto world_idx : iota(p_scene_info->world_idx) | take(p_scene_info->world_count))
		//	{
		//		auto* p_w_info = _get_world_info(world_idx);
		//		auto  w_id	   = world::create(p_scene->id, p_w_info->name, {});
		//		auto  p_world  = world::find(w_id);
		//		{
		//			p_world->id		  = w_id;
		//			p_world->scene_id = s_id;
		//			p_world->ecs_idx  = world_idx;
		//			p_world->name	  = p_w_info->name;
		//		}

		//		for (const auto struct_idx : p_w_info->struct_idx_vec | take(p_w_info->struct_count))
		//		{
		//			auto* p_s = reflection::find_struct(_get_struct_info(struct_idx)->name);
		//			world::add_struct(w_id, p_s->id);
		//		}

		//		for (const auto entity_idx : iota(0ul, _get_registered_entity_count(world_idx)))
		//		{
		//			auto* p_e_info = _get_entity_info(world_idx, entity_idx);
		//			auto  e_id	   = entity::create(w_id);
		//			auto  p_entity = entity::find(e_id);
		//			{
		//				p_entity->id		= e_id;
		//				p_entity->world_id	= w_id;
		//				p_entity->name		= p_e_info->name;
		//				p_entity->archetype = p_e_info->archetype;
		//				p_entity->ecs_idx	= p_e_info->idx;
		//			}

		//			for (const auto world_s_idx : iota(0ul, p_world->structs.size()) | filter([p_entity](const auto idx) { return ((p_entity->archetype >> idx) & 1ul) != 0; }))
		//			{
		//				// todo c_info.p_value is invalid
		//				auto component_idx = __popcnt(p_entity->archetype & ((1ul << world_s_idx) - 1));
		//				auto c_info		   = _get_component_info(world_idx, entity_idx, component_idx);
		//				// auto c_id		   = component::add(e_id, p_world->structs[world_s_idx], c_info.p_value);
		//			}
		//		}
		//	}
		//}

		// apply .assemble to ecs models

		return true;
	}
}	 // namespace editor::game::ecs

namespace editor::game::ecs
{
	using namespace editor::models;
	using namespace std::views;

	bool init_from_project_data(std::string& project_file_path)
	{
		auto project_file_xml = pugi::xml_document();
		auto root_node		  = pugi::xml_node();
		{
			if (project_file_xml.load_file(project_file_path.c_str()) is_false)
			{
				return false;
			}

			root_node = project_file_xml.child("project");
		}

		for (auto struct_node : root_node.child("structs").children())
		{
			auto ecs_struct_idx = game::ecs::new_struct();
			auto s_id			= reflection::create_struct(
				  struct_node.attribute("name").as_string(),
				  struct_node.attribute("hash_id").as_ullong(),
				  ecs_struct_idx);

			for (auto field_node : struct_node.child("fields"))
			{
				auto field_type = editor::models::reflection::utils::string_to_type(field_node.attribute("type").as_string());
				reflection::add_field(
					s_id,
					field_type,
					field_node.attribute("name").as_string(),
					field_node.attribute("offset").as_ullong(),
					game::ecs::add_field(ecs_struct_idx, field_type, field_node.attribute("value").as_string()));
			}
		}

		for (auto scene_node : root_node.child("scenes").children())
		{
			auto  s_id		 = scene::create(scene_node.attribute("name").as_string(), game::ecs::new_scene());
			auto* p_em_scene = scene::find(s_id);

			for (auto world_node : scene_node.child("worlds").children())
			{
				auto  ecs_world_idx = game::ecs::new_world(p_em_scene->ecs_idx);
				auto  w_id			= world::create(s_id, world_node.attribute("name").as_string(), ecs_world_idx);
				auto* p_em_world	= world::find(w_id);

				for (auto* p_em_struct : world_node.child("structs").children() | std::views::transform([](const auto node) { return reflection::find_struct(node.attribute("name").value()); }))
				{
					game::ecs::world_add_struct(p_em_scene->ecs_idx, ecs_world_idx, p_em_struct->ecs_idx);
					world::add_struct(w_id, p_em_struct->id);
				}

				for (auto entity_node : world_node.child("entities"))
				{
					auto  archetype	  = entity_node.attribute("archetype").as_ullong();
					auto  e_id		  = entity::create(w_id, entity_node.attribute("name").as_string(), archetype, game::ecs::new_entity(p_em_scene->ecs_idx, p_em_world->ecs_idx, archetype));
					auto* p_em_entity = entity::find(e_id);

					for (auto component_idx = 0; auto component_node : entity_node.child("components"))
					{
						auto* p_em_struct = reflection::find_struct(component_node.attribute("name").value());
						auto* p_mem		  = game::ecs::get_component_memory(p_em_scene->ecs_idx, p_em_world->ecs_idx, p_em_entity->ecs_idx, p_em_struct->ecs_idx);
						for (auto f_info_it = struct_info_vec[p_em_struct->ecs_idx].field_info_vec.begin();
							 auto field_node : component_node.child("fields").children())
						{
							reflection::utils::serialize(f_info_it->type, field_node.attribute("value").value(), (uint8*)p_mem + f_info_it->mem_offset);
							++f_info_it;
						}
					}
				}
			}
		}
		return true;
	}

	void clear_models()
	{
		for (auto& info : struct_info_vec)
		{
			free(info.p_default_value);
		}

		std::ranges::for_each(std::views::iota(scenes) | take(scene_count), [](auto* p_scene) {
			for (auto* p_w : std::views::iota(p_scene->worlds) | take(p_scene->world_count))
			{
				p_w->~ecs_world();
			}

			free(p_scene->worlds);
		});

		free(scenes);
		struct_info_vec.clear();
		scene_hole_begin_idx = -1;
		scene_hole_count	 = 0;
	}

	bool update_models()
	{
		return true;
	}

	component_info get_component(editor_id entity_id, uint64 component_idx)
	{
		auto* p_e = entity::find(entity_id);
		auto* p_w = world::find(p_e->world_id);

		return _get_component_info(p_w->ecs_idx, p_e->ecs_idx, component_idx);
	}

	std::vector<void*> get_components(editor_id entity_id)
	{
		auto p_e   = entity::find(entity_id);
		auto p_w   = world::find(p_e->world_id);
		auto e_idx = p_e->ecs_idx;
		auto w_idx = p_w->ecs_idx;

		return std::vector<void*>();
	}

	void set_components(editor_id entity_id, uint64 component_idx, void* p_value)
	{
	}
}	 // namespace editor::game::ecs

namespace editor::game::ecs
{
	ecs::struct_idx new_struct()
	{
		struct_info_vec.emplace_back();
		return struct_info_vec.size() - 1;
	}

	ecs::field_idx add_field(ecs::struct_idx s_idx, e_primitive_type f_type, std::string f_value)
	{
		auto* p_s		 = &struct_info_vec[s_idx];
		auto* p_f		 = &p_s->field_info_vec.emplace_back();
		p_f->type		 = f_type;
		p_f->mem_offset	 = p_s->size;
		p_s->size		+= editor::models::reflection::utils::type_size(p_f->type);

		if (p_s->p_default_value is_nullptr)
		{
			p_s->p_default_value = malloc(p_s->size);
		}
		else
		{
			p_s->p_default_value = realloc(p_s->p_default_value, p_s->size);
		}

		editor::models::reflection::utils::serialize(p_f->type, f_value, (uint8*)p_s->p_default_value + p_f->mem_offset);
		return (ecs::field_idx)p_s->field_info_vec.size() - 1;
	}

	void* get_field_pvalue(ecs::struct_idx struct_idx, ecs::field_idx field_idx)
	{
		auto* p_s = &struct_info_vec[struct_idx];
		auto* p_f = &p_s->field_info_vec[field_idx];

		return (uint8*)p_s->p_default_value + p_f->mem_offset;
	}
}	 // namespace editor::game::ecs

namespace editor::game::ecs
{
	ecs::scene_idx new_scene()
	{
		ecs::scene_idx s_idx;
		if (scenes is_nullptr)
		{
			scenes = (ecs_scene*)malloc(sizeof(ecs_scene));
			s_idx  = 0;
		}
		if (scene_hole_count > 0)
		{
			s_idx				 = scene_hole_begin_idx;
			scene_hole_begin_idx = scenes[s_idx].idx;
			--scene_hole_count;
		}
		else
		{
			s_idx  = scene_count;
			scenes = (ecs_scene*)realloc(scenes, sizeof(ecs_scene) * (scene_count + 1));
		}

		++scene_count;
		new (scenes + s_idx) ecs_scene();
		scenes[s_idx].idx = s_idx;
		return s_idx;
	}

	void delete_scene(ecs::scene_idx ecs_scene_idx, utilities::memory_handle* p_backup)
	{
		auto* p_s = &scenes[ecs_scene_idx];

		if (p_backup)
		{
			p_backup->p_data		= malloc(sizeof(ecs_scene));
			p_backup->clean_up_func = [](void* p_mem) {
				auto p_scene = (ecs_scene*)p_mem;
				for (auto* p_w : std::views::iota(p_scene->worlds) | take(p_scene->world_count))
				{
					p_w->~ecs_world();
				}

				free(p_scene->worlds);
			};

			memcpy(p_backup->p_data, p_s, sizeof(ecs_scene));
		}

		--scene_count;
		++scene_hole_count;
		std::swap(scene_hole_begin_idx, p_s->idx);
	}

	void restore_scene(ecs::scene_idx ecs_scene_idx, utilities::memory_handle* p_backup)
	{
		auto* p_s = &scenes[ecs_scene_idx];

		assert(scene_hole_begin_idx == ecs_scene_idx);

		scene_hole_begin_idx = p_s->idx;
		--scene_hole_count;
		++scene_count;

		memcpy(p_s, p_backup->p_data, sizeof(ecs_scene));
		p_backup->clean_up_func = nullptr;
	}

	ecs::world_idx new_world(ecs::scene_idx ecs_scene_idx)
	{
		auto* p_scene	= &scenes[ecs_scene_idx];
		auto* p_world	= (ecs_world*)nullptr;
		auto  world_idx = (ecs::world_idx)0;
		{
			if (p_scene->world_count == 0)
			{
				p_scene->world_capacity = 1;
				p_scene->worlds			= (ecs_world*)malloc(sizeof(ecs_world));

				p_world = &p_scene->worlds[0];
			}
			else if (p_scene->world_count < p_scene->world_capacity)
			{
				world_idx					  = p_scene->world_hole_begin_idx;
				p_world						  = &p_scene->worlds[world_idx];
				p_scene->world_hole_begin_idx = p_world->idx;
				--p_scene->world_hole_count;
			}
			else
			{
				world_idx = p_scene->world_count;
				++p_scene->world_capacity;
				p_scene->worlds = (ecs_world*)realloc(p_scene->worlds, sizeof(ecs_world) * (p_scene->world_count + 1));

				p_world = &p_scene->worlds[p_scene->world_count];
			}

			++p_scene->world_count;
		}

		assert(p_world);
		new (p_world) ecs_world();
		{
			p_world->idx = p_scene->world_count - 1;
			p_world->idx = world_idx;
		}

		return p_world->idx;
	}

	void delete_world(ecs::scene_idx ecs_scene_idx, ecs::world_idx ecs_world_idx, utilities::memory_handle* p_backup)
	{
		auto* p_s = &scenes[ecs_scene_idx];
		auto* p_w = &p_s->worlds[ecs_world_idx];

		if (p_backup)
		{
			p_backup->p_data		= malloc(sizeof(ecs_world));
			p_backup->clean_up_func = [](void* p_mem) {
				((ecs_world*)p_mem)->~ecs_world();
				free(p_mem);
			};

			memcpy(p_backup->p_data, p_w, sizeof(ecs_world));
		}

		--p_s->world_count;
		++p_s->world_hole_count;
		std::swap(p_s->world_hole_begin_idx, p_w->idx);
	}

	void restore_world(ecs::scene_idx ecs_scene_idx, ecs::world_idx ecs_world_idx, utilities::memory_handle* p_backup)
	{
		auto* p_s = &scenes[ecs_scene_idx];
		auto* p_w = &p_s->worlds[ecs_world_idx];

		assert(p_s->world_hole_begin_idx == ecs_world_idx);

		p_s->world_hole_begin_idx = p_w->idx;
		--p_s->world_hole_count;
		++p_s->world_count;

		memcpy(p_w, p_backup->p_data, sizeof(ecs_world));
		p_backup->clean_up_func = nullptr;
	}

	void world_add_struct(ecs::scene_idx scene_idx, ecs::world_idx world_idx, ecs::struct_idx struct_idx)
	{
		auto* p_scene = &scenes[scene_idx];
		auto* p_world = &p_scene->worlds[world_idx];
		p_world->ecs_struct_idx_vec.emplace_back(struct_idx);
	}

	void world_remove_struct(ecs::scene_idx scene_idx, ecs::world_idx world_idx, ecs::struct_idx struct_idx)
	{
		// A B C D
		// archetype = 0101(C, A)

		// delete B
		// A C D
		// archetype = 011(C, A) = 010 | 0001
		auto* p_scene		 = &scenes[scene_idx];
		auto* p_world		 = &p_scene->worlds[world_idx];
		auto  nth_struct_idx = std::distance(p_world->ecs_struct_idx_vec.begin(), std::ranges::find(p_world->ecs_struct_idx_vec, struct_idx));

		for (auto& e : p_world->entities)
		{
			e.archetype = calc_archetype_remove_component(e.archetype, nth_struct_idx);
		}

		// p_world->memory_block_vec_map = decltype(p_world->memory_block_vec_map) {};
		auto backup = decltype(p_world->memory_block_vec_map) {};
		backup.merge(p_world->memory_block_vec_map);
		for (auto&& [archetype, mem_block_vec] : backup)
		{
			auto new_archetype							 = calc_archetype_remove_component(archetype, nth_struct_idx);
			p_world->memory_block_vec_map[new_archetype] = std::move(mem_block_vec);
		}
	}

	uint64 new_entity(ecs::scene_idx ecs_scene_idx, ecs::world_idx ecs_world_idx, ecs::archetype_t archetype)
	{
		assert(ecs_scene_idx < scene_count);
		auto* p_world	   = &scenes[ecs_scene_idx].worlds[ecs_world_idx];
		auto* p_ecs_entity = p_world->new_entity(archetype);
		return p_ecs_entity->idx;
	}

	void delete_entity(ecs::scene_idx ecs_scene_idx, ecs::world_idx ecs_world_idx, ecs::entity_idx ecs_entity_idx)
	{
		assert(ecs_scene_idx < scene_count);
		auto* p_world = &scenes[ecs_scene_idx].worlds[ecs_world_idx];
		p_world->delete_entity(ecs_entity_idx);
	}

	void add_component(ecs::scene_idx ecs_scene_idx, ecs::world_idx ecs_world_idx, ecs::entity_idx ecs_entity_idx, ecs::struct_idx ecs_struct_idx)
	{
		assert(ecs_scene_idx < scene_count);
		auto* p_world = &scenes[ecs_scene_idx].worlds[ecs_world_idx];
		// p_world->_calc_component_idx
		p_world->add_component(ecs_entity_idx, ecs_struct_idx);
	}

	archetype_t get_archetype(ecs::scene_idx ecs_scene_idx, ecs::world_idx ecs_world_idx, ecs::entity_idx ecs_entity_idx)
	{
		return scenes[ecs_scene_idx].worlds[ecs_world_idx].entities[ecs_entity_idx].archetype;
	}

	std::vector<struct_idx> get_struct_idx_vec(ecs::scene_idx ecs_scene_idx, ecs::world_idx ecs_world_idx, archetype_t archetype)
	{
		auto* p_w = &scenes[ecs_scene_idx].worlds[ecs_world_idx];
		return std::views::iota(0, std::bit_width(archetype))
			 | std::views::filter([archetype](auto nth_bit) { return (archetype >> nth_bit) & 1; })
			 | std::views::transform([=](auto nth_bit) { return p_w->ecs_struct_idx_vec[nth_bit]; })
			 | std::ranges::to<std::vector>();
	}

	size_t get_archetype_size(ecs::scene_idx s_idx, ecs::world_idx w_idx, archetype_t a)
	{
		if (scenes[s_idx].worlds[w_idx].archetype_size_map.contains(a))
		{
			return scenes[s_idx].worlds[w_idx].archetype_size_map[a];
		}
		else
		{
			auto p_world = &scenes[s_idx].worlds[w_idx];
			return std::ranges::fold_left(std::views::iota(0, std::bit_width(a))
											  | std::views::filter([a](auto nth_bit) { return (a >> nth_bit) & 1; })
											  | std::views::transform([=](auto nth_bit) { return struct_info_vec[p_world->ecs_struct_idx_vec[nth_bit]].size; }),
										  0, std::plus {});
		}
	}

	size_t get_struct_size(ecs::struct_idx ecs_struct_idx)
	{
		return struct_info_vec[ecs_struct_idx].size;
	}

	void remove_component(ecs::scene_idx ecs_scene_idx, ecs::world_idx ecs_world_idx, ecs::entity_idx ecs_entity_idx, ecs::struct_idx ecs_struct_idx)
	{
		assert(ecs_scene_idx < scene_count);
		auto* p_world = &scenes[ecs_scene_idx].worlds[ecs_world_idx];
		// p_world->_calc_component_idx
		p_world->remove_component(ecs_entity_idx, ecs_struct_idx);
	}

	void* get_component_memory(ecs::scene_idx ecs_scene_idx, ecs::world_idx ecs_world_idx, ecs::entity_idx ecs_entity_idx, ecs::struct_idx ecs_struct_idx)
	{
		auto* p_world  = &scenes[ecs_scene_idx].worlds[ecs_world_idx];
		auto* p_entity = &p_world->entities[ecs_entity_idx];
		return p_world->get_component(ecs_entity_idx, ecs_struct_idx);
	}

	void copy_archetype_memory(void* p_dest, ecs::scene_idx ecs_scene_idx, ecs::world_idx ecs_world_idx, ecs::entity_idx ecs_entity_idx)
	{
		auto* p_w		= &scenes[ecs_scene_idx].worlds[ecs_world_idx];
		auto* p_e		= &p_w->entities[ecs_entity_idx];
		auto  archetype = p_e->archetype;
		for (auto struct_idx : std::views::iota(0, std::bit_width(archetype))
								   | std::views::filter([archetype](auto nth_bit) { return (archetype >> nth_bit) & 1; })
								   | std::views::transform([=](auto nth_bit) { return p_w->ecs_struct_idx_vec[nth_bit]; }))
		{
			memcpy(p_dest, p_w->get_component(ecs_entity_idx, struct_idx), struct_info_vec[struct_idx].size);
		}
	}

	void restore_archetype_memory(ecs::scene_idx ecs_scene_idx, ecs::world_idx ecs_world_idx, ecs::entity_idx ecs_entity_idx, void* p_src)
	{
		auto* p_w = &scenes[ecs_scene_idx].worlds[ecs_world_idx];
		auto* p_e = &p_w->entities[ecs_entity_idx];

		auto ecs_struct_idx_view = std::views::iota(0, std::bit_width(p_e->archetype))
								 | std::views::filter([archetype = p_e->archetype](auto nth_bit) { return (archetype >> nth_bit) & 1; })
								 | std::views::transform([p_w](auto nth_component) { return p_w->ecs_struct_idx_vec[nth_component]; });


		std::ranges::for_each(ecs_struct_idx_view, [p_mem = (uint8*)p_src, p_w, ecs_entity_idx](auto s_idx) mutable {
			memcpy(p_w->get_component(ecs_entity_idx, s_idx), p_mem, struct_info_vec[s_idx].size);
			p_mem += struct_info_vec[s_idx].size;
		});
	}

	archetype_t calc_archetype_remove_component(archetype_t a, uint8 nth_component)
	{
		return (a >> 1) | (((1ull << nth_component) - 1) & a);
	}
}	 // namespace editor::game::ecs

// helper
namespace editor::game::ecs
{
	ecs::world_idx new_world(editor_id s_id)
	{
		auto* p_s = scene::find(s_id);
		return ecs::new_world(p_s->ecs_idx);
	}

	void delete_world(editor_id s_id, editor_id w_id)
	{
		auto* p_s = scene::find(s_id);
		auto* p_w = world::find(w_id);
		ecs::delete_world(p_s->ecs_idx, p_w->ecs_idx);
	}

	ecs::entity_idx new_entity(editor_id w_id, archetype_t a)
	{
		auto* p_w = world::find(w_id);
		auto* p_s = scene::find(p_w->scene_id);

		return ecs::new_entity(p_s->ecs_idx, p_w->ecs_idx, a);
	}

	void delete_entity(editor_id e_id)
	{
		auto* p_entity = entity::find(e_id);
		auto* p_world  = world::find(p_entity->world_id);
		auto* p_scene  = scene::find(p_world->scene_id);
		ecs::delete_entity(p_scene->ecs_idx, p_world->ecs_idx, p_entity->ecs_idx);
	}

	void world_add_struct(editor_id w_id, editor_id s_id)
	{
		auto* p_em_world  = world::find(w_id);
		auto* p_em_scene  = scene::find(p_em_world->scene_id);
		auto* p_em_struct = reflection::find_struct(s_id);

		world_add_struct(p_em_scene->ecs_idx, p_em_world->ecs_idx, p_em_struct->ecs_idx);
	}

	void world_remove_struct(editor_id w_id, editor_id s_id)
	{
		auto* p_em_world  = world::find(w_id);
		auto* p_em_scene  = scene::find(p_em_world->scene_id);
		auto* p_em_struct = reflection::find_struct(s_id);

		world_remove_struct(p_em_scene->ecs_idx, p_em_world->ecs_idx, p_em_struct->ecs_idx);
	}
}	 // namespace editor::game::ecs
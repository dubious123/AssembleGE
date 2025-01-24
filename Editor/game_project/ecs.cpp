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
}	 // namespace

namespace
{
	using ecs_entity = ecs::entity;

	struct ecs_world
	{
		std::vector<ecs_entity>												  entities;
		std::map<ecs::archetype_t, data_structure::vector<ecs::memory_block>> memory_block_vec_map;

		// editor only
		std::vector<editor_id>			   component_info_vec;
		std::map<ecs::archetype_t, size_t> _archetype_size_map;


		size_t entity_hole_begin_idx = -1;
		size_t entity_hole_count	 = 0;

		ecs_world(const ecs_world&&)	  = delete;
		ecs_world(const ecs_world&)		  = delete;
		ecs_world& operator=(ecs_world&&) = delete;
		ecs_world& operator=(ecs_world&)  = delete;

		ecs_world() = default;

		size_t calc_archetype_size(ecs::archetype_t archetype)
		{
			if (_archetype_size_map.contains(archetype))
			{
				return _archetype_size_map.find(archetype)->second;
			}

			_archetype_size_map[archetype] = (size_t)std::ranges::fold_left(
				std::views::iota(0, std::bit_width(archetype))
					| std::views::filter([archetype](auto nth_bit) { return (archetype >> nth_bit) & 1; })
					| std::views::transform([this](auto nth_bit) { return editor::models::reflection::find_struct(component_info_vec[nth_bit])->size; }),
				0ull, std::plus {});
			return _archetype_size_map[archetype];
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

		void _init_mem_block(ecs::archetype_t archetype, size_t size_per_archetype, ecs::memory_block* p_block)
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
				auto c_size = editor::models::reflection::find_struct(component_info_vec[nth_bit])->size;
				p_block->write_component_data(c_idx, offset, c_size);
				offset += c_size * capacity;
				++c_idx;
			}

			assert(p_block->get_header_size() == 6 + sizeof(uint32) * component_count);
			assert(offset + p_block->calc_unused_mem_size() == MEMORY_BLOCK_SIZE);
		}

		ecs::entity* _new_entity(ecs::archetype_t archetype)
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

				for (const auto nth_bit : std::views::iota(0, std::bit_width(archetype)) | std::views::filter([archetype](auto nth_bit) { return (archetype >> nth_bit) & 1; }))
				{
					auto p_c_info = editor::models::reflection::find_struct(component_info_vec[nth_bit]);
				}
				_init_mem_block(archetype, (calc_archetype_size(archetype) + sizeof(ecs::entity_idx)), p_block);
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
					auto p_c_info = editor::models::reflection::find_struct(component_info_vec[nth_bit]);
					memcpy(p_block->get_component_ptr(m_idx, nth_bit), p_c_info->p_default_value, p_c_info->size);
				}

				p_block->write_count(m_idx + 1);
				p_entity->memory_idx = m_idx;
			}

			return p_entity;
		};
	};

	struct ecs_scene
	{
		ecs_world* worlds;

		ecs_scene(const ecs_scene&&)	  = delete;
		ecs_scene(const ecs_scene&)		  = delete;
		ecs_scene& operator=(ecs_scene&&) = delete;
		ecs_scene& operator=(ecs_scene&)  = delete;

		ecs_scene() = default;
	};

	uint32			   scene_count;
	ecs_scene*		   scenes;
	std::vector<void*> mem_to_free;
}	 // namespace

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
		_get_registered_struct_count = LOAD_FUNC(size_t(*)(), "get_registered_struct_count", proj_dll);
		_get_registered_scene_count	 = LOAD_FUNC(size_t(*)(), "get_registered_scene_count", proj_dll);
		_get_registered_world_count	 = LOAD_FUNC(size_t(*)(), "get_registered_world_count", proj_dll);
		_get_registered_entity_count = LOAD_FUNC(size_t(*)(size_t), "get_registered_entity_count", proj_dll);

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

		for (const auto struct_idx : iota(0ul, _get_registered_struct_count()))
		{
			auto* p_struct_info = _get_struct_info(struct_idx);
			auto  s_id			= reflection::create_struct();
			auto* p_s			= reflection::find_struct(s_id);
			{
				p_s->name			 = p_struct_info->name;
				p_s->p_default_value = p_struct_info->p_default_value;
				p_s->hash_id		 = p_struct_info->hash_id;
				p_s->field_count	 = p_struct_info->field_count;
			}

			for (const auto field_idx : iota(0ul, p_struct_info->field_count))
			{
				auto* p_field_info = &p_struct_info->fields[field_idx];
				auto  f_id		   = reflection::add_field(s_id);
				auto* p_f		   = reflection::find_field(f_id);
				{
					p_f->struct_id = s_id;
					p_f->name	   = p_field_info->name;
					p_f->p_value   = (void*)((char*)(p_s->p_default_value) + p_field_info->offset);
					p_f->type	   = p_field_info->type;
					p_f->offset	   = p_field_info->offset;
				}
				{
					p_s->size += reflection::utils::type_size(p_field_info->type);
				}
			}
		}

		for (const auto scene_idx : iota(0ul, _get_registered_scene_count()))
		{
			auto* p_scene_info = _get_scene_info(scene_idx);
			auto  s_id		   = scene::create();
			auto  p_scene	   = scene::find(s_id);
			{
				p_scene->name = p_scene_info->name;
			}

			for (const auto world_idx : iota(p_scene_info->world_idx) | take(p_scene_info->world_count))
			{
				auto* p_w_info = _get_world_info(world_idx);
				auto  w_id	   = world::create(p_scene->id);
				auto  p_world  = world::find(w_id);
				{
					p_world->id			   = w_id;
					p_world->scene_id	   = s_id;
					p_world->ecs_world_idx = world_idx;
					p_world->name		   = p_w_info->name;
				}

				for (const auto struct_idx : p_w_info->struct_idx_vec | take(p_w_info->struct_count))
				{
					auto* p_s = reflection::find_struct(_get_struct_info(struct_idx)->name);
					world::add_struct(w_id, p_s->id);
				}

				for (const auto entity_idx : iota(0ul, _get_registered_entity_count(world_idx)))
				{
					auto* p_e_info = _get_entity_info(world_idx, entity_idx);
					auto  e_id	   = entity::create(w_id);
					auto  p_entity = entity::find(e_id);
					{
						p_entity->id			 = e_id;
						p_entity->world_id		 = w_id;
						p_entity->name			 = p_e_info->name;
						p_entity->archetype		 = p_e_info->archetype;
						p_entity->ecs_entity_idx = p_e_info->idx;
					}

					for (const auto world_s_idx : iota(0ul, p_world->structs.size()) | filter([p_entity](const auto idx) { return ((p_entity->archetype >> idx) & 1ul) != 0; }))
					{
						// todo c_info.p_value is invalid
						auto component_idx = __popcnt(p_entity->archetype & ((1ul << world_s_idx) - 1));
						auto c_info		   = _get_component_info(world_idx, entity_idx, component_idx);
						auto c_id		   = component::create(e_id, p_world->structs[world_s_idx], c_info.p_value);
					}
				}
			}
		}

		// apply .assemble to ecs models

		return true;
	}

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
			auto  s_id				= reflection::create_struct();
			auto* p_s				= reflection::find_struct(s_id);
			auto  default_value_vec = std::vector<std::string>();
			{
				p_s->name = struct_node.attribute("name").value();	  // p_struct_info->name;
				// p_s->p_default_value = p_struct_info->p_default_value;
				p_s->hash_id = struct_node.attribute("hash_id").as_ullong();
			}

			for (auto field_node : struct_node.child("fields"))
			{
				++p_s->field_count;

				auto  f_id = reflection::add_field(s_id);
				auto* p_f  = reflection::find_field(f_id);
				{
					p_f->struct_id = s_id;
					p_f->name	   = field_node.attribute("name").value();
					p_f->type	   = editor::models::reflection::utils::string_to_type(field_node.attribute("type").value());	 // p_field_info->type;
					p_f->offset	   = field_node.attribute("offset").as_ullong();												 // p_field_info->offset;
				}
				{
					p_s->size += reflection::utils::type_size(p_f->type);
					default_value_vec.push_back(field_node.attribute("value").value());
				}
			}

			p_s->p_default_value = malloc(p_s->size);
			mem_to_free.push_back(p_s->p_default_value);
			auto it = default_value_vec.begin();
			for (auto p_field : reflection::all_fields(p_s->id))
			{
				p_field->p_value = (void*)((char*)(p_s->p_default_value) + p_field->offset);
				reflection::utils::serialize(p_field->type, *it, p_field->p_value);
				++it;
			}
		}

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
		//		auto  w_id	   = world::create(p_scene->id);
		//		auto  p_world  = world::find(w_id);
		//		{
		//			p_world->id			   = w_id;
		//			p_world->scene_id	   = s_id;
		//			p_world->ecs_world_idx = world_idx;
		//			p_world->name		   = p_w_info->name;
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
		//				p_entity->id			 = e_id;
		//				p_entity->world_id		 = w_id;
		//				p_entity->name			 = p_e_info->name;
		//				p_entity->archetype		 = p_e_info->archetype;
		//				p_entity->ecs_entity_idx = p_e_info->idx;
		//			}

		//			for (const auto world_s_idx : iota(0ul, p_world->structs.size()) | filter([p_entity](const auto idx) { return ((p_entity->archetype >> idx) & 1ul) != 0; }))
		//			{
		//				// todo c_info.p_value is invalid
		//				auto component_idx = __popcnt(p_entity->archetype & ((1ul << world_s_idx) - 1));
		//				auto c_info		   = _get_component_info(world_idx, entity_idx, component_idx);
		//				auto c_id		   = component::create(e_id, p_world->structs[world_s_idx], c_info.p_value);
		//			}
		//		}
		//	}
		//}

		scene_count = std::distance(root_node.child("scenes").children().begin(), root_node.child("scenes").children().end());
		scenes		= (ecs_scene*)malloc(sizeof(ecs_scene) * scene_count);

		auto scene_idx = 0;

		for (auto scene_node : root_node.child("scenes").children())
		{
			auto* p_ecs_scene = &scenes[scene_idx];
			{
				auto world_count	= std::distance(scene_node.child("worlds").children().begin(), scene_node.child("worlds").children().end());
				p_ecs_scene->worlds = (ecs_world*)malloc(sizeof(ecs_world) * world_count);

				for (auto i : std::views::iota(0, world_count))
				{
					new (p_ecs_scene->worlds + i) ecs_world();
				}
			}

			auto s_id	 = scene::create();
			auto p_scene = scene::find(s_id);
			{
				p_scene->name = scene_node.attribute("name").value();
			}

			auto world_idx = 0;

			for (auto world_node : scene_node.child("worlds").children())
			{
				auto* p_ecs_world = &p_ecs_scene->worlds[world_idx];
				auto  w_id		  = world::create(p_scene->id);
				auto  p_world	  = world::find(w_id);
				{
					p_world->id			   = w_id;
					p_world->scene_id	   = s_id;
					p_world->ecs_world_idx = world_idx;
					p_world->name		   = world_node.attribute("name").value();
				}

				for (auto struct_node : world_node.child("structs").children())
				{
					auto* p_s = reflection::find_struct(struct_node.attribute("name").value());
					models::world::add_struct(w_id, p_s->id);
				}

				for (auto s_id : p_world->structs)
				{
					p_ecs_world->component_info_vec.push_back(s_id);
				}


				auto entities_node = world_node.child("entities");
				{
					for (auto entity_node : entities_node)
					{
						auto e_id	  = entity::create(w_id);
						auto p_entity = entity::find(e_id);
						{
							p_entity->id			 = e_id;
							p_entity->world_id		 = w_id;
							p_entity->name			 = entity_node.attribute("name").value();
							p_entity->archetype		 = entity_node.attribute("archetype").as_ullong();
							p_entity->ecs_entity_idx = p_ecs_world->entities.size();
						}

						auto* p_ecs_entity = p_ecs_world->_new_entity(p_entity->archetype);

						auto components_node = entity_node.child("components");

						auto component_idx = 0;
						for (auto component_node : components_node)
						{
							auto* p_struct				  = reflection::find_struct(component_node.attribute("name").value());
							auto  p_ecs_compoonent_memory = p_ecs_world->get_p_mem_block(*p_ecs_entity)->get_component_ptr(p_ecs_entity->memory_idx, component_idx);
							auto  c_id					  = component::create(e_id, p_struct->id, p_ecs_compoonent_memory);
							auto  p_component			  = component::find(c_id);
							for (auto field_node : component_node.child("fields").children())
							{
								auto offset = field_node.attribute("offset").as_uint();
								auto c_type = reflection::utils::string_to_type(field_node.attribute("type").value());
								reflection::utils::serialize(c_type, field_node.attribute("value").value(), (uint8*)p_ecs_compoonent_memory + offset);
							}

							++component_idx;
						}
					}
				}

				++world_idx;
			}

			++scene_idx;
		}
		return true;
	}

	void clear_models()
	{
		for (auto* mem : mem_to_free)
		{
			free(mem);
		}

		for (auto scene_idx : std::views::iota(0ul, scene_count))
		{
			free(scenes[scene_idx].worlds);
		}

		free(scenes);
		mem_to_free.clear();
	}

	bool update_models()
	{
		return true;
	}

	component_info get_component(editor_id entity_id, uint64 component_idx)
	{
		auto* p_e = entity::find(entity_id);
		auto* p_w = world::find(p_e->world_id);

		return _get_component_info(p_w->ecs_world_idx, p_e->ecs_entity_idx, component_idx);
	}

	std::vector<void*> get_components(editor_id entity_id)
	{
		auto p_e   = entity::find(entity_id);
		auto p_w   = world::find(p_e->world_id);
		auto e_idx = p_e->ecs_entity_idx;
		auto w_idx = p_w->ecs_world_idx;

		return std::vector<void*>();
	}

	void set_components(editor_id entity_id, uint64 component_idx, void* p_value)
	{
	}
}	 // namespace editor::game::ecs

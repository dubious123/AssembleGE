#include "pch.h"
#include "editor.h"
#include "game.h"

#include "../Engine/__reflection.h"

namespace
{
	using struct_info	 = reflection::struct_info;
	using scene_info	 = reflection::scene_info;
	using world_info	 = reflection::world_info;
	using component_info = reflection::component_info;
	using entity_info	 = reflection::entity_info;
}	 // namespace

namespace editor::game::ecs
{
	using namespace editor::models;

	size_t (*_get_registered_struct_count)();
	size_t (*_get_registered_scene_count)();
	size_t (*_get_registered_world_count)();
	size_t (*_get_registered_entity_count)(size_t world_index);
	struct_info* (*_get_struct_info)(size_t index);
	scene_info* (*_get_scene_info)(size_t index);
	world_info* (*_get_world_info)(size_t index);
	component_info (*_get_component_info)(size_t world_idx, size_t entity_idx, size_t component_idx);
	entity_info* (*_get_entity_info)(size_t world_index, size_t entity_index);

	bool init(HMODULE proj_dll)
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
			   && _get_registered_scene_count
			   && _get_registered_world_count
			   && _get_registered_entity_count
			   && _get_struct_info
			   && _get_scene_info
			   && _get_world_info
			   && _get_component_info
			   && _get_entity_info);


		std::ranges::for_each(std::views::iota(0ul, _get_registered_struct_count()), [](auto struct_idx) {
			auto* p_struct_info = _get_struct_info(struct_idx);
			auto  s_id			= reflection::create_struct();
			auto* p_s			= reflection::find_struct(s_id);
			{
				p_s->name			 = p_struct_info->name;
				p_s->p_default_value = p_struct_info->p_default_value;
				p_s->hash_id		 = p_struct_info->hash_id;
			}

			std::ranges::for_each(std::views::iota(0ul, p_struct_info->field_count), [=](auto field_idx) {
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
			});
		});

		std::ranges::for_each(std::views::iota(0ul, _get_registered_scene_count()), [](auto scene_idx) {
			auto* p_scene_info = _get_scene_info(scene_idx);
			auto  s_id		   = scene::create();
			auto  p_scene	   = scene::find(s_id);
			{
				p_scene->name = p_scene_info->name;
			}

			std::ranges::for_each(std::views::iota(p_scene_info->world_idx) | std::views::take(p_scene_info->world_count), [=](auto world_idx) {
				auto* p_w_info = _get_world_info(world_idx);
				auto  w_id	   = world::create(p_scene->id);
				auto  p_world  = world::find(w_id);
				{
					p_world->id			   = w_id;
					p_world->scene_id	   = s_id;
					p_world->ecs_world_idx = world_idx;
					p_world->name		   = p_w_info->name;
				}

				std::ranges::for_each(p_w_info->struct_idx_vec | std::views::take(p_w_info->struct_count), [=](auto struct_idx) {
					auto* p_s = reflection::find_struct(_get_struct_info(struct_idx)->name);
					world::add_struct(w_id, p_s->id);
				});

				std::ranges::for_each(std::views::iota(0ul, _get_registered_entity_count(world_idx)), [=](auto entity_idx) {
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

					std::ranges::for_each(std::views::iota(0ul, p_world->structs.size()), [=](auto world_s_idx) {
						if (((p_entity->archetype >> world_s_idx) & 1ul) != 0)
						{
							// todo c_info.p_value is invalid
							auto  component_idx	 = __popcnt(p_entity->archetype & ((1ul << world_s_idx) - 1));
							auto  c_info		 = _get_component_info(world_idx, entity_idx, component_idx);
							auto  c_id			 = component::create(e_id, p_world->structs[world_s_idx]);
							auto* p_component	 = component::find(c_id);
							p_component->p_value = c_info.p_value;
						}
					});
				});
			});
		});

		return true;
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

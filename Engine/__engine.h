#pragma once
#include "__ecs.h"
#include "__reflection.h"

namespace reflection
{
	EDITOR_API size_t		   get_registered_struct_count();
	EDITOR_API size_t		   get_registered_scene_count();
	EDITOR_API size_t		   get_registered_world_count();
	EDITOR_API struct_info*	   get_struct_info(uint64 component_id);
	EDITOR_API scene_info*	   get_scene_info(size_t index);
	EDITOR_API world_info*	   get_world_info(size_t index);
	EDITOR_API component_info* get_component_info(size_t index);
}	 // namespace reflection

namespace ecs
{
	// EDITOR_API scene_base get_scene(size_t index);
}	 // namespace ecs

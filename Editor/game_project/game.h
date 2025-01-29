#pragma once
#include <string>

namespace editor::game
{
#define SCENE_NAME_MAX_LEN 20

	// using namespace editor::models;
	struct project_open_data
	{
		std::string name;
		std::string desc;
		std::string last_opened_date;
		std::string path;
		bool		starred = false;

		project_open_data() = default;
		project_open_data(const char* name, const char* desc, const char* date, const char* path, bool starred);
	};

	struct game_project
	{
		std::string directory_path;
		std::string project_file_path;
		std::string name;
		std::string description;
		std::string last_opened_date;
		bool		is_ready = false;
	};

	void init();

	bool project_opened();

	void on_project_loaded();

	void on_project_unloaded();

	void save_project_open_datas();

	bool save();

	game_project* get_current_p_project();
}	 // namespace editor::game

namespace editor::game::ecs
{
	using struct_idx  = uint64;
	using scene_idx	  = uint32;
	using world_idx	  = uint32;
	using entity_idx  = uint64;
	using archetype_t = uint64;

	bool init_from_dll(HMODULE proj_dll);
	bool init_from_project_data(std::string& project_file_path);

	bool update_models();	 // for runtime
	void clear_models();

	world_idx new_world(scene_idx ecs_scene_idx, std::vector<struct_idx>&& ecs_struct_idx_vec);

	entity_idx				new_entity(scene_idx ecs_scene_idx, world_idx ecs_world_idx, archetype_t archetype = 0ull);
	void					delete_entity(scene_idx ecs_scene_idx, world_idx ecs_world_idx, entity_idx ecs_entity_idx);
	void					add_component(scene_idx ecs_scene_idx, world_idx ecs_world_idx, entity_idx ecs_entity_idx, struct_idx ecs_struct_idx);
	archetype_t				get_archetype(scene_idx ecs_scene_idx, world_idx ecs_world_idx, entity_idx ecs_entity_idx);
	std::vector<struct_idx> get_struct_idx_vec(scene_idx ecs_scene_idx, world_idx ecs_world_idx, archetype_t archetype);
	void					remove_component(scene_idx ecs_scene_idx, world_idx ecs_world_idx, entity_idx ecs_entity_idx, struct_idx ecs_struct_idx);
	size_t					get_archetype_size(scene_idx, world_idx, archetype_t);
	size_t					get_struct_size(struct_idx ecs_struct_idx);
	void*					get_component_memory(scene_idx ecs_scene_idx, world_idx ecs_world_idx, entity_idx ecs_entity_idx, struct_idx ecs_struct_idx);
	void					copy_archetype_memory(void* p_dest, scene_idx ecs_scene_idx, world_idx ecs_world_idx, entity_idx ecs_entity_idx);

	std::vector<void*> get_components(editor_id entity_id);
	void			   set_components(editor_id entity_id, uint64 component_idx, void* p_value);
}	 // namespace editor::game::ecs

namespace editor::view::project_browser
{
	void update_dpi_scale();
	void show();
}	 // namespace editor::view::project_browser

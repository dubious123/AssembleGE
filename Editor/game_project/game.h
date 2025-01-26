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
	bool init_from_dll(HMODULE proj_dll);
	bool init_from_project_data(std::string& project_file_path);

	bool update_models();	 // for runtime
	void clear_models();

	uint64 new_entity(uint32 ecs_scene_idx, uint32 ecs_world_idx, uint64 archetype = 0ull);
	void   delete_entity(uint32 ecs_scene_idx, uint32 ecs_world_idx, uint64 ecs_entity_idx);
	void   add_component(uint32 ecs_scene_idx, uint32 ecs_world_idx, uint64 ecs_entity_idx, uint64 ecs_struct_idx);
	void   remove_component(uint32 ecs_scene_idx, uint32 ecs_world_idx, uint64 ecs_entity_idx, uint64 ecs_struct_idx);
	void*  get_component_memory(uint32 ecs_scene_idx, uint32 ecs_world_idx, uint64 ecs_entity_idx, uint64 ecs_struct_idx);

	std::vector<void*> get_components(editor_id entity_id);
	void			   set_components(editor_id entity_id, uint64 component_idx, void* p_value);
}	 // namespace editor::game::ecs

namespace editor::view::project_browser
{
	void update_dpi_scale();
	void show();
}	 // namespace editor::view::project_browser

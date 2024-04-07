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

	void save_project_open_datas();

	bool save();

	game_project* get_current_p_project();
}	 // namespace editor::game

namespace editor::game::ecs
{
	/*bool is_entity_alive(editor_id entity_id);*/
	// what we know
	// 1. struct name
	// 2. fields count
	// 3. fields type (const char*)
	// 4. fileds name
	// 5. field offsets
	// 5. fields value(const char*)
	// 6. entity key => archetype, memory block idx, memory idx

	// what we need
	// draw field(field type, void* p_data)
	// draw component(editor_id struct_id, void* p_data)
	bool init(HMODULE proj_dll);

	bool			   update_models();	   // for runtime
	void*			   get_p_component(editor_id entity_id, uint64 component_idx);
	std::vector<void*> get_p_components(editor_id entity_id);
	void			   set_components(editor_id entity_id, uint64 component_idx, void* p_value);
}	 // namespace editor::game::ecs

namespace editor::view::project_browser
{
	void update_dpi_scale();
	void show();
}	 // namespace editor::view::project_browser

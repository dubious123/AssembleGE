#pragma once
#include <string>

namespace editor::game
{
#define SCENE_NAME_MAX_LEN 20
	using namespace editor::models;

	struct project_open_data
	{
		editor_id	id;
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

namespace editor::view::project_browser
{
	void update_dpi_scale();
	void show();
}	 // namespace editor::view::project_browser

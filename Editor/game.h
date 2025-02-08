#pragma once

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

		project_open_data(const char* name, const char* desc, const char* date, const char* path, bool starred) : name(name), desc(desc), last_opened_date(date), path(path), starred(starred) { }
	};

	struct game_project
	{
		editor_id	id;
		std::string directory_path;
		std::string project_file_path;
		std::string name;
		std::string description;
		std::string last_opened_date;
		bool		is_ready = false;

		pugi::xml_document project_data_xml;
	};

	void init();

	void deinit();

	bool project_opened();

	void on_project_loaded();

	void on_project_unloaded();

	void save_project_open_datas();

	bool save();

	void update_proj_data(editor_id model_id, std::function<void(pugi::xml_node)>);

	game_project* get_pproject();
}	 // namespace editor::game

// reflection
namespace editor::game::ecs
{
	struct_idx new_struct();
	field_idx  add_field(struct_idx, e_primitive_type f_type, std::string field_value);
	void*	   get_field_pvalue(struct_idx, field_idx);
}	 // namespace editor::game::ecs

namespace editor::game::ecs
{
	bool init_from_dll(HMODULE proj_dll);
	bool init_from_project_data(std::string& project_file_path);

	bool update_models();	 // for runtime
	void clear_models();

	scene_idx new_scene();
	void	  delete_scene(scene_idx ecs_scene_idx, utilities::memory_handle* p_backup);
	void	  restore_scene(scene_idx, utilities::memory_handle* p_backup);

	world_idx new_world(scene_idx ecs_scene_idx);
	void	  delete_world(scene_idx, world_idx, utilities::memory_handle* p_backup = nullptr);
	void	  restore_world(scene_idx, world_idx, utilities::memory_handle* p_backup);
	/// <summary>
	/// assume the world does not contain the struct
	/// </summary>
	void world_add_struct(scene_idx, world_idx, struct_idx);
	/// <summary>
	/// <para> assume the world contains the struct </para>
	///
	/// <para> assume no entities exists with the struct to remove </para>
	/// </summary>
	void world_remove_struct(scene_idx, world_idx, struct_idx);

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
	void					restore_archetype_memory(scene_idx ecs_scene_idx, world_idx ecs_world_idx, entity_idx ecs_entity_idx, void* p_src);

	std::vector<void*> get_components(editor_id entity_id);
	void			   set_components(editor_id entity_id, uint64 component_idx, void* p_value);

	archetype_t calc_archetype_remove_component(archetype_t, uint8 nth_component);
}	 // namespace editor::game::ecs

// helper functions
namespace editor::game::ecs
{
	world_idx new_world(editor_id s_id);
	void	  delete_world(editor_id s_id, editor_id w_id);

	entity_idx new_entity(editor_id w_id, ecs::archetype_t a = 0ull);
	void	   delete_entity(editor_id e_id);

	void world_add_struct(editor_id w_id, editor_id s_id);
	void world_remove_struct(editor_id w_id, editor_id s_id);
}	 // namespace editor::game::ecs

namespace editor::game::code
{
	void init();
	bool open_visual_studio();
	void close_visual_studio();
	bool visual_studio_open_file(const char* filename, unsigned int line);
	void deinit();
	void on_project_loaded();
}	 // namespace editor::game::code

namespace editor::view::project_browser
{
	void update_dpi_scale();
	void show();
}	 // namespace editor::view::project_browser

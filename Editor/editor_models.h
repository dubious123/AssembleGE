#pragma once

namespace editor::models
{
	using archetype_t = uint64;

	struct project_open_data;
	struct game_project;

	struct em_field;
	struct em_struct;
	struct em_scene;
	struct em_world;
	struct em_subworld;
	struct em_entity;
	struct em_component;
	struct em_system;

	struct em_struct
	{
		editor_id	id;
		uint64		hash_id;
		std::string name;
		uint64		field_count;
		size_t		size = 0;

		void* p_default_value;

		uint64 ecs_idx;
	};

	struct em_field
	{
		editor_id		 id;
		editor_id		 struct_id;
		e_primitive_type type;
		size_t			 offset;
		// void*			 p_value;
		uint32		ecs_idx;
		std::string name;
	};

	struct em_scene
	{
		editor_id	id;
		std::string name;

		uint32 ecs_idx;
	};

	struct em_world
	{
		editor_id			   id;
		editor_id			   scene_id;
		std::string			   name;
		std::vector<editor_id> structs;

		uint32 ecs_idx;
	};

	struct em_subworld
	{
		std::string name;
	};

	struct em_entity
	{
		editor_id	id;
		editor_id	world_id;
		std::string name;
		uint64		archetype;

		uint64 ecs_idx;
	};

	struct em_component
	{
		editor_id id;
		editor_id struct_id;
		editor_id entity_id;
	};

	struct em_system
	{
		editor_id	world_id;
		std::string name;
	};

	namespace reflection
	{
		em_struct* find_struct(editor_id id);
		em_struct* find_struct(const char* name);
		editor_id  create_struct(std::string name, uint64 hash_id, uint64 ecs_idx);
		void	   remove_struct(editor_id struct_id);

		em_field*			   find_field(editor_id id);
		std::vector<em_field*> find_fields(std::vector<editor_id> struct_id_vec);
		editor_id			   add_field(editor_id struct_id, e_primitive_type, std::string name, uint32 offset, uint32 ecs_idx);
		void				   remove_field(editor_id field_id);

		std::vector<em_struct*> all_structs();
		std::vector<em_field*>	all_fields(editor_id struct_id);
	};	  // namespace reflection

	namespace reflection::utils
	{
		e_primitive_type string_to_type(const char*);
		e_primitive_type string_to_type(std::string);
		size_t			 type_size(e_primitive_type type);
		const char*		 type_to_string(e_primitive_type type);
		// void					 serialize(e_primitive_type type, const char* s_str, void* p_mem);
		void					 serialize(e_primitive_type type, std::string s_str, void* p_mem);
		std::string				 deserialize(e_primitive_type type, const void* ptr);
		std::vector<std::string> deserialize(editor_id struct_id);
		std::vector<std::string> deserialize(editor_id struct_id, const void* ptr);
	}	 // namespace reflection::utils

	namespace scene
	{
		extern editor_command cmd_create;
		extern editor_command cmd_remove;
		extern editor_command cmd_set_current;
		// editor_id => idx => scene_key_list
		//
		// remove => increase generation => different id
		em_scene*			   find(editor_id id);
		editor_id			   create(std::string name, uint16 s_ecs_idx);
		void				   remove(editor_id id);
		editor_id			   restore(const em_scene& em_s);
		size_t				   count();
		std::vector<em_scene*> all();
		void				   set_current(editor_id id);

		em_scene* get_current();
	}	 // namespace scene

	namespace world
	{
		extern editor_command cmd_create;
		extern editor_command cmd_remove;
		extern editor_command cmd_add_struct;
		extern editor_command cmd_remove_struct;

		em_world*			   find(editor_id id);
		editor_id			   create(editor_id scene_id, std::string name, uint16 ecs_world_idx);
		void				   add_struct(editor_id world_id, editor_id struct_id);
		void				   remove_struct(editor_id world_id, editor_id struct_id);
		uint64				   archetype(editor_id world_id, editor_id struct_id);
		void				   remove(editor_id world_id);
		editor_id			   restore(const em_world& em_w);
		std::vector<em_world*> all(editor_id scene_id);
	}	 // namespace world

	namespace entity
	{
		extern editor_command cmd_create;
		extern editor_command cmd_remove;
		extern editor_command cmd_create_empty;

		em_entity*				find(editor_id id);
		editor_id				create(editor_id world_id, archetype_t archetype, uint64 ecs_e_idx);
		editor_id				create(editor_id world_id, std::string name, archetype_t archetype, uint64 ecs_e_idx);
		void					remove(editor_id entity_id);
		editor_id				restore(const em_entity& em_c);
		editor_id				add_component(editor_id entity_id, editor_id struct_id);
		void					remove_component(editor_id entity_id, editor_id struct_id);
		std::vector<em_entity*> all(editor_id world_id);
		size_t					count(editor_id world_id);
	}	 // namespace entity

	namespace component
	{
		em_component*			   find(editor_id id);
		em_component*			   find(editor_id entity_id, editor_id struct_id);
		editor_id				   create(editor_id entity_id, editor_id struct_id);
		void					   remove(editor_id component_id);
		void					   restore(const em_component& em_c);
		std::vector<em_component*> all(editor_id entity_id);
		size_t					   count(editor_id entity_id);

		void* get_memory(editor_id id);
	}	 // namespace component

	namespace text
	{
		editor_id	create(const char* text);
		void		remove(editor_id id);
		const char* find(editor_id id);
	}	 // namespace text

	extern editor_command cmd_rename_selection;

	void*		 find(editor_id);
	std::string* get_name(editor_id);

	// any change from editor needs to be applied to project exists?
	bool change_exists();

	void on_project_loaded();
	void on_project_unloaded();

}	 // namespace editor::models
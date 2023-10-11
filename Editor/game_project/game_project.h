#pragma once
#include <string>
#include <vector>
#include "../utilities/id.h"

namespace Editor::GameProject
{
#define SCENE_NAME_MAX_LEN 20

	enum ComponentType
	{
		ComponentType_Transform,
		ComponentType_Player,
		ComponentType_Count,
		ComponentType_InValid = -1,
	};

	enum SystemType
	{
		SystemType_Count,
		SystemType_InValid = -1,
	};

	struct Project_Open_Data;
	struct Game_Project;
	struct Scene;
	struct World;
	struct SubWorld;
	struct Entity;
	struct Component;
	struct System;

	struct Project_Open_Data
	{
		Editor::ID::AssembleID Id;
		std::string			   Name;
		std::string			   Desc;
		std::string			   Last_Opened_Date;
		std::string			   Path;
		bool				   Starred = false;

		Project_Open_Data() = default;
		Project_Open_Data(const char* name, const char* desc, const char* date, const char* path, bool starred);
	};

	struct Game_Project
	{
		Editor::ID::AssembleID Id;
		std::string			   Project_Data_File_Path;
		std::string			   Name;
		std::string			   Description;
		std::string			   Last_Opened_Date;
		std::vector<Scene>	   Scenes;
		bool				   Ready = false;
	};

	struct Scene
	{
		Editor::ID::AssembleID Id;
		std::string			   Name;
		std::vector<World>	   Worlds;
	};

	struct World
	{
		Editor::ID::AssembleID Id;
		std::string			   Name;
		std::vector<Entity>	   Entities;
		std::vector<System>	   Systems;
		std::vector<SubWorld>  SubWorlds;
	};

	struct SubWorld
	{
		Editor::ID::AssembleID Id;
		std::string			   Name;
	};

	struct Entity
	{
		Editor::ID::AssembleID Id;
		std::string			   Name;
		std::vector<Component> Components;
	};

	struct Component
	{
		Editor::ID::AssembleID Id;
		std::string			   Name;
	};

	struct System
	{
		Editor::ID::AssembleID Id;
		std::string			   Name;
	};

	void Init();

	bool Project_Opened();

	void Save_Project_Open_Datas();

	bool Save();

	const Game_Project* Get_Current_PProject();

	bool Add_Scene(std::string name);

	bool Add_World(std::string name, Editor::ID::AssembleID scene_id);

	bool Add_SubWorld(std::string name, Editor::ID::AssembleID world_id);

	bool Add_Entity(std::string name, Editor::ID::AssembleID world_id);

	bool Add_System(SystemType system_type, Editor::ID::AssembleID world_id);

	bool Add_Component(ComponentType component_type, Editor::ID::AssembleID entity_id);
}	 // namespace Editor::GameProject

namespace Editor::GameProject::Browser
{
	void Update_Dpi_Scale();
	void Draw();
}	 // namespace Editor::GameProject::Browser

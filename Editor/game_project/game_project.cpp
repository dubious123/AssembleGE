#include <Windows.h>
#include <array>
#include <shlobj_core.h>
#include <filesystem>
#include <fstream>
#include <chrono>

#include "../editor.h"
#include "../editor_common.h"
#include "game_project.h"
#include "pugixml/pugixml.hpp"
#include "nativefiledialog\nfd.h"
#include "../utilities/Timing.h"

namespace Editor::GameProject
{
	namespace
	{
		std::vector<Project_Open_Data> _project_open_datas;
		std::wstring				   _application_data_directory_path;
		std::wstring				   _project_open_data_path;
		Game_Project				   _current_project;
		pugi::xml_document			   _project_open_data_xml;
		pugi::xml_node				   _project_list_node;

		void _load_project_open_datas()
		{
			_project_open_datas.clear();
			for (auto project_node : _project_list_node)
			{
				_project_open_datas.emplace_back(Project_Open_Data(
					project_node.attribute("name").value(),
					project_node.attribute("desc").value(),
					project_node.attribute("last_opened_date").value(),
					project_node.attribute("path").value(),
					project_node.attribute("starred").as_bool()));
			}
		}
	}	 // namespace

	Project_Open_Data::Project_Open_Data(const char* name, const char* desc, const char* date, const char* path, bool starred)
	{
		Id				 = Editor::ID::GetNew(DataType_Project);
		Name			 = name;
		Desc			 = desc;
		Last_Opened_Date = date;
		Path			 = path;
		Starred			 = starred;
	}

	void Init()
	{
		PWSTR p_folder_path;
		::SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, NULL, &p_folder_path);
		_application_data_directory_path = std::wstring(p_folder_path) + std::wstring(L"\\AssembleGE\\");
		_project_open_data_path			 = _application_data_directory_path + L"project_open_data.xml";
		if (std::filesystem::exists(_application_data_directory_path) is_false)
		{
			std::filesystem::create_directory(_application_data_directory_path);
			std::wofstream xml_stream(_project_open_data_path);
			xml_stream.close();
		}

		assert(_project_open_data_xml.load_file(_project_open_data_path.c_str()));
		_project_list_node = _project_open_data_xml.child("project_list");
		_load_project_open_datas();
		::CoTaskMemFree((void*)p_folder_path);
	}

	void Save_Project_Open_Datas()
	{
		std::wofstream xml_stream(_project_open_data_path);
		_project_open_data_xml.reset();
		_project_list_node = _project_open_data_xml.append_child("project_list");
		for (auto project_od : _project_open_datas)
		{
			auto project_node = _project_list_node.append_child("project");
			project_node.append_attribute("name").set_value(project_od.Name.c_str());
			project_node.append_attribute("desc").set_value(project_od.Desc.c_str());
			project_node.append_attribute("last_opened_date").set_value(project_od.Last_Opened_Date.c_str());
			project_node.append_attribute("path").set_value(project_od.Path.c_str());
			project_node.append_attribute("starred").set_value(project_od.Starred);
		}

		_project_open_data_xml.save(xml_stream);
		xml_stream.close();
	}

	bool Save()
	{
		auto doc		  = pugi::xml_document();
		auto success	  = doc.load_file(_current_project.Project_Data_File_Path.c_str());
		auto xml_stream	  = std::wofstream(_current_project.Project_Data_File_Path);
		auto project_node = doc.child("project");
		project_node.remove_children();

		auto scenes_node = project_node.append_child("scenes");
		for (auto scene : _current_project.Scenes)
		{
			auto scene_node	 = scenes_node.append_child("scene");
			auto worlds_node = scene_node.append_child("worlds");
			scene_node.append_attribute("id").set_value(scene.Id.ToString().c_str());
			scene_node.append_attribute("name").set_value(scene.Name.c_str());

			for (auto world : scene.Worlds)
			{
				auto world_node		 = worlds_node.append_child("world");
				auto entities_node	 = world_node.append_child("entities");
				auto systems_node	 = world_node.append_child("systems");
				auto sub_worlds_node = world_node.append_child("sub_worlds");

				world_node.append_attribute("id").set_value(world.Id.ToString().c_str());
				world_node.append_attribute("name").set_value(world.Name.c_str());

				for (auto entity : world.Entities)
				{
					auto entity_node	 = entities_node.append_child("entity");
					auto components_node = entity_node.append_child("components");
					entity_node.append_attribute("id").set_value(entity.Id.ToString().c_str());
					entity_node.append_attribute("name").set_value(entity.Name.c_str());

					for (auto component : entity.Components)
					{
						auto component_node = components_node.append_child("component");
						entity_node.append_attribute("id").set_value(entity.Id.ToString().c_str());
						entity_node.append_attribute("name").set_value(entity.Name.c_str());
					}
				}

				for (auto system : world.Systems)
				{
					auto system_node = systems_node.append_child("system");
					system_node.append_attribute("id").set_value(system.Id.ToString().c_str());
					system_node.append_attribute("name").set_value(system.Name.c_str());
				}

				for (auto sub_world : world.SubWorlds)
				{
					auto sub_world_node = sub_worlds_node.append_child("sub_world");
					sub_world_node.append_attribute("id").set_value(sub_world.Id.ToString().c_str());
					sub_world_node.append_attribute("name").set_value(sub_world.Name.c_str());
				}
			}
		}

		doc.save(xml_stream);
		xml_stream.close();

		Editor::Logger::Info("Project save completed");
		return success;
	}

	bool Open(std::filesystem::path project_directory_path)
	{
		auto project_data_path = std::filesystem::path(project_directory_path)
									 .append(PROJECT_EXTENSION)
									 .append(PROJECT_DATA_FILE_NAME);
		auto doc		  = pugi::xml_document();
		bool success	  = doc.load_file(project_data_path.c_str());
		auto project_node = doc.child("project");

		success &= project_node.attribute("path").empty() is_false;
		success &= project_node.attribute("name").empty() is_false;
		success &= project_node.attribute("desc").empty() is_false;
		success &= project_node.attribute("last_opened_date").empty() is_false;

		if (!success) return false;

		_current_project.Project_Data_File_Path = project_data_path.string();
		_current_project.Name					= project_node.attribute("name").value();
		_current_project.Description			= project_node.attribute("desc").value();
		_current_project.Last_Opened_Date		= Editor::Utilities::Timing::Now_Str();
		_current_project.Ready					= true;

		for (auto& data : _project_open_datas)
		{
			if (_current_project.Name == data.Name)
			{
				data.Last_Opened_Date = _current_project.Last_Opened_Date;
				return true;
			}
		}

		Project_Open_Data od;
		od.Name				= _current_project.Name;
		od.Desc				= _current_project.Description;
		od.Last_Opened_Date = _current_project.Last_Opened_Date;
		od.Path.assign(project_directory_path.native().begin(), project_directory_path.native().end());
		od.Starred = false;

		_project_open_datas.emplace_back(od);

		return success;
	}

	bool Project_Opened()
	{
		return _current_project.Ready;
	}

	const Game_Project* Get_Current_PProject()
	{
		return &_current_project;
	}

	bool Create(const char* proj_name, const char* path, char* err_msg = nullptr)
	{
		constexpr int min_proj_name_len = 3;
		char		  bad_chars[]		= { '!', '@', '%', '^', '*', '~', '|' };
		bool		  valid_path		= std::filesystem::is_directory(path) and std::filesystem::exists(path);
		bool		  valid_name		= true;
		bool		  success			= true;

		std::string proj_name_str(proj_name);
		std::string solution_template_str;
		std::string project_template_str;
		std::string project_data_template_str;
		std::string arg_0, arg_1, arg_2, arg_3;

		std::filesystem::path directory_path;
		std::filesystem::path gamecode_directory_path;
		std::filesystem::path internal_project_dir_path;
		std::filesystem::path solution_template_path;
		std::filesystem::path project_template_path;
		std::filesystem::path project_data_template_path;

		for (auto c : bad_chars)
		{
			if (proj_name_str.find(proj_name_str, c) != -1)
			{
				valid_name = false;
				break;
			}
		}

		valid_name &= proj_name_str.length() >= min_proj_name_len;

		if (not valid_path or not valid_name) return false;

		// create directory
		{
			directory_path = std::filesystem::path(path);
			directory_path.append(proj_name_str);
			gamecode_directory_path = std::filesystem::path(directory_path);
			gamecode_directory_path.append(GAMECODE_DIRECTORY);
			internal_project_dir_path = std::filesystem::path(directory_path);
			internal_project_dir_path.append(PROJECT_EXTENSION);
			success &= std::filesystem::create_directory(directory_path);
			success &= std::filesystem::create_directory(gamecode_directory_path);
			success &= std::filesystem::create_directory(internal_project_dir_path);
			::SetFileAttributes(internal_project_dir_path.c_str(), FILE_ATTRIBUTE_HIDDEN);
		}

		// get template str
		{
			TCHAR current_dirctory_path[MAX_PATH];
			::GetCurrentDirectory(MAX_PATH, current_dirctory_path);

			{
				solution_template_path = std::filesystem::path(current_dirctory_path).append("project_template\\").append("msvc_solution");
				std::ifstream solution_template;
				solution_template.open(solution_template_path);
				std::stringstream ss_solution;
				ss_solution << solution_template.rdbuf();
				solution_template_str = ss_solution.str();
			}

			{
				project_template_path = std::filesystem::path(current_dirctory_path).append("project_template\\").append("msvc_project");
				std::ifstream project_template;
				project_template.open(project_template_path);
				std::stringstream ss_project;
				ss_project << project_template.rdbuf();
				project_template_str = ss_project.str();
			}

			{
				project_data_template_path = std::filesystem::path(current_dirctory_path).append("project_template\\").append("project_data");
				std::ifstream project_data_template;
				project_data_template.open(project_data_template_path);
				std::stringstream ss_project_data;
				ss_project_data << project_data_template.rdbuf();
				project_data_template_str = ss_project_data.str();
			}
		}

		// get template arguments
		{
			UUID		new_id;
			RPC_CSTR	sz_uuid = nullptr;
			std::string uuid_str_0;
			std::string uuid_str_1;
			auto		uuid_res = ::UuidCreate(&new_id);
			assert(uuid_res == RPC_S_OK);
			assert(UuidToStringA(&new_id, &sz_uuid) == RPC_S_OK);
			uuid_str_0 = std::string((char*)sz_uuid);
			for (int i = 0; i < uuid_str_0.length(); i++)
			{
				uuid_str_0[i] = toupper(uuid_str_0[i]);
			}

			::RpcStringFreeA(&sz_uuid);

			uuid_res = ::UuidCreate(&new_id);
			assert(uuid_res == RPC_S_OK);
			assert(UuidToStringA(&new_id, &sz_uuid) == RPC_S_OK);
			uuid_str_1 = std::string((char*)sz_uuid);
			for (int i = 0; i < uuid_str_0.length(); i++)
			{
				uuid_str_1[i] = toupper(uuid_str_1[i]);
			}

			::RpcStringFreeA(&sz_uuid);

			arg_0 = proj_name_str;
			arg_1 = "{" + uuid_str_0 + "}";
			arg_2 = "{" + uuid_str_1 + "}";
			arg_3 = directory_path.string();
		}

		// create msvc solution file
		{
			auto solution_file_content = std::vformat(solution_template_str, std::make_format_args(arg_0, arg_1, arg_2));
			auto solution_file_path	   = std::filesystem::path(directory_path);

			solution_file_path.append(proj_name + std::string(".sln"));
			std::wofstream solution_file(solution_file_path);
			solution_file << solution_file_content.c_str();
			solution_file.close();
		}

		// create msvc project file
		{
			auto project_file_content = std::vformat(project_template_str, std::make_format_args(arg_0, arg_1, arg_2, arg_3));
			auto project_file_path	  = std::filesystem::path(gamecode_directory_path);

			project_file_path.append(proj_name + std::string(".vcxproj"));
			std::wofstream project_file(project_file_path);
			project_file << project_file_content.c_str();
			project_file.close();
		}

		// create project data
		{
			auto project_data_content = std::vformat(project_data_template_str, std::make_format_args(arg_0, arg_3));
			auto project_data_path	  = std::filesystem::path(internal_project_dir_path);

			project_data_path.append(PROJECT_DATA_FILE_NAME);
			std::wofstream project_file(project_data_path);
			project_file << project_data_content.c_str();
			project_file.close();
		}

		assert(success);
		return success;
	}

	bool Add_Scene(std::string name)
	{
		if (name.empty())
		{
			Editor::Logger::Warn("Scene name is empty");
			return false;
		}

		for (auto scene : _current_project.Scenes)
		{
			if (scene.Name == name)
			{
				Editor::Logger::Warn("Scenes can not have the same name");
				return false;
			}
		}

		auto scene = Scene();
		scene.Id   = Editor::ID::GetNew(DataType_Scene);
		scene.Name = name;

		_current_project.Scenes.emplace_back(scene);
		return true;
	}
}	 // namespace Editor::GameProject

namespace Editor::GameProject::Browser
{
	namespace
	{
		constexpr size_t _char_buffer_size = 100;

		Project_Open_Data* _p_selected_project_open_data = nullptr;

		char _buffer[_char_buffer_size];

		ImVec2 _item_size;
		ImVec2 _header_size;
		ImVec2 _next_draw_pos;
		ImVec2 _project_list_child_window_size;
		ImVec2 _draw_pos_item;
		ImVec2 _draw_pos_star;
		ImVec2 _draw_pos_name;
		ImVec2 _draw_pos_desc;
		ImVec2 _draw_pos_last_opened;
		ImVec2 _draw_pos_path;

		float _header_height;
		float _width_star;
		float _width_path;
		float _width_name_desc;
		float _width_last_opened;
		float _item_rounding;

		bool _project_need_sort				 = false;
		bool _open_dialog_new_project		 = false;
		bool _open_dialog_add_project		 = false;
		bool _open_popup_open_project_failed = false;
		bool _open_popup_add_project_failed	 = false;

		consteval std::array<ImVec2, 10> _cal_star_points_pos_rel(const float outer_radius, const float inner_radius)
		{
			float COS_PI_0_2 = 0.80901699437f;
			float COS_PI_0_4 = 0.30901699437f;
			float SIN_PI_0_2 = 0.58778525229f;
			float SIN_PI_0_4 = 0.95105651629f;

			std::array<ImVec2, 10> points;
			ImVec2				   p_out = ImVec2(0, outer_radius);
			ImVec2				   p_in	 = ImVec2(0 * COS_PI_0_2 - inner_radius * SIN_PI_0_2, -0 * SIN_PI_0_2 + inner_radius * COS_PI_0_2);

			for (int i = 0; i < 5; ++i)
			{
				points[i * 2]	  = p_in;
				points[i * 2 + 1] = p_out;
				p_in			  = ImVec2(p_in.x * COS_PI_0_4 + p_in.y * SIN_PI_0_4, -p_in.x * SIN_PI_0_4 + p_in.y * COS_PI_0_4);
				p_out			  = ImVec2(p_out.x * COS_PI_0_4 + p_out.y * SIN_PI_0_4, -p_out.x * SIN_PI_0_4 + p_out.y * COS_PI_0_4);
			}

			auto offset_y = (points[1].y + points[5].y) * 0.5f;
			for (int i = 0; i < 9; ++i)
			{
				points[i].y -= offset_y;
			}

			return points;
		}

		void _draw_star(ImDrawList* p_draw_list, ImVec2 pos_abs, float size, ImColor col)
		{
			static constexpr auto star_offsets = _cal_star_points_pos_rel(1, 0.5f);

			ImVec2 star_points_abs[10];
			for (int i = 0; i < 10; ++i)
			{
				star_points_abs[i] = pos_abs - ImVec2(star_offsets[i].x * size, star_offsets[i].y * size);
			}

			p_draw_list->AddConvexPolyFilled(star_points_abs, 10, col);
		}

		void _sort_project_open_data()
		{
			int star_index = 0;
			for (int i = 0; i < _project_open_datas.size(); ++i)
			{
				auto project = _project_open_datas[i];
				if (project.Starred)
				{
					if (i != star_index)
					{
						std::swap(_project_open_datas[i], _project_open_datas[star_index]);
					}

					++star_index;
				}
			}
		}

		void _item_project_open_data(Project_Open_Data& project_od)
		{
			auto   p_draw_list = ImGui::GetCurrentWindowRead()->DrawList;
			bool   item_hovered, item_clicked, star_hovered, star_clicked;
			ImVec2 local_pos = _next_draw_pos - ImGui::GetCurrentWindowRead()->Pos;	   // ImVec2(_next_item_pos.x - ImGui::GetCurrentWindowRead()->Pos.x, _next_item_pos.y - ImGui::GetCurrentWindowRead()->Pos.y);

			ImGui::ItemAdd(ImRect(_next_draw_pos + _draw_pos_star, _next_draw_pos + _draw_pos_star + ImVec2(_width_star, _width_star)), ImGui::GetID(project_od.Name.c_str()));
			star_hovered = ImGui::IsItemHovered();
			star_clicked = ImGui::IsItemClicked(ImGuiMouseButton_Left);

			ImGui::ItemAdd(ImRect(_next_draw_pos + _draw_pos_item, _next_draw_pos + _draw_pos_item + _item_size), ImGui::GetID("##item_rect"));
			item_hovered = ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
			item_clicked = ImGui::IsItemClicked(ImGuiMouseButton_Left) and not star_clicked;

			if (item_hovered)
			{
				p_draw_list->AddRectFilled(_next_draw_pos + _draw_pos_item, _next_draw_pos + _draw_pos_item + _item_size, ImColor(COL_GRAY_2), _item_rounding);
				if (star_hovered)
				{
					p_draw_list->AddRectFilled(_next_draw_pos + _draw_pos_star, _next_draw_pos + _draw_pos_star + ImVec2(_width_star, _width_star), ImColor(COL_GRAY_3), _item_rounding);
					_draw_star(p_draw_list, _next_draw_pos + _draw_pos_star + ImVec2(_width_star, _width_star) * 0.5f, _width_star * 0.2f, ImColor(COL_GRAY_8));
				}
				else
				{
					_draw_star(p_draw_list, _next_draw_pos + _draw_pos_star + ImVec2(_width_star, _width_star) * 0.5f, _width_star * 0.2f, ImColor(COL_TEXT_GRAY));
					// p_draw_list->AddRectFilled(_next_item_pos + _draw_pos_star, _next_item_pos + _draw_pos_star + ImVec2(_width_star, _width_star), ImColor(COL_GRAY_3));
				}
			}
			else if (project_od.Starred)
			{
				_draw_star(p_draw_list, _next_draw_pos + _draw_pos_star + ImVec2(_width_star, _width_star) * 0.5f, _width_star * 0.2f, ImColor(COL_TEXT_GRAY));
			}

			if (star_clicked)
			{
				project_od.Starred = !project_od.Starred;
				_project_need_sort = true;
			}

			if (item_clicked)
			{
				_p_selected_project_open_data = &project_od;

				bool success = Open(project_od.Path);
				if (success)
				{
					project_od.Name				= _current_project.Name;
					project_od.Desc				= _current_project.Description;
					project_od.Last_Opened_Date = _current_project.Last_Opened_Date;
					_project_need_sort			= true;
				}
				else
				{
					_open_popup_open_project_failed = true;
				}
			}

			ImGui::PushFont(GEctx->P_Font_Arial_Bold_13_5);
			p_draw_list->AddText(_next_draw_pos + _draw_pos_name, ImColor(GImGui->Style.Colors[ImGuiCol_Text]), project_od.Name.c_str());
			ImGui::PopFont();

			p_draw_list->AddText(_next_draw_pos + _draw_pos_desc, ImColor(COL_TEXT_GRAY), project_od.Desc.c_str());
			p_draw_list->AddText(_next_draw_pos + _draw_pos_last_opened, ImColor(COL_TEXT_GRAY), project_od.Last_Opened_Date.c_str());

			ImVec2 path_text_size = ImGui::CalcTextSize(project_od.Path.c_str());
			if (path_text_size.x > _width_path)
			{
				auto target_text_width = _width_path - ImGui::CalcTextSize("...").x;
				auto location_text_end = &project_od.Path[project_od.Path.length() - 1];
				while (path_text_size.x > target_text_width)
				{
					const char	char_end	= *(location_text_end--);
					const float char_width	= (int)char_end < GEctx->P_Font_Arial_Default_13_5->IndexAdvanceX.Size ? GEctx->P_Font_Arial_Default_13_5->IndexAdvanceX.Data[char_end] : GEctx->P_Font_Arial_Default_13_5->FallbackAdvanceX;
					path_text_size.x	   -= char_width;
				}

				p_draw_list->AddText(_next_draw_pos + _draw_pos_path, ImColor(COL_TEXT_GRAY), project_od.Path.c_str(), location_text_end + 1);
				p_draw_list->AddText(_next_draw_pos + _draw_pos_path + ImVec2(path_text_size.x, 0), ImColor(COL_TEXT_GRAY), "...");
			}
			else
			{
				p_draw_list->AddText(_next_draw_pos + _draw_pos_path, ImColor(COL_TEXT_GRAY), project_od.Path.c_str());
			}

			ImGui::SetCursorPos(local_pos + _draw_pos_item);
			ImGui::ItemSize(_item_size);
			_next_draw_pos.y += _item_size.y;	 // +GImGui->Style.ItemSpacing.y;
		}

		void _dialog_new_project()
		{
			static constexpr int str_name_max_len = 20;
			static char			 new_project_name[str_name_max_len];
			static std::string	 new_project_path;
			static bool			 show_error_msg = false;
			float				 button_size_x;
			ImVec2				 input_text_size;
			ImRect				 input_text_newProject_rect;

			if (ImGui::BeginPopupModal("New Project", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings))
			{
				ImGui::SetCursorPosY(ImGui::GetCursorPosY() + GImGui->Style.FramePadding.y);
				ImGui::Text("Project Name");
				button_size_x = ImGui::GetItemRectSize().x;

				ImGui::SameLine();

				ImGui::SetCursorPosY(ImGui::GetCursorPosY() - GImGui->Style.FramePadding.y);
				ImGui::InputText("##new project name", new_project_name, str_name_max_len);
				input_text_size			   = ImGui::GetItemRectSize();
				input_text_newProject_rect = ImRect(ImGui::GetItemRectMin() + ImVec2(0, input_text_size.y + GImGui->Style.ItemSpacing.y), ImGui::GetItemRectMax() + ImVec2(0, input_text_size.y + GImGui->Style.ItemSpacing.y));
				ImGui::SetCursorPosY(ImGui::GetCursorPosY() + GImGui->Style.FramePadding.y);
				ImGui::Text("Project Path");
				ImGui::SameLine(button_size_x + GImGui->Style.ItemSpacing.x + GImGui->Style.WindowPadding.x);

				ImGui::SetCursorPosY(ImGui::GetCursorPosY() - GImGui->Style.FramePadding.y);
				ImGui::ItemAdd(input_text_newProject_rect, ImGui::GetCurrentWindowRead()->GetID("##new project path"));
				ImGui::ItemSize(input_text_newProject_rect);

				ImGui::RenderFrame(input_text_newProject_rect.Min, input_text_newProject_rect.Max, ImGui::GetColorU32(ImGuiCol_FrameBg), true, GImGui->Style.FrameRounding);
				ImGui::SetCursorPos((ImGui::GetCursorPos() + GImGui->Style.FramePadding));
				auto open_dialog_directory = ImGui::IsItemClicked();
				auto path_text_size		   = ImGui::CalcTextSize(new_project_path.c_str());
				auto path_text_need_clip   = path_text_size.x > input_text_newProject_rect.GetSize().x;
				auto p_draw_list		   = ImGui::GetWindowDrawList();
				if (path_text_need_clip)
				{
					auto target_text_width	  = input_text_newProject_rect.GetSize().x - GImGui->Style.FramePadding.x * 2 - ImGui::CalcTextSize("...").x;
					auto new_project_text_end = new_project_path.end() - 1;
					while (path_text_size.x > target_text_width)
					{
						const char	char_end	= *(new_project_text_end--);
						const float char_width	= (int)char_end < GEctx->P_Font_Arial_Default_13_5->IndexAdvanceX.Size ? GEctx->P_Font_Arial_Default_13_5->IndexAdvanceX.Data[char_end] : GEctx->P_Font_Arial_Default_13_5->FallbackAdvanceX;
						path_text_size.x	   -= char_width;
					}
					p_draw_list->AddText(input_text_newProject_rect.Min + GImGui->Style.FramePadding, ImColor(COL_TEXT), new_project_path.c_str(), new_project_text_end._Ptr + 1);
					p_draw_list->AddText(input_text_newProject_rect.Min + GImGui->Style.FramePadding + ImVec2(path_text_size.x, 0), ImColor(COL_TEXT), "...");
				}
				else
				{
					ImGui::RenderTextClipped(input_text_newProject_rect.Min + GImGui->Style.FramePadding, input_text_newProject_rect.Max - GImGui->Style.FramePadding, new_project_path.c_str(), nullptr, 0, ImVec2(), &input_text_newProject_rect);
				}

				if (open_dialog_directory)
				{
					nfdchar_t* outPath = nullptr;

					nfdresult_t result = NFD_PickFolder(nullptr, &outPath);
					assert(result != NFD_ERROR);
					if (result == NFD_OKAY)
					{
						new_project_path = std::string(outPath);

						free(outPath);
					}

					GImGui->IO.AddMouseSourceEvent(ImGuiMouseSource_Mouse);
					GImGui->IO.AddMouseButtonEvent(0, false);
				}

				ImGui::Separator();

				ImGui::SetCursorPosY(ImGui::GetCursorPosY() + GImGui->Style.FramePadding.y);
				if (ImGui::Button("Create", ImVec2(button_size_x, 0)))
				{
					auto success = true;
					if (Create(new_project_name, new_project_path.c_str()))
					{
						// todo : error handling, is it possible to fail to open after create?
						assert(Open(std::filesystem::path(new_project_path).append(new_project_name)));
						_open_dialog_new_project = false;
						show_error_msg			 = false;
					}
					else
					{
						show_error_msg = true;
					}
				}

				ImGui::SameLine();
				if (ImGui::Button("Cancel", ImVec2(button_size_x, 0)))
				{
					_open_dialog_new_project = false;
					show_error_msg			 = false;
				}

				if (show_error_msg)
				{
					ImGui::TextColored(ImColor(COL_RED), "Create Project Failed");
				}

				ImGui::EndPopup();
			}

			ImGui::OpenPopup("New Project");
		}

		void _dialog_add_project()
		{
			std::string open_project_path;
			{
				nfdchar_t* outPath = nullptr;

				nfdresult_t result = NFD_PickFolder(nullptr, &outPath);
				assert(result != NFD_ERROR);
				if (result == NFD_OKAY)
				{
					open_project_path = std::string(outPath);
					free(outPath);
				}
			}

			if (open_project_path.empty() is_false and Open(open_project_path) is_false)
			{
				_open_popup_add_project_failed = true;
			}

			_open_dialog_add_project = false;
		}

		void _popup_open_project_failed()
		{
			if (ImGui::BeginPopupModal("Open Project Failed", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoTitleBar))
			{
				assert(_p_selected_project_open_data is_not_nullptr);
				{
					ImGui::PushFont(GEctx->P_Font_Arial_Bold_18);
					ImGui::Text("Project failed to open");
					ImGui::SetCursorPosY(ImGui::GetCursorPosY() + GImGui->Style.ItemSpacing.y);
					ImGui::PopFont();
				}

				ImGui::Separator();
				ImGui::SetCursorPosY(ImGui::GetCursorPosY() + GImGui->Style.ItemSpacing.y);
				ImGui::Text("%s could not be opened and will be removed from the list.", _p_selected_project_open_data->Name.c_str());
				ImGui::SetCursorPos(ImVec2(ImGui::GetItemRectSize().x - (ImGui::CalcTextSize("  Ok  ").x + GImGui->Style.FramePadding.x) /*+ GImGui->Style.FramePadding.x * 2*/, ImGui::GetCursorPosY() + GImGui->Style.ItemSpacing.y));
				if (ImGui::Button("  Ok  "))
				{
					_project_open_datas.erase(std::find_if(_project_open_datas.begin(), _project_open_datas.end(),
														   [](Project_Open_Data od)
														   { return od.Name == _p_selected_project_open_data->Name; }));

					_open_popup_open_project_failed = false;
					_p_selected_project_open_data	= nullptr;
				}

				ImGui::EndPopup();
			}

			ImGui::OpenPopup("Open Project Failed");
		}

		void _popup_add_project_failed()
		{
			if (ImGui::BeginPopupModal("Add Project Failed", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoTitleBar))
			{
				ImGui::PushFont(GEctx->P_Font_Arial_Bold_18);
				ImGui::Text("Project failed to add");
				ImGui::SetCursorPosY(ImGui::GetCursorPosY() + GImGui->Style.ItemSpacing.y);
				ImGui::PopFont();

				auto got_it_button_pos_x = ImGui::GetItemRectSize().x - GImGui->Style.FramePadding.x - ImGui::CalcTextSize("  Got it  ").x;
				ImGui::Separator();

				ImGui::SetCursorPosY(ImGui::GetCursorPosY() + GImGui->Style.ItemSpacing.y);
				ImGui::Text("Project is not valid");
				ImGui::SetCursorPos(ImVec2(got_it_button_pos_x, ImGui::GetCursorPosY() + GImGui->Style.ItemSpacing.y));
				if (ImGui::Button("  Got it  "))
				{
					_open_popup_add_project_failed = false;
					_p_selected_project_open_data  = nullptr;
				}

				ImGui::EndPopup();
			}

			ImGui::OpenPopup("Add Project Failed");
		}

		void _childwindow_project_list()
		{
			if (ImGui::BeginChild("Header", ImVec2(_project_list_child_window_size.x, _header_height + GImGui->Style.ItemSpacing.y * 4)))
			{
				_next_draw_pos = ImGui::GetCurrentWindowRead()->Pos + GImGui->Style.FramePadding;

				auto p_draw_list			= ImGui::GetCurrentWindowRead()->DrawList;
				auto item_spacing_x			= GImGui->Style.ItemSpacing.x;
				auto frame_padding_x		= GImGui->Style.FramePadding.x;
				auto draw_pos_header_text_y = (_header_height - GImGui->FontSize) / 2;

				p_draw_list->AddRectFilled(_next_draw_pos, _next_draw_pos + ImVec2(_header_size.x, _header_height), ImColor(COL_GRAY_1));
				p_draw_list->AddRect(_next_draw_pos, _next_draw_pos + ImVec2(_header_size.x, _header_height), ImColor(COL_TEXT_GRAY));
				p_draw_list->AddLine(_next_draw_pos + ImVec2(_draw_pos_star.x - frame_padding_x, 0), _next_draw_pos + ImVec2(_draw_pos_star.x - frame_padding_x, _header_height), ImColor(COL_TEXT_GRAY));
				p_draw_list->AddLine(_next_draw_pos + ImVec2(_draw_pos_name.x - frame_padding_x, 0), _next_draw_pos + ImVec2(_draw_pos_name.x - frame_padding_x, _header_height), ImColor(COL_TEXT_GRAY));
				p_draw_list->AddLine(_next_draw_pos + ImVec2(_draw_pos_last_opened.x - frame_padding_x, 0), _next_draw_pos + ImVec2(_draw_pos_last_opened.x - frame_padding_x, _header_height), ImColor(COL_TEXT_GRAY));
				p_draw_list->AddLine(_next_draw_pos + ImVec2(_draw_pos_path.x - frame_padding_x, 0), _next_draw_pos + ImVec2(_draw_pos_path.x - frame_padding_x, _header_height), ImColor(COL_TEXT_GRAY));
				p_draw_list->AddLine(_next_draw_pos + ImVec2(_draw_pos_path.x + _width_path + frame_padding_x, 0), _next_draw_pos + ImVec2(_draw_pos_path.x + _width_path + frame_padding_x, _header_height), ImColor(COL_TEXT_GRAY));

				_draw_star(p_draw_list, _next_draw_pos + ImVec2(_draw_pos_star.x, 0) + ImVec2(_width_star, _header_height) * 0.5f, _width_star * 0.25f, ImColor(COL_TEXT_GRAY));

				ImGui::PushFont(GEctx->P_Font_Arial_Bold_13_5);
				p_draw_list->AddText(_next_draw_pos + ImVec2(_draw_pos_name.x, draw_pos_header_text_y), ImColor(COL_TEXT), "Name");
				p_draw_list->AddText(_next_draw_pos + ImVec2(_draw_pos_last_opened.x, draw_pos_header_text_y), ImColor(COL_TEXT), "Last Opened");
				p_draw_list->AddText(_next_draw_pos + ImVec2(_draw_pos_path.x, draw_pos_header_text_y), ImColor(COL_TEXT), "Path");
				ImGui::PopFont();

				_next_draw_pos.y += _header_height + GImGui->Style.ItemSpacing.y * 2 + GImGui->Style.FramePadding.y;
			}

			ImGui::EndChild();

			if (ImGui::BeginChild("Project List", _project_list_child_window_size, false, ImGuiWindowFlags_NoScrollbar))
			{
				static auto scroll_ratio	 = 0.f;
				int			item_count		 = _project_list_child_window_size.y / _item_size.y;
				auto		need_scrollbar	 = item_count < _project_open_datas.size();
				int			item_start_index = (_project_open_datas.size() - item_count) * scroll_ratio;
				int			item_end_index	 = min(_project_open_datas.size(), item_start_index + item_count);

				for (int i = item_start_index; i < item_end_index; ++i)
				{
					_item_project_open_data(_project_open_datas[i]);
				}

				if (_project_need_sort)
				{
					_project_need_sort = false;
					_sort_project_open_data();
				}

				if (need_scrollbar)
				{
					auto   grab_hovered	  = false;
					auto   grab_held	  = false;
					auto   scrollbar_held = false;
					auto   window		  = ImGui::GetCurrentWindowRead();
					auto   scrollbar_id	  = window->GetID("scrollbar");
					auto   grab_id		  = window->GetID("scrollbar_grab");
					auto   grab_size	  = 60.f * GEctx->Dpi_Scale;
					ImRect scrollbar_bb {
						ImVec2(_next_draw_pos.x + _draw_pos_path.x + _width_path + GImGui->Style.FramePadding.x * 2, window->Pos.y),
						ImVec2(_next_draw_pos.x + _header_size.x, _next_draw_pos.y)
					};

					auto grab_bb = ImRect(scrollbar_bb);
					{
						grab_bb.Min	  = ImVec2(scrollbar_bb.Min.x, scrollbar_bb.Min.y + (scrollbar_bb.GetSize().y - grab_size) * scroll_ratio);
						grab_bb.Max.y = grab_bb.Min.y + grab_size;
					}

					ImGui::ItemAdd(scrollbar_bb, scrollbar_id, nullptr, ImGuiItemFlags_NoNav);
					ImGui::ItemAdd(grab_bb, grab_id);
					ImGui::ButtonBehavior(grab_bb, grab_id, &grab_hovered, &grab_held);
					ImGui::ButtonBehavior(scrollbar_bb, scrollbar_id, nullptr, &scrollbar_held);

					if (scrollbar_held)
					{
						scroll_ratio  = std::clamp((ImGui::GetMousePos().y - (scrollbar_bb.Min.y + grab_size * 0.5f)) / (scrollbar_bb.GetSize().y - grab_size), 0.f, 1.f);
						grab_bb.Min	  = ImVec2(scrollbar_bb.Min.x, scrollbar_bb.Min.y + (scrollbar_bb.GetSize().y - grab_size) * scroll_ratio);
						grab_bb.Max.y = grab_bb.Min.y + grab_size;
					}
					else if (grab_held)
					{
						static float anchor			  = 0.f;
						static auto	 before_grab_rect = ImRect();

						if (GImGui->IO.MouseClicked[0])
						{
							anchor			 = ImGui::GetMousePos().y;
							before_grab_rect = grab_bb;
						}

						auto delta_move = ImGui::GetMousePos().y - anchor;
						auto new_pos_y	= std::clamp(before_grab_rect.Min.y + delta_move, scrollbar_bb.Min.y, scrollbar_bb.Max.y - grab_size);

						grab_bb.Min.y = new_pos_y;
						grab_bb.Max.y = new_pos_y + grab_size;

						scroll_ratio = (grab_bb.Min.y - scrollbar_bb.Min.y) / (scrollbar_bb.GetSize().y - grab_size);
					}
					else if (GImGui->IO.MouseWheel)
					{
						// todo
						scroll_ratio += -GImGui->IO.MouseWheel * 0.1f;
						scroll_ratio  = std::clamp(scroll_ratio, 0.f, 1.f);
					}

					// window->DrawList->AddRectFilled(scrollbar_bb.Min, scrollbar_bb.Max, ImGui::GetColorU32(ImGuiCol_ScrollbarBg));
					window->DrawList->AddRectFilled(grab_bb.Min, grab_bb.Max, ImGui::GetColorU32(grab_held ? ImGuiCol_ScrollbarGrabActive : grab_hovered ? ImGuiCol_ScrollbarGrabHovered
																																						 : ImGuiCol_ScrollbarGrab),
													_item_rounding);
				}

				ImGui::EndChild();
			}
		}

	}	 // namespace

	void Update_Dpi_Scale()
	{
		auto frame_padding = GImGui->Style.FramePadding;
		auto _item_spacing = GImGui->Style.ItemSpacing;

		_project_list_child_window_size	   = ImGui::GetPlatformIO().Monitors[0].MainSize;
		_project_list_child_window_size.x *= 0.50f;
		_project_list_child_window_size.y *= 0.50f;

		_header_size.x = _project_list_child_window_size.x - frame_padding.x * 2;
		_header_size.y = frame_padding.y * 2 + _item_spacing.y * 3 + GImGui->FontSize * 2;

		_width_star		   = _header_size.y - frame_padding.y * 2;
		_width_name_desc   = _project_list_child_window_size.x * 0.4;
		_width_last_opened = ImGui::CalcTextSize("0000-00-00-00:00").x;
		_width_path		   = _header_size.x - (_width_star + _width_name_desc + _width_last_opened + frame_padding.x * 14);

		_header_height = _width_star;
		{
			int max_rendered_item_num		  = _project_list_child_window_size.y / _header_size.y;
			_project_list_child_window_size.y = max_rendered_item_num * _header_size.y + frame_padding.y;
		}

		_draw_pos_item		  = ImVec2(frame_padding.x * 3, 0);
		_draw_pos_star		  = ImVec2(frame_padding.x * 4, frame_padding.y);
		_draw_pos_name		  = _draw_pos_star + ImVec2(_width_star + frame_padding.x * 2, _item_spacing.y);
		_draw_pos_desc		  = _draw_pos_name + ImVec2(0, GImGui->FontSize + _item_spacing.y);
		_draw_pos_last_opened = ImVec2(_draw_pos_desc.x + _width_name_desc + frame_padding.x * 2, (_header_size.y - GImGui->FontSize) / 2);
		_draw_pos_path		  = _draw_pos_last_opened + ImVec2(_width_last_opened + frame_padding.x * 2, 0);

		_item_size		= _header_size;
		_item_size.x   -= _draw_pos_item.x * 2;
		_item_rounding	= 5.f * GEctx->Dpi_Scale;
	}

	void Draw()
	{
		bool filter_changed = ImGui::InputTextWithHint("##Search", "Search", _buffer, _char_buffer_size);

		ImGui::PushFont(GEctx->P_Font_Arial_Bold_13_5);
		ImGui::SameLine();
		if (ImGui::Button(" Add "))
		{
			_open_dialog_add_project = true;
		}

		ImGui::SameLine();
		if (ImGui::Button(" New "))
		{
			_open_dialog_new_project = true;
		}

		ImGui::PopFont();
		ImGui::Separator();

		_childwindow_project_list();

		if (_open_dialog_new_project)
		{
			_dialog_new_project();
		}
		else if (_open_dialog_add_project)
		{
			_dialog_add_project();
		}

		if (_open_popup_open_project_failed)
		{
			_popup_open_project_failed();
		}
		else if (_open_popup_add_project_failed)
		{
			_popup_add_project_failed();
		}
	}
}	 // namespace Editor::GameProject::Browser
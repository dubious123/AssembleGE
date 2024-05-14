#include "pch.h"

#include "../editor.h"
#include "../utilities/timing.h"
#include "game.h"

namespace editor::game
{
	using namespace editor::models;

	namespace
	{
		void _unload_dll();
		bool _generate_code(std::string& project_directory_path, std::string& proj_name);
		bool _build_load_dll(std::string& project_directory_path, std::string& proj_name);
	}	 // namespace

	namespace
	{
		auto _project_open_datas			  = std::vector<project_open_data>();
		auto _application_data_directory_path = std::wstring();
		auto _project_open_data_path		  = std::wstring();
		auto _current_project				  = game_project();
		auto _project_open_data_xml			  = pugi::xml_document();
		auto _project_list_node				  = pugi::xml_node();
		auto _project_dll					  = HMODULE();

		const editor_command _cmd_save {
			"Save",
			ImGuiKey_S | ImGuiMod_Ctrl,
			[](editor_id _) {
				// todo only when modified
				return true;
			},
			[](editor_id _) {
				save();
			}
		};

		const editor_command _cmd_build_load {
			"Build",
			ImGuiKey_B | ImGuiMod_Ctrl,
			[](editor_id) {
				return true;
			},
			[](editor_id) {
				editor::widgets::progress_modal("Building project",
												[] {
													auto res = true;
													if (editor::models::change_exists())
													{
														editor::widgets::progress_modal_msg("generating code");
														res &= _generate_code(_current_project.directory_path, _current_project.name);
													}

													if (_project_dll)
													{
														editor::widgets::progress_modal_msg("unloading dll");
														_unload_dll();
													}

													editor::widgets::progress_modal_msg("building dll");
													res &= _build_load_dll(_current_project.directory_path, _current_project.name);
													if (res)
													{
														logger::info("Build Success");
													}
													else
													{
														logger::error("Build Failed");
													}
													return res;
												});
			}
		};

		int32 _run_cmd(std::wstring cmd, uint32* p_out_exit_code, void* p_out_buf, size_t buf_size, size_t* p_write_size)
		{
			// auto h_child_std_in_rd	= HANDLE(nullptr);
			// auto h_child_std_in_wr	= HANDLE(nullptr);
			auto error		 = 0;
			auto want_output = p_out_buf is_not_nullptr;

			// todo std input
			// todo code clean up
			auto want_input	  = false;
			auto want_inherit = want_output or want_input;
			auto error_code	  = 1ul;

			auto hchild_in_rd  = HANDLE(nullptr);
			auto hchild_out_rd = HANDLE(nullptr);
			auto hchild_out_wr = HANDLE(nullptr);

			auto saAttr					= SECURITY_ATTRIBUTES();
			saAttr.nLength				= sizeof(SECURITY_ATTRIBUTES);
			saAttr.bInheritHandle		= true;
			saAttr.lpSecurityDescriptor = nullptr;

			// parent          child
			// child_in_wr	-> child_in_rd
			// child_out_rd <- child_out_wr
			if (::CreatePipe(&hchild_out_rd, &hchild_out_wr, &saAttr, 0) is_false) return 1;

			// if (SetHandleInformation(hchild_out_rd, HANDLE_FLAG_INHERIT, 0) is_false) return 1;
			if (::SetHandleInformation(hchild_out_rd, HANDLE_FLAG_INHERIT, 0) is_false) return 1;
			// if (SetHandleInformation(hchild_out_wr, HANDLE_FLAG_INHERIT, 0) is_false) return 1;

			auto pi = PROCESS_INFORMATION();
			auto si = STARTUPINFO();
			ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));
			ZeroMemory(&si, sizeof(STARTUPINFO));
			si.cb = sizeof(STARTUPINFO);
			if (want_input)
			{
				// si.hStdInput = hchild_in_rd;
			}
			if (want_output)
			{
				si.hStdError  = hchild_out_wr;
				si.hStdOutput = hchild_out_wr;
			}
			if (want_inherit)
			{
				si.dwFlags |= STARTF_USESTDHANDLES;
			}

			if (::CreateProcess(NULL,
								_wcsdup(cmd.c_str()),													// command line
								NULL,																	// process security attributes
								NULL,																	// primary thread security attributes
								want_inherit,															// handles are inherited
								NORMAL_PRIORITY_CLASS | CREATE_NO_WINDOW | CREATE_NEW_PROCESS_GROUP,	// creation flags
								NULL,																	// use parent's environment
								NULL,																	// use parent's current directory
								&si,																	// STARTUPINFO pointer
								&pi) is_false)															// receives PROCESS_INFORMATION)
			{
				return GetLastError();
			}


			::CloseHandle(hchild_out_wr);
			//::CloseHandle(hchild_in_rd);

			if (want_output)
			{
				auto max_buf_size  = buf_size - 1;
				auto read_count	   = 0ul;
				auto write_pos	   = 0ul;
				auto bytes_to_read = 0ul;

				while (true)
				{
					if (::PeekNamedPipe(
							hchild_out_rd,	   //[in] HANDLE				  hNamedPipe,
							nullptr,		   //[ out, optional ] LPVOID  lpBuffer,
							0,				   //[in] DWORD				  nBufferSize,
							nullptr,		   //[ out, optional ] LPDWORD lpBytesRead,
							&bytes_to_read,	   //[ out, optional ] LPDWORD lpTotalBytesAvail,
							nullptr			   //[ out, optional ] LPDWORD lpBytesLeftThisMessage))
							))
					{
						if (max_buf_size < write_pos + bytes_to_read)
						{
							*p_write_size = write_pos;
							logger::error("not enough buffer size");
							break;
						}
					}

					if (::ReadFile(hchild_out_rd, p_out_buf, bytes_to_read, &read_count, nullptr))
					{
						write_pos += read_count;
						if (write_pos >= max_buf_size)
						{
							assert(false);
						}

						p_out_buf = (char*)p_out_buf + read_count;
					}
					else
					{
						error = GetLastError();
						if (error == ERROR_BROKEN_PIPE)
						{
							*p_write_size = write_pos;
							break;
						}

						assert(false and "wut?");
					}
				}
			}

			//::CloseHandle(hchild_in_wr);
			::CloseHandle(hchild_out_rd);
			if (p_out_exit_code is_not_nullptr)
			{
				::GetExitCodeProcess(pi.hProcess, (unsigned long*)p_out_exit_code);
			}

			::CloseHandle(pi.hThread);
			::CloseHandle(pi.hProcess);

			return ERROR_SUCCESS;
		}

		void _load_project_open_datas()
		{
			_project_open_datas.clear();
			for (auto project_node : _project_list_node)
			{
				_project_open_datas.emplace_back(project_open_data(
					project_node.attribute("name").value(),
					project_node.attribute("desc").value(),
					project_node.attribute("last_opened_date").value(),
					project_node.attribute("path").value(),
					project_node.attribute("starred").as_bool()));
			}
		}

		void _unload_dll()
		{
			assert(_project_dll is_not_nullptr);
			::FreeLibrary(_project_dll);
			_current_project.is_ready = false;
			editor::on_project_unloaded();
		}

		bool _generate_code(std::string& project_directory_path, std::string& proj_name)
		{
			using namespace editor::models;
			using namespace std::ranges;
			static constexpr auto* target_file_name = "components.h";

			static constexpr auto* file_begin = "#pragma once \n#include \"__components.h\"\n\n //generated from editor\n\n";

			static constexpr auto* template_component_begin = "COMPONENT_BEGIN({})\n";
			static constexpr auto* template_serialize_field = "__SERIALIZE_FIELD({}, {}, {})\n";
			static constexpr auto* template_component_end	= "COMPNENT_END()\n\n";

			static constexpr auto* template_scene_begin			 = "SCENE_BEGIN({})\n";
			static constexpr auto* template_world_begin			 = "__WORLD_BEGIN({}{})\n";
			static constexpr auto* template_entity_begin		 = "____ENTITY_BEGIN({}{})\n";
			static constexpr auto* template_entity_set_component = "______SET_COMPONENT({}, {}, {})\n";
			static constexpr auto* template_entity_end			 = "____ENTITY_END()\n";
			static constexpr auto* template_world_end			 = "__WORLD_END()\n";
			static constexpr auto* template_scene_end			 = "SCENE_END()\n\n";

			auto target_file = std::filesystem::path(project_directory_path)
								   .append(GAMECODE_DIRECTORY)
								   .append(target_file_name);

			auto content = std::string(file_begin);

			// todo maybe better way to concat string than just +=
			for_each(reflection::all_structs(), [&](const em_struct* p_struct) {
				content += std::format(template_component_begin, p_struct->name);

				for_each(reflection::all_fields(p_struct->id), [&](const em_field* p_field) {
					content += std::format(template_serialize_field, reflection::utils::type_to_string(p_field->type), p_field->name, reflection::utils::deserialize(p_field->type, p_field->p_value));
				});

				content += template_component_end;
			});

			for_each(scene::all(), [&](const em_scene* p_scene) {
				content += std::format(template_scene_begin, p_scene->name);

				for_each(world::all(p_scene->id), [&](const em_world* p_world) {
					auto world_struct_str = std::string();

					for_each(p_world->structs, [&](editor_id s_id) {
						world_struct_str += ", " + reflection::find_struct(s_id)->name;
					});

					content += std::format(template_world_begin, p_world->name, world_struct_str);

					for_each(entity::all(p_world->id), [&](const em_entity* p_entity) {
						auto archetype_str = std::string();
						for_each(component::all(p_entity->id), [&](const em_component* p_component) {
							archetype_str += ", " + reflection::find_struct(p_component->struct_id)->name;
						});

						content += std::format(template_entity_begin, p_entity->name, archetype_str);

						for_each(component::all(p_entity->id), [&](const em_component* p_component) {
							auto* p_struct = reflection::find_struct(p_component->struct_id);
							for_each(reflection::all_fields(p_struct->id), [&](const em_field* p_field) {
								if (memcmp((char*)p_component->p_value + p_field->offset, p_field->p_value, reflection::utils::type_size(p_field->type)) != 0)
								{
									content += std::format(template_entity_set_component, p_struct->name, "." + p_field->name, "{" + reflection::utils::deserialize(p_field->type, p_component->p_value) + "}");
								}
							});

							// content += std::format(template_entity_set_component,)
						});

						content += template_entity_end;
					});

					content += template_world_end;
				});

				content += template_scene_end;
			});


			editor::utilities::create_file(target_file, content);
			return true;
		}

		bool _build_load_dll(std::string& project_directory_path, std::string& proj_name)
		{
			char		   buf[4096 * 3];
			constexpr auto buf_size		  = 4096ui64 * 3;
			auto		   buf_write_size = 0ui64;

			auto sln_path		  = std::format("{}\\{}.sln", project_directory_path, proj_name);
			auto p_program86_path = PWSTR { nullptr };
			::SHGetKnownFolderPath(FOLDERID_ProgramFilesX86, 0, NULL, &p_program86_path);

			auto find_msbuild_command = std::format(L"\"{}\\Microsoft Visual Studio\\Installer\\vswhere.exe\" -latest -prerelease -products * -requires Microsoft.Component.MSBuild -find MSBuild\\**\\Bin\\MSBuild.exe", p_program86_path);
			_run_cmd(find_msbuild_command, nullptr, (void*)(buf), buf_size, &buf_write_size);

			auto ms_build_path = std::string(buf, buf_write_size);
			ms_build_path.resize(ms_build_path.size() - 2);	   // to remove /r/n

			memset(buf, 0, sizeof(buf));

#ifdef _DEBUG_EDITOR
			auto build_command = std::format("\"{}\" \"{}\" /p:Configuration=DebugEditor /p:platform=x64 /p:PreBuildEventUseInBuild=true", ms_build_path, sln_path);
			auto dll_path	   = std::format("{}\\x64\\DebugEditor\\{}.dll", project_directory_path, proj_name);
#else ifdef _RELEASE_EDITOR
			auto build_command = std::format("\"{}\" \"{}\" /p:Configuration=ReleaseEditor /p:platform=x64 /p:PreBuildEventUseInBuild=true", ms_build_path, sln_path);
			auto dll_path	   = std::format("{}\\x64\\ReleaseEditor\\{}.dll", project_directory_path, proj_name);
#endif	  // _DEBUG_EDITOR

			wchar_t bffer[500] { 0 };
			std::copy_n(build_command.begin(), build_command.size(), bffer);

			auto ms_build_exit_code = 0ui32;
			auto cmd_res			= _run_cmd(bffer, &ms_build_exit_code, buf, buf_size, &buf_write_size);

			auto build_success = SUCCEEDED(cmd_res) and (ms_build_exit_code == 0);
			auto w_out		   = std::string(buf, buf_write_size + 1);
			// w_out.append("\n");

			if (build_success is_false)
			{
				// todo enter safe mode
				logger::error(w_out);
				return false;
			}

			_project_dll = LoadLibraryA(dll_path.c_str());

			if (game::ecs::init(_project_dll) is_false)
			{
				return false;
			}

			_current_project.is_ready = true;
			return true;
		}
	}	 // namespace

	project_open_data::project_open_data(const char* name, const char* desc, const char* date, const char* path, bool starred)
	{
		// this->id			   = editor::id::get(DataType_Project);
		this->name			   = name;
		this->desc			   = desc;
		this->last_opened_date = date;
		this->path			   = path;
		this->starred		   = starred;
	}

	void init()
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

		if (_project_open_data_xml.load_file(_project_open_data_path.c_str()) is_false)
		{
			// todo create raw xml file
		}

		_project_list_node = _project_open_data_xml.child("project_list");
		_load_project_open_datas();
		::CoTaskMemFree((void*)p_folder_path);
	}

	void save_project_open_datas()
	{
		std::wofstream xml_stream(_project_open_data_path);
		_project_open_data_xml.reset();
		_project_list_node = _project_open_data_xml.append_child("project_list");
		for (auto project_od : _project_open_datas)
		{
			auto project_node = _project_list_node.append_child("project");
			project_node.append_attribute("name").set_value(project_od.name.c_str());
			project_node.append_attribute("desc").set_value(project_od.desc.c_str());
			project_node.append_attribute("last_opened_date").set_value(project_od.last_opened_date.c_str());
			project_node.append_attribute("path").set_value(project_od.path.c_str());
			project_node.append_attribute("starred").set_value(project_od.starred);
		}

		_project_open_data_xml.save(xml_stream);
		xml_stream.close();
	}

	bool save()
	{
		auto doc		  = pugi::xml_document();
		auto success	  = doc.load_file(_current_project.project_file_path.c_str());
		auto xml_stream	  = std::wofstream(_current_project.project_file_path);
		auto project_node = doc.child("project");
		project_node.remove_children();

		auto scenes_node = project_node.append_child("scenes");
		for (auto p_scene : models::scene::all())
		{
			auto scene_node	 = scenes_node.append_child("scene");
			auto worlds_node = scene_node.append_child("worlds");
			scene_node.append_attribute("name").set_value(p_scene->name.c_str());

			// for (auto& world_id : scene.worlds)
			//{
			//	auto p_world		 = world::get(world_id);
			//	auto world_node		 = worlds_node.append_child("world");
			//	auto entities_node	 = world_node.append_child("entities");
			//	auto systems_node	 = world_node.append_child("systems");
			//	auto sub_worlds_node = world_node.append_child("sub_worlds");

			//	world_node.append_attribute("id").set_value(world_id.ToString().c_str());
			//	world_node.append_attribute("name").set_value(p_world->name.c_str());

			//	for (auto& entity_id : p_world->entities)
			//	{
			//		auto p_entity		 = entity::find(entity_id);
			//		auto entity_node	 = entities_node.append_child("entity");
			//		auto components_node = entity_node.append_child("components");
			//		entity_node.append_attribute("id").set_value(entity_id.ToString().c_str());
			//		entity_node.append_attribute("name").set_value(p_entity->name.c_str());

			//		for (auto& component_id : p_entity->components)
			//		{
			//			auto p_component	= component::find(component_id);
			//			auto component_node = components_node.append_child("component");
			//			component_node.append_attribute("id").set_value(component_id.ToString().c_str());
			//			component_node.append_attribute("name").set_value(p_component->name.c_str());
			//		}
			//	}

			//	for (auto& system_id : p_world->systems)
			//	{
			//		auto system_node = systems_node.append_child("system");
			//		auto p_system	 = system::find(system_id);
			//		system_node.append_attribute("id").set_value(system_id.ToString().c_str());
			//		system_node.append_attribute("name").set_value(p_system->name.c_str());
			//	}

			//	// for (auto& sub_world : world.SubWorlds)
			//	//{
			//	//	auto sub_world_node = sub_worlds_node.append_child("sub_world");
			//	//	sub_world_node.append_attribute("id").set_value(sub_world.Id.ToString().c_str());
			//	//	sub_world_node.append_attribute("name").set_value(sub_world.Name.c_str());
			//	// }
			//}
		}

		auto active_scene_node = project_node.append_child("active_scene");
		active_scene_node.append_attribute("name").set_value(models::scene::get_current()->name.c_str());

		doc.save(xml_stream);
		xml_stream.close();

		editor::logger::info("Project save completed");
		return success;
	}

	bool open_async(std::filesystem::path project_directory_path)
	{
		widgets::progress_modal_msg("Reading project data");
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

		_current_project.directory_path	   = project_directory_path.string();
		_current_project.project_file_path = project_data_path.string();
		_current_project.name			   = project_node.attribute("name").value();
		_current_project.description	   = project_node.attribute("desc").value();
		_current_project.last_opened_date  = editor::utilities::timing::now_str();

		widgets::progress_modal_msg("Build and load dll");

		if (_build_load_dll(_current_project.directory_path, _current_project.name) is_false)
		{
			return false;
		}

		widgets::progress_modal_msg("Loading project");
		logger::info("Open Completed");


		for (auto& data : _project_open_datas)
		{
			if (_current_project.name == data.name)
			{
				data.last_opened_date = _current_project.last_opened_date;
				return true;
			}
		}

		project_open_data od;
		od.name				= _current_project.name;
		od.desc				= _current_project.description;
		od.last_opened_date = _current_project.last_opened_date;
		od.path				= project_directory_path.string();
		// od.Path.assign(project_directory_path.native().begin(), project_directory_path.native().end());
		od.starred = false;

		_project_open_datas.emplace_back(od);

		widgets::progress_modal_msg("Open Project Done");
		return success;
	}

	void on_project_loaded()
	{
		editor::add_context_item("Main Menu\\File\\Save", &_cmd_save);
		editor::add_context_item("Main Menu\\File\\Build", &_cmd_build_load);
	}

	bool project_opened()
	{
		return _current_project.is_ready;
	}

	game_project* get_current_p_project()
	{
		return &_current_project;
	}

	bool create(const char* proj_name, const char* path, char* err_msg = nullptr)
	{
		constexpr int min_proj_name_len = 3;
		char		  bad_chars[]		= { '!', '@', '%', '^', '*', '~', '|' };
		bool		  valid_path		= std::filesystem::is_directory(path) and std::filesystem::exists(path);
		bool		  valid_name		= true;
		bool		  success			= true;

		std::string proj_name_str(proj_name);
		std::string solution_template_str, project_template_str, project_data_template_str, main_cpp_str, components_h_str, __common_h_str;
		std::string arg_0, arg_1, arg_2, arg_3;

		std::filesystem::path directory_path;
		std::filesystem::path gamecode_directory_path;
		std::filesystem::path gamecode_generated_directory_path;
		std::filesystem::path internal_project_dir_path;
		std::filesystem::path solution_template_path;
		std::filesystem::path project_template_path;
		std::filesystem::path project_data_template_path;

		valid_name = std::ranges::all_of(bad_chars, [&](const auto& c) { return std::ranges::find(proj_name_str, c) == proj_name_str.end(); });

		valid_name &= proj_name_str.length() >= min_proj_name_len;

		if (not valid_path or not valid_name) return false;

		// create directory
		{
			directory_path = std::filesystem::path(path);
			directory_path.append(proj_name_str);
			gamecode_directory_path = std::filesystem::path(directory_path);
			gamecode_directory_path.append(GAMECODE_DIRECTORY);
			gamecode_generated_directory_path = gamecode_directory_path;
			gamecode_generated_directory_path.append(GAMECODE_GENERATED_DIRECTORY);
			internal_project_dir_path = std::filesystem::path(directory_path);
			internal_project_dir_path.append(PROJECT_EXTENSION);
			success &= std::filesystem::create_directory(directory_path);
			success &= std::filesystem::create_directory(gamecode_directory_path);
			success &= std::filesystem::create_directory(gamecode_generated_directory_path);
			success &= std::filesystem::create_directory(internal_project_dir_path);
			::SetFileAttributes(internal_project_dir_path.c_str(), FILE_ATTRIBUTE_HIDDEN);
		}

		// get template str
		{
			TCHAR current_dirctory_path[MAX_PATH];
			::GetCurrentDirectory(MAX_PATH, current_dirctory_path);

			solution_template_str	  = utilities::read_file(std::filesystem::path(current_dirctory_path).append("resources\\project_template\\").append("msvc_solution"));
			project_template_str	  = utilities::read_file(std::filesystem::path(current_dirctory_path).append("resources\\project_template\\").append("msvc_project"));
			project_data_template_str = utilities::read_file(std::filesystem::path(current_dirctory_path).append("resources\\project_template\\").append("project_data"));
			main_cpp_str			  = utilities::read_file(std::filesystem::path(current_dirctory_path).append("resources\\project_template\\").append("main_cpp"));
			components_h_str		  = utilities::read_file(std::filesystem::path(current_dirctory_path).append("resources\\project_template\\").append("components_h"));
			__common_h_str			  = utilities::read_file(std::filesystem::path(current_dirctory_path).append("resources\\project_template\\").append("common_h"));
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
			// todo lib path
			arg_3 = std::filesystem::current_path().parent_path().string();	   // std::string("C:\\Users\\JH\\Desktop\\projects\\AssembleGE");	engine lib path
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
		utilities::create_file(
			std::filesystem::path(gamecode_directory_path) / std::format("{}.vcxproj", proj_name),
			std::vformat(project_template_str, std::make_format_args(arg_0, arg_1, arg_2, arg_3)));

		// create project data
		utilities::create_file(
			std::filesystem::path(internal_project_dir_path) / PROJECT_DATA_FILE_NAME,
			std::vformat(project_data_template_str, std::make_format_args(arg_0, arg_3)));

		utilities::create_file(
			std::filesystem::path(gamecode_directory_path) / "main.cpp",
			main_cpp_str);

		utilities::create_file(
			std::filesystem::path(gamecode_directory_path) / "components.h",
			components_h_str);
		utilities::create_file(
			std::filesystem::path(gamecode_generated_directory_path) / "__common.h", __common_h_str);

		assert(success);
		return success;
	}
}	 // namespace editor::game

namespace editor::view::project_browser
{
	using editor::game::_project_open_datas;

	namespace
	{
		constexpr size_t _char_buffer_size = 100;

		editor::game::project_open_data* _p_selected_project_open_data = nullptr;

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

		void _draw_star(ImVec2 pos_abs, float size, ImColor col)
		{
			static constexpr auto star_offsets = _cal_star_points_pos_rel(1, 0.5f);

			ImVec2 star_points_abs[10];
			for (int i = 0; i < 10; ++i)
			{
				star_points_abs[i] = pos_abs - ImVec2(star_offsets[i].x * size, star_offsets[i].y * size);
			}

			widgets::draw_poly_filled(star_points_abs, 10, col);
		}

		void _sort_project_open_data()
		{
			int star_index = 0;
			for (int i = 0; i < _project_open_datas.size(); ++i)
			{
				auto project = _project_open_datas[i];
				if (project.starred)
				{
					if (i != star_index)
					{
						std::swap(_project_open_datas[i], _project_open_datas[star_index]);
					}

					++star_index;
				}
			}
		}

		void _item_project_open_data(editor::game::project_open_data& project_od)
		{
			bool item_hovered, item_clicked, star_hovered, star_clicked;

			ImVec2 local_pos = _next_draw_pos - platform::get_window_pos();	   // ImVec2(_next_item_pos.x - ImGui::GetCurrentWindowRead()->Pos.x, _next_item_pos.y - ImGui::GetCurrentWindowRead()->Pos.y);

			widgets::add_item(ImRect(_next_draw_pos + _draw_pos_star, _next_draw_pos + _draw_pos_star + ImVec2(_width_star, _width_star)), widgets::get_id(project_od.name.c_str()));
			star_hovered = widgets::is_item_hovered();
			star_clicked = widgets::is_item_clicked(ImGuiMouseButton_Left);

			widgets::add_item(ImRect(_next_draw_pos + _draw_pos_item, _next_draw_pos + _draw_pos_item + _item_size), widgets::get_id("##item_rect"));
			item_hovered = widgets::is_item_hovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
			item_clicked = widgets::is_item_clicked(ImGuiMouseButton_Left) and not star_clicked;

			if (item_hovered)
			{
				widgets::draw_rect_filled({ _next_draw_pos + _draw_pos_item, _next_draw_pos + _draw_pos_item + _item_size }, ImColor(COL_GRAY_2), _item_rounding);
				if (star_hovered)
				{

					widgets::draw_rect_filled({ _next_draw_pos + _draw_pos_star, _next_draw_pos + _draw_pos_star + ImVec2(_width_star, _width_star) }, ImColor(COL_GRAY_3), _item_rounding);
					_draw_star(_next_draw_pos + _draw_pos_star + ImVec2(_width_star, _width_star) * 0.5f, _width_star * 0.2f, ImColor(COL_GRAY_8));
				}
				else
				{
					_draw_star(_next_draw_pos + _draw_pos_star + ImVec2(_width_star, _width_star) * 0.5f, _width_star * 0.2f, ImColor(COL_TEXT_GRAY));
					// p_draw_list->AddRectFilled(_next_item_pos + _draw_pos_star, _next_item_pos + _draw_pos_star + ImVec2(_width_star, _width_star), ImColor(COL_GRAY_3));
				}
			}
			else if (project_od.starred)
			{
				_draw_star(_next_draw_pos + _draw_pos_star + ImVec2(_width_star, _width_star) * 0.5f, _width_star * 0.2f, ImColor(COL_TEXT_GRAY));
			}

			if (star_clicked)
			{
				project_od.starred = !project_od.starred;
				_project_need_sort = true;
			}

			if (item_clicked)
			{
				_p_selected_project_open_data = &project_od;

				widgets::progress_modal(
					"Open project",
					[project_od]() {
						return editor::game::open_async(project_od.path);
					},
					[](bool successed) {
						if (successed)
						{
							_sort_project_open_data();
							editor::on_project_loaded();
							editor::game::_current_project.is_ready = true;
						}
						else
						{
							editor::on_project_unloaded();
							_open_popup_open_project_failed = true;
						}
					});
			}

			widgets::push_font(GEctx->p_font_arial_bold_13_5);
			widgets::draw_text(_next_draw_pos + _draw_pos_name, style::get_color_u32(ImGuiCol_Text), project_od.name.c_str());
			widgets::pop_font();

			widgets::draw_text(_next_draw_pos + _draw_pos_desc, ImColor(COL_TEXT_GRAY), project_od.desc.c_str());
			widgets::draw_text(_next_draw_pos + _draw_pos_last_opened, ImColor(COL_TEXT_GRAY), project_od.last_opened_date.c_str());

			ImVec2 path_text_size = widgets::calc_text_size(project_od.path.c_str());
			if (path_text_size.x > _width_path)
			{
				auto target_text_width = _width_path - widgets::calc_text_size("...").x;
				auto location_text_end = &project_od.path[project_od.path.length() - 1];
				while (path_text_size.x > target_text_width)
				{
					const char	char_end	= *(location_text_end--);
					const float char_width	= (int)char_end < GEctx->p_font_arial_default_13_5->IndexAdvanceX.Size ? GEctx->p_font_arial_default_13_5->IndexAdvanceX.Data[char_end] : GEctx->p_font_arial_default_13_5->FallbackAdvanceX;
					path_text_size.x	   -= char_width;
				}

				widgets::draw_text(_next_draw_pos + _draw_pos_path, ImColor(COL_TEXT_GRAY), project_od.path.c_str(), location_text_end + 1);
				widgets::draw_text(_next_draw_pos + _draw_pos_path + ImVec2(path_text_size.x, 0), ImColor(COL_TEXT_GRAY), "...");
			}
			else
			{
				widgets::draw_text(_next_draw_pos + _draw_pos_path, ImColor(COL_TEXT_GRAY), project_od.path.c_str());
			}

			widgets::set_cursor_pos(local_pos + _draw_pos_item);
			widgets::item_size(_item_size);
			_next_draw_pos.y += _item_size.y;	 // +style::item_spacing().y;
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

			if (widgets::begin_popup_modal("New Project", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings))
			{
				widgets::set_cursor_pos_y(widgets::get_cursor_pos_y() + style::frame_padding().y);
				widgets::text("Project Name");
				button_size_x = widgets::get_item_rect().GetSize().x;

				widgets::sameline();

				widgets::set_cursor_pos_y(widgets::get_cursor_pos_y() - style::frame_padding().y);
				widgets::input_text("##new project name", new_project_name, str_name_max_len);
				input_text_size			   = widgets::get_item_rect().GetSize();
				input_text_newProject_rect = ImRect(widgets::get_item_rect().Min + ImVec2(0, input_text_size.y + style::item_spacing().y), widgets::get_item_rect().Max + ImVec2(0, input_text_size.y + style::item_spacing().y));
				widgets::set_cursor_pos_y(widgets::get_cursor_pos_y() + style::frame_padding().y);
				widgets::text("Project Path");
				widgets::sameline(button_size_x + style::item_spacing().x + style::window_padding().x);

				widgets::set_cursor_pos_y(widgets::get_cursor_pos_y() - style::frame_padding().y);
				widgets::add_item(input_text_newProject_rect, widgets::get_id("##new project path"));
				widgets::item_size(input_text_newProject_rect.GetSize());

				widgets::draw_frame(input_text_newProject_rect.Min, input_text_newProject_rect.Max, style::get_color_u32(ImGuiCol_FrameBg), true, style::frame_rounding());
				widgets::set_cursor_pos((widgets::get_cursor_pos() + style::frame_padding()));
				auto open_dialog_directory = widgets::is_item_clicked();
				auto path_text_size		   = widgets::calc_text_size(new_project_path.c_str());
				auto path_text_need_clip   = path_text_size.x > input_text_newProject_rect.GetSize().x;
				// auto p_draw_list		   = ImGui::GetWindowDrawList();
				if (path_text_need_clip)
				{
					auto target_text_width	  = input_text_newProject_rect.GetSize().x - style::frame_padding().x * 2 - widgets::calc_text_size("...").x;
					auto new_project_text_end = new_project_path.end() - 1;
					while (path_text_size.x > target_text_width)
					{
						const char	char_end	= *(new_project_text_end--);
						const float char_width	= (int)char_end < GEctx->p_font_arial_default_13_5->IndexAdvanceX.Size ? GEctx->p_font_arial_default_13_5->IndexAdvanceX.Data[char_end] : GEctx->p_font_arial_default_13_5->FallbackAdvanceX;
						path_text_size.x	   -= char_width;
					}

					widgets::draw_text(input_text_newProject_rect.Min + style::frame_padding(), ImColor(COL_TEXT), new_project_path.c_str(), new_project_text_end._Ptr + 1);
					widgets::draw_text(input_text_newProject_rect.Min + style::frame_padding() + ImVec2(path_text_size.x, 0), ImColor(COL_TEXT), "...");
				}
				else
				{
					widgets::draw_text_clipped(input_text_newProject_rect.Min + style::frame_padding(), input_text_newProject_rect.Max - style::frame_padding(), new_project_path.c_str(), nullptr, 0, ImVec2(), &input_text_newProject_rect);
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

					platform::add_mouse_source_event(ImGuiMouseSource_Mouse);
					platform::add_mouse_button_event(ImGuiMouseSource_Mouse, false);
				}

				widgets::separator();

				widgets::set_cursor_pos_y(widgets::get_cursor_pos_y() + style::frame_padding().y);
				if (widgets::button("Create", ImVec2(button_size_x, 0)))
				{
					if (editor::game::create(new_project_name, new_project_path.c_str()))
					{
						// todo : error handling, is it possible to fail to open after create? => yes when build is failed

						assert(editor::game::open_async(std::filesystem::path(new_project_path).append(new_project_name)));

						_sort_project_open_data();
						editor::on_project_loaded();
						editor::game::_current_project.is_ready = true;

						_open_dialog_new_project = false;
						show_error_msg			 = false;
					}
					else
					{
						show_error_msg = true;
					}
				}

				widgets::sameline();
				if (widgets::button("Cancel", ImVec2(button_size_x, 0)))
				{
					_open_dialog_new_project = false;
					show_error_msg			 = false;
				}

				if (show_error_msg)
				{
					widgets::text_colored(ImColor(COL_RED), "Create Project Failed");
				}

				widgets::end_popup();
			}

			widgets::open_popup("New Project");
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

			editor::widgets::progress_modal(
				"Open project", [open_project_path]() { return editor::game::open_async(open_project_path); }, [](bool res) {
					if (res)
					{
						_sort_project_open_data();
						game::on_project_loaded();
						editor::game::_current_project.is_ready = true;
					}
					else
					{
						_open_popup_add_project_failed = true;
					} });


			_open_dialog_add_project = false;
		}

		void _popup_open_project_failed()
		{
			if (_p_selected_project_open_data is_nullptr) return;

			if (widgets::begin_popup_modal("Open Project Failed", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoTitleBar))
			{
				{
					widgets::push_font(GEctx->p_font_arial_bold_18);
					widgets::text("Project failed to open");
					widgets::set_cursor_pos_y(widgets::get_cursor_pos_y() + style::item_spacing().y);
					widgets::pop_font();
				}

				widgets::separator();
				widgets::set_cursor_pos_y(widgets::get_cursor_pos_y() + style::item_spacing().y);
				widgets::text("%s could not be opened and will be removed from the list.", _p_selected_project_open_data->name.c_str());
				widgets::set_cursor_pos(ImVec2(widgets::get_item_rect().GetSize().x - (widgets::calc_text_size("  Ok  ").x + style::frame_padding().x) /*+ style::frame_padding().x * 2*/, widgets::get_cursor_pos_y() + style::item_spacing().y));
				if (widgets::button("  Ok  "))
				{
					_project_open_datas.erase(std::find_if(_project_open_datas.begin(), _project_open_datas.end(),
														   [](editor::game::project_open_data od) { return od.name == _p_selected_project_open_data->name; }));

					_p_selected_project_open_data = nullptr;
				}

				widgets::end_popup();
			}
		}

		void _popup_add_project_failed()
		{
			if (widgets::begin_popup_modal("Add Project Failed", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoTitleBar))
			{
				widgets::push_font(GEctx->p_font_arial_bold_18);
				widgets::text("Project failed to add");
				widgets::set_cursor_pos_y(widgets::get_cursor_pos_y() + style::item_spacing().y);
				widgets::pop_font();

				auto got_it_button_pos_x = widgets::get_item_rect().GetSize().x - style::frame_padding().x - widgets::calc_text_size("  Got it  ").x;
				widgets::separator();

				widgets::set_cursor_pos_y(widgets::get_cursor_pos_y() + style::item_spacing().y);
				widgets::text("Project is not valid");
				widgets::set_cursor_pos({ got_it_button_pos_x, widgets::get_cursor_pos_y() + style::item_spacing().y });
				if (widgets::button("  Got it  "))
				{
					widgets::close_popup();
					_p_selected_project_open_data = nullptr;
				}

				widgets::end_popup();
			}
		}

		void _childwindow_project_list()
		{
			if (widgets::begin_child("Header", ImVec2(_project_list_child_window_size.x, _header_height + style::item_spacing().y * 4)))
			{
				_next_draw_pos = platform::get_window_pos() + style::frame_padding();

				auto item_spacing_x			= style::item_spacing().x;
				auto frame_padding_x		= style::frame_padding().x;
				auto draw_pos_header_text_y = (_header_height - style::font_size()) / 2;

				widgets::draw_rect_filled({ _next_draw_pos, _next_draw_pos + ImVec2(_header_size.x, _header_height) }, ImColor(COL_GRAY_1));
				widgets::draw_rect(_next_draw_pos, _next_draw_pos + ImVec2(_header_size.x, _header_height), ImColor(COL_TEXT_GRAY));
				widgets::draw_line(_next_draw_pos + ImVec2(_draw_pos_star.x - frame_padding_x, 0), _next_draw_pos + ImVec2(_draw_pos_star.x - frame_padding_x, _header_height), ImColor(COL_TEXT_GRAY));
				widgets::draw_line(_next_draw_pos + ImVec2(_draw_pos_name.x - frame_padding_x, 0), _next_draw_pos + ImVec2(_draw_pos_name.x - frame_padding_x, _header_height), ImColor(COL_TEXT_GRAY));
				widgets::draw_line(_next_draw_pos + ImVec2(_draw_pos_last_opened.x - frame_padding_x, 0), _next_draw_pos + ImVec2(_draw_pos_last_opened.x - frame_padding_x, _header_height), ImColor(COL_TEXT_GRAY));
				widgets::draw_line(_next_draw_pos + ImVec2(_draw_pos_path.x - frame_padding_x, 0), _next_draw_pos + ImVec2(_draw_pos_path.x - frame_padding_x, _header_height), ImColor(COL_TEXT_GRAY));
				widgets::draw_line(_next_draw_pos + ImVec2(_draw_pos_path.x + _width_path + frame_padding_x, 0), _next_draw_pos + ImVec2(_draw_pos_path.x + _width_path + frame_padding_x, _header_height), ImColor(COL_TEXT_GRAY));

				_draw_star(_next_draw_pos + ImVec2(_draw_pos_star.x, 0) + ImVec2(_width_star, _header_height) * 0.5f, _width_star * 0.25f, ImColor(COL_TEXT_GRAY));

				widgets::push_font(GEctx->p_font_arial_bold_13_5);
				widgets::draw_text(_next_draw_pos + ImVec2(_draw_pos_name.x, draw_pos_header_text_y), ImColor(COL_TEXT), "Name");
				widgets::draw_text(_next_draw_pos + ImVec2(_draw_pos_last_opened.x, draw_pos_header_text_y), ImColor(COL_TEXT), "Last Opened");
				widgets::draw_text(_next_draw_pos + ImVec2(_draw_pos_path.x, draw_pos_header_text_y), ImColor(COL_TEXT), "Path");
				widgets::pop_font();

				_next_draw_pos.y += _header_height + style::item_spacing().y * 2 + style::frame_padding().y;
			}

			widgets::end_child();

			if (widgets::begin_child("Project List", _project_list_child_window_size, false, ImGuiWindowFlags_NoScrollbar))
			{
				static auto scroll_ratio	 = 0.f;
				auto		item_count		 = _project_list_child_window_size.y / _item_size.y;
				auto		need_scrollbar	 = item_count < _project_open_datas.size();
				auto		item_start_index = (int)((_project_open_datas.size() - item_count) * scroll_ratio);
				auto		item_end_index	 = min(_project_open_datas.size(), item_start_index + item_count);

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
					auto   scrollbar_id	  = widgets::get_id("scrollbar");
					auto   grab_id		  = widgets::get_id("scrollbar_grab");
					auto   grab_size	  = 60.f * platform::dpi_scale();
					ImRect scrollbar_bb {
						ImVec2(_next_draw_pos.x + _draw_pos_path.x + _width_path + style::frame_padding().x * 2, platform::get_window_pos().y),
						ImVec2(_next_draw_pos.x + _header_size.x, _next_draw_pos.y)
					};

					auto grab_bb = ImRect(scrollbar_bb);
					{
						grab_bb.Min	  = ImVec2(scrollbar_bb.Min.x, scrollbar_bb.Min.y + (scrollbar_bb.GetSize().y - grab_size) * scroll_ratio);
						grab_bb.Max.y = grab_bb.Min.y + grab_size;
					}

					widgets::add_item(scrollbar_bb, scrollbar_id, nullptr, ImGuiItemFlags_NoNav);
					widgets::add_item(grab_bb, grab_id);
					widgets::button_behavior(grab_bb, grab_id, &grab_hovered, &grab_held);
					widgets::button_behavior(scrollbar_bb, scrollbar_id, nullptr, &scrollbar_held);

					if (scrollbar_held)
					{
						scroll_ratio  = std::clamp((platform::get_mouse_pos().y - (scrollbar_bb.Min.y + grab_size * 0.5f)) / (scrollbar_bb.GetSize().y - grab_size), 0.f, 1.f);
						grab_bb.Min	  = ImVec2(scrollbar_bb.Min.x, scrollbar_bb.Min.y + (scrollbar_bb.GetSize().y - grab_size) * scroll_ratio);
						grab_bb.Max.y = grab_bb.Min.y + grab_size;
					}
					else if (grab_held)
					{
						static float anchor			  = 0.f;
						static auto	 before_grab_rect = ImRect();

						if (platform::mouse_clicked())
						{
							anchor			 = platform::get_mouse_pos().y;
							before_grab_rect = grab_bb;
						}

						auto delta_move = platform::get_mouse_pos().y - anchor;
						auto new_pos_y	= std::clamp(before_grab_rect.Min.y + delta_move, scrollbar_bb.Min.y, scrollbar_bb.Max.y - grab_size);

						grab_bb.Min.y = new_pos_y;
						grab_bb.Max.y = new_pos_y + grab_size;

						scroll_ratio = (grab_bb.Min.y - scrollbar_bb.Min.y) / (scrollbar_bb.GetSize().y - grab_size);
					}
					else if (platform::get_mouse_wheel() > 0)
					{
						// todo
						scroll_ratio += -platform::get_mouse_wheel() * 0.1f;
						scroll_ratio  = std::clamp(scroll_ratio, 0.f, 1.f);
					}

					// window->DrawList->AddRectFilled(scrollbar_bb.Min, scrollbar_bb.Max, ImGui::GetColorU32(ImGuiCol_ScrollbarBg));
					widgets::draw_rect_filled(grab_bb, style::get_color_u32(grab_held ? ImGuiCol_ScrollbarGrabActive : grab_hovered ? ImGuiCol_ScrollbarGrabHovered
																																	: ImGuiCol_ScrollbarGrab),
											  _item_rounding);
				}
			}

			widgets::end_child();
		}

		void _draw()
		{
			bool filter_changed = widgets::input_text_with_hint("##Search", "Search", _buffer, _char_buffer_size);

			widgets::push_font(GEctx->p_font_arial_bold_13_5);
			widgets::sameline();
			if (widgets::button(" Add "))
			{
				_open_dialog_add_project = true;
			}

			widgets::sameline();
			if (widgets::button(" New "))
			{
				_open_dialog_new_project = true;
			}

			widgets::pop_font();
			widgets::separator();

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
				widgets::open_popup("Open Project Failed");
				_open_popup_open_project_failed = false;
			}
			else if (_open_popup_add_project_failed)
			{
				widgets::open_popup("Add Project Failed");
				_open_popup_add_project_failed = false;
			}


			_popup_add_project_failed();
			_popup_open_project_failed();
		}
	}	 // namespace

	void update_dpi_scale()
	{
		auto frame_padding = style::frame_padding();
		auto item_spacing  = style::item_spacing();

		_project_list_child_window_size	   = platform::get_monitor_size();
		_project_list_child_window_size.x *= 0.50f;
		_project_list_child_window_size.y *= 0.50f;

		_header_size.x = _project_list_child_window_size.x - frame_padding.x * 2;
		_header_size.y = frame_padding.y * 2 + item_spacing.y * 3 + style::font_size() * 2;

		_width_star		   = _header_size.y - frame_padding.y * 2;
		_width_name_desc   = _project_list_child_window_size.x * 0.4f;
		_width_last_opened = widgets::calc_text_size("0000-00-00-00:00").x;
		_width_path		   = _header_size.x - (_width_star + _width_name_desc + _width_last_opened + frame_padding.x * 14);

		_header_height = _width_star;
		{
			auto max_rendered_item_num		  = (int)(_project_list_child_window_size.y / _header_size.y);
			_project_list_child_window_size.y = max_rendered_item_num * _header_size.y + frame_padding.y;
		}

		_draw_pos_item		  = ImVec2(frame_padding.x * 3, 0);
		_draw_pos_star		  = ImVec2(frame_padding.x * 4, frame_padding.y);
		_draw_pos_name		  = _draw_pos_star + ImVec2(_width_star + frame_padding.x * 2, item_spacing.y);
		_draw_pos_desc		  = _draw_pos_name + ImVec2(0, style::font_size() + item_spacing.y);
		_draw_pos_last_opened = ImVec2(_draw_pos_desc.x + _width_name_desc + frame_padding.x * 2, (_header_size.y - style::font_size()) / 2);
		_draw_pos_path		  = _draw_pos_last_opened + ImVec2(_width_last_opened + frame_padding.x * 2, 0);

		_item_size		= _header_size;
		_item_size.x   -= _draw_pos_item.x * 2;
		_item_rounding	= 5.f * GEctx->dpi_scale;
	}

	void show()
	{
		static auto opened = false;

		if (widgets::begin_popup_modal("Project_Browser", NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings))
		{
			_draw();
			widgets::end_popup();
		}

		if (opened is_false)
		{
			widgets::open_popup("Project_Browser");
			opened = true;
		}
	}
}	 // namespace editor::view::project_browser
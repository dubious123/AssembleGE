#pragma once
#include "editor_common.h"

constexpr auto PROJECT_EXTENSION			= ".assemble";
constexpr auto GAMECODE_DIRECTORY			= "game_code";
constexpr auto GAMECODE_GENERATED_DIRECTORY = "generated";
constexpr auto PROJECT_DATA_FILE_NAME		= "project_data.xml";

struct editor_command;

extern editor_context* GEctx;

namespace editor
{
	extern const editor_command cmd_add_new;
	extern const editor_command cmd_remove;
	extern const editor_command cmd_select_new;
	extern const editor_command cmd_add_select;
	extern const editor_command cmd_deselect;

	void init();
	void run();
	void on_project_loaded();
	void on_project_unloaded();

	void select_new(editor_id);
	void add_select(editor_id);
	void deselect(editor_id);
	bool is_selected(editor_id id);
	void add_right_click_source(editor_id id);
	void add_left_click_source(editor_id id);
	void add_left_right_click_source(editor_id id);

	editor_id					  get_current_selection();
	const std::vector<editor_id>& get_all_selections();
	bool						  is_selection_vec_empty();

	// bool add_context_item(std::string path, const editor_command* command, editor_id id = INVALID_ID);
}	 // namespace editor

namespace editor::id
{
	editor_id get_new(editor_data_type type);

	void delete_id(editor_id id);

	void restore(editor_id id);

	void reset();
}	 // namespace editor::id

namespace editor::undoredo
{
	struct undo_redo_cmd
	{
		std::string											   name;
		std::function<void(editor::utilities::memory_handle*)> undo;
		std::function<void(editor::utilities::memory_handle*)> redo;
		editor::utilities::memory_handle					   _memory_handle;

		undo_redo_cmd(std::string name, std::function<void(editor::utilities::memory_handle*)> redo, std::function<void(editor::utilities::memory_handle*)> undo)
			: name(name), redo(redo), undo(undo), _memory_handle() { }

		undo_redo_cmd(std::string name, std::function<void(utilities::memory_handle*)> redo, std::function<void(utilities::memory_handle*)> undo, void* p_mem)
			: name(name), redo(redo), undo(undo), _memory_handle({ nullptr, p_mem }) { }

		undo_redo_cmd(undo_redo_cmd&& other) noexcept
			: name(other.name), undo(other.undo), redo(other.redo), _memory_handle(std::move(other._memory_handle)) { }

		undo_redo_cmd& operator=(undo_redo_cmd&& other) noexcept
		{
			if (this != &other)
			{
				name		   = other.name;
				undo		   = other.undo;
				redo		   = other.redo;
				_memory_handle = std::move(other._memory_handle);
			}

			return *this;
		}

		undo_redo_cmd(const undo_redo_cmd&)		 = delete;
		undo_redo_cmd& operator=(undo_redo_cmd&) = delete;
	};

	void add(undo_redo_cmd&& undo_redo);
	void add_and_redo(undo_redo_cmd&& undo_redo);
	void on_project_loaded();

	// void Init();
}	 // namespace editor::undoredo

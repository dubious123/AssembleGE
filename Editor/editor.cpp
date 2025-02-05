#include "pch.h"
#include "editor.h"
#include "editor_style.h"
#include "editor_widgets.h"
#include "platform.h"
#include "editor_view.h"
#include "editor_ctx_item.h"
#include "game_project\game.h"
#include "editor_models.h"

editor_context* GEctx = nullptr;

void editor::init()
{
	auto hwnd = platform::init(editor::platform::wnd_proc);

	GEctx				   = new editor_context();
	GEctx->hwnd			   = hwnd;
	GEctx->dpi_scale	   = ::GetDpiForSystem() / 96.f;
	GEctx->icon_texture_id = platform::load_icon_image("Resources/AssembleGE_Icon.png");

	editor::style::reset_colors();
	editor::style::update_dpi_scale(GEctx->dpi_scale);

	// editor::id::init();
	//_load_ctx_menu();
	editor::logger::init();
	editor::view::init();
	editor::game::init();
	// Editor::UndoRedo::Init();
}

void editor::run()
{
	// DragAcceptFiles(hwnd, TRUE);

	platform::window_loop([]() {
		if (GEctx->dpi_changed)
		{
			editor::view::update_dpi_scale();
			GEctx->dpi_changed = false;
		}

		editor::view::show();

		ImGui::ShowDemoWindow();

		{
			ctx_item::on_frame_end();
			widgets::on_frame_end();
		}
	});


	delete GEctx;
}

namespace editor
{
	namespace
	{
		auto _selected_vec = std::vector<editor_id>();
	}	 // namespace

	bool is_selected(editor_id id)
	{
		return std::find(_selected_vec.begin(), _selected_vec.end(), id) != _selected_vec.end();
	}

	void select_new(editor_id id)
	{
		_selected_vec.clear();
		_selected_vec.push_back(id);
	}

	void add_select(editor_id id)
	{
		_selected_vec.push_back(id);
	}

	void deselect(editor_id id)
	{
		_selected_vec.erase(std::ranges::find(_selected_vec, id));
	}

	void add_right_click_source(editor_id id)
	{
		if (ImGui::IsMouseReleased(ImGuiMouseButton_Right) and widgets::is_item_hovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup))
		{
			auto it = std::find(_selected_vec.begin(), _selected_vec.end(), id);
			if (it == _selected_vec.end())
			{
				select_new(id);
			}
			else
			{
				std::iter_swap(it, _selected_vec.rbegin());
			}

			editor::view::ctx_popup::open();
		}
	}

	void add_left_click_source(editor_id id)
	{
		if (widgets::is_item_clicked(ImGuiMouseButton_Left))
		{
			if (platform::is_key_down(ImGuiKey_LeftCtrl))
			{
				if (is_selected(id))
				{
					cmd_deselect(id);

					// deselect(id);
				}
				else
				{
					cmd_add_select(id);
					// add_select(id);
				}
			}
			else
			{
				cmd_select_new(id);
				// select_new(id);
			}

			widgets::open_popup("ctx menu");
		}
	}

	void add_left_right_click_source(editor_id id)
	{
		add_left_click_source(id);
		add_right_click_source(id);
	}

	editor_id get_current_selection()
	{
		return _selected_vec.empty() ? editor_id() : _selected_vec.back();
	}

	const std::vector<editor_id>& get_all_selections()
	{
		return _selected_vec;
	}

	bool is_selection_vec_empty()
	{
		return _selected_vec.empty();
	}

	const editor_command cmd_select_new(
		"Select new",
		ImGuiKey_None,
		[](editor_id id) { return _selected_vec.empty() or get_current_selection() != id; },
		[](editor_id id) {
			auto _selected_before = _selected_vec;

			undoredo::add_and_redo({ std::format("Select {}", id.str()),
									 [id](utilities::memory_handle*) {
										 _selected_vec.clear();
										 _selected_vec.push_back(id);
									 },
									 [id, _selected_before](utilities::memory_handle*) {
										 _selected_vec = _selected_before;
									 } });
		});

	const editor_command cmd_add_select(
		"Add Select",
		ImGuiKey_None,
		[](editor_id id) {
			return get_current_selection().type() == id.type() and std::find(_selected_vec.begin(), _selected_vec.end(), id) == _selected_vec.end();
		},
		[](editor_id id) {
			undoredo::add_and_redo(
				{ std::format("Add Select {}", id.str()),
				  [id](utilities::memory_handle*) { _selected_vec.push_back(id); },
				  [id](utilities::memory_handle*) { _selected_vec.pop_back(); } });
		});

	const editor_command cmd_deselect(
		"Deselect",
		ImGuiKey_None,
		[](editor_id id) { return std::ranges::find(_selected_vec, id) != _selected_vec.end(); },
		[](editor_id id) {
			auto index = std::ranges::find(_selected_vec, id) - _selected_vec.begin();

			undoredo::add_and_redo(
				{ std::format("Deselect {}", id.str()),
				  [=](utilities::memory_handle*) { _selected_vec.erase(std::next(_selected_vec.begin(), index)); },
				  [=](utilities::memory_handle*) { _selected_vec.insert(std::next(_selected_vec.begin(), index), id); } });
		});
}	 // namespace editor

namespace editor::id
{
	namespace
	{
		std::array<editor_id, DataType_Count> _next_id_vec = []() {
			std::array<editor_id, DataType_Count> arr;
			for (auto i = 0; i < DataType_Count; ++i)
			{
				arr[i] = editor_id(i, 0, 0);
			}

			return arr;
		}();
		std::vector<editor_id> _deleted_id_set[DataType_Count];
	}	 // namespace

	void reset()
	{
		for (auto vec : _deleted_id_set)
		{
			vec.clear();
		}

		for (auto i = 0; i < DataType_Count; ++i)
		{
			_next_id_vec[i] = editor_id(i, 0, 0);
		}
	}

	editor_id get_new(editor_data_type type)
	{
		if (_deleted_id_set[type].empty())
		{
			auto res		   = _next_id_vec[type];
			_next_id_vec[type] = editor_id(type, 0, (res.key() + 1));
			return res;
		}
		else
		{
			auto res = _deleted_id_set[type].back();
			res.increase_gen();

			_deleted_id_set[type].pop_back();
			return res;
		}
	}

	void delete_id(editor_id id)
	{
		if (std::ranges::find(_deleted_id_set[id.type()], id) == _deleted_id_set[id.type()].end())
		{
			_deleted_id_set[id.type()].push_back(id);
		}
	}

	void restore(editor_id id)
	{
		auto it = std::ranges::find(_deleted_id_set[id.type()], id);
		assert(it != _deleted_id_set[id.type()].end());
		_deleted_id_set[id.type()].erase(it);
	}
}	 // namespace editor::id

namespace editor::undoredo
{
	namespace
	{
		std::vector<undo_redo_cmd> _undo_vec;
		std::vector<undo_redo_cmd> _redo_vec;

		editor_command _cmd_undo {
			"Undo",
			ImGuiKey_Z | ImGuiKey_ModCtrl,
			[](editor_id _) {
				return _undo_vec.empty() is_false;
			},
			[](editor_id _) {
				_undo_vec.back().undo(&_undo_vec.back()._memory_handle);
				_redo_vec.emplace_back(std::move(_undo_vec.back()));
				_undo_vec.pop_back();

				editor::logger::info(std::format("undo command : {}", _redo_vec.back().name));
			}
		};
		editor_command _cmd_redo {
			"Redo",
			ImGuiKey_Z | ImGuiKey_ModCtrl | ImGuiKey_ModShift,
			[](editor_id _) {
				return _redo_vec.empty() is_false;
			},
			[](editor_id _) {
				_redo_vec.back().redo(&_redo_vec.back()._memory_handle);
				_undo_vec.emplace_back(std::move(_redo_vec.back()));
				_redo_vec.pop_back();

				editor::logger::info(std::format("redo command : {}", _undo_vec.back().name));
			}
		};
	}	 // namespace

	void on_project_loaded()
	{
		_redo_vec.clear();
		_undo_vec.clear();

		auto res  = editor::ctx_item::add_context_item("Main Menu\\Edit\\Undo", &_cmd_undo);
		res		 &= editor::ctx_item::add_context_item("Main Menu\\Edit\\Redo", &_cmd_redo);
		assert(res);
	}

	void add(undo_redo_cmd&& undo_redo)
	{
		logger::info(undo_redo.name);
		_undo_vec.emplace_back(std::move(undo_redo));
		_redo_vec.clear();
	}

	void add_and_redo(undo_redo_cmd&& undo_redo)
	{
		undo_redo.redo(&undo_redo._memory_handle);
		undoredo::add(std::move(undo_redo));
	}

	void print_all()
	{
		logger::info("Redo----------------------------");
		for (auto& redo : _redo_vec)
		{
			logger::info("Redo : {}", redo.name);
		}

		logger::info("Undo----------------------------");

		for (auto& undo : _undo_vec)
		{
			logger::info("Undo : {}", undo.name);
		}
		logger::info("----------------------------");
	}
}	 // namespace editor::undoredo

namespace editor
{
	void on_project_loaded()
	{
		logger::clear();
		ctx_item::on_project_loaded();
		undoredo::on_project_loaded();
		models::on_project_loaded();
		view::on_project_loaded();
		game::on_project_loaded();
		editor::game::save_project_open_datas();
		_selected_vec.clear();
	}

	void on_project_unloaded()
	{
		ctx_item::on_project_unloaded();
		models::on_project_unloaded();
		game::on_project_unloaded();
		//_cmd_id = 0;
		view::on_project_unloaded();
		_selected_vec.clear();
		id::reset();
		//_current_select_ctx_item = INVALID_ID;
	}
}	 // namespace editor

std::string editor::utilities::read_file(const std::filesystem::path path)
{
	std::ifstream stream;
	stream.open(path);
	std::stringstream ss_project_data;
	ss_project_data << stream.rdbuf();
	return ss_project_data.str();
}

void editor::utilities::create_file(const std::filesystem::path path, const std::string content)
{
	std::ofstream project_file(path);
	project_file << content.c_str();
	project_file.close();
}

std::vector<std::string> editor::utilities::split_string(const std::string& str, const std::string& delims)
{
	auto re	   = std::regex(std::format("[{}]", delims));
	auto first = std::sregex_token_iterator { str.begin(), str.end(), re, -1 };
	auto last  = std::sregex_token_iterator {};
	return { first, last };
}

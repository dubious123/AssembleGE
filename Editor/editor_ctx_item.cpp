#include <pch.h>
#include <editor.h>
// #include "editor_common.h"
#include "editor_style.h"
#include "editor_widgets.h"
#include <editor_ctx_item.h>
#include "platform.h"

// helper
namespace
{
	using namespace editor::ctx_item;

	ctx_menu_node xml_node_to_ctx_menu_node(pugi::xml_node node)
	{
		auto name	  = node.attribute("name").as_string();
		auto ctx_menu = ctx_menu_node(name);
		{
			ctx_menu.separator	  = node.attribute("separator").as_bool();
			ctx_menu.shortcut_str = node.attribute("shortcut").as_string();
		}

		ctx_menu.child_count = std::distance(node.children().begin(), node.children().end());
		if (ctx_menu.child_count)
		{
			ctx_menu.p_children = (ctx_menu_node*)malloc(sizeof(ctx_menu_node) * ctx_menu.child_count);

			for (auto* p_child = ctx_menu.p_children; auto child_node : node.children())
			{
				new (p_child) ctx_menu_node(xml_node_to_ctx_menu_node(child_node));
				++p_child;
			}
		}

		return ctx_menu;
	}

	editor_data_type str_to_type(std::string str)
	{
		if (str.compare("Entity") == 0)
		{
			return DataType_Entity;
		}
		else if (str.compare("Project") == 0)
		{
			return DataType_Project;
		}
		else if (str.compare("Scene") == 0)
		{
			return DataType_Scene;
		}
		else if (str.compare("World") == 0)
		{
			return DataType_World;
		}
		else if (str.compare("SubWorld") == 0)
		{
			return DataType_SubWorld;
		}
		else if (str.compare("Component") == 0)
		{
			return DataType_Component;
		}
		else if (str.compare("System") == 0)
		{
			return DataType_System;
		}
		else if (str.compare("Struct") == 0)
		{
			return DataType_Struct;
		}
		else if (str.compare("Field") == 0)
		{
			return DataType_Field;
		}
		else if (str.compare("Text") == 0)
		{
			return DataType_Editor_Text;
		}
		else if (str.compare("Editor_Command") == 0)
		{
			return DataType_Editor_Command;
		}
		else if (str.compare("Editor_UndoRedo") == 0)
		{
			return DataType_Editor_UndoRedo;
		}
		else
		{
			assert(false and "invalid data type");
			return DataType_InValid;
		}
	}

	std::string type_to_str(editor_data_type type)
	{
		switch (type)
		{
		case DataType_Entity:
			return "Entity";

		case DataType_Project:
			return "Project";

		case DataType_Scene:
			return "Scene";

		case DataType_World:
			return "World";

		case DataType_SubWorld:
			return "SubWorld";

		case DataType_Component:
			return "Component";

		case DataType_System:
			return "System";

		case DataType_Struct:
			return "Struct";

		case DataType_Field:
			return "Field";

		case DataType_Editor_Text:
			return "Text";

		case DataType_Editor_Command:
			return "Editor_Command";

		case DataType_Editor_UndoRedo:
			return "Editor_UndoRedo";

		case DataType_Count:
		case DataType_InValid:
		default:
			assert(false and "invalid type");
			return "invalid_type";
		}
	}

	ctx_menu_node* add_child(ctx_menu_node* p_parent, ctx_menu_node&& child)
	{
		++p_parent->child_count;
		if (p_parent->p_children == nullptr)
		{
			assert(p_parent->child_count == 1);
			p_parent->p_children = (ctx_menu_node*)malloc(sizeof(ctx_menu_node));
		}
		else
		{
			p_parent->p_children = (ctx_menu_node*)realloc(p_parent->p_children, sizeof(ctx_menu_node) * p_parent->child_count);
		}

		new (&p_parent->p_children[p_parent->child_count - 1]) ctx_menu_node { std::move(child) };
		return &p_parent->p_children[p_parent->child_count - 1];
	}

	void clear_node(ctx_menu_node* p_node)
	{
		if (p_node->child_count > 0)
		{
			std::ranges::for_each(std::views::iota(p_node->p_children) | std::views::take(p_node->child_count), clear_node);
		}

		free(p_node->p_children);	 // also free p_leaf
	}
}	 // namespace

// data
namespace
{
	static constinit auto						   _context_menu_xml_path = "Resources/Editor_CtxMenu.xml";
	std::unordered_map<std::string, ctx_menu_node> _ctx_node_map;
	auto										   _shortcut_command_vec = std::vector<const editor_command*>();
}	 // namespace

namespace editor::ctx_item
{
	void on_project_loaded()
	{
		auto ctx_menu_doc = pugi::xml_document();
		ctx_menu_doc.load_file(_context_menu_xml_path);

		auto _ctx_item_xml_node = ctx_menu_doc.first_child().first_child();
		for (auto node = _ctx_item_xml_node; node; node = node.next_sibling())
		{
			auto name			= node.attribute("name").as_string();
			_ctx_node_map[name] = xml_node_to_ctx_menu_node(node);
		}
	}

	bool add_context_item(std::string path, const editor_command* p_command, editor_id arg_id)
	{
		auto tokens = editor::utilities::split_string(path, "\\\\");
		if (tokens.empty())
		{
			assert(false and "invalid path");
		}

		auto* p_ctx_node = [=]() {
			auto* p_node = &_ctx_node_map[*tokens.begin()];
			for (auto&& token : tokens | std::views::drop(1))
			{
				auto children_view = std::views::iota(p_node->p_children) | std::views::take(p_node->child_count);
				auto child_it	   = std::ranges::find_if(children_view, [token](auto* p_child) { return p_child->name == token; });
				if (child_it != children_view.end())
				{
					p_node = *child_it;
				}
				else
				{
					p_node = add_child(p_node, { token });
				}
			}

			return p_node;
		}();

		if (p_ctx_node->leaf is_nullptr)
		{
			p_ctx_node->leaf = (ctx_menu_leaf*)malloc(sizeof(ctx_menu_leaf));
		}

		p_ctx_node->leaf->p_cmd	  = p_command;
		p_ctx_node->leaf->cmd_arg = arg_id;

		if (p_command->_shortcut_key != ImGuiKey_None)
		{
			_shortcut_command_vec.push_back(p_command);
		}

		return true;
	}

	void on_frame_end()
	{
		for (auto p_command : _shortcut_command_vec)
		{
			constexpr auto key_mask = 0x07ff;

			auto mode_shift = (p_command->_shortcut_key & ImGuiMod_Shift) != 0;
			auto mode_ctrl	= (p_command->_shortcut_key & ImGuiMod_Ctrl) != 0;
			auto key		= p_command->_shortcut_key & key_mask;
			auto res		= true;

			//               has mod / does not have mod
			// pressed         0             X
			// not pressed     X             O            not A xor B
			res &= (mode_shift == platform::is_key_down(ImGuiMod_Shift));
			res &= (mode_ctrl == platform::is_key_down(ImGuiMod_Ctrl));
			res &= ((key != ImGuiKey_None) == platform::is_key_pressed((ImGuiKey)key));

			if (res)
			{
				(*p_command)();
				// close_ctx_popup = true;
			}
		}
	}

	void on_project_unloaded()
	{
		for (auto& node : _ctx_node_map | std::views::values)
		{
			clear_node(&node);
		}

		_ctx_node_map.clear();
	}
}	 // namespace editor::ctx_item

namespace
{
	bool _context_item(const ctx_menu_node* p_node)
	{
		auto executed = false;
		if (p_node->child_count > 0)
		{
			if (editor::widgets::begin_menu(p_node->name, nullptr))
			{
				std::ranges::for_each(std::views::iota(p_node->p_children) | std::views::take(p_node->child_count), _context_item);

				editor::widgets::end_menu();
			}
		}
		else
		{
			if (editor::widgets::menu_item(p_node->name,
										   nullptr,
										   p_node->shortcut_str,
										   false,
										   p_node->leaf is_not_nullptr and p_node->leaf->p_cmd->can_execute(p_node->leaf->cmd_arg)))
			{
				p_node->leaf->p_cmd->execute(p_node->leaf->cmd_arg);
				executed = true;
			}
		}

		if (p_node->separator)
		{
			editor::widgets::separator();
		}

		return executed;
	}
}	 // namespace

namespace editor::view::ctx_popup
{
	auto _open = false;

	void open()
	{
		_open = true;
	}

	void show()
	{
		if (_open)
		{
			editor::widgets::open_popup("ctx menu");
			_open = false;
		}

		editor::style::push_var(ImGuiStyleVar_FramePadding, ImVec2(8.4f, 3.f) * GEctx->dpi_scale);
		editor::style::push_color(ImGuiCol_Header, style::get_color_v4(ImGuiCol_HeaderHovered));
		if (editor::widgets::begin_popup("ctx menu"))
		{
			assert(editor::get_current_selection() != INVALID_ID);
			auto& node = _ctx_node_map[type_to_str(editor::get_current_selection().type())];
			std::ranges::for_each(std::views::iota(node.p_children) | std::views::take(node.child_count), _context_item);

			editor::widgets::end_popup();
		}
		editor::style::pop_color();
		editor::style::pop_var();
	}
}	 // namespace editor::view::ctx_popup

namespace editor::view::main_menu_bar
{
	void show()
	{
		static auto window_size	 = ImVec2();
		auto		window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDocking;

		widgets::set_cursor_pos({ CAPTION_ICON_CPOS.x * GEctx->dpi_scale, CAPTION_ICON_CPOS.y * GEctx->dpi_scale });
		widgets::image(GEctx->icon_texture_id, ImVec2(CAPTION_ICON_SIZE.x * GEctx->dpi_scale, CAPTION_ICON_SIZE.y * GEctx->dpi_scale));
		widgets::sameline();

		style::push_var(ImGuiStyleVar_FramePadding, ImVec2(8.4f, 3.f) * GEctx->dpi_scale);
		style::push_var(ImGuiStyleVar_ItemSpacing, ImVec2());
		style::push_color(ImGuiCol_HeaderHovered, COL_BG_SELECTED);
		style::push_color(ImGuiCol_HeaderActive, style::get_color_v4(ImGuiCol_PopupBg));
		platform::set_next_window_pos(platform::get_window_pos() /*GImGui->CurrentWindow->Pos*/ + ImVec2(widgets::get_cursor_pos_x(), (CAPTION_ICON_CPOS.y * 2 * GEctx->dpi_scale + CAPTION_ICON_SIZE.y * GEctx->dpi_scale - (style::font_size() + style::frame_padding().y * 2)) * 0.5f));
		if (widgets::begin_child("Main Menu Bar", window_size, false, window_flags))
		{
			platform::get_window_info().layout()			= ImGuiLayoutType_Horizontal;
			platform::get_window_info().menubar_appending() = true;
			platform::get_window_info().nav_layer()			= ImGuiNavLayer_Menu;
			window_size										= ImVec2();
			style::push_var(ImGuiStyleVar_WindowPadding, ImVec2(8.f, 8.f) * GEctx->dpi_scale);

			auto& main_menu_ctx_node = _ctx_node_map["Main Menu"];
			for (auto* p_node : std::views::iota(main_menu_ctx_node.p_children) | std::views::take(main_menu_ctx_node.child_count))
			{
				_context_item(p_node);

				window_size.x += widgets::get_item_rect().GetSize().x;
				window_size.y  = widgets::get_item_rect().GetSize().y;
			}

			window_size.y			  += 1;
			GEctx->main_menu_rect.Max  = widgets::get_item_rect().Max;

			style::pop_var(1);
		}
		style::pop_var(2);
		style::pop_color(2);
		widgets::end_child();
	}
}	 // namespace editor::view::main_menu_bar

#pragma once

namespace editor::ctx_item
{
	struct ctx_menu_leaf
	{
		const editor_command* p_cmd;
		editor_id			  cmd_arg;
	};

	struct ctx_menu_node
	{
		std::string name;
		bool		separator;
		uint32		child_count;
		std::string shortcut_str;
		ImGuiKey	shortcut;

		union
		{
			ctx_menu_node* p_children = nullptr;
			ctx_menu_leaf* leaf;
		};

		ctx_menu_node() = default;
		ctx_menu_node(std::string _name) : name(_name), separator(false), p_children(nullptr), child_count(0), shortcut(ImGuiKey_None) { };
		ctx_menu_node(std::string _name, bool _saperator) : name(_name), separator(_saperator), p_children(nullptr), child_count(0), shortcut(ImGuiKey_None) { };

		ctx_menu_node(const ctx_menu_node&)			   = delete;
		ctx_menu_node& operator=(const ctx_menu_node&) = delete;

		ctx_menu_node(ctx_menu_node&& other) noexcept : name(std::move(other.name)), separator(other.separator), child_count(other.child_count), shortcut(other.shortcut), shortcut_str(std::move(other.shortcut_str)), p_children(other.p_children)
		{
			other.p_children  = nullptr;
			other.child_count = 0;
		}

		ctx_menu_node& operator=(ctx_menu_node&& other) noexcept
		{
			name		 = std::move(other.name);
			separator	 = other.separator;
			child_count	 = other.child_count;
			shortcut_str = std::move(other.shortcut_str);
			shortcut	 = other.shortcut;
			p_children	 = other.p_children;

			other.p_children  = nullptr;
			other.child_count = 0;

			return *this;
		}
	};

	void on_project_loaded();
	bool add_context_item(std::string path, const editor_command* p_command, editor_id arg_id = INVALID_ID);

	void on_frame_end();
	void on_project_unloaded();
}	 // namespace editor::ctx_item

namespace editor::view::ctx_popup
{
	void open();
	void show();
}	 // namespace editor::view::ctx_popup

namespace editor::view::main_menu_bar
{
	void show();
}
#pragma once

namespace editor::widgets
{
	uint32 get_id(const char* str);

	void draw_line(const ImVec2& p1, const ImVec2& p2, ImU32 col, float thickness = 1.0f);
	void draw_rect(const ImVec2& p_min, const ImVec2& p_max, ImU32 col, float rounding = 0.0f, ImDrawFlags flags = 0, float thickness = 1.0f);	  // a: upper-left, b: lower-right (== upper-left + size)
	void draw_rect_filled(const ImRect& rect, ImU32 col, float rounding = 0.0f, ImDrawFlags flags = 0);
	void draw_poly_filled(const ImVec2* points, const int points_count, ImU32 col);
	void draw_text(const ImVec2& pos, ImU32 col, const char* text_begin, const char* text_end = nullptr);
	void draw_text_clipped(const ImVec2& pos_min, const ImVec2& pos_max, const char* text, const char* text_end, const ImVec2* text_size_if_known, const ImVec2& align = ImVec2(0, 0), const ImRect* clip_rect = nullptr);
	void draw_frame(ImVec2 p_min, ImVec2 p_max, ImU32 fill_col, bool border = true, float rounding = 0.0f);

	bool button_behavior(const ImRect& bb, ImGuiID id, bool* out_hovered, bool* out_held, ImGuiButtonFlags flags = 0);
	bool is_item_clicked(ImGuiMouseButton mouse_button = 0);
	bool is_item_hovered(ImGuiHoveredFlags flags = 0);
	bool is_item_active();
	bool is_item_edited();
	bool is_item_activated();
	bool is_item_deactivated();
	bool is_item_deactivated_after_edit();


	bool begin(const char* name, bool* p_open = nullptr, ImGuiWindowFlags flags = 0);
	void end();
	bool begin_child(const char* str_id, const ImVec2& size = ImVec2(0, 0), bool border = false, ImGuiWindowFlags flags = 0);
	void end_child();

	bool begin_popup(const char* str_id, ImGuiWindowFlags flags = 0);

	bool begin_popup_modal(const char* name, bool* p_open, ImGuiWindowFlags flags);
	bool begin_popup_modal(ImGuiID id, const char* name, bool* p_open, ImGuiWindowFlags flags);
	void end_popup();

	void open_popup(const char* str_id, ImGuiPopupFlags popup_flags = 0);

	void close_popup();

	bool menu_item(std::string label, const char* icon, std::string shortcut, bool selected = false, bool enabled = true);
	bool menu_item(const char* label, const char* icon, const char* shortcut, bool selected = false, bool enabled = true);

	bool begin_menu(std::string label, const char* icon, bool enabled = true);
	bool begin_menu(const char* label, const char* icon, bool enabled = true);

	void end_menu();

	bool add_item(const ImRect& bb, ImGuiID id, const ImRect* nav_bb = nullptr, ImGuiItemFlags extra_flags = 0);

	void item_size(const ImVec2& size, float text_baseline_y = -1.0f);

	const ImRect& get_item_rect();

	const ImVec2& get_cursor_pos();
	float		  get_cursor_pos_x();
	float		  get_cursor_pos_y();

	void set_cursor_pos(const ImVec2& local_pos);
	void set_cursor_pos_x(float local_x);
	void set_cursor_pos_y(float local_y);

	void image(ImTextureID user_texture_id, const ImVec2& size, const ImVec2& uv0 = ImVec2(0, 0), const ImVec2& uv1 = ImVec2(1, 1), const ImVec4& tint_col = ImVec4(1, 1, 1, 1), const ImVec4& border_col = ImVec4(0, 0, 0, 0));

	void text(const char* fmt, ...);

	void text(std::string, ...);

	void text_colored(const ImVec4& col, const char* fmt, ...);

	bool selectable(const char* label, bool selected, ImGuiSelectableFlags flags = 0, const ImVec2& size_arg = { 0, 0 }, bool border = false);

	bool selectable(const char* label, bool* p_selected, ImGuiSelectableFlags flags = 0, const ImVec2& size_arg = { 0, 0 }, bool border = false);

	bool selectable(std::string label, bool selected, ImGuiSelectableFlags flags = 0, const ImVec2& size_arg = { 0, 0 }, bool border = false);

	bool selectable(std::string label, bool* p_selected, ImGuiSelectableFlags flags = 0, const ImVec2& size_arg = { 0, 0 }, bool border = false);

	bool button(const char* label, const ImVec2& size = ImVec2(0, 0));

	bool tree_node(std::string label, ImGuiTreeNodeFlags flags = 0);

	void tree_pop();

	void separator();
	void sameline(float offset_from_start_x = 0.0f, float spacing = -1.0f);
	void newline();
	void spacing();

	bool input_text(const char* label, char* buf, size_t buf_size, ImGuiInputTextFlags flags = 0, ImGuiInputTextCallback callback = nullptr, void* user_data = nullptr);
	bool input_text_with_hint(const char* label, const char* hint, char* buf, size_t buf_size, ImGuiInputTextFlags flags = 0, ImGuiInputTextCallback callback = nullptr, void* user_data = nullptr);

	void push_font(ImFont* font);
	void pop_font();

	ImVec2 calc_text_size(const char* text, const char* text_end = nullptr, bool hide_text_after_double_hash = false, float wrap_width = -1.0f);

	void progress_modal_msg(std::string msg);
	void progress_modal(const char* title, std::function<bool()> task, std::function<void(bool)> callback = std::function<void(bool)>());

	void on_frame_end();
}	 // namespace editor::widgets

namespace editor::widgets
{
	bool component_drag(editor_id c_id);
	bool editable_header(editor_id target_id, std::string target_name);
}	 // namespace editor::widgets
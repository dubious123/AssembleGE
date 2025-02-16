#pragma once

struct window_info
{
	ImGuiWindow* p_window;

	auto& default_item_width() { return p_window->ItemWidthDefault; }

	auto& layout() { return p_window->DC.LayoutType; }

	auto& menubar_appending() { return p_window->DC.MenuBarAppending; }

	auto& nav_layer() { return p_window->DC.NavLayerCurrent; }
};

struct viewport_info
{
	ImGuiViewport* p_viewport;

	auto& id() { return p_viewport->ID; }

	auto& pos() { return p_viewport->Pos; }

	auto& size() { return p_viewport->Size; }
};

namespace editor::platform
{
	const ImVec2& get_window_pos();
	const ImVec2& get_window_size();
	const ImVec2& get_monitor_size();
	const ImVec2& get_mouse_pos();
	float		  get_mouse_wheel();
	void		  add_mouse_source_event(ImGuiMouseSource source);
	void		  add_mouse_button_event(ImGuiMouseButton mouse_button, bool down);

	bool is_key_down(ImGuiKey key);
	bool is_key_pressed(ImGuiKey key);

	viewport_info get_main_viewport();

	window_info get_window_info();

	void set_next_window_pos(const ImVec2& pos, ImGuiCond cond = 0, const ImVec2& pivot = ImVec2(0, 0));
	void set_next_window_size(const ImVec2& size, ImGuiCond cond = 0);
	void set_next_window_viewport(ImGuiID id);

	bool mouse_clicked(ImGuiMouseButton mouse_button = 0);

	bool is_window_maximized(void* hwnd);
	bool is_in_client(const void* p_screen_pos);
}	 // namespace editor::platform

namespace editor::platform
{
	HWND init(LRESULT(WndProc)(HWND, UINT, WPARAM, LPARAM));

	void add_on_wm_activate(std::function<void()>);

	void			   close();
	bool			   CreateDeviceD3D(HWND hwnd);
	void			   CleanupDeviceD3D();
	unsigned long long load_icon_image(const char* path);

	void new_frame();

	void render();

	void wm_size(LPARAM, WPARAM);

	LRESULT WINAPI wnd_proc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

	void window_loop(void (*editor_loop)());
}	 // namespace editor::platform
#include "editor.h"

const ImVec2& editor::platform::get_window_pos()
{
	return GImGui->CurrentWindow->Pos;
}

const ImVec2& editor::platform::get_window_size()
{
	return GImGui->CurrentWindow->Size;
}

const ImVec2& editor::platform::get_monitor_size()
{
	return ImGui::GetPlatformIO().Monitors[0].MainSize;
}

const ImVec2& editor::platform::get_mouse_pos()
{
	return GImGui->IO.MousePos;
}

float editor::platform::get_mouse_wheel()
{
	return GImGui->IO.MouseWheel;
}

void editor::platform::add_mouse_source_event(ImGuiMouseSource source)
{
	GImGui->IO.AddMouseSourceEvent(source);
}

void editor::platform::add_mouse_button_event(ImGuiMouseButton mouse_button, bool down)
{
	GImGui->IO.AddMouseButtonEvent(mouse_button, down);
}

bool editor::platform::is_key_down(ImGuiKey key)
{
	return ImGui::IsKeyDown(key);
}

bool editor::platform::is_key_pressed(ImGuiKey key)
{
	return ImGui::IsKeyPressed(key);
}

viewport_info editor::platform::get_main_viewport()
{
	return { GImGui->Viewports[0] };
}

window_info editor::platform::get_window_info()
{
	return { ImGui::GetCurrentWindowRead() };
}

void editor::platform::set_next_window_pos(const ImVec2& pos, ImGuiCond cond, const ImVec2& pivot)
{
	assert(cond == 0 || (cond != 0 && (cond & (cond - 1)) == 0));	 // Make sure the user doesn't attempt to combine multiple condition flags.
	GImGui->NextWindowData.Flags	   |= ImGuiNextWindowDataFlags_HasPos;
	GImGui->NextWindowData.PosVal		= pos;
	GImGui->NextWindowData.PosPivotVal	= pivot;
	GImGui->NextWindowData.PosCond		= cond ? cond : ImGuiCond_Always;
	GImGui->NextWindowData.PosUndock	= true;
}

void editor::platform::set_next_window_size(const ImVec2& size, ImGuiCond cond)
{
	assert(cond == 0 || (cond != 0 && (cond & (cond - 1)) == 0));	 // Make sure the user doesn't attempt to combine multiple condition flags.
	GImGui->NextWindowData.Flags	|= ImGuiNextWindowDataFlags_HasSize;
	GImGui->NextWindowData.SizeVal	 = size;
	GImGui->NextWindowData.SizeCond	 = cond ? cond : ImGuiCond_Always;
}

void editor::platform::set_next_window_viewport(ImGuiID id)
{
	GImGui->NextWindowData.Flags	  |= ImGuiNextWindowDataFlags_HasViewport;
	GImGui->NextWindowData.ViewportId  = id;
}

bool editor::platform::mouse_clicked(ImGuiMouseButton mouse_button)
{
	return GImGui->IO.MouseClicked[mouse_button];
}

#include "pch.h"
#include "editor_common.h"
#include "editor_style.h"

const ImVec2& editor::style::frame_padding()
{
	return GImGui->Style.FramePadding;
}

const ImVec2& editor::style::item_spacing()
{
	return GImGui->Style.ItemSpacing;
}

const ImVec2& editor::style::item_inner_spacing()
{
	return GImGui->Style.ItemInnerSpacing;
}

const float& editor::style::frame_rounding()
{
	return GImGui->Style.FrameRounding;
}

const ImVec2& editor::style::window_padding()
{
	return GImGui->Style.WindowPadding;
}

uint32 editor::style::get_color_u32(ImGuiCol idx, float alpha_mul)
{
	auto c	= GImGui->Style.Colors[idx];
	c.w	   *= GImGui->Style.Alpha * alpha_mul;
	return ImGui::ColorConvertFloat4ToU32(c);
}

uint32 editor::style::get_color_u32(ImGuiCol idx)
{
	const auto& c = GImGui->Style.Colors[idx];
	ImU32		out;
	out	 = ((ImU32)IM_F32_TO_INT8_SAT(c.x)) << IM_COL32_R_SHIFT;
	out |= ((ImU32)IM_F32_TO_INT8_SAT(c.y)) << IM_COL32_G_SHIFT;
	out |= ((ImU32)IM_F32_TO_INT8_SAT(c.z)) << IM_COL32_B_SHIFT;
	out |= ((ImU32)IM_F32_TO_INT8_SAT(c.w)) << IM_COL32_A_SHIFT;
	return out;
}

uint32 editor::style::get_color_u32(const ImVec4& col)
{
	return ImGui::ColorConvertFloat4ToU32({ col.x, col.y, col.z, col.w * GImGui->Style.Alpha });
}

uint32 editor::style::get_color_u32(uint32 col)
{
	return ImGui::GetColorU32(col);
}

void editor::style::push_color(ImGuiCol idx, uint32 col)
{
	ImGui::PushStyleColor(idx, col);
}

void editor::style::push_color(ImGuiCol idx, const ImVec4& col)
{
	ImGui::PushStyleColor(idx, col);
}

void editor::style::pop_color(int count)
{
	ImGui::PopStyleColor(count);
}

void editor::style::push_var(ImGuiStyleVar idx, float val)
{
	ImGui::PushStyleVar(idx, val);
}

void editor::style::push_var(ImGuiStyleVar idx, const ImVec2& val)
{
	ImGui::PushStyleVar(idx, val);
}

void editor::style::pop_var(int count)
{
	ImGui::PopStyleVar(count);
}

const ImVec4& editor::style::get_color_v4(ImGuiCol idx)
{
	return GImGui->Style.Colors[idx];
}

float editor::style::font_size()
{
	return GImGui->FontSize;
}

void editor::style::reset_colors()
{
	ImGuiStyle& style							 = ImGui::GetStyle();
	style.Colors[ImGuiCol_Text]					 = COL_TEXT;
	style.Colors[ImGuiCol_TextDisabled]			 = COL_TEXT_DISABLED;
	style.Colors[ImGuiCol_WindowBg]				 = COL_BLACK;		   // COL_BG_WINDOW;
	style.Colors[ImGuiCol_ChildBg]				 = COL_BLACK;		   // COL_BG_WINDOW;	   // COL_BG_POPUP;
	style.Colors[ImGuiCol_PopupBg]				 = COL_BG_ACTIVE;	   // COL_BG_WINDOW;	   // COL_BG_POPUP;
	style.Colors[ImGuiCol_Border]				 = COL_BD_SELECTED;	   // ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
	style.Colors[ImGuiCol_BorderShadow]			 = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	style.Colors[ImGuiCol_FrameBg]				 = ImVec4(0.24f, 0.24f, 0.24f, 1.00f);
	style.Colors[ImGuiCol_FrameBgHovered]		 = ImVec4(0.38f, 0.38f, 0.38f, 1.00f);
	style.Colors[ImGuiCol_FrameBgActive]		 = ImVec4(0.67f, 0.67f, 0.67f, 0.39f);
	style.Colors[ImGuiCol_TitleBg]				 = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
	style.Colors[ImGuiCol_TitleBgActive]		 = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
	style.Colors[ImGuiCol_TitleBgCollapsed]		 = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
	style.Colors[ImGuiCol_MenuBarBg]			 = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
	style.Colors[ImGuiCol_ScrollbarBg]			 = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
	style.Colors[ImGuiCol_ScrollbarGrab]		 = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
	style.Colors[ImGuiCol_ScrollbarGrabHovered]	 = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
	style.Colors[ImGuiCol_ScrollbarGrabActive]	 = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
	style.Colors[ImGuiCol_CheckMark]			 = ImVec4(0.11f, 0.64f, 0.92f, 1.00f);
	style.Colors[ImGuiCol_SliderGrab]			 = ImVec4(0.11f, 0.64f, 0.92f, 1.00f);
	style.Colors[ImGuiCol_SliderGrabActive]		 = ImVec4(0.08f, 0.50f, 0.72f, 1.00f);
	style.Colors[ImGuiCol_Button]				 = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
	style.Colors[ImGuiCol_ButtonHovered]		 = ImVec4(0.24f, 0.24f, 0.24f, 1.00f);
	style.Colors[ImGuiCol_ButtonActive]			 = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);
	style.Colors[ImGuiCol_Header]				 = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);
	style.Colors[ImGuiCol_HeaderHovered]		 = COL_BG_SELECTED;	   // ImVec4(0.25f, 0.25f, 0.25f, 1.00f); //selectable hovered
	style.Colors[ImGuiCol_HeaderActive]			 = COL_GRAY_2;		   // ImVec4(0.67f, 0.67f, 0.67f, 0.39f); //selectable active
	style.Colors[ImGuiCol_Separator]			 = style.Colors[ImGuiCol_Border];
	style.Colors[ImGuiCol_SeparatorHovered]		 = ImVec4(0.41f, 0.42f, 0.44f, 1.00f);
	style.Colors[ImGuiCol_SeparatorActive]		 = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
	style.Colors[ImGuiCol_ResizeGrip]			 = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	style.Colors[ImGuiCol_ResizeGripHovered]	 = ImVec4(0.29f, 0.30f, 0.31f, 0.67f);
	style.Colors[ImGuiCol_ResizeGripActive]		 = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
	style.Colors[ImGuiCol_Tab]					 = ImVec4(0.08f, 0.08f, 0.09f, 0.83f);
	style.Colors[ImGuiCol_TabHovered]			 = ImVec4(0.33f, 0.34f, 0.36f, 0.83f);
	style.Colors[ImGuiCol_TabActive]			 = ImVec4(0.23f, 0.23f, 0.24f, 1.00f);
	style.Colors[ImGuiCol_TabUnfocused]			 = ImVec4(0.08f, 0.08f, 0.09f, 1.00f);
	style.Colors[ImGuiCol_TabUnfocusedActive]	 = ImVec4(0.13f, 0.14f, 0.15f, 1.00f);
	style.Colors[ImGuiCol_DockingPreview]		 = ImVec4(0.26f, 0.59f, 0.98f, 0.70f);
	style.Colors[ImGuiCol_DockingEmptyBg]		 = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
	style.Colors[ImGuiCol_PlotLines]			 = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
	style.Colors[ImGuiCol_PlotLinesHovered]		 = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
	style.Colors[ImGuiCol_PlotHistogram]		 = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
	style.Colors[ImGuiCol_PlotHistogramHovered]	 = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
	style.Colors[ImGuiCol_TextSelectedBg]		 = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
	style.Colors[ImGuiCol_DragDropTarget]		 = ImVec4(0.11f, 0.64f, 0.92f, 1.00f);
	style.Colors[ImGuiCol_NavHighlight]			 = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
	style.Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
	style.Colors[ImGuiCol_NavWindowingDimBg]	 = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
	style.Colors[ImGuiCol_ModalWindowDimBg]		 = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
	// When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
	if (GImGui->IO.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		style.Colors[ImGuiCol_WindowBg].w = 1.0f;
	}
}

void editor::style::reset_style()
{
	ImGuiStyle& style		 = ImGui::GetStyle();
	style.WindowBorderSize	 = 1.0f;
	style.ChildBorderSize	 = 1.0f;
	style.PopupBorderSize	 = 1.0f;
	style.FrameBorderSize	 = 1.0f;
	style.TabBorderSize		 = 0.0f;
	style.WindowRounding	 = 0.0f;
	style.ChildRounding		 = 0.0f;
	style.PopupRounding		 = 0.0f;
	style.FrameRounding		 = 2.3f;
	style.ScrollbarRounding	 = 0.0f;
	style.GrabRounding		 = 2.3f;
	style.TabRounding		 = 0.0f;
	style.FramePadding		 = ImVec2(6.f, 6.f);	// ImVec2(6.f, 6.f);
	style.PopupBorderSize	 = .4f;
	style.ItemSpacing.y		 = 3.f;
	style.ItemInnerSpacing.x = 5.f;
	// style.ItemSpacing		= ImVec2(8.4f, 3.f);	// ImVec2(8.4f, 3.f);					   // 3.f;
	style.WindowPadding = ImVec2(8.f, 8.f);	   // 3.f;
	style.WindowMinSize = ImVec2(1.f, 1.f);

	// style.SelectableTextAlign = ImVec2(0, 0.5f);
	// style.ItemSpacing		= MENU_ITEM_PADDING * 2;				// ImVec2(8.4f, 3.f);					   // 3.f;
	// style.WindowPadding		= POPUP_PADDING + style.ItemSpacing;	// 3.f;
	//     When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
	if (GImGui->IO.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		style.WindowRounding = 0.0f;
	}
}

void editor::style::update_dpi_scale(float new_dpi_scale)
{
	ImGuiIO& io = ImGui::GetIO();

	ImGui_ImplDX12_InvalidateDeviceObjects();

	// Setup Dear ImGui style
	ImGuiStyle& style	 = ImGui::GetStyle();
	ImGuiStyle	styleold = style;
	style				 = ImGuiStyle();
	editor::style::reset_style();
	memcpy(style.Colors, styleold.Colors, sizeof(style.Colors));

	style.ScaleAllSizes(new_dpi_scale);
	GEctx->dpi_scale = new_dpi_scale;
	io.Fonts->Clear();

	auto res = std::filesystem::absolute("Resources/arial.ttf");

	GEctx->p_font_arial_default_13_5 = io.Fonts->AddFontFromFileTTF("Resources/arial.ttf", 13.5f * new_dpi_scale);
	GEctx->p_font_arial_bold_13_5	 = io.Fonts->AddFontFromFileTTF("Resources/arialbd.ttf", 13.5f * new_dpi_scale);

	GEctx->p_font_arial_default_16 = io.Fonts->AddFontFromFileTTF("Resources/arial.ttf", 16.f * new_dpi_scale);
	GEctx->p_font_arial_bold_16	   = io.Fonts->AddFontFromFileTTF("Resources/arialbd.ttf", 16.f * new_dpi_scale);

	GEctx->p_font_arial_default_18 = io.Fonts->AddFontFromFileTTF("Resources/arial.ttf", 18.f * new_dpi_scale);
	GEctx->p_font_arial_bold_18	   = io.Fonts->AddFontFromFileTTF("Resources/arialbd.ttf", 18.f * new_dpi_scale);

	GImGui->Font = GEctx->p_font_arial_default_13_5;

	ImGui_ImplDX12_CreateDeviceObjects();

	GEctx->dpi_changed = true;
	// todo
	// Editor::View::Update_Dpi_Scale();
}

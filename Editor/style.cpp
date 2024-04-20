#include "editor.h"

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

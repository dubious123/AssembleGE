#pragma once

namespace editor::style
{
	const ImVec2& frame_padding();
	const ImVec2& item_spacing();
	const ImVec2& item_inner_spacing();
	const float&  frame_rounding();
	const ImVec2& window_padding();
	float		  font_size();

	uint32		  get_color_u32(ImGuiCol idx, float alpha_mul);
	uint32		  get_color_u32(ImGuiCol idx);
	const ImVec4& get_color_v4(ImGuiCol idx);
	uint32		  get_color_u32(const ImVec4& col);
	uint32		  get_color_u32(uint32 col);

	void push_color(ImGuiCol idx, uint32 col);
	void push_color(ImGuiCol idx, const ImVec4& col);
	void pop_color(int count = 1);
	void push_var(ImGuiStyleVar idx, float val);
	void push_var(ImGuiStyleVar idx, const ImVec2& val);
	void pop_var(int count = 1);

	void reset_colors();
	void reset_style();
	void update_dpi_scale(float dpi_scale);
}	 // namespace editor::style
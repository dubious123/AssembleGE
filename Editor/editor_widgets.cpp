#include "pch.h"
#include "editor.h"
#include "editor_common.h"
#include "editor_widgets.h"
#include "editor_style.h"
#include "editor_models.h"

using namespace ImGui;
#define STR_MENU_SHORTCUT_SPACE_COUNT 8

namespace
{
	static bool _root_of_open_menu_set()
	{
		auto& g		   = *GImGui;
		auto* p_window = g.CurrentWindow;
		if ((g.OpenPopupStack.Size <= g.BeginPopupStack.Size) or (p_window->Flags & ImGuiWindowFlags_ChildMenu))
			return false;

		// Initially we used 'upper_popup->OpenParentId == window->IDStack.back()' to differentiate multiple menu sets from each others
		// (e.g. inside menu bar vs loose menu items) based on parent ID.
		// This would however prevent the use of e.g. PushID() user code submitting menus.
		// Previously this worked between popup and a first child menu because the first child menu always had the _ChildWindow flag,
		// making hovering on parent popup possible while first child menu was focused - but this was generally a bug with other side effects.
		// Instead we don't treat Popup specifically (in order to consistently support menu features in them), maybe the first child menu of a Popup
		// doesn't have the _ChildWindow flag, and we rely on this _root_of_open_menu_set() check to allow hovering between root window/popup and first child menu.
		// In the end, lack of ID check made it so we could no longer differentiate between separate menu sets. To compensate for that, we at least check parent window nav layer.
		// This fixes the most common case of menu opening on hover when moving between window content and menu bar. Multiple different menu sets in same nav layer would still
		// open on hover, but that should be a lesser problem, because if such menus are close in proximity in window content then it won't feel weird and if they are far apart
		// it likely won't be a problem anyone runs into.
		const auto* upper_popup = &g.OpenPopupStack[g.BeginPopupStack.Size];

		if (p_window->DC.NavLayerCurrent != upper_popup->ParentNavLayer) return false;

		return upper_popup->Window and (upper_popup->Window->Flags & ImGuiWindowFlags_ChildMenu) and ImGui::IsWindowChildOf(upper_popup->Window, p_window, true, false);
	}

	static inline float _calc_delay_from_hovered_flags(ImGuiHoveredFlags flags)
	{
		ImGuiContext& g = *GImGui;
		if (flags & ImGuiHoveredFlags_DelayShort)
			return g.Style.HoverDelayShort;
		if (flags & ImGuiHoveredFlags_DelayNormal)
			return g.Style.HoverDelayNormal;
		return 0.0f;
	}
}	 // namespace

uint32 editor::widgets::get_id(const char* str)
{
	return ImGui::GetID(str);
}

void editor::widgets::draw_line(const ImVec2& p1, const ImVec2& p2, ImU32 col, float thickness)
{
	ImGui::GetCurrentWindowRead()->DrawList->AddLine(p1, p2, col, thickness);
}

void editor::widgets::draw_rect(const ImVec2& p_min, const ImVec2& p_max, ImU32 col, float rounding, ImDrawFlags flags, float thickness)
{
	ImGui::GetCurrentWindowRead()->DrawList->AddRect(p_min, p_max, col, rounding, flags, thickness);
}

void editor::widgets::draw_rect_filled(const ImRect& rect, ImU32 col, float rounding, ImDrawFlags flags)
{
	ImGui::GetCurrentWindowRead()->DrawList->AddRectFilled(rect.Min, rect.Max, col, rounding, flags);
}

void editor::widgets::draw_poly_filled(const ImVec2* points, const int points_count, ImU32 col)
{
	ImGui::GetCurrentWindowRead()->DrawList->AddConvexPolyFilled(points, points_count, col);
}

void editor::widgets::draw_text(const ImVec2& pos, ImU32 col, const char* text_begin, const char* text_end)
{
	ImGui::GetCurrentWindowRead()->DrawList->AddText(nullptr, 0.f, pos, col, text_begin, text_end);
}

void editor::widgets::draw_text_clipped(const ImVec2& pos_min, const ImVec2& pos_max, const char* text, const char* text_end, const ImVec2* text_size_if_known, const ImVec2& align, const ImRect* clip_rect)
{
	// Hide anything after a '##' string
	const char* text_display_end = ImGui::FindRenderedTextEnd(text, text_end);
	const int	text_len		 = (int)(text_display_end - text);
	if (text_len == 0)
		return;

	ImGuiContext& g		 = *GImGui;
	ImGuiWindow*  window = g.CurrentWindow;
	ImGui::RenderTextClippedEx(window->DrawList, pos_min, pos_max, text, text_display_end, text_size_if_known, align, clip_rect);
	// if (g.LogEnabled)
	//{
	//	ImGui::LogRenderedText(&pos_min, text, text_display_end);
	// }
}

void editor::widgets::draw_frame(ImVec2 p_min, ImVec2 p_max, ImU32 fill_col, bool border, float rounding)
{
	ImGuiContext& g		 = *GImGui;
	ImGuiWindow*  window = GImGui->CurrentWindow;
	window->DrawList->AddRectFilled(p_min, p_max, fill_col, rounding);
	const float border_size = GImGui->Style.FrameBorderSize;
	if (border && border_size > 0.0f)
	{
		window->DrawList->AddRect(p_min + ImVec2(1, 1), p_max + ImVec2(1, 1), GetColorU32(ImGuiCol_BorderShadow), rounding, 0, border_size);
		window->DrawList->AddRect(p_min, p_max, GetColorU32(ImGuiCol_Border), rounding, 0, border_size);
	}
}

bool editor::widgets::button_behavior(const ImRect& bb, ImGuiID id, bool* out_hovered, bool* out_held, ImGuiButtonFlags flags)
{
	return ImGui::ButtonBehavior(bb, id, out_hovered, out_held, flags);
}

bool editor::widgets::is_item_clicked(ImGuiMouseButton mouse_button)
{
	return ImGui::IsMouseClicked(mouse_button) && editor::widgets::is_item_hovered(ImGuiHoveredFlags_None);
}

bool editor::widgets::is_item_hovered(ImGuiHoveredFlags flags)
{
	ImGuiContext& g		 = *GImGui;
	ImGuiWindow*  window = g.CurrentWindow;
	IM_ASSERT((flags & ~ImGuiHoveredFlags_AllowedMaskForIsItemHovered) == 0 && "Invalid flags for IsItemHovered()!");

	if (g.NavDisableMouseHover && !g.NavDisableHighlight && !(flags & ImGuiHoveredFlags_NoNavOverride))
	{
		if ((g.LastItemData.InFlags & ImGuiItemFlags_Disabled) && !(flags & ImGuiHoveredFlags_AllowWhenDisabled))
			return false;
		if (!IsItemFocused())
			return false;

		if (flags & ImGuiHoveredFlags_ForTooltip)
			flags |= g.Style.HoverFlagsForTooltipNav;
	}
	else
	{
		// Test for bounding box overlap, as updated as ItemAdd()
		ImGuiItemStatusFlags status_flags = g.LastItemData.StatusFlags;
		if (!(status_flags & ImGuiItemStatusFlags_HoveredRect))
			return false;

		if (flags & ImGuiHoveredFlags_ForTooltip)
			flags |= g.Style.HoverFlagsForTooltipMouse;

		IM_ASSERT((flags & (ImGuiHoveredFlags_AnyWindow | ImGuiHoveredFlags_RootWindow | ImGuiHoveredFlags_ChildWindows | ImGuiHoveredFlags_NoPopupHierarchy | ImGuiHoveredFlags_DockHierarchy)) == 0);	   // Flags not supported by this function

		// Done with rectangle culling so we can perform heavier checks now
		// Test if we are hovering the right window (our window could be behind another window)
		// [2021/03/02] Reworked / reverted the revert, finally. Note we want e.g. BeginGroup/ItemAdd/EndGroup to work as well. (#3851)
		// [2017/10/16] Reverted commit 344d48be3 and testing RootWindow instead. I believe it is correct to NOT test for RootWindow but this leaves us unable
		// to use IsItemHovered() after EndChild() itself. Until a solution is found I believe reverting to the test from 2017/09/27 is safe since this was
		// the test that has been running for a long while.
		if (g.HoveredWindow != window && (status_flags & ImGuiItemStatusFlags_HoveredWindow) == 0)
			if ((flags & ImGuiHoveredFlags_AllowWhenOverlappedByWindow) == 0)
				return false;

		// Test if another item is active (e.g. being dragged)
		const ImGuiID id = g.LastItemData.ID;
		if ((flags & ImGuiHoveredFlags_AllowWhenBlockedByActiveItem) == 0)
			if (g.ActiveId != 0 && g.ActiveId != id && !g.ActiveIdAllowOverlap)
				if (g.ActiveId != window->MoveId && g.ActiveId != window->TabId)
					return false;

		// Test if interactions on this window are blocked by an active popup or modal.
		// The ImGuiHoveredFlags_AllowWhenBlockedByPopup flag will be tested here.
		if (!IsWindowContentHoverable(window, flags) && !(g.LastItemData.InFlags & ImGuiItemFlags_NoWindowHoverableCheck))
			return false;

		// Test if the item is disabled
		if ((g.LastItemData.InFlags & ImGuiItemFlags_Disabled) && !(flags & ImGuiHoveredFlags_AllowWhenDisabled))
			return false;

		// Special handling for calling after Begin() which represent the title bar or tab.
		// When the window is skipped/collapsed (SkipItems==true) that last item (always ->MoveId submitted by Begin)
		// will never be overwritten so we need to detect the case.
		if (id == window->MoveId && window->WriteAccessed)
			return false;

		// Test if using AllowOverlap and overlapped
		if ((g.LastItemData.InFlags & ImGuiItemflags_AllowOverlap) && id != 0)
			if ((flags & ImGuiHoveredFlags_AllowWhenOverlappedByItem) == 0)
				if (g.HoveredIdPreviousFrame != g.LastItemData.ID)
					return false;
	}

	// Handle hover delay
	// (some ideas: https://www.nngroup.com/articles/timing-exposing-content)
	const float delay = _calc_delay_from_hovered_flags(flags);
	if (delay > 0.0f || (flags & ImGuiHoveredFlags_Stationary))
	{
		ImGuiID hover_delay_id = (g.LastItemData.ID != 0) ? g.LastItemData.ID : window->GetIDFromRectangle(g.LastItemData.Rect);
		if ((flags & ImGuiHoveredFlags_NoSharedDelay) && (g.HoverItemDelayIdPreviousFrame != hover_delay_id))
			g.HoverItemDelayTimer = 0.0f;
		g.HoverItemDelayId = hover_delay_id;

		// When changing hovered item we requires a bit of stationary delay before activating hover timer,
		// but once unlocked on a given item we also moving.
		// if (g.HoverDelayTimer >= delay && (g.HoverDelayTimer - g.IO.DeltaTime < delay || g.MouseStationaryTimer - g.IO.DeltaTime < g.Style.HoverStationaryDelay)) { IMGUI_DEBUG_LOG("HoverDelayTimer = %f/%f, MouseStationaryTimer = %f\n", g.HoverDelayTimer, delay, g.MouseStationaryTimer); }
		if ((flags & ImGuiHoveredFlags_Stationary) != 0 && g.HoverItemUnlockedStationaryId != hover_delay_id)
			return false;

		if (g.HoverItemDelayTimer < delay)
			return false;
	}

	return true;
}

bool editor::widgets::is_item_active()
{
	return ImGui::IsItemActive();
}

bool editor::widgets::is_item_edited()
{
	return ImGui::IsItemEdited();
}

bool editor::widgets::is_item_activated()
{
	return ImGui::IsItemActivated();
}

bool editor::widgets::is_item_deactivated()
{
	return ImGui::IsItemDeactivated();
}

bool editor::widgets::is_item_deactivated_after_edit()
{
	return ImGui::IsItemDeactivatedAfterEdit();
}

bool editor::widgets::begin(const char* name, bool* p_open, ImGuiWindowFlags flags)
{
	return ImGui::Begin(name, p_open, flags);
}

void editor::widgets::end()
{
	ImGui::End();
}

bool editor::widgets::begin_child(const char* str_id, const ImVec2& size, bool border, ImGuiWindowFlags flags)
{
	return ImGui::BeginChild(str_id, size, border, flags);
}

void editor::widgets::end_child()
{
	ImGui::EndChild();
}

bool editor::widgets::begin_popup(const char* str_id, ImGuiWindowFlags flags)
{
	return ImGui::BeginPopup(str_id, flags);
}

bool editor::widgets::begin_popup_modal(const char* name, bool* p_open, ImGuiWindowFlags flags)
{
	return editor::widgets::begin_popup_modal(GImGui->CurrentWindow->GetID(name), name, p_open, flags);
}

bool editor::widgets::begin_popup_modal(ImGuiID id, const char* name, bool* p_open, ImGuiWindowFlags flags)
{
	auto& g		 = *GImGui;
	auto  window = g.CurrentWindow;
	if (!IsPopupOpen(id, ImGuiPopupFlags_None))
	{
		g.NextWindowData.ClearFlags();	  // We behave like Begin() and need to consume those values
		return false;
	}

	// Center modal windows by default for increased visibility
	// (this won't really last as settings will kick in, and is mostly for backward compatibility. user may do the same themselves)
	// FIXME: Should test for (PosCond & window->SetWindowPosAllowFlags) with the upcoming window.
	if ((g.NextWindowData.Flags & ImGuiNextWindowDataFlags_HasPos) == 0)
	{
		const ImGuiViewport* viewport = window->WasActive ? window->Viewport : GetMainViewport();	 // FIXME-VIEWPORT: What may be our reference viewport?
		SetNextWindowPos(viewport->GetCenter(), ImGuiCond_FirstUseEver, ImVec2(0.5f, 0.5f));
	}

	flags			   |= ImGuiWindowFlags_Popup | ImGuiWindowFlags_Modal | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking;
	const bool is_open	= Begin(name, p_open, flags);
	if (!is_open || (p_open and !*p_open))	  // NB: is_open can be 'false' when the popup is completely clipped (e.g. zero size display)
	{
		end_popup();
		if (is_open)
			ClosePopupToLevel(g.BeginPopupStack.Size, true);
		return false;
	}
	return is_open;
}

void editor::widgets::end_popup()
{
	ImGui::EndPopup();
}

void editor::widgets::open_popup(const char* str_id, ImGuiPopupFlags popup_flags)
{
	ImGui::OpenPopup(str_id, popup_flags);
}

void editor::widgets::close_popup()
{
	ImGui::CloseCurrentPopup();
}

bool editor::widgets::selectable(const char* label, bool* p_selected, ImGuiSelectableFlags flags, const ImVec2& size_arg, bool border)
{
	if (selectable(label, *p_selected, flags, size_arg, border))
	{
		*p_selected = !*p_selected;
		return true;
	}
	return false;
}

bool editor::widgets::selectable(std::string label, bool* p_selected, ImGuiSelectableFlags flags, const ImVec2& size_arg, bool border)
{
	if (selectable(label.c_str(), *p_selected, flags, size_arg, border))
	{
		*p_selected = !*p_selected;
		return true;
	}
	return false;
}

bool editor::widgets::button(const char* label, const ImVec2& size)
{
	return ImGui::Button(label, size);
}

bool editor::widgets::tree_node(std::string label, ImGuiTreeNodeFlags flags)
{
	ImGuiWindow* window = GetCurrentWindow();
	if (window->SkipItems)
	{
		return false;
	}

	return TreeNodeBehavior(window->GetID(label.c_str()), flags, label.c_str(), NULL);
}

void editor::widgets::tree_pop()
{
	ImGuiContext& g		 = *GImGui;
	ImGuiWindow*  window = g.CurrentWindow;
	Unindent();

	window->DC.TreeDepth--;
	ImU32 tree_depth_mask = (1 << window->DC.TreeDepth);

	// Handle Left arrow to move to parent tree node (when ImGuiTreeNodeFlags_NavLeftJumpsBackHere is enabled)
	if (g.NavMoveDir == ImGuiDir_Left && g.NavWindow == window && NavMoveRequestButNoResultYet())
		if (g.NavIdIsAlive && (window->DC.TreeJumpToParentOnPopMask & tree_depth_mask))
		{
			SetNavID(window->IDStack.back(), g.NavLayer, 0, ImRect());
			NavMoveRequestCancel();
		}
	window->DC.TreeJumpToParentOnPopMask &= tree_depth_mask - 1;

	IM_ASSERT(window->IDStack.Size > 1);	// There should always be 1 element in the IDStack (pushed during window creation). If this triggers you called TreePop/PopID too much.
	PopID();
}

bool editor::widgets::selectable(std::string label, bool selected, ImGuiSelectableFlags flags, const ImVec2& size_arg, bool border)
{
	return selectable(label.c_str(), selected, flags, size_arg, border);
}

bool editor::widgets::selectable(const char* label, bool selected, ImGuiSelectableFlags flags, const ImVec2& size_arg, bool border)
{
	auto* p_window = GetCurrentWindow();
	if (p_window->SkipItems) return false;

	auto&		g	  = *GImGui;
	const auto& style = g.Style;

	// Submit label or explicit size to ItemSize(), whereas ItemAdd() will submit a larger/spanning rectangle.
	auto id			= p_window->GetID(label);
	auto label_size = CalcTextSize(label, NULL, true);
	// auto size		  = ImVec2(size_arg.x != 0.0f ? size_arg.x : label_size.x, size_arg.y != 0.0f ? size_arg.y : label_size.y);
	auto size  = ImVec2(size_arg.x != 0.0f ? size_arg.x : label_size.x + g.Style.FramePadding.x * 2, size_arg.y != 0.0f ? size_arg.y : label_size.y + g.Style.FramePadding.y * 2);
	auto pos   = p_window->DC.CursorPos;
	pos.y	  += p_window->DC.CurrLineTextBaseOffset;
	ItemSize(size, 0.0f);

	// Fill horizontal space
	// We don't support (size < 0.0f) in Selectable() because the ItemSpacing extension would make explicitly right-aligned sizes not visibly match other widgets.
	const bool	span_all_columns = (flags & ImGuiSelectableFlags_SpanAllColumns) != 0;
	const float min_x			 = span_all_columns ? p_window->ParentWorkRect.Min.x : pos.x;
	const float max_x			 = span_all_columns ? p_window->ParentWorkRect.Max.x : p_window->WorkRect.Max.x;
	if (size_arg.x == 0.0f || (flags & ImGuiSelectableFlags_SpanAvailWidth))
	{
		size.x = ImMax(label_size.x, max_x - min_x);
	}

	// Text stays at the submission position, but bounding box may be extended on both sides
	const auto text_min = pos + g.Style.FramePadding;
	const auto text_max = ImVec2(min_x, pos.y) + size;

	// Selectables are meant to be tightly packed together with no click-gap, so we extend their box to cover spacing between selectable.
	auto bb = ImRect(min_x, pos.y, text_max.x, text_max.y);
	if ((flags & ImGuiSelectableFlags_NoPadWithHalfSpacing) == 0)
	{
		const float spacing_x  = span_all_columns ? 0.0f : style.ItemSpacing.x;
		const float spacing_y  = style.ItemSpacing.y;
		const float spacing_L  = IM_FLOOR(spacing_x * 0.50f);
		const float spacing_U  = IM_FLOOR(spacing_y * 0.50f);
		bb.Min.x			  -= spacing_L;
		bb.Min.y			  -= spacing_U;
		bb.Max.x			  += (spacing_x - spacing_L);
		bb.Max.y			  += (spacing_y - spacing_U);
	}
	// if (g.IO.KeyCtrl) { GetForegroundDrawList()->AddRect(bb.Min, bb.Max, IM_COL32(0, 255, 0, 255)); }

	// Modify ClipRect for the ItemAdd(), faster than doing a PushColumnsBackground/PushTableBackground for every Selectable..
	const float backup_clip_rect_min_x = p_window->ClipRect.Min.x;
	const float backup_clip_rect_max_x = p_window->ClipRect.Max.x;
	if (span_all_columns)
	{
		p_window->ClipRect.Min.x = p_window->ParentWorkRect.Min.x;
		p_window->ClipRect.Max.x = p_window->ParentWorkRect.Max.x;
	}

	const bool disabled_item = (flags & ImGuiSelectableFlags_Disabled) != 0;
	const bool item_add		 = ItemAdd(bb, id, NULL, disabled_item ? ImGuiItemFlags_Disabled : ImGuiItemFlags_None);
	if (span_all_columns)
	{
		p_window->ClipRect.Min.x = backup_clip_rect_min_x;
		p_window->ClipRect.Max.x = backup_clip_rect_max_x;
	}

	if (item_add is_false) return false;

	const auto disabled_global = (g.CurrentItemFlags & ImGuiItemFlags_Disabled) != 0;
	if (disabled_item and disabled_global is_false)	   // Only testing this as an optimization
	{
		BeginDisabled();
	}

	// FIXME: We can standardize the behavior of those two, we could also keep the fast path of override ClipRect + full push on render only,
	// which would be advantageous since most selectable are not selected.
	if (span_all_columns and p_window->DC.CurrentColumns is_not_nullptr)
	{
		PushColumnsBackground();
	}
	else if (span_all_columns and g.CurrentTable is_not_nullptr)
	{
		TablePushBackgroundChannel();
	}

	// We use NoHoldingActiveID on menus so user can click and _hold_ on a menu then drag to browse child entries
	ImGuiButtonFlags button_flags = 0;
	if (flags & ImGuiSelectableFlags_NoHoldingActiveID)
	{
		button_flags |= ImGuiButtonFlags_NoHoldingActiveId;
	}
	if (flags & ImGuiSelectableFlags_NoSetKeyOwner)
	{
		button_flags |= ImGuiButtonFlags_NoSetKeyOwner;
	}
	if (flags & ImGuiSelectableFlags_SelectOnClick)
	{
		button_flags |= ImGuiButtonFlags_PressedOnClick;
	}
	if (flags & ImGuiSelectableFlags_SelectOnRelease)
	{
		button_flags |= ImGuiButtonFlags_PressedOnRelease;
	}
	if (flags & ImGuiSelectableFlags_AllowDoubleClick)
	{
		button_flags |= ImGuiButtonFlags_PressedOnClickRelease | ImGuiButtonFlags_PressedOnDoubleClick;
	}
	if ((flags & ImGuiSelectableFlags_AllowOverlap) || (g.LastItemData.InFlags & ImGuiItemflags_AllowOverlap))
	{
		button_flags |= ImGuiButtonFlags_AllowOverlap;
	}

	const auto was_selected = selected;
	auto	   hovered = false, held = false, pressed = ButtonBehavior(bb, id, &hovered, &held, button_flags);

	// Auto-select when moved into
	// - This will be more fully fleshed in the range-select branch
	// - This is not exposed as it won't nicely work with some user side handling of shift/control
	// - We cannot do 'if (g.NavJustMovedToId != id) { selected = false; pressed = was_selected; }' for two reasons
	//   - (1) it would require focus scope to be set, need exposing PushFocusScope() or equivalent (e.g. BeginSelection() calling PushFocusScope())
	//   - (2) usage will fail with clipped items
	//   The multi-select API aim to fix those issues, e.g. may be replaced with a BeginSelection() API.
	if ((flags & ImGuiSelectableFlags_SelectOnNav) and g.NavJustMovedToId != 0 and g.NavJustMovedToFocusScopeId == g.CurrentFocusScopeId)
	{
		if (g.NavJustMovedToId == id)
		{
			selected = pressed = true;
		}
	}

	// Update NavId when clicking or when Hovering (this doesn't happen on most widgets), so navigation can be resumed with gamepad/keyboard
	if (pressed || (hovered and (flags & ImGuiSelectableFlags_SetNavIdOnHover)))
	{
		if (!g.NavDisableMouseHover and g.NavWindow == p_window and g.NavLayer == p_window->DC.NavLayerCurrent)
		{
			SetNavID(id, p_window->DC.NavLayerCurrent, g.CurrentFocusScopeId, WindowRectAbsToRel(p_window, bb));	// (bb == NavRect)
			g.NavDisableHighlight = true;
		}
	}
	if (pressed)
	{
		MarkItemEdited(id);
	}

	// In this branch, Selectable() cannot toggle the selection so this will never trigger.
	if (selected != was_selected)	 //-V547
	{
		g.LastItemData.StatusFlags |= ImGuiItemStatusFlags_ToggledSelection;
	}

	// Render
	if (hovered or selected)
	{
		const ImU32 col = GetColorU32((held and hovered) ? ImGuiCol_HeaderActive : hovered ? ImGuiCol_HeaderHovered
																						   : ImGuiCol_Header);
		// RenderFrame(bb.Min, bb.Max, col, false, 0.0f);
		RenderFrame(bb.Min, bb.Max, col, border, 0.0f);
	}

	RenderNavHighlight(bb, id, ImGuiNavHighlightFlags_TypeThin | ImGuiNavHighlightFlags_NoRounding);

	if (span_all_columns and p_window->DC.CurrentColumns is_not_nullptr)
	{
		PopColumnsBackground();
	}
	else if (span_all_columns and g.CurrentTable is_not_nullptr)
	{
		TablePopBackgroundChannel();
	}

	RenderTextClipped(text_min, text_max, label, NULL, &label_size, style.SelectableTextAlign, &bb);

	// Automatically close popups
	if (pressed and (p_window->Flags & ImGuiWindowFlags_Popup) and not(flags & ImGuiSelectableFlags_DontClosePopups) and not(g.LastItemData.InFlags & ImGuiItemFlags_SelectableDontClosePopup))
	{
		CloseCurrentPopup();
	}

	if (disabled_item and disabled_global is_false)
	{
		EndDisabled();
	}

	// IMGUI_TEST_ENGINE_ITEM_INFO(id, label, g.LastItemData.StatusFlags);
	return pressed;	   //-V1020
}

bool editor::widgets::menu_item(std::string label, const char* icon, std::string shortcut, bool selected, bool enabled)
{
	return menu_item(label.c_str(), icon, shortcut.c_str(), selected, enabled);
}

bool editor::widgets::menu_item(const char* label, const char* icon, const char* shortcut, bool selected, bool enabled)
{
	auto* p_window = GetCurrentWindow();
	if (p_window->SkipItems) return false;

	auto&		g				 = *GImGui;
	auto&		style			 = g.Style;
	auto		pos				 = p_window->DC.CursorPos;
	auto		label_size		 = CalcTextSize(label, NULL, true);
	const auto	selectable_flags = ImGuiSelectableFlags_SelectOnRelease | ImGuiSelectableFlags_NoSetKeyOwner | ImGuiSelectableFlags_SetNavIdOnHover;	// We use ImGuiSelectableFlags_NoSetKeyOwner to allow down on one menu item, move, up on another.
	const auto* offsets			 = &p_window->DC.MenuColumns;
	auto		pressed			 = false;

	// See BeginMenuEx() for comments about this.
	const bool menuset_is_open = _root_of_open_menu_set();
	if (menuset_is_open)
	{
		PushItemFlag(ImGuiItemFlags_NoWindowHoverableCheck, true);
	}
	// We've been using the equivalent of ImGuiSelectableFlags_SetNavIdOnHover on all Selectable() since early Nav system days (commit 43ee5d73),
	// but I am unsure whether this should be kept at all. For now moved it to be an opt-in feature used by menus only.
	PushID(label);

	if (enabled is_false)
	{
		BeginDisabled();
	}

	if (p_window->DC.LayoutType == ImGuiLayoutType_Horizontal)	  // no shortcut
	{
		// Mimic the exact layout spacing of BeginMenu() to allow MenuItem() inside a menu bar, which is a little misleading but may be useful
		// Note that in this situation: we don't render the shortcut, we render a highlight instead of the selected tick mark.
		p_window->DC.CursorPos.x += IM_FLOOR(style.ItemSpacing.x * 0.5f);
		auto w					  = label_size.x;
		auto text_pos			  = ImVec2(p_window->DC.CursorPos.x + offsets->OffsetLabel, p_window->DC.CursorPos.y + p_window->DC.CurrLineTextBaseOffset);
		PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(style.ItemSpacing.x * 2.0f, style.ItemSpacing.y));
		pressed = selectable("", selected, selectable_flags, ImVec2(w, 0.0f));
		PopStyleVar();

		if (g.LastItemData.StatusFlags & ImGuiItemStatusFlags_Visible)
		{
			RenderText(text_pos, label);
		}

		p_window->DC.CursorPos.x += IM_FLOOR(style.ItemSpacing.x * (-1.0f + 0.5f));	   // -1 spacing to compensate the spacing added when Selectable() did a SameLine(). It would also work to call SameLine() ourselves after the PopStyleVar().
	}
	else
	{
		// Menu item inside a vertical menu
		// (In a typical menu window where all items are BeginMenu() or MenuItem() calls, extra_w will always be 0.0f.
		//  Only when they are other items sticking out we're going to add spacing, yet only register minimum width into the layout system.
		auto icon_w		 = (icon and icon[0]) ? CalcTextSize(icon, NULL).x : 0.0f;
		auto shortcut_w	 = (shortcut and shortcut[0]) ? CalcTextSize(shortcut, NULL).x : 0.0f;
		auto checkmark_w = IM_FLOOR(g.FontSize * 1.20f);
		auto stretch_w	 = STR_MENU_SHORTCUT_SPACE_COUNT * GImGui->Font->IndexAdvanceX[' '];								   // max(0.0f, GetContentRegionAvail().x - min_w);
		auto min_w		 = p_window->DC.MenuColumns.DeclColumns(icon_w, label_size.x, shortcut_w, checkmark_w) + stretch_w;	   // Feedback for next frame
		auto text_pos	 = pos + g.Style.FramePadding;
		pressed			 = selectable("", false, selectable_flags | ImGuiSelectableFlags_NoPadWithHalfSpacing | ImGuiSelectableFlags_SpanAvailWidth, ImVec2(min_w, label_size.y) + g.Style.FramePadding * 2, true);
		if (g.LastItemData.StatusFlags & ImGuiItemStatusFlags_Visible)
		{
			RenderText(text_pos + ImVec2(offsets->OffsetLabel, 0.0f), label);

			if (icon_w > 0.0f)
			{
				RenderText(text_pos + ImVec2(offsets->OffsetIcon, 0.0f), icon);
			}

			if (shortcut_w > 0.0f)
			{
				RenderText(text_pos + ImVec2(offsets->OffsetShortcut + stretch_w, 0.0f), shortcut, NULL, false);
			}

			if (selected)
			{
				RenderCheckMark(p_window->DrawList, text_pos + ImVec2(offsets->OffsetMark + stretch_w + g.FontSize * 0.40f, g.FontSize * 0.134f * 0.5f), GetColorU32(ImGuiCol_Text), g.FontSize * 0.866f);
			}
		}
	}

	if (enabled is_false)
	{
		EndDisabled();
	}

	PopID();

	if (menuset_is_open)
	{
		PopItemFlag();
	}

	return pressed;
}

bool editor::widgets::begin_menu(std::string label, const char* icon, bool enabled)
{
	return begin_menu(label.c_str(), icon, enabled);
}

bool editor::widgets::begin_menu(const char* label, const char* icon, bool enabled)
{
	auto* p_window = GetCurrentWindow();

	if (p_window->SkipItems) return false;

	auto&		g			 = *GImGui;
	const auto& style		 = g.Style;
	const auto	id			 = p_window->GetID(label);
	auto		menu_is_open = IsPopupOpen(id, ImGuiPopupFlags_None);

	// Sub-menus are ChildWindow so that mouse can be hovering across them (otherwise top-most popup menu would steal focus and not allow hovering on parent menu)
	// The first menu in a hierarchy isn't so hovering doesn't get across (otherwise e.g. resizing borders with ImGuiButtonFlags_FlattenChildren would react), but top-most BeginMenu() will bypass that limitation.
	auto window_flags = ImGuiWindowFlags_ChildMenu | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoNavFocus;
	if (p_window->Flags & ImGuiWindowFlags_ChildMenu) window_flags |= ImGuiWindowFlags_ChildWindow;

	// If a menu with same the ID was already submitted, we will append to it, matching the behavior of Begin().
	// We are relying on a O(N) search - so O(N log N) over the frame - which seems like the most efficient for the expected small amount of BeginMenu() calls per frame.
	// If somehow this is ever becoming a problem we can switch to use e.g. ImGuiStorage mapping key to last frame used.
	if (g.MenusIdSubmittedThisFrame.contains(id))
	{
		if (menu_is_open)
		{
			menu_is_open = BeginPopupEx(id, window_flags);	  // menu_is_open can be 'false' when the popup is completely clipped (e.g. zero size display)
		}
		else
		{
			g.NextWindowData.ClearFlags();					  // we behave like Begin() and need to consume those values
		}

		return menu_is_open;
	}

	// Tag menu as used. Next time BeginMenu() with same ID is called it will append to existing menu
	g.MenusIdSubmittedThisFrame.push_back(id);

	auto label_size = CalcTextSize(label, NULL, true);

	// Odd hack to allow hovering across menus of a same menu-set (otherwise we wouldn't be able to hover parent without always being a Child window)
	// This is only done for items for the menu set and not the full parent window.
	const bool menuset_is_open = _root_of_open_menu_set();
	if (menuset_is_open) PushItemFlag(ImGuiItemFlags_NoWindowHoverableCheck, true);

	// The reference position stored in popup_pos will be used by Begin() to find a suitable position for the child menu,
	// However the final position is going to be different! It is chosen by FindBestWindowPosForPopup().
	// e.g. Menus tend to overlap each other horizontally to amplify relative Z-ordering.
	ImVec2 popup_pos, pos = p_window->DC.CursorPos;
	PushID(label);

	if (enabled is_false) BeginDisabled();

	const auto* offsets = &p_window->DC.MenuColumns;
	auto		pressed = false;

	// We use ImGuiSelectableFlags_NoSetKeyOwner to allow down on one menu item, move, up on another.
	const auto selectable_flags = ImGuiSelectableFlags_NoHoldingActiveID | ImGuiSelectableFlags_NoSetKeyOwner | ImGuiSelectableFlags_SelectOnClick | ImGuiSelectableFlags_DontClosePopups;
	if (p_window->DC.LayoutType == ImGuiLayoutType_Horizontal)
	{
		// Menu inside an horizontal menu bar
		// Selectable extend their highlight by half ItemSpacing in each direction.
		// For ChildMenu, the popup position will be overwritten by the call to FindBestWindowPosForPopup() in Begin()
		// popup_pos				  = ImVec2(pos.x - 1.0f - IM_FLOOR(style.ItemSpacing.x * 0.5f), pos.y - style.FramePadding.y + p_window->MenuBarHeight());
		if (menuset_is_open)
		{
			PushStyleColor(ImGuiCol_HeaderHovered, g.Style.Colors[ImGuiCol_PopupBg]);
			PushStyleColor(ImGuiCol_Header, g.Style.Colors[ImGuiCol_PopupBg]);
		}
		popup_pos				  = ImVec2(pos.x, pos.y + label_size.y + GImGui->Style.FramePadding.y * 2);
		p_window->DC.CursorPos.x += IM_FLOOR(style.ItemSpacing.x * 0.5f);
		PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(style.ItemSpacing.x * 2.0f, style.ItemSpacing.y));
		auto w		  = label_size.x;
		auto text_pos = ImVec2(p_window->DC.CursorPos.x + offsets->OffsetLabel, p_window->DC.CursorPos.y + p_window->DC.CurrLineTextBaseOffset) + GImGui->Style.FramePadding;

		pressed = selectable("", menu_is_open, selectable_flags | ImGuiSelectableFlags_NoPadWithHalfSpacing, label_size + GImGui->Style.FramePadding * 2);

		// pressed		  = Selectable("", menu_is_open, selectable_flags, ImVec2(w, label_size.y));
		RenderText(text_pos, label);
		PopStyleVar();
		if (menuset_is_open)
		{
			PopStyleColor(2);
		}
		p_window->DC.CursorPos.x += IM_FLOOR(style.ItemSpacing.x * (-1.0f + 0.5f));	   // -1 spacing to compensate the spacing added when Selectable() did a SameLine(). It would also work to call SameLine() ourselves after the PopStyleVar().

		if (IsItemHovered() and menuset_is_open is_false)
		{
			p_window->DrawList->AddRect(g.LastItemData.Rect.Min, g.LastItemData.Rect.Max, ImColor(COL_BD_SELECTED));
		}
	}
	else
	{
		// Menu inside a regular/vertical menu
		// (In a typical menu window where all items are BeginMenu() or MenuItem() calls, extra_w will always be 0.0f.
		//  Only when they are other items sticking out we're going to add spacing, yet only register minimum width into the layout system.

		popup_pos		 = ImVec2(pos.x, pos.y);																		// only y is used
		auto icon_w		 = (icon and icon[0]) ? CalcTextSize(icon, NULL).x : 0.0f;
		auto checkmark_w = IM_FLOOR(g.FontSize * 1.20f);
		auto stretch_w	 = STR_MENU_SHORTCUT_SPACE_COUNT * GImGui->Font->IndexAdvanceX[' '];
		auto min_w		 = p_window->DC.MenuColumns.DeclColumns(icon_w, label_size.x, 0.f, checkmark_w) + stretch_w;	// Feedback to next frame
		// auto stretch_w	 = p_window->DC.MenuColumns.Widths[2] == 0.f ? 0 : STR_MENU_SHORTCUT_SPACE_COUNT * GImGui->Font->IndexAdvanceX[' '];
		auto extra_w  = ImMax(0.0f, GetContentRegionAvail().x - min_w);
		auto text_pos = ImVec2(p_window->DC.CursorPos.x, p_window->DC.CursorPos.y + p_window->DC.CurrLineTextBaseOffset) + GImGui->Style.FramePadding;
		pressed		  = selectable("", menu_is_open, selectable_flags | ImGuiSelectableFlags_NoPadWithHalfSpacing | ImGuiSelectableFlags_SpanAvailWidth, ImVec2(min_w, label_size.y) + GImGui->Style.FramePadding * 2, true);
		RenderText(text_pos + ImVec2(offsets->OffsetLabel, 0.f), label);

		if (icon_w > 0.0f)
		{
			RenderText(text_pos + ImVec2(offsets->OffsetIcon, 0.f), icon);
		}

		RenderArrow(p_window->DrawList, text_pos + ImVec2(offsets->OffsetMark + stretch_w + g.FontSize * 0.30f /* + , 0.0f*/, 0.f), GetColorU32(ImGuiCol_Text), ImGuiDir_Right);
	}

	if (enabled is_false) EndDisabled();

	const auto hovered	 = (g.HoveredId == id) and enabled and !g.NavDisableMouseHover;
	auto	   want_open = false, want_close = false;

	if (menuset_is_open) PopItemFlag();

	if (p_window->DC.LayoutType == ImGuiLayoutType_Vertical)	// (window->Flags & (ImGuiWindowFlags_Popup|ImGuiWindowFlags_ChildMenu))
	{
		// Close menu when not hovering it anymore unless we are moving roughly in the direction of the menu
		// Implement http://bjk5.com/post/44698559168/breaking-down-amazons-mega-dropdown to avoid using timers, so menus feels more reactive.
		auto  moving_toward_child_menu = false;
		auto* child_popup			   = (g.BeginPopupStack.Size < g.OpenPopupStack.Size) ? &g.OpenPopupStack[g.BeginPopupStack.Size] : NULL;	 // Popup candidate (testing below)
		auto* child_menu_window		   = (child_popup and child_popup->Window and child_popup->Window->ParentWindow == p_window) ? child_popup->Window : NULL;
		if (g.HoveredWindow == p_window and child_menu_window != NULL)
		{
			float  ref_unit			  = g.FontSize;																								 // FIXME-DPI
			float  child_dir		  = (p_window->Pos.x < child_menu_window->Pos.x) ? 1.0f : -1.0f;
			ImRect next_window_rect	  = child_menu_window->Rect();
			ImVec2 ta				  = (g.IO.MousePos - g.IO.MouseDelta);
			ImVec2 tb				  = (child_dir > 0.0f) ? next_window_rect.GetTL() : next_window_rect.GetTR();
			ImVec2 tc				  = (child_dir > 0.0f) ? next_window_rect.GetBL() : next_window_rect.GetBR();
			float  extra			  = ImClamp(ImFabs(ta.x - tb.x) * 0.30f, ref_unit * 0.5f, ref_unit * 2.5f);	   // add a bit of extra slack.
			ta.x					 += child_dir * -0.5f;
			tb.x					 += child_dir * ref_unit;
			tc.x					 += child_dir * ref_unit;
			tb.y					  = ta.y + ImMax((tb.y - extra) - ta.y, -ref_unit * 8.0f);					   // triangle has maximum height to limit the slope and the bias toward large sub-menus
			tc.y					  = ta.y + ImMin((tc.y + extra) - ta.y, +ref_unit * 8.0f);
			moving_toward_child_menu  = ImTriangleContainsPoint(ta, tb, tc, g.IO.MousePos);
			// GetForegroundDrawList()->AddTriangleFilled(ta, tb, tc, moving_toward_child_menu ? IM_COL32(0, 128, 0, 128) : IM_COL32(128, 0, 0, 128));	   // [DEBUG]
		}

		// The 'HovereWindow == window' check creates an inconsistency (e.g. moving away from menu slowly tends to hit same window, whereas moving away fast does not)
		// But we also need to not close the top-menu menu when moving over void. Perhaps we should extend the triangle check to a larger polygon.
		// (Remember to test this on BeginPopup("A")->BeginMenu("B") sequence which behaves slightly differently as B isn't a Child of A and hovering isn't shared.)
		if (menu_is_open and not hovered and g.HoveredWindow == p_window and not moving_toward_child_menu and g.NavDisableMouseHover is_false)
		{
			want_close = true;
		}

		// Open
		if (menu_is_open is_false and pressed)								 // Click/activate to open
		{
			want_open = true;
		}
		else if (!menu_is_open and hovered and !moving_toward_child_menu)	 // Hover to open
		{
			want_open = true;
		}

		if (g.NavId == id and g.NavMoveDir == ImGuiDir_Right)				 // Nav-Right to open
		{
			want_open = true;
			NavMoveRequestCancel();
		}
	}
	else
	{
		// Menu bar
		if (menu_is_open and pressed and menuset_is_open)								// Click an open menu again to close it
		{
			want_close = true;
			want_open = menu_is_open = false;
		}
		else if (pressed || (hovered and menuset_is_open and menu_is_open is_false))	// First click to open, then hover to open others
		{
			want_open = true;
		}
		else if (g.NavId == id and g.NavMoveDir == ImGuiDir_Down)						// Nav-Down to open
		{
			want_open = true;
			NavMoveRequestCancel();
		}
	}

	if (enabled is_false)
	{
		want_close = true;	  // explicitly close if an open menu becomes disabled, facilitate users code a lot in pattern such as 'if (BeginMenu("options", has_object)) { ..use object.. }'
	}

	if (want_close and IsPopupOpen(id, ImGuiPopupFlags_None))
	{
		ClosePopupToLevel(g.BeginPopupStack.Size, true);
	}

	PopID();

	if (want_open and menu_is_open is_false and g.OpenPopupStack.Size > g.BeginPopupStack.Size)
	{
		// Don't reopen/recycle same menu level in the same frame, first close the other menu and yield for a frame.
		OpenPopup(label);
	}
	else if (want_open)
	{
		menu_is_open = true;
		OpenPopup(label);
	}

	if (menu_is_open)
	{
		if (p_window->DC.MenuBarAppending)
		{
			PushStyleVar(ImGuiStyleVar_PopupBorderSize, 0);
		}
		if (p_window->DC.LayoutType == ImGuiLayoutType_Vertical)
		{
			PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2());
		}

		auto last_item_in_parent = g.LastItemData;
		SetNextWindowPos(popup_pos, ImGuiCond_Always);					   // Note: misleading: the value will serve as reference for FindBestWindowPosForPopup(), not actual pos.

		PushStyleVar(ImGuiStyleVar_ChildRounding, style.PopupRounding);	   // First level will use _PopupRounding, subsequent will use _ChildRounding
		menu_is_open = BeginPopupEx(id, window_flags);					   // menu_is_open can be 'false' when the popup is completely clipped (e.g. zero size display)
		PopStyleVar();

		if (p_window->DC.MenuBarAppending)
		{
			PopStyleVar();
		}
		if (p_window->DC.LayoutType == ImGuiLayoutType_Vertical)
		{
			PopStyleVar();
		}

		if (menu_is_open)
		{
			// Restore LastItemData so IsItemXXXX functions can work after BeginMenu()/EndMenu()
			// (This fixes using IsItemClicked() and IsItemHovered(), but IsItemHovered() also relies on its support for ImGuiItemFlags_NoWindowHoverableCheck)
			g.LastItemData = last_item_in_parent;
			if (g.HoveredWindow == p_window)
			{
				g.LastItemData.StatusFlags |= ImGuiItemStatusFlags_HoveredWindow;
			}
		}

		const auto popup_dir = ImGui::GetCurrentWindowRead()->AutoPosLastDirection;
		if (menu_is_open and p_window->DC.LayoutType == ImGuiLayoutType_Horizontal and p_window->DC.MenuBarAppending and popup_dir == ImGuiDir_Down or popup_dir == ImGuiDir_Up)
		{
			const auto& upper_rect = popup_dir == ImGuiDir_Down ? g.LastItemData.Rect : ImGui::GetCurrentWindowRead()->Rect();
			const auto& lower_rect = popup_dir == ImGuiDir_Down ? ImGui::GetCurrentWindowRead()->Rect() : g.LastItemData.Rect;
			ImVec2		border_points[8] {
				 upper_rect.GetBL(),
				 upper_rect.GetTL(),
				 upper_rect.GetTR(),
				 upper_rect.GetBR(),
				 lower_rect.GetTR(),
				 lower_rect.GetBR(),
				 lower_rect.GetBL(),
				 lower_rect.GetTL(),
			};

			ImGui::GetForegroundDrawList()->AddPolyline(border_points, 8, ImColor(COL_BD_SELECTED), ImDrawFlags_Closed, POPUP_BORDER * GEctx->dpi_scale);
		}
	}
	else
	{
		g.NextWindowData.ClearFlags();	  // We behave like Begin() and need to consume those values
	}

	return menu_is_open;
}

void editor::widgets::end_menu()
{
	// Nav: When a left move request our menu failed, close ourselves.
	ImGuiContext& g		   = *GImGui;
	ImGuiWindow*  p_window = g.CurrentWindow;
	assert(p_window->Flags & ImGuiWindowFlags_Popup);		// Mismatched BeginMenu()/EndMenu() calls
	ImGuiWindow* parent_window = p_window->ParentWindow;	// Should always be != NULL is we passed assert.
	if (p_window->BeginCount == p_window->BeginCountPreviousFrame)
	{
		if (g.NavMoveDir == ImGuiDir_Left and NavMoveRequestButNoResultYet())
		{
			if (g.NavWindow and (g.NavWindow->RootWindowForNav == p_window) and parent_window->DC.LayoutType == ImGuiLayoutType_Vertical)
			{
				ClosePopupToLevel(g.BeginPopupStack.Size - 1, true);
				NavMoveRequestCancel();
			}
		}
	}

	end_popup();
}

bool editor::widgets::add_item(const ImRect& bb, ImGuiID id, const ImRect* nav_bb, ImGuiItemFlags extra_flags)
{
	return ImGui::ItemAdd(bb, id, nav_bb, extra_flags);
}

void editor::widgets::item_size(const ImVec2& size, float text_baseline_y)
{
	ImGui::ItemSize(size, text_baseline_y);
}

const ImRect& editor::widgets::get_item_rect()
{
	return GImGui->LastItemData.Rect;
}

const ImVec2& editor::widgets::get_cursor_pos()
{
	ImGuiWindow* window = GetCurrentWindowRead();
	return window->DC.CursorPos - window->Pos + window->Scroll;
}

float editor::widgets::get_cursor_pos_x()
{
	ImGuiWindow* window = GetCurrentWindowRead();
	return window->DC.CursorPos.x - window->Pos.x + window->Scroll.x;
}

float editor::widgets::get_cursor_pos_y()
{
	ImGuiWindow* window = GetCurrentWindowRead();
	return window->DC.CursorPos.y - window->Pos.y + window->Scroll.y;
}

void editor::widgets::set_cursor_pos(const ImVec2& local_pos)
{
	ImGuiWindow* window	 = GetCurrentWindow();
	window->DC.CursorPos = window->Pos - window->Scroll + local_pos;
	// window->DC.CursorMaxPos = ImMax(window->DC.CursorMaxPos, window->DC.CursorPos);
	window->DC.IsSetPos = true;
}

void editor::widgets::set_cursor_pos_x(float local_x)
{
	ImGuiWindow* window	   = GetCurrentWindow();
	window->DC.CursorPos.x = window->Pos.x - window->Scroll.x + local_x;
	// window->DC.CursorMaxPos = ImMax(window->DC.CursorMaxPos, window->DC.CursorPos);
	window->DC.IsSetPos = true;
}

void editor::widgets::set_cursor_pos_y(float local_y)
{
	ImGuiWindow* window	   = GetCurrentWindow();
	window->DC.CursorPos.y = window->Pos.y - window->Scroll.y + local_y;
	// window->DC.CursorMaxPos = ImMax(window->DC.CursorMaxPos, window->DC.CursorPos);
	window->DC.IsSetPos = true;
}

void editor::widgets::image(ImTextureID user_texture_id, const ImVec2& size, const ImVec2& uv0, const ImVec2& uv1, const ImVec4& tint_col, const ImVec4& border_col)
{
	auto* window = GetCurrentWindow();
	if (window->SkipItems)
		return;

	auto bb = ImRect(window->DC.CursorPos, window->DC.CursorPos + size);
	if (border_col.w > 0.0f)
		bb.Max += ImVec2(2, 2);
	ItemSize(bb);
	if (!ItemAdd(bb, 0))
		return;

	if (border_col.w > 0.0f)
	{
		window->DrawList->AddRect(bb.Min, bb.Max, GetColorU32(border_col), 0.0f);
		window->DrawList->AddImage(user_texture_id, bb.Min + ImVec2(1, 1), bb.Max - ImVec2(1, 1), uv0, uv1, GetColorU32(tint_col));
	}
	else
	{
		window->DrawList->AddImage(user_texture_id, bb.Min, bb.Max, uv0, uv1, GetColorU32(tint_col));
	}
}

void editor::widgets::text(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	ImGui::TextV(fmt, args);
	va_end(args);
}

void editor::widgets::text(std::string fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	ImGui::TextV(fmt.c_str(), args);
	va_end(args);
}

void editor::widgets::text_colored(const ImVec4& col, const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);

	ImGui::PushStyleColor(ImGuiCol_Text, col);
	ImGui::TextV(fmt, args);
	ImGui::PopStyleColor();

	va_end(args);
}

void editor::widgets::separator()
{
	ImGui::Separator();
}

void editor::widgets::sameline(float offset_from_start_x, float spacing)
{
	ImGui::SameLine(offset_from_start_x, spacing);
}

void editor::widgets::newline()
{
	ImGui::NewLine();
}

void editor::widgets::spacing()
{
	ImGui::Spacing();
}

bool editor::widgets::input_text(const char* label, char* buf, size_t buf_size, ImGuiInputTextFlags flags, ImGuiInputTextCallback callback, void* user_data)
{
	IM_ASSERT(!(flags & ImGuiInputTextFlags_Multiline));	// call InputTextMultiline()
	return ImGui::InputTextEx(label, NULL, buf, (int)buf_size, ImVec2(0, 0), flags, callback, user_data);
}

bool editor::widgets::input_text_with_hint(const char* label, const char* hint, char* buf, size_t buf_size, ImGuiInputTextFlags flags, ImGuiInputTextCallback callback, void* user_data)
{
	return ImGui::InputTextWithHint(label, hint, buf, buf_size, flags, callback, user_data);
}

void editor::widgets::push_font(ImFont* font)
{
	ImGui::PushFont(font);
}

void editor::widgets::pop_font()
{
	ImGui::PopFont();
}

ImVec2 editor::widgets::calc_text_size(const char* text, const char* text_end, bool hide_text_after_double_hash, float wrap_width)
{
	return ImGui::CalcTextSize(text, text_end, hide_text_after_double_hash, wrap_width);
}

namespace
{
	constexpr auto _wait_modal_id_str			   = "__wait_modal";
	auto		   _progress_modal_popup_id		   = ImGuiID();
	auto		   _wait_madal_title			   = std::string();
	auto		   _wait_modal_msg				   = std::string();
	auto		   _p_window_progress_modal_caller = (ImGuiWindow*)nullptr;
	auto		   _callback					   = std::function<void(bool)>();
	auto		   _aysnc_result				   = std::future<bool>();
}	 // namespace

void editor::widgets::progress_modal(const char* title, std::function<bool()> task, std::function<void(bool)> callback)
{
	_p_window_progress_modal_caller = GImGui->CurrentWindow->Flags & ImGuiWindowFlags_ChildMenu
										? ImGui::FindWindowByName("Debug##Default")
										: GImGui->CurrentWindow;

	_wait_madal_title		 = title;
	_progress_modal_popup_id = _p_window_progress_modal_caller->GetID(title);
	OpenPopupEx(_progress_modal_popup_id, ImGuiPopupFlags_None);
	_callback	  = callback;
	_aysnc_result = std::async(task);
}

void editor::widgets::progress_modal_msg(std::string msg)
{
	_wait_modal_msg = msg;
}

void close_popup(ImGuiID id)
{
	auto& g			= *GImGui;
	auto  popup_idx = -1;
	for (auto i = 0; i < g.BeginPopupStack.Size; ++i)
	{
		if (g.BeginPopupStack[i].PopupId == id)
		{
			popup_idx = i;
			break;
		}
	}

	if (popup_idx < 0 or popup_idx >= g.OpenPopupStack.Size or g.BeginPopupStack[popup_idx].PopupId != g.OpenPopupStack[popup_idx].PopupId)
		return;

	// Trim open popup stack
	auto* popup_window			  = g.OpenPopupStack[popup_idx].Window;
	auto* popup_backup_nav_window = g.OpenPopupStack[popup_idx].BackupNavWindow;
	g.OpenPopupStack[popup_idx]	  = g.OpenPopupStack.back();
	g.OpenPopupStack.pop_back();

	auto* focus_window = (popup_window and popup_window->Flags & ImGuiWindowFlags_ChildMenu) ? popup_window->ParentWindow : popup_backup_nav_window;
	if (focus_window and !focus_window->WasActive and popup_window)
	{
		ImGui::FocusTopMostWindowUnderOne(popup_window, NULL, NULL, ImGuiFocusRequestFlags_RestoreFocusedChild);
	}	 // Fallback
	else
	{
		ImGui::FocusWindow(focus_window, (g.NavLayer == ImGuiNavLayer_Main) ? ImGuiFocusRequestFlags_RestoreFocusedChild : ImGuiFocusRequestFlags_None);
	}

	// A common pattern is to close a popup when selecting a menu item/selectable that will open another window.
	// To improve this usage pattern, we avoid nav highlight for a single frame in the parent window.
	// Similarly, we could avoid mouse hover highlight in this window but it is less visually problematic.
	if (ImGuiWindow* window = g.NavWindow)
	{
		window->DC.NavHideHighlightOneFrame = true;
	}
}

void _handle_progress_modal()
{
	if (_p_window_progress_modal_caller is_nullptr)
	{
		return;
	}

	auto window_stack	= std::vector<ImGuiWindow*>();
	auto end_call_stack = std::vector<ImGuiWindow*>();
	for (auto p_window = _p_window_progress_modal_caller; p_window->ParentWindow != nullptr; p_window = p_window->ParentWindow)
	{
		window_stack.emplace_back(p_window);
	}

	for (auto i = (int)window_stack.size() - 1; i > -1; --i)
	{
		auto p_window = window_stack[i];

		if (ImGui::FindWindowByID(p_window->ID)->Active is_false)
		{
			break;
		}

		if (Begin(p_window->Name) is_false)
		{
			break;
		}

		end_call_stack.emplace_back(p_window);
	}

	if (window_stack.size() == end_call_stack.size())
	{
		ImGui::SetNextWindowSizeConstraints({ ImGui::CalcTextSize(_wait_madal_title.c_str()).x + editor::style::window_padding().x * 2, 0 }, { FLT_MAX, FLT_MAX });
		if (BeginPopupModal(_wait_madal_title.c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings))	 //, _wait_modal_id_str, nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings))
		{
			editor::widgets::text(_wait_modal_msg);
			EndPopup();
		}
	}

	while (end_call_stack.empty() is_false)
	{
		auto p_window = end_call_stack.back();
		if (p_window->Flags & ImGuiWindowFlags_ChildWindow)
		{
			EndChild();
		}
		else if (p_window->Flags & ImGuiWindowFlags_Popup)
		{
			EndPopup();
		}
		else
		{
			End();
		}

		end_call_stack.pop_back();
	}

	assert(end_call_stack.empty());

	if (_aysnc_result._Is_ready())
	{
		if (_callback)
		{
			_callback(_aysnc_result.get());
			_aysnc_result._Abandon();
		}

		close_popup(_progress_modal_popup_id);
		_p_window_progress_modal_caller = nullptr;
	}
}

void editor::widgets::on_frame_end()
{
	_handle_progress_modal();
}

// widgets for editor_id and undo-redo

namespace
{
	constexpr const auto DRAG_MOUSE_THRESHOLD_FACTOR = 0.5f;

	std::unordered_map<ImGuiID, uint64> _backup_map;

	bool _drag_scalar(ImGuiDataType data_type, void* p_data, float v_speed = 1.0f, const void* p_min = NULL, const void* p_max = NULL, const char* format = NULL, ImGuiSliderFlags flags = 0);
	bool _drag_scalar_n(editor_id id, ImGuiDataType data_type, void* p_data, int components, float v_speed = 1.0f, const void* p_min = NULL, const void* p_max = NULL, const char* format = NULL, ImGuiSliderFlags flags = 0);

	bool _drag_scalar(ImGuiDataType data_type, void* p_data, float v_speed, const void* p_min, const void* p_max, const char* format, ImGuiSliderFlags flags)
	{
		auto res	 = ImGui::DragScalar("", data_type, p_data, v_speed, p_min, p_max, format, flags);
		auto item_id = GImGui->LastItemData.ID;
		if (editor::widgets::is_item_activated())
		{
			_backup_map[item_id] = *(uint64*)p_data;
		}

		if (editor::widgets::is_item_deactivated_after_edit())
		{
			editor::undoredo::add(
				{ "edit scalar",
				  [=](editor::utilities::memory_handle* p_mem_handle) {
					  // assert(_backup_map.contains(GImGui->LastItemData.ID));
					  // p_mem_handle->p_data : scalar, not pointer
					  std::swap(*(uint64*)p_data, *(uint64*)&p_mem_handle->p_data);
				  },
				  [=](editor::utilities::memory_handle* p_mem_handle) {
					  std::swap(*(uint64*)p_data, *(uint64*)&p_mem_handle->p_data);
				  },
				  (void*)_backup_map[item_id] });
			_backup_map.erase(item_id);
		}

		return res;
	}

	bool _drag_scalar_n(editor_id id, ImGuiDataType data_type, void* p_data, int components, float v_speed, const void* p_min, const void* p_max, const char* format, ImGuiSliderFlags flags)
	{
		auto* window = GetCurrentWindow();
		if (window->SkipItems)
			return false;

		bool value_changed = false;
		BeginGroup();
		PushID((const void*)id.value);
		PushMultiItemsWidths(components, CalcItemWidth());
		size_t type_size = DataTypeGetInfo(data_type)->Size;

		for (int i = 0; i < components; i++)
		{
			PushID(i);
			if (i > 0)
			{
				SameLine(0, editor::style::item_inner_spacing().x);
			}

			value_changed |= _drag_scalar(data_type, p_data, v_speed, p_min, p_max, format, flags);
			PopID();
			PopItemWidth();
			p_data = (void*)((char*)p_data + type_size);
		}
		PopID();

		// const char* label_end = FindRenderedTextEnd(label);
		// if (label != label_end)
		//{
		//	SameLine(0, editor::style::item_inner_spacing().x);
		//	TextEx(label, label_end);
		// }

		EndGroup();
		return value_changed;
	}

	void* _offset_to_ptr(size_t offset, const void* ptr)
	{
		return (void*)((char*)ptr + offset);
	}
}	 // namespace

bool primitive_drag(editor_id id, e_primitive_type type, void* ptr)
{
	constexpr const float f_max = FLT_MAX;
	constexpr const float f_min = -FLT_MAX;
	switch (type)
	{
	case primitive_type_int2:
		return _drag_scalar_n(id, ImGuiDataType_S32, ptr, 2);
	case primitive_type_int3:
		return _drag_scalar_n(id, ImGuiDataType_S32, ptr, 3);
	case primitive_type_int4:
		return _drag_scalar_n(id, ImGuiDataType_S32, ptr, 4);

	case primitive_type_uint2:
		return _drag_scalar_n(id, ImGuiDataType_U32, ptr, 2);
	case primitive_type_uint3:
		return _drag_scalar_n(id, ImGuiDataType_U32, ptr, 3);
	case primitive_type_uint4:
		return _drag_scalar_n(id, ImGuiDataType_U32, ptr, 4);

	case primitive_type_float2:
	case primitive_type_float2a:
		return _drag_scalar_n(id, ImGuiDataType_Float, ptr, 2);
	case primitive_type_float3:
	case primitive_type_float3a:
		return _drag_scalar_n(id, ImGuiDataType_Float, ptr, 3);
	case primitive_type_float4:
	case primitive_type_float4a:
		return _drag_scalar_n(id, ImGuiDataType_Float, ptr, 4);
	case primitive_type_uint64:
		return _drag_scalar(ImGuiDataType_U64, ptr);
	case primitive_type_uint32:
		return _drag_scalar(ImGuiDataType_U32, ptr);
	case primitive_type_uint16:
		return _drag_scalar(ImGuiDataType_U16, ptr);
	case primitive_type_uint8:
		return _drag_scalar(ImGuiDataType_U8, ptr);
	case primitive_type_int64:
		return _drag_scalar(ImGuiDataType_U64, ptr);
	case primitive_type_int32:
		return _drag_scalar(ImGuiDataType_U32, ptr);
	case primitive_type_int16:
		return _drag_scalar(ImGuiDataType_U16, ptr);
	case primitive_type_int8:
		return _drag_scalar(ImGuiDataType_U8, ptr);
	case primitive_type_float32:
		return _drag_scalar(ImGuiDataType_Float, ptr);
	case primitive_type_double64:
		return _drag_scalar(ImGuiDataType_Double, ptr);
	case primitive_type_float3x3:
	{
		auto res = false;
		std::ranges::for_each(std::views::iota(0, 3), [&](auto row) {
			PushID(row);
			res |= _drag_scalar_n(id, ImGuiDataType_Float, ptr, 3);
			ptr	 = (void*)((char*)ptr + sizeof(float) * 3);
			PopID();
		});
		return res;
	}
	case primitive_type_float4x4:
	case primitive_type_float4x4a:
	{
		auto res = false;
		std::ranges::for_each(std::views::iota(0, 4), [&](auto row) {
			PushID(row);
			res |= _drag_scalar_n(id, ImGuiDataType_Float, ptr, 4);
			ptr	 = (void*)((char*)ptr + sizeof(float) * 4);
			PopID();
		});
		return res;
	}
	default:
		break;
	}

	return false;
}

bool editor::widgets::component_drag(editor_id c_id)
{
	using namespace editor::models;
	constexpr const float f_max = FLT_MAX;
	constexpr const float f_min = -FLT_MAX;

	auto* p_component = component::find(c_id);
	auto* p_struct	  = reflection::find_struct(p_component->struct_id);
	ImGui::PushID((const void*)c_id.value);

	auto node_opened = widgets::tree_node(p_struct->name);
	editor::add_left_right_click_source(c_id);

	if (node_opened)
	{
		if (ImGui::BeginTable("", 2, ImGuiTableFlags_SizingStretchProp))
		{
			std::ranges::for_each(reflection::all_fields(p_struct->id), [=](auto* p_f) {
				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				widgets::text(p_f->name);

				ImGui::TableSetColumnIndex(1);
				ImGui::BeginGroup();
				primitive_drag(p_f->id, p_f->type, _offset_to_ptr(p_f->offset, component::get_memory(p_component->id)));
				ImGui::EndGroup();
			});

			ImGui::EndTable();
		}

		widgets::tree_pop();
	}


	ImGui::PopID();

	return false;
}

bool editor::widgets::editable_header(editor_id target_id, std::string target_name)
{
	char c_buf[50] { 0 };
	memcpy(c_buf, target_name.c_str(), target_name.size());
	style::push_color(ImGuiCol_FrameBg, COL_BLACK);
	style::push_var(ImGuiStyleVar_FrameRounding, 0);
	auto res = widgets::input_text(std::format("##{}", target_id.value).c_str(), c_buf, 50);

	if (widgets::is_item_deactivated_after_edit())
	{
		assert(editor::get_current_selection() == target_id);
		editor::models::cmd_rename_selection(editor::models::text::create(c_buf));
	}

	style::pop_var(1);
	style::pop_color(1);
	return res;
}

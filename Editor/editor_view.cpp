#define IMGUI_DEFINE_MATH_OPERATORS
#define STR_MENU_SHORTCUT_SPACE_COUNT 8

#include "editor_view.h"
#include "game_project/game_project.h"
#include "logger.h"
#include "views/view_hierarchy.h"
#include "pugixml/pugixml.hpp"

namespace Editor::View
{
	namespace
	{
#define max(a, b) a > b ? a : b;

		constexpr auto POPUP_PADDING	 = ImVec2(6.f, 6.f);
		constexpr auto POPUP_BORDER		 = .4f;
		constexpr auto MENU_ITEM_PADDING = ImVec2(8.4f, 2.4f);
		constexpr auto MENU_ITEM_BORDER	 = .4f;

		struct _menu
		{
			std::string name;
			std::string shortcut;

			_menu *p_parent = nullptr, *p_sibling = nullptr, *p_child = nullptr;

			float shortcut_pos_x = 0;

			ImVec2 popup_size;
			ImRect rect;

			void (*callback)();
		};

		enum _main_menu_state
		{
			None,
			Hovered,
			Selected,
		};

		// auto _call_back_buffer	= (std::vector<std::pair<std::string, void (*)()>>*)(nullptr);
		auto _menu_vec			= std::vector<_menu>();
		auto _menu_header_state = None;
		auto _menu_item_padding = ImVec2();
		auto _popup_padding		= ImVec2();

		void _init_main_menu()
		{
			auto doc = pugi::xml_document();
			doc.load_file("CaptionMenu.xml");
			auto   index = 0, parent_index = -1;
			auto   stack = std::vector { std::pair(parent_index, doc.child("caption_menu").first_child()) };
			_menu* p_menu;

			while (stack.empty() is_false)
			{
				auto parent_index	= stack.back().first;
				auto node			= stack.back().second;
				auto max_name_width = 0.f, max_shortcut_width = 0.f;

				stack.pop_back();

				for (auto sibling = node; sibling.empty() is_false; ++index)
				{
					if (parent_index != -1 && _menu_vec[parent_index].rect.Max.x == -1) _menu_vec[parent_index].rect.Max.x = index;
					if (sibling.first_child()) stack.emplace_back(std::pair(index, sibling.first_child()));
					p_menu			 = &_menu_vec.emplace_back();
					p_menu->name	 = sibling.attribute("name").value();
					p_menu->shortcut = sibling.attribute("shortcut").value();
					sibling			 = sibling.next_sibling();
					p_menu->rect	 = ImRect { (float)parent_index, sibling ? index + 1 : -1.f, -1.f, 0 };	   // parent, sibling ,child,
				}
			}

			for (auto& menu : _menu_vec)
			{
				int parent_index = menu.rect.Min.x, sibling_index = menu.rect.Min.y, child_index = menu.rect.Max.x;

				if (parent_index != -1) menu.p_parent = &_menu_vec[parent_index];
				if (sibling_index != -1) menu.p_sibling = &_menu_vec[sibling_index];
				if (child_index != -1) menu.p_child = &_menu_vec[child_index];
			}

			assert(_menu_vec.empty() is_false);
		}

		void _update_dpi_scale_menu_items(_menu* p_item = &_menu_vec[0])
		{
			if (p_item is_nullptr) return;

			auto is_header = p_item->p_parent is_nullptr;

			auto max_name_width = 0.f, max_shortcut_width = 0.f;
			auto sibling_count = 0;
			if (is_header is_false)
			{
				for (auto p_sibling = p_item; p_sibling != nullptr; p_sibling = p_sibling->p_sibling)
				{
					max_name_width	   = max(max_name_width, ImGui::CalcTextSize(p_sibling->name.c_str()).x);
					max_shortcut_width = max(max_shortcut_width, ImGui::CalcTextSize(p_sibling->shortcut.c_str()).x);
					++sibling_count;
				}
			}

			auto shortcut_offset_x	   = max_name_width + STR_MENU_SHORTCUT_SPACE_COUNT * GImGui->Font->IndexAdvanceX[' '];
			auto extra_width_for_arrow = GImGui->FontSize * 1.50f;
			auto item_height		   = GImGui->Font->FontSize + _menu_item_padding.y * 2;
			auto now_draw_pos		   = is_header ? ImVec2((CAPTION_ICON_CPOS.x + CAPTION_ICON_SIZE.x + CAPTION_ICON_MENU_SPACE) * GEctx->Dpi_Scale, (CAPTION_HIGHT * GEctx->Dpi_Scale - item_height) / 2) : _popup_padding;
			auto main_menu_pos		   = now_draw_pos;

			auto child_menu_width = shortcut_offset_x + max_shortcut_width + extra_width_for_arrow + _menu_item_padding.x * 2;
			auto main_menu_width  = 0;
			for (auto p_sibling = p_item; p_sibling != nullptr; p_sibling = p_sibling->p_sibling)
			{
				auto item_width = is_header ? ImGui::CalcTextSize(p_sibling->name.c_str()).x + _menu_item_padding.x * 2 : child_menu_width;
				auto size		= ImVec2(item_width, item_height);

				p_sibling->rect			   = ImRect(now_draw_pos, now_draw_pos + size);
				p_sibling->shortcut_pos_x  = shortcut_offset_x;
				now_draw_pos			  += is_header ? ImVec2(item_width, 0) : ImVec2(0, item_height);
				main_menu_width			  += item_width;
				_update_dpi_scale_menu_items(p_sibling->p_child);
			}

			if (is_header)
			{
				GEctx->Main_Menu_Rect = ImRect(main_menu_pos, ImVec2(now_draw_pos.x, main_menu_pos.y + item_height));
			}
			else
			{
				p_item->p_parent->popup_size = ImVec2(child_menu_width, item_height * sibling_count) + _popup_padding * 2;
			}
		}

		void _menu_item(const _menu* const p_menu)
		{
			bool   hovered	= false;
			bool   pressed	= false;
			auto   p_window = ImGui::GetCurrentWindowRead();
			ImRect rect_abs = p_menu->rect;
			{
				rect_abs.Min += p_window->Pos;
				rect_abs.Max += p_window->Pos;
			}
			ImVec2 render_pos				   = rect_abs.GetTL() + _menu_item_padding;
			p_window->DC.CursorPos			   = render_pos;
			p_window->WorkRect				   = ImRect(p_window->Pos + _popup_padding, p_window->Pos + p_window->Size - _popup_padding);
			p_window->WorkRect.Max.x		  -= _menu_item_padding.x;
			p_window->ContentRegionRect.Max.x  = rect_abs.Max.x - _menu_item_padding.x;

			if (p_menu->p_child)
			{
				auto popup_pos = ImVec2(p_window->Rect().Max.x, rect_abs.Min.y);
				ImGui::SetNextWindowSize(p_menu->popup_size);
				ImGui::SetNextWindowPos(popup_pos);

				if (ImGui::BeginMenu(p_menu->name.c_str()))
				{
					hovered = true;
					for (auto child = p_menu->p_child; child != nullptr; child = child->p_sibling)
					{
						_menu_item(child);
					}

					ImGui::EndMenu();
				}
			}
			else
			{
				pressed = ImGui::Selectable(p_menu->name.c_str(), false, ImGuiSelectableFlags_SelectOnRelease | ImGuiSelectableFlags_NoSetKeyOwner | ImGuiSelectableFlags_SetNavIdOnHover | ImGuiSelectableFlags_SpanAvailWidth);
				hovered = ImGui::IsItemHovered();
				// hovered = p_window->GetID(item.Name.c_str()) == GImGui->HoveredId;
			}

			if (p_menu->shortcut.empty() is_false)
			{
				p_window->DrawList->AddText(render_pos + ImVec2(p_menu->shortcut_pos_x, 0), ImColor(ImGui::GetColorU32(ImGuiCol_Text)), p_menu->shortcut.c_str());
			}

			if (hovered)
			{
				p_window->DrawList->AddRect(rect_abs.Min, rect_abs.Max, ImColor(COL_BD_SELECTED), 0, ImDrawFlags_Closed, MENU_ITEM_BORDER * GEctx->Dpi_Scale);
			}
			if (pressed and p_menu->callback)
			{
				p_menu->callback();
			}
		}

		void _main_menu()
		{
			static const _menu *p_hovered = nullptr, *p_selected = nullptr, *p_canceled = nullptr;
			static auto			_open_popup = false;

			auto p_window	 = ImGui::GetCurrentWindowRead();
			auto p_draw_list = p_window->DrawList;
			auto text_pos	 = _menu_vec[0].rect.GetTL() + p_window->Pos + _menu_item_padding;

			bool selected = p_selected and p_canceled is_nullptr, hovered = not selected and p_hovered and p_canceled is_nullptr;

			if (hovered)
			{
				const auto rect_abs = ImRect(p_hovered->rect.Min + p_window->Pos, p_hovered->rect.Max + p_window->Pos);

				p_draw_list->AddRectFilled(rect_abs.GetTL(), rect_abs.GetBR(), ImColor(COL_BG_SELECTED));
				p_draw_list->AddRect(rect_abs.GetTL(), rect_abs.GetBR(), ImColor(COL_BD_SELECTED), 0, 0, 1.f);
			}
			if (selected)
			{
				const auto rect_abs = ImRect(p_selected->rect.Min + p_window->Pos, p_selected->rect.Max + p_window->Pos);

				p_draw_list->AddRectFilled(rect_abs.GetTL(), rect_abs.GetBR(), ImColor(COL_BG_ACTIVE));
				{
					ImGui::PushStyleColor(ImGuiCol_PopupBg, COL_BG_ACTIVE);
					ImGui::PushStyleColor(ImGuiCol_HeaderHovered, COL_BG_SELECTED);
					ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, _menu_item_padding * 2);
					ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2());
					ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2());
					ImGui::PushStyleVar(ImGuiStyleVar_PopupBorderSize, 0);
					ImGui::SetNextWindowPos(rect_abs.GetBL());
					ImGui::SetNextWindowSize(p_selected->popup_size);

					if (ImGui::BeginPopup(p_selected->name.c_str()))
					{
						const auto&	 popup_rect = ImGui::GetCurrentWindowRead()->Rect();
						const ImVec2 border_points[] {
							rect_abs.GetTL(), rect_abs.GetTR(), rect_abs.GetBR(),
							popup_rect.GetTR(), popup_rect.GetBR(), popup_rect.GetBL()
						};

						ImGui::GetForegroundDrawList()->AddPolyline(border_points, 6, ImColor(COL_BD_SELECTED), ImDrawFlags_Closed, POPUP_BORDER * GEctx->Dpi_Scale);

						{
							ImGui::PushStyleVar(ImGuiStyleVar_PopupBorderSize, POPUP_BORDER * GEctx->Dpi_Scale);
							for (auto p_child = p_selected->p_child; p_child is_not_nullptr; p_child = p_child->p_sibling)
							{
								_menu_item(p_child);
							}
							ImGui::PopStyleVar(1);
						}

						ImGui::EndPopup();
					}

					ImGui::PopStyleVar(4);
					ImGui::PopStyleColor(2);
				}

				if (_open_popup)
				{
					ImGui::OpenPopup(p_selected->name.c_str());
					_open_popup = false;
				}
			}

			p_hovered = nullptr;
			for (auto* p_header = &_menu_vec[0]; p_header is_not_nullptr; p_header = p_header->p_sibling)
			{
				auto id	   = p_window->GetID(p_header->name.c_str());
				auto rect  = p_header->rect;
				rect.Min  += p_window->Pos;
				rect.Max  += p_window->Pos;
				ImGui::ItemAdd(rect, id);

				if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup))
				{
					p_hovered = p_header;
				}

				ImGui::RenderTextWrapped(text_pos, p_header->name.c_str(), nullptr, 0.f);
				text_pos.x += rect.GetSize().x;

				// Init_Child_Calc_PopupSize(header, dpi_scale);
			}

			switch (_menu_header_state)
			{
			case None:
			{
				if (p_canceled)
				{
					if (GImGui->IO.MouseClicked[0])
					{
						_menu_header_state = Selected;
						_open_popup		   = true;
						p_canceled		   = nullptr;
					}
					else if (p_canceled != p_hovered)
					{
						_menu_header_state = Hovered;
						p_canceled		   = nullptr;
						p_selected		   = nullptr;
					}
				}
				else if (p_hovered)
				{
					_menu_header_state = Hovered;
				}

				break;
			}
			case Hovered:
			{
				if (p_hovered is_nullptr)
				{
					_menu_header_state = None;
				}
				else if (GImGui->IO.MouseClicked[0])
				{
					_menu_header_state = Selected;
					_open_popup		   = true;
				}

				break;
			}
			case Selected:
			{
				if (p_hovered and p_hovered != p_selected)
				{
					p_selected	= p_hovered;
					_open_popup = true;
				}
				else if (not ImGui::IsPopupOpen(p_selected->name.c_str()))
				{
					_menu_header_state = None;
					p_canceled		   = p_selected;
				}

				break;
			}
			}
		}

		void _main_dock_space()
		{
			auto window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
								ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoDocking;
			auto   mainViewPort = ImGui::GetMainViewport();
			auto   size			= mainViewPort->Size - ImVec2(0, CAPTION_HIGHT * GEctx->Dpi_Scale);
			ImVec2 pos			= mainViewPort->Pos + ImVec2(0, CAPTION_HIGHT * GEctx->Dpi_Scale);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2());
			ImGui::SetNextWindowPos(pos);
			ImGui::SetNextWindowSize(size);
			ImGui::SetNextWindowViewport(mainViewPort->ID);
			if (ImGui::Begin("MainView", NULL, window_flags))
			{
				ImGui::DockSpace(ImGui::GetID("DockSpace"), size, ImGuiDockNodeFlags_None);	   // | ImGuiDockNodeFlags_NoSplit);
			}

			ImGui::End();
			ImGui::PopStyleVar(3);
		}
	}	 // namespace

	void Update_Dpi_Scale()
	{
		_menu_item_padding = MENU_ITEM_PADDING * GEctx->Dpi_Scale;
		_popup_padding	   = POPUP_PADDING * GEctx->Dpi_Scale;
		_update_dpi_scale_menu_items();
	}

	void Init()
	{
		_init_main_menu();
		Add_Callback_To_Main_Menu("Window\\Hierarchy", Editor::View::Hierarchy::Open);
		Add_Callback_To_Main_Menu("Window\\Console", Editor::Logger::View::Open);
		Add_Callback_To_Main_Menu("File\\Save", []()
								  { Editor::GameProject::Save(); });
	}

	bool Add_Callback_To_Main_Menu(std::string path, void (*callback)())
	{
		auto p_menu = (_menu*)nullptr;
		auto stream = std::istringstream(path);
		for (std::string str; std::getline(stream, str, '\\');)
		{
			p_menu = p_menu is_nullptr ? &_menu_vec[0] : p_menu->p_child;
			for (;; p_menu = p_menu->p_sibling)
			{
				if (p_menu is_nullptr) return false;
				if (p_menu->name == str)
				{
					break;
				}
			}
		}

		p_menu->callback = callback;
	}

	void Main_Menu()
	{
		_main_menu();
	}

	void Main_Dock()
	{
		_main_dock_space();
	}

	void Show()
	{
		Editor::Logger::View::Show();
		Hierarchy::Show();
	}
}	 // namespace Editor::View

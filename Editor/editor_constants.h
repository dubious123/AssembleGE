#pragma once
#include "external/include/imgui/imgui.h"

typedef unsigned int UINT;
constexpr float		 CAPTION_HIGHT			  = 32.f;
constexpr float		 MAIN_MENU_HEADER_BORDER  = 1.f;
constexpr float		 CAPTION_ICON_MENU_SPACE  = 3.F;
constexpr ImVec2	 MAIN_MENU_HEADER_PADDING = ImVec2(6.f, 2.f);
constexpr ImVec2	 CAPTION_BUTTON_SIZE	  = ImVec2(45.f, CAPTION_HIGHT);
constexpr ImVec2	 CAPTION_ICON_SIZE		  = ImVec2(26.f, 26.f);
constexpr ImVec2	 CAPTION_ICON_CPOS		  = ImVec2(8.f, 4.f);
constexpr ImVec4	 COL_GRAY_8				  = ImVec4(0.79f, 0.79f, 0.79f, 1.f);
constexpr ImVec4	 COL_GRAY_6				  = ImVec4(0.6f, 0.6f, 0.6f, 1.f);
constexpr ImVec4	 COL_GRAY_5				  = ImVec4(0.55f, 0.55f, 0.55f, 1.f);
constexpr ImVec4	 COL_GRAY_3				  = ImVec4(0.24f, 0.24f, 0.24f, 1.f);
constexpr ImVec4	 COL_GRAY_2				  = ImVec4(0.18f, 0.18f, 0.18f, 1.f);
constexpr ImVec4	 COL_GRAY_1				  = ImVec4(0.14f, 0.14f, 0.14f, 1.f);
constexpr ImVec4	 COL_BLACK				  = ImVec4(0.08f, 0.08f, 0.08f, 1.f);
constexpr ImVec4	 COL_RED				  = ImVec4(0.67f, 0.f, 0.f, 1.f);

constexpr ImVec4 COL_TEXT = ImVec4(0.98f, 0.98f, 0.98f, 1.f);

constexpr ImVec4 COL_TEXT_GRAY	   = ImVec4(0.55f, 0.55f, 0.55f, 1.f);
constexpr ImVec4 COL_TEXT_DISABLED = ImVec4(0.36f, 0.36f, 0.36f, 1.f);

constexpr ImVec4 COL_BG_WINDOW	 = ImVec4(0.12f, 0.12f, 0.12f, 1.f);
constexpr ImVec4 COL_BG_POPUP	 = COL_GRAY_2;
constexpr ImVec4 COL_BG_CAPTION	 = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
constexpr ImVec4 COL_BG_SELECTED = ImVec4(0.24f, 0.24f, 0.24f, 1.f);
constexpr ImVec4 COL_BG_ACTIVE	 = COL_BG_POPUP;

constexpr ImVec4 COL_BD_SELECTED = ImVec4(0.44f, 0.44f, 0.44f, 1.f);
constexpr ImVec4 COL_BD_ACTIVE	 = ImVec4(0.26f, 0.26f, 0.26f, 1.f);

constexpr UINT LOG_MAX_COUNT_PER_FRAME = 1024;
constexpr UINT LOG_BUFFER_SIZE		   = 1024 * 1024 * 256;	   // 256 Mb

constexpr auto PROJECT_EXTENSION	  = ".assemble";
constexpr auto GAMECODE_DIRECTORY	  = "game_code";
constexpr auto PROJECT_DATA_FILE_NAME = "project_data.xml";
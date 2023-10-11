#pragma once

#include "imgui\imgui_internal.h"

namespace Editor
{
	void Init();
	void Run();
	void Close();
	void Update_Dpi_Scale(float new_dpi_scale);
	void Show_Custom_Caption();
	void Draw_Caption_Buttons();
	void Show_Project_Browser();
	bool Is_Window_Maximized(void* hwnd);
}	 // namespace Editor

enum Editor_DataType
{
	DataType_Project,
	DataType_Scene,
	DataType_World,
	DataType_SubWorld,
	DataType_Entity,
	DataType_Component,
	DataType_System,
	DataType_Count,
	DataType_InValid = 0xff,
};

enum CaptionButton
{
	Caption_Button_Min,
	Caption_Button_Max,
	Caption_Button_Close,
	Caption_Button_None,
};

struct EditorContext
{
	void*		  Hwnd;
	float		  Dpi_Scale;
	CaptionButton Caption_Hovered_Button = Caption_Button_None;
	CaptionButton Caption_Held_Button	 = Caption_Button_None;
	ImU64		  Icon_Texture_Id;
	ImRect		  Main_Menu_Rect;
	ImFont*		  P_Font_Arial_Default_13_5;
	ImFont*		  P_Font_Arial_Bold_13_5;
	ImFont*		  P_Font_Arial_Default_16;
	ImFont*		  P_Font_Arial_Bold_16;
	ImFont*		  P_Font_Arial_Default_18;
	ImFont*		  P_Font_Arial_Bold_18;
	bool		  Dpi_Changed = false;
};

extern EditorContext* GEctx;	// Current implicit context pointer

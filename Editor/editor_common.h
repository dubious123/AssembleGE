#pragma once
#include "..\Engine\__math.h"

#define INVALID_ID	0x7f00'0000'0000'0000
#define INVALID_IDX 0xffff

constexpr auto POPUP_PADDING = ImVec2(6.f, 6.f);

constexpr auto POPUP_BORDER		 = .4f;
constexpr auto MENU_ITEM_PADDING = ImVec2(8.4f, 2.4f);
constexpr auto MENU_ITEM_BORDER	 = .4f;

constexpr auto CAPTION_HIGHT			= 32.f;
constexpr auto MAIN_MENU_HEADER_BORDER	= 1.f;
constexpr auto CAPTION_ICON_MENU_SPACE	= 3.F;
constexpr auto MAIN_MENU_HEADER_PADDING = ImVec2(6.f, 2.f);
constexpr auto CAPTION_BUTTON_SIZE		= ImVec2(45.f, CAPTION_HIGHT);
constexpr auto CAPTION_ICON_SIZE		= ImVec2(26.f, 26.f);
constexpr auto CAPTION_ICON_CPOS		= ImVec2(8.f, 4.f);
constexpr auto COL_GRAY_8				= ImVec4(0.79f, 0.79f, 0.79f, 1.f);
constexpr auto COL_GRAY_6				= ImVec4(0.6f, 0.6f, 0.6f, 1.f);
constexpr auto COL_GRAY_5				= ImVec4(0.55f, 0.55f, 0.55f, 1.f);
constexpr auto COL_GRAY_3				= ImVec4(0.24f, 0.24f, 0.24f, 1.f);
constexpr auto COL_GRAY_2				= ImVec4(0.18f, 0.18f, 0.18f, 1.f);
constexpr auto COL_GRAY_1				= ImVec4(0.14f, 0.14f, 0.14f, 1.f);
constexpr auto COL_BLACK				= ImVec4(0.08f, 0.08f, 0.08f, 1.f);
constexpr auto COL_RED					= ImVec4(0.67f, 0.f, 0.f, 1.f);

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

constexpr uint32 LOG_BUFFER_SIZE = 1024 * 1024 * 256;	 // 256 Mb

enum editor_data_type : unsigned long long
{
	DataType_Entity,
	DataType_Project,
	DataType_Scene,
	DataType_World,
	DataType_SubWorld,
	DataType_Component,
	DataType_System,
	DataType_Struct,
	DataType_Field,
	DataType_Editor_Text,
	DataType_Editor_Command,
	DataType_Editor_UndoRedo,
	DataType_Count,

	DataType_InValid = 0xff,
};

enum caption_button
{
	Caption_Button_Min,
	Caption_Button_Max,
	Caption_Button_Close,
	Caption_Button_None,
};

struct editor_id
{
	//[type (8)][gen (4)][key (52)]
  public:
	uint64 value;

	struct hash_func
	{
		size_t operator()(const editor_id& id) const
		{
			return id.value;
		}
	};

	constexpr inline editor_id(uint64 value) : value(value) { };

	constexpr inline editor_id() : value(0x7fff'ffff'ffff'ffff) { };

	constexpr inline editor_id(uint64 type, uint8 gen, uint64 key) : value((type << 56) | (((uint64)gen & 0x0f) << 52) | (key & 0x000f'ffff'ffff'ffff)) { };

	constexpr inline editor_id(uint64 type, uint8 gen) : value((type << 56) | (((uint64)gen & 0x0f) << 52)) { };

	inline std::string str() const
	{
		return std::format("{:#016x}", value);
	}

	inline editor_data_type type() const
	{
		const auto type = (editor_data_type)((value >> 56) & 0x00ff);
		return type;
	}

	inline uint8 gen() const
	{

		const uint8 gen = (value >> 52) & 0x0f;
		return gen;
	}

	inline uint64 key() const
	{
		const uint64 key = value & 0x000f'ffff'ffff'ffff;
		return key;
	}

	inline void increase_gen()
	{
		value += 0x0010'0000'0000'0000;
	}

	inline bool operator==(const editor_id& other) const
	{
		return value == other.value;
	};

	inline bool operator<=(const editor_id& other) const
	{
		return value <= other.value;
	}

	inline bool operator<(const editor_id& other) const
	{
		return value < other.value;
	}

	inline bool operator>=(const editor_id& other) const
	{
		return value >= other.value;
	}

	inline bool operator>(const editor_id& other) const
	{
		return value > other.value;
	}
};

struct editor_context
{
	void*		   hwnd = nullptr;
	float		   dpi_scale {};
	caption_button caption_hovered_btn = Caption_Button_None;
	caption_button caption_held_btn	   = Caption_Button_None;
	ImU64		   icon_texture_id {};
	ImRect		   main_menu_rect {};
	ImFont*		   p_font_arial_default_13_5 = nullptr;
	ImFont*		   p_font_arial_bold_13_5	 = nullptr;
	ImFont*		   p_font_arial_default_16	 = nullptr;
	ImFont*		   p_font_arial_bold_16		 = nullptr;
	ImFont*		   p_font_arial_default_18	 = nullptr;
	ImFont*		   p_font_arial_bold_18		 = nullptr;
	bool		   dpi_changed				 = false;
};

struct editor_command
{
	std::string						  _name;
	int								  _shortcut_key = ImGuiKey_None;
	std::function<bool(editor_id id)> _can_execute_func;
	std::function<void(editor_id)>	  _execute_func;
	void*							  _memory;

	bool can_execute(editor_id id = INVALID_ID) const
	{
		return _can_execute_func(id);
	}

	void execute(editor_id id = INVALID_ID) const
	{
		_execute_func(id);
	}

	void operator()(editor_id id) const
	{
		if (_can_execute_func(id))
		{
			_execute_func(id);
		}
	}

	void operator()() const
	{
		if (_can_execute_func(INVALID_ID))
		{
			_execute_func(INVALID_ID);
		}
	}
};

extern editor_context* GEctx;

namespace editor::utilities
{
	struct memory_handle
	{
		std::function<void(void*)> clean_up_func = nullptr;
		void*					   p_data		 = nullptr;

		memory_handle() = default;
		memory_handle(std::function<void(void*)> clean_up_func, void* p_data) : clean_up_func(clean_up_func), p_data(p_data) { };

		memory_handle(const memory_handle&)			   = delete;
		memory_handle& operator=(const memory_handle&) = delete;

		memory_handle(memory_handle&& other) noexcept : clean_up_func(other.clean_up_func), p_data(other.p_data)
		{
			other.clean_up_func = nullptr;
		}

		memory_handle& operator=(memory_handle&& other) noexcept
		{
			if (this != &other)
			{
				clean_up_func = other.clean_up_func;
				p_data		  = other.p_data;

				other.clean_up_func = nullptr;

				return *this;
			}
		}

		~memory_handle()
		{
			if (p_data and clean_up_func)
			{
				clean_up_func(p_data);
			}
		}

		void release()
		{
			if (p_data and clean_up_func)
			{
				clean_up_func(p_data);
				clean_up_func = nullptr;
			}
		}
	};
}	 // namespace editor::utilities

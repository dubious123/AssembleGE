#pragma once
#include "imgui\imgui_internal.h"
#include <string>
#include <functional>
#include <filesystem>
#include <ranges>


#define LOAD_FUNC(func_type, func_name, library)                       \
	[=]() {                                                            \
		using lib_func = func_type;                                    \
		auto func	   = (lib_func)GetProcAddress(library, func_name); \
		assert(func);                                                  \
		return func;                                                   \
	}()

#define LOAD_RUN_FUNC(func_type, func_name, library)                   \
	[=]() {                                                            \
		using lib_func = func_type;                                    \
		auto func	   = (lib_func)GetProcAddress(library, func_name); \
		return func();                                                 \
	}()

#define STR_MENU_SHORTCUT_SPACE_COUNT 8
#define INVALID_ID					  0x7f00'0000'0000'0000
#define INVALID_IDX					  0xffff

typedef unsigned int UINT;

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

constexpr UINT LOG_BUFFER_SIZE = 1024 * 1024 * 256;	   // 256 Mb

constexpr auto PROJECT_EXTENSION			= ".assemble";
constexpr auto GAMECODE_DIRECTORY			= "game_code";
constexpr auto GAMECODE_GENERATED_DIRECTORY = "generated";
constexpr auto PROJECT_DATA_FILE_NAME		= "project_data.xml";

using uint64 = uint64_t;
using uint32 = uint32_t;
using uint16 = uint16_t;
using uint8	 = uint8_t;

using int64 = int64_t;
using int32 = int32_t;
using int16 = int16_t;
using int8	= int8_t;

using float32  = float;
using double64 = double;

#if defined(_WIN64)
	#include <DirectXMath.h>
using float_2	 = DirectX::XMFLOAT2;
using float_2a	 = DirectX::XMFLOAT2A;
using float_3	 = DirectX::XMFLOAT3;
using float_3a	 = DirectX::XMFLOAT3A;
using float_4	 = DirectX::XMFLOAT4;
using float_4a	 = DirectX::XMFLOAT4A;
using uint_2	 = DirectX::XMUINT2;
using uint_3	 = DirectX::XMUINT3;
using uint_4	 = DirectX::XMUINT4;
using int_2		 = DirectX::XMINT2;
using int_3		 = DirectX::XMINT3;
using int_4		 = DirectX::XMINT4;
using float_3x3	 = DirectX::XMFLOAT3X3;
using float_4x4	 = DirectX::XMFLOAT4X4;
using float_4x4a = DirectX::XMFLOAT4X4A;
#endif

// constexpr auto CONTEXT_POPUP		  = "ctx popup";

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

enum PrimitiveType
{
	PrimitiveType_Int64,
	PrimitiveType_Int16,
	PrimitiveType_Int8,
	PrimitiveType_Uint64,
	PrimitiveType_Uint16,
	PrimitiveType_Uint8,
	PrimitiveType_Double64,

	PrimitiveType_Int32,
	PrimitiveType_Int2,
	PrimitiveType_Int3,
	PrimitiveType_Int4,
	PrimitiveType_Uint2,
	PrimitiveType_Uint3,
	PrimitiveType_Uint4,

	PrimitiveType_Float32,
	PrimitiveType_Float2,
	PrimitiveType_Float3,
	PrimitiveType_Float4,
	PrimitiveType_Float2a,
	PrimitiveType_Float3a,
	PrimitiveType_Float4a,


	PrimitiveType_Float3x3,
	PrimitiveType_Float4x4,
	PrimitiveType_Float4x4a,
	PrimitiveType_Count
};

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

struct editor_command;

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

	constexpr inline editor_id(uint64 value) : value(value) {};

	constexpr inline editor_id() : value(0x7fff'ffff'ffff'ffff) {};

	constexpr inline editor_id(uint64 type, uint8 gen, uint64 key) : value((type << 56) | (((uint64)gen & 0x0f) << 52) | (key & 0x000f'ffff'ffff'ffff)) {};

	constexpr inline editor_id(uint64 type, uint8 gen) : value((type << 56) | (((uint64)gen & 0x0f) << 52)) {};

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

	// editor_command(std::string name, int shortcut_key, bool (*can_execute_func)(editor_id), void (*execute_func)(editor_id))
	//	: _name(name), _shortcut_key(shortcut_key), _can_execute_func(can_execute_func), _execute_func(execute_func) {};

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

extern editor_context* GEctx;	 // Current implicit context pointer

namespace editor
{
	extern const editor_command cmd_add_new;
	extern const editor_command cmd_remove;
	extern const editor_command cmd_select_new;
	extern const editor_command cmd_add_select;
	extern const editor_command cmd_deselect;

	void init();
	void run();
	void on_frame_end();
	void on_project_loaded();
	void on_project_unloaded();

	void select_new(editor_id);
	void add_select(editor_id);
	void deselect(editor_id);
	bool is_selected(editor_id id);
	void add_right_click_source(editor_id id);
	void add_left_click_source(editor_id id);

	editor_id					  get_current_selection();
	const std::vector<editor_id>& get_all_selections();
	bool						  is_selection_vec_empty();

	bool add_context_item(std::string path, const editor_command* command, editor_id id = INVALID_ID);
}	 // namespace editor

namespace editor::id
{
	editor_id get_new(editor_data_type type);

	void delete_id(editor_id id);

	void restore(editor_id id);

	void reset();
}	 // namespace editor::id

namespace editor::style
{
	const ImVec2& frame_padding();
	const ImVec2& item_spacing();
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
}	 // namespace editor::style

namespace editor::platform
{
	inline float dpi_scale() { return GEctx->dpi_scale; }

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

	bool mouse_clicked(ImGuiMouseButton mouse_button = 0);
}	 // namespace editor::platform

namespace editor::widgets
{
	uint32 get_id(const char* str);

	void draw_line(const ImVec2& p1, const ImVec2& p2, ImU32 col, float thickness = 1.0f);
	void draw_rect(const ImVec2& p_min, const ImVec2& p_max, ImU32 col, float rounding = 0.0f, ImDrawFlags flags = 0, float thickness = 1.0f);	  // a: upper-left, b: lower-right (== upper-left + size)
	void draw_rect_filled(const ImRect& rect, ImU32 col, float rounding = 0.0f, ImDrawFlags flags = 0);
	void draw_poly_filled(const ImVec2* points, const int points_count, ImU32 col);
	void draw_text(const ImVec2& pos, ImU32 col, const char* text_begin, const char* text_end = nullptr);
	void draw_text_clipped(const ImVec2& pos_min, const ImVec2& pos_max, const char* text, const char* text_end, const ImVec2* text_size_if_known, const ImVec2& align = ImVec2(0, 0), const ImRect* clip_rect = nullptr);
	void draw_frame(ImVec2 p_min, ImVec2 p_max, ImU32 fill_col, bool border = true, float rounding = 0.0f);


	bool button_behavior(const ImRect& bb, ImGuiID id, bool* out_hovered, bool* out_held, ImGuiButtonFlags flags = 0);
	bool is_item_clicked(ImGuiMouseButton mouse_button = 0);
	bool is_item_hovered(ImGuiHoveredFlags flags = 0);


	bool begin(const char* name, bool* p_open = nullptr, ImGuiWindowFlags flags = 0);
	void end();
	bool begin_child(const char* str_id, const ImVec2& size = ImVec2(0, 0), bool border = false, ImGuiWindowFlags flags = 0);
	void end_child();

	bool begin_popup(const char* str_id, ImGuiWindowFlags flags = 0);

	bool begin_popup_modal(const char* name, bool* p_open, ImGuiWindowFlags flags);
	bool begin_popup_modal(ImGuiID id, const char* name, bool* p_open, ImGuiWindowFlags flags);
	void end_popup();

	void open_popup(const char* str_id, ImGuiPopupFlags popup_flags = 0);

	void close_popup();

	bool menu_item(std::string label, const char* icon, std::string shortcut, bool selected = false, bool enabled = true);
	bool menu_item(const char* label, const char* icon, const char* shortcut, bool selected = false, bool enabled = true);

	bool begin_menu(std::string label, const char* icon, bool enabled = true);
	bool begin_menu(const char* label, const char* icon, bool enabled = true);

	void end_menu();

	bool add_item(const ImRect& bb, ImGuiID id, const ImRect* nav_bb = nullptr, ImGuiItemFlags extra_flags = 0);

	void item_size(const ImVec2& size, float text_baseline_y = -1.0f);

	void set_next_window_pos(const ImVec2& pos, ImGuiCond cond = 0, const ImVec2& pivot = ImVec2(0, 0));
	void set_next_window_size(const ImVec2& size, ImGuiCond cond = 0);
	void set_next_window_viewport(ImGuiID id);

	const ImRect& get_item_rect();

	const ImVec2& get_cursor_pos();
	float		  get_cursor_pos_x();
	float		  get_cursor_pos_y();

	void set_cursor_pos(const ImVec2& local_pos);
	void set_cursor_pos_x(float local_x);
	void set_cursor_pos_y(float local_y);

	void image(ImTextureID user_texture_id, const ImVec2& size, const ImVec2& uv0 = ImVec2(0, 0), const ImVec2& uv1 = ImVec2(1, 1), const ImVec4& tint_col = ImVec4(1, 1, 1, 1), const ImVec4& border_col = ImVec4(0, 0, 0, 0));

	void text(const char* fmt, ...);

	void text_colored(const ImVec4& col, const char* fmt, ...);

	bool selectable(const char* label, bool selected, ImGuiSelectableFlags flags = 0, const ImVec2& size_arg = { 0, 0 }, bool border = false);

	bool selectable(const char* label, bool* p_selected, ImGuiSelectableFlags flags = 0, const ImVec2& size_arg = { 0, 0 }, bool border = false);

	bool selectable(std::string label, bool selected, ImGuiSelectableFlags flags = 0, const ImVec2& size_arg = { 0, 0 }, bool border = false);

	bool selectable(std::string label, bool* p_selected, ImGuiSelectableFlags flags = 0, const ImVec2& size_arg = { 0, 0 }, bool border = false);

	bool button(const char* label, const ImVec2& size = ImVec2(0, 0));

	bool tree_node(std::string label, ImGuiTreeNodeFlags flags = 0);

	void tree_pop();

	void separator();
	void sameline(float offset_from_start_x = 0.0f, float spacing = -1.0f);
	void newline();
	void spacing();

	bool input_text(const char* label, char* buf, size_t buf_size, ImGuiInputTextFlags flags = 0, ImGuiInputTextCallback callback = nullptr, void* user_data = nullptr);
	bool input_text_with_hint(const char* label, const char* hint, char* buf, size_t buf_size, ImGuiInputTextFlags flags = 0, ImGuiInputTextCallback callback = nullptr, void* user_data = nullptr);

	void push_font(ImFont* font);
	void pop_font();

	ImVec2 calc_text_size(const char* text, const char* text_end = nullptr, bool hide_text_after_double_hash = false, float wrap_width = -1.0f);

	void update_progress(int progress, const char* msg);
	void progress_modal(const char* title, std::function<bool()> task, std::function<void(bool)> callback = std::function<void(bool)>());

	void on_frame_end();
}	 // namespace editor::widgets

namespace editor::undoredo
{
	struct undo_redo_cmd
	{
		// Editor::ID::AssembleID Id;
		std::string			  name;
		std::function<void()> undo;
		std::function<void()> redo;
	};

	void add(undo_redo_cmd& undo_redo);
	void on_project_loaded();

	// void Init();
}	 // namespace editor::undoredo

namespace editor::models
{
	using scene_id	   = uint16;
	using world_idx	   = uint16;
	using entity_idx   = uint64;
	using component_id = uint64;
	using archetype_t  = uint64;

	struct project_open_data;
	struct game_project;

	struct em_field;
	struct em_struct;
	struct em_scene;
	struct em_world;
	struct em_subworld;
	struct em_entity;
	struct em_component;
	struct em_system;

	struct field_info
	{
		const char* name;
		const char* type;
		const char* serialized_value;
		size_t		offset;
		// std::string				child_count;
		// std::vector<field_info> childs;
	};

	struct struct_info
	{
		uint64		idx;
		uint64		hash_id;
		const char* name;
		size_t		field_count;
		field_info* fields;
	};

	struct scene_info
	{
		const char* name;
		size_t		world_count;
		size_t		world_idx;
	};

	struct world_info
	{
		const char* name;
		size_t		scene_idx;
		uint64		struct_count;
		uint64		struct_idx_vec[64];
	};

	struct component_info
	{
		uint64 struct_idx;
		size_t scene_idx;
		size_t world_idx;
	};

	struct entity_info
	{
		const char* name;
		uint64		idx;
		uint64		archetype;
	};

	struct em_struct
	{
		editor_id	 id;
		struct_info* p_info;
		std::string	 name;
		uint64		 field_count;

		em_struct(struct_info* p_info) : p_info(p_info), field_count(0) {};
		em_struct() : em_struct(nullptr) {};
	};

	struct em_field
	{
		editor_id	id;
		editor_id	struct_id;
		field_info* p_info;
		std::string name;

		em_field(field_info* p_info) : p_info(p_info) {};
		em_field() : em_field(nullptr) {};
	};

	struct em_scene
	{
		editor_id	id;
		scene_info* p_info;
		std::string name;

		em_scene() : p_info(nullptr) {};
	};

	struct em_world
	{
		editor_id			   id;
		editor_id			   scene_id;
		world_info*			   p_info;
		std::string			   name;
		std::vector<editor_id> structs;

		em_world() : p_info(nullptr) {};

		void inline init(editor_id w_id, editor_id s_id, world_info* p_info)
		{
			id			 = w_id;
			scene_id	 = s_id;
			this->p_info = p_info;
			name		 = p_info->name;
		}
	};

	struct em_subworld
	{
		std::string name;
	};

	struct em_entity
	{
		editor_id			   id;
		editor_id			   world_id;
		std::string			   name;
		uint64				   archetype;
		uint64				   ecs_entity_idx;
		std::vector<editor_id> components;

		void inline init(editor_id e_id, editor_id w_id, entity_info* p_info)
		{
			id			   = e_id;
			world_id	   = w_id;
			name		   = p_info->name;
			archetype	   = p_info->archetype;
			ecs_entity_idx = p_info->idx;
		}
	};

	struct em_component
	{
		editor_id	id;
		editor_id	struct_id;
		editor_id	entity_id;
		std::string name;
	};

	struct em_system
	{
		editor_id	world_id;
		std::string name;
	};

	namespace reflection
	{
		em_struct* find_struct(editor_id id);
		em_struct* find_struct(const char* name);
		editor_id  create_struct();
		void	   remove_struct(editor_id struct_id);

		em_field*			   find_field(editor_id id);
		std::vector<em_field*> find_fields(std::vector<editor_id> struct_id_vec);
		editor_id			   add_field(editor_id struct_id);
		void				   remove_field(editor_id field_id);

		std::vector<em_struct*> all_structs();
		std::vector<em_field*>	all_fields(editor_id struct_id);
	};	  // namespace reflection

	namespace scene
	{
		extern editor_command cmd_create;
		extern editor_command cmd_remove;
		extern editor_command cmd_set_current;
		// editor_id => idx => scene_key_list
		//
		// remove => increase generation => different id
		em_scene*			   find(editor_id id);
		editor_id			   create();
		void				   remove(editor_id id);
		size_t				   count();
		std::vector<em_scene*> all();
		void				   set_current(editor_id id);

		em_scene* get_current();
	}	 // namespace scene

	namespace world
	{
		extern editor_command cmd_create;
		extern editor_command cmd_remove;

		em_world*			   find(editor_id id);
		editor_id			   create(editor_id scene_id);
		void				   add_struct(editor_id world_id, editor_id struct_id);
		void				   remove_struct(editor_id world_id, editor_id struct_id);
		std::vector<em_world*> all(editor_id scene_id);
	}	 // namespace world

	namespace entity
	{
		extern editor_command cmd_create;
		extern editor_command cmd_remove;

		em_entity*				find(editor_id id);
		editor_id				create(editor_id entity_id);
		std::vector<em_entity*> all(editor_id world_id);

		void update();
		// 에디터에서 ecs의 무언가를 변경했을때
		// 1. 단순 수치변경 (ex. entity component's value change) : get entity idx and access to p_momory directly
		// 2. sturctural change (ex. add new component to entity or world) : rebuild?
		// 3. how to render from editor
		//	a. render after game play : build => run game loop
		//	b. render before game play : ???
		//		ii) 서로 다른 world중 어떤 world는 서로 다른 render pipeline의 대상이 된다 => editor에서 어떻게 이를 알수 있지? => serialize scene => add new macro (world type alias)
		//		xi) 아니면 그냥 각 scene마다 phase를 정해두고 그중 render phase에 실행되는 모든 pipe들은 render pipeline이라고 치는 것은?
		//		play mode에서는 update - render ... 모두 실행하는데
		//		edit mode에서는 render만 실행하는거지
		//		즉 각 phase와 pipe들을 연결하는것 또한 serialized되야한다.
		//		나중에 scene 정의시  scene_builder<phase_builder, world_builder> 이렇게 하고 그 결과로
		//		static inline auto& s = scene<w1, w2, w3...>를 생성하게 하는것은 가능할것 같음
		//		phase는 pipe인가 function인가
		//
		//		scene<world_group<w1, w2, w3...>, phase_group<bind<p1,w1> ,<p2,w2>,<p3,w3>...>>
		//
		//
		// system = system<seq<f1,f2>,par<f1,f2>, cond<f1,f2>, ...>(); //do not need scene
		// pipeline = p_seq<p_seq<s,0,1,2>,p_par<s2,0,1>,...>          //backed as a scene template parameter, value (funciton pointer)
		//
		//
		// system :
		// s_seq<param> param : any func(entity_idx, component&...) or s_seq or s_par or s_cond or...
		// s_seq<f1> (o)
		// s_seq<f1,s_seq<f2,f3>> => s_seq<f1,f2,f3>
		//
		// s_seq<f1,s_par<f2,f3>> => ???
		//
		// system().update(world) =>
		//
		// pipeline
		//
		// p_seq<s,0,1,2> => system().update(scene.get_world<0>()); ...
		// p_par<s,0,1,2> => ...
		//
		//
		// let's define loop
		//
		// game => graphic
		//										(same result)
		// game_mode => game => graphic				|=> write some data to p_memory(not sure what)
		// edit_mode => game(empty) => graphic      |=> write some data to p_memory
		//
		// scene::game_pipeline = p_seq<p_seq<s,0,1,2>, p_par<s, 0,1,2>, ...
		// scene::graphic_pipeline = p_seq<graphic_system, 0,1,2,3,...>
		//
		// engine => while(!done){current_scene::game_pipeline(); current_scene::graphic_pipeline();} ...
		//
		//
	}	 // namespace entity

	namespace component
	{

	}	 // namespace component

	// namespace entity
	//{
	//	// extern const Editor_Command Cmd_Add_New;
	//	// extern const Editor_Command Cmd_Set_Active;
	//	// extern const Editor_Command Cmd_Remove;
	//	extern const editor_command cmd_add_component;

	//	editor_id create(editor_id parent_id);
	//	void	  remove(editor_id entity_id);
	//	void	  add_component(editor_id entity_id, editor_id component_id);

	//	em_entity& get(editor_id id);

	//	bool					is_alive(editor_id id);
	//	std::vector<em_entity>& all();
	//}	 // namespace entity

	// namespace component
	//{
	//	// extern const Editor_Command Cmd_Add_New;
	//	// extern const Editor_Command Cmd_Set_Active;
	//	// extern const Editor_Command Cmd_Remove;
	//	bool					   is_valid(editor_id component_id);
	//	em_component*			   find(editor_id id);
	//	std::vector<em_component>& all();
	// }	 // namespace component

	// namespace system
	//{
	//	// extern const Editor_Command Cmd_Add_New;
	//	// extern const Editor_Command Cmd_Set_Active;
	//	// extern const Editor_Command Cmd_Remove;

	//	em_system* find(editor_id id);
	//}	 // namespace system

	// void register_component(pod::component_info* p_info);
	void update();
	void on_project_loaded();
	void on_project_unloaded();
}	 // namespace editor::models

namespace editor::utilities
{
	std::string read_file(const std::filesystem::path path);
	void		create_file(const std::filesystem::path path, const std::string content);
}	 // namespace editor::utilities

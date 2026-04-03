#pragma once
#include "age.hpp"

namespace age::ui::e
{
	AGE_DEFINE_ENUM(shape_kind, uint8,
					rect,
					circle,
					arrow_right,
					text);

	AGE_DEFINE_ENUM(brush_kind, uint8,
					color);

	AGE_DEFINE_ENUM(widget_layout, uint8, horizontal, vertical)

	AGE_DEFINE_ENUM(widget_overflow, uint8, draw_all, discard, clip, scroll)

	AGE_DEFINE_ENUM(widget_align, uint8, begin, center, end)

	AGE_DEFINE_ENUM(size_mode_kind, uint8, fixed, fit, grow, grow_weight, text, aspect_ratio)

	AGE_DEFINE_ENUM(style_state, uint8, idle, hover, active)

	AGE_DEFINE_ENUM(font_size_kind, uint8, small, normal, big)
}	 // namespace age::ui::e

namespace age::ui
{
	using t_hash = uint64;

	struct id_scope
	{
		t_hash hash_id;
		uint32 counter;
		uint32 _;
	};
}	 // namespace age::ui

namespace age::ui
{
	struct ui_shape_data
	{
		union
		{
			struct
			{
				float2 atlas_uv_min;
				float2 atlas_uv_max;
				uint32 atlas_id;
			} text;

			uint32 data[5];
		};

		// TBD, corner radius, circle radius, ...
	};

	struct ui_brush_data
	{
		uint32_4 data;											   // TBD, texture_id, static_color, calculate from shadow with delta time...
	};

	struct render_data
	{
		float2 pivot_pos;										   // screen pos of pivot
		float2 pivot_uv = float2{ 0.5f, 0.5f };
		float2 size;											   // pixel size
		float  rotation			= 0.f;							   // z rotation, radian
		float  border_thickness = 0.f;

		float4 clip_rect;

		e::shape_kind shape_kind		= e::shape_kind::rect;	   // 1. rect, 2. rounded_rect, 3. circle, 4. star??? ...
		e::brush_kind body_brush_kind	= e::brush_kind::color;	   // 1. texture_id, 2. color, 3. generated from uv, ...
		e::brush_kind border_brush_kind = e::brush_kind::color;	   // 1. texture_id, 2. color, 3. generated from uv, ...

		uint8 _;

		ui_shape_data shape_data;
		ui_brush_data body_brush_data;
		ui_brush_data border_brush_data;
	};

	struct widget_size_mode
	{
		float min = 0.f;
		float max = std::numeric_limits<float>::max();

		e::size_mode_kind size_mode = e::size_mode_kind::grow;

		std::array<uint8, 3> _;
	};

	struct widget_desc
	{
		bool			   draw		= true;
		e::widget_layout   layout	= e::widget_layout::horizontal;
		e::widget_overflow overflow = e::widget_overflow::draw_all;
		e::widget_align	   align	= e::widget_align::begin;

		float width_min = 0.f;
		float width_max = std::numeric_limits<float>::max();

		float height_min = 0.f;
		float height_max = std::numeric_limits<float>::max();

		e::size_mode_kind width_size_mode  = e::size_mode_kind::grow;
		e::size_mode_kind height_size_mode = e::size_mode_kind::grow;

		uint16 z_offset = 1;

		float2 offset = float2{ 0, 0 };

		float child_gap = 4.f;

		float padding_left	 = 4.f;
		float padding_right	 = 4.f;
		float padding_top	 = 4.f;
		float padding_bottom = 4.f;

		float2 pivot_uv			= float2{ 0.5f, 0.5f };
		float  rotation			= 0.f;
		float  border_thickness = 1.f;


		ui_shape_data shape_data;
		ui_brush_data body_brush_data;
		ui_brush_data border_brush_data;

		e::shape_kind shape_kind		= e::shape_kind::rect;
		e::brush_kind body_brush_kind	= e::brush_kind::color;
		e::brush_kind border_brush_kind = e::brush_kind::color;

		bool  interact = false;
		uint8 _[4];

		union
		{
			struct
			{
				uint32 text_data_idx;
				uint32 font_idx;

				const char* p_str;
				float		font_size;
			} text;

			uint64 extra;
		};
	};
}	 // namespace age::ui

namespace age::ui
{
	// data that needs to preserve between frames
	struct element_state
	{
		float2 size;
		float2 pivot_pos;
		float2 pivot_uv;
		float  rotation;
	};

	struct layout_data
	{
		e::widget_layout  layout;
		e::size_mode_kind mode;

		uint8_2 _;
		uint32	pos_data_idx;
		uint32	grow_child_count;

		float size;

		float size_min;
		float size_max;
	};

	struct layout_data_common
	{
		uint32 child_count;
		uint32 parent_h_idx;
		uint32 parent_v_idx;
		float  padding_sum;
		float  child_gap;
		uint16 z_offset;
		bool   child_height_depends_on_width_solved;
		uint8  _;
	};

	struct layout_pos_data
	{
		t_hash id;
		uint32 render_data_idx;
		uint32 render_data_count;

		e::widget_layout layout;
		e::widget_align	 align;
		uint16			 z_offset;
		uint32			 child_count;
		float2			 offset;
		float			 width;
		float			 height;
		float			 child_gap;
		float			 padding_left;
		float			 padding_right;
		float			 padding_top;
		float			 padding_bottom;
		float4			 clip_rect;	   // rect_min, rect_max

		bool	interact;
		uint8_3 _;

		union
		{
			struct
			{
				uint32 idx;	   // 1. text_data_idx 2. char_pos_data_idx
				uint32 atlas_id;
			} text;

			uint64 extra;
		};

		template <bool is_width>
		FORCE_INLINE void
		set_size(float size) noexcept
		{
			if constexpr (is_width)
			{
				width = size;
			}
			else
			{
				height = size;
			}
		}
	};
}	 // namespace age::ui

// text
namespace age::ui
{
	struct text_data
	{
		uint32 font_idx;
		uint32 char_data_offset;
		uint32 char_data_count;
		uint32 word_data_offset;
		uint32 word_data_count;

		float line_height;
		float space_advance;
	};

	struct word_data
	{
		uint32 char_count;
		float  width;
		float  leading_space;
		uint32 line_offset;
	};

	struct char_data
	{
		float  advance;
		float2 offset;
		float2 size;
		float2 atlas_uv_min;
		float2 atlas_uv_max;
	};

	struct char_pos_data
	{
		float2 offset;
		float2 size;
		float2 atlas_uv_min;
		float2 atlas_uv_max;
	};

	struct font_data
	{
		uint32		  atlas_id;
		asset::handle h_font;
	};
}	 // namespace age::ui

namespace age::ui::g
{
	inline constexpr uint64 fnv1a_offset_basis = 0xcbf29ce484222325ull;
	inline constexpr uint64 fnv1a_prime		   = 0x100000001b3ull;

	inline float window_width;
	inline float window_height;

	inline const age::input::input_context* p_input_ctx;
	inline t_hash							hover_id;
	inline t_hash							mouse_l_pressed_id;
	inline t_hash							mouse_l_clicked_id;
	inline t_hash							mouse_r_clicked_id;

	inline age::vector<id_scope> id_stack;

	inline age::unordered_map<uint64, element_state> element_state_map;

	// layout stack
	inline age::vector<layout_data>		   element_layout_data_h_stack;
	inline age::vector<layout_data>		   element_layout_data_v_stack;
	inline age::vector<layout_data_common> element_layout_data_common_stack;

	// layout vec
	inline age::vector<layout_pos_data> element_layout_pos_data_vec;
	inline age::vector<render_data>		element_render_data_vec;
	inline age::vector<uint32>			element_z_order_count_vec;

	// layout text
	inline age::vector<text_data>	  text_data_vec;
	inline age::vector<word_data>	  word_data_vec;
	inline age::vector<char_data>	  char_data_vec;
	inline age::vector<char_pos_data> char_pos_data_vec;

	// scratch
	inline age::vector<uint64> element_layout_grow_event_vec;
	inline age::vector<uint32> element_pos_parent_idx_stack;

	inline uint32 layout_h_current_idx;
	inline uint32 layout_v_current_idx;

	// font
	inline age::vector<std::pair<t_hash, font_data>> font_data_vec;
	inline uint32									 current_font_idx;

	// theme
	inline float  theme_opacity[9];
	inline float3 theme_color[5];

	inline float theme_font_size_base[e::size<e::font_size_kind>()];	// unscaled, don't use
	inline float theme_font_size[e::size<e::font_size_kind>()];			// scaled
	inline float theme_font_scale;

	// theme configs
	inline constexpr float theme_opacity_default[9] = {
		0.00f,						   // 0
		0.05f,						   // 1
		0.12f,						   // 2
		0.22f,						   // 3
		0.35f,						   // 4
		0.50f,						   // 5
		0.68f,						   // 6
		0.82f,						   // 7
		1.00f,						   // 8 (opaque)
	};

	inline constexpr float3 theme_color_default[5] = {
		{ 1.0f, 1.0f, 1.0f },		   // 0: white
		{ 0.0f, 0.0f, 0.0f },		   // 1: black
		{ 0.29f, 0.565f, 0.851f },	   // 2: accent   // #4A90D9
		{ 0.314f, 0.784f, 0.471f },	   // 3: positive // #50C878
		{ 0.878f, 0.314f, 0.314f },	   // 4: negative // #E05050
	};

	inline constexpr float theme_font_size_defaults[e::size<e::font_size_kind>()] = {
		11.f,						   // 0: small, hint, label, info
		13.f,						   // 1: normal
		16.f,						   // 2: big, header, panel title
	};

	inline constexpr float theme_font_scale_default = 1.f;

	inline constexpr uint8 opacity_0	  = 0;
	inline constexpr uint8 opacity_1	  = 1;
	inline constexpr uint8 opacity_2	  = 2;
	inline constexpr uint8 opacity_3	  = 3;
	inline constexpr uint8 opacity_4	  = 4;
	inline constexpr uint8 opacity_5	  = 5;
	inline constexpr uint8 opacity_6	  = 6;
	inline constexpr uint8 opacity_7	  = 7;
	inline constexpr uint8 opacity_opaque = 8;

	inline constexpr uint8 white	= 0;
	inline constexpr uint8 black	= 1;
	inline constexpr uint8 accent	= 2;
	inline constexpr uint8 positive = 3;
	inline constexpr uint8 negative = 4;

	struct style_color
	{
		uint8 color;
		uint8 opacity[3];
	};

	struct theme_text
	{
		uint8			  color;
		uint8			  opacity[3];
		e::font_size_kind font_size;
	};

	// shared
	inline constexpr style_color bg_panel		= { black, { opacity_5, opacity_6, opacity_6 } };										   // panel background
	inline constexpr style_color bg_surface		= { white, { opacity_1, opacity_1, opacity_1 } };										   // input field, dropdown bg
	inline constexpr style_color bg_interactive = { white, { opacity_0, opacity_1, opacity_1 } };										   // button, list item bg
	inline constexpr style_color bg_accent		= { accent, { opacity_0, opacity_2, opacity_3 } };										   // active button, selected item bg
	inline constexpr style_color bg_popup		= { black, { opacity_7, opacity_7, opacity_7 } };										   // dropdown menu, tooltip, context menu

	inline constexpr style_color border_default = { white, { opacity_1, opacity_2, opacity_2 } };										   // panel, input border
	inline constexpr style_color border_accent	= { accent, { opacity_0, opacity_5, opacity_5 } };										   // focused input, focused panel border

	inline constexpr theme_text text_primary	 = { white, { opacity_7, opacity_7, opacity_7 }, e::font_size_kind::big };				   // heading, selected item text
	inline constexpr theme_text text_secondary	 = { white, { opacity_5, opacity_5, opacity_5 }, e::font_size_kind::normal };			   // input value, normal body
	inline constexpr theme_text text_tertiary	 = { white, { opacity_4, opacity_4, opacity_4 }, e::font_size_kind::normal };			   // section header, label
	inline constexpr theme_text text_hint		 = { white, { opacity_3, opacity_3, opacity_3 }, e::font_size_kind::small };			   // input label, placeholder
	inline constexpr theme_text text_disabled	 = { white, { opacity_2, opacity_2, opacity_2 }, e::font_size_kind::normal };			   // disabled widget text
	inline constexpr theme_text text_accent		 = { accent, { opacity_7, opacity_7, opacity_7 }, e::font_size_kind::normal };			   // link, asset reference, clickable path
	inline constexpr theme_text text_positive	 = { positive, { opacity_7, opacity_7, opacity_7 }, e::font_size_kind::normal };		   // success msg, y axis, fps ok
	inline constexpr theme_text text_negative	 = { negative, { opacity_7, opacity_7, opacity_7 }, e::font_size_kind::normal };		   // error msg, x axis, warning
	inline constexpr theme_text text_interactive = { white, { opacity_opaque, opacity_opaque, opacity_7 }, e::font_size_kind::normal };	   // button text, list item text

	inline constexpr style_color separator = { white, { opacity_1, opacity_1, opacity_1 } };											   // section separator

	// widget_specific
	inline constexpr style_color toggle_off = { white, { opacity_2, opacity_2, opacity_2 } };		 // toggle track (off)
	inline constexpr style_color toggle_on	= { accent, { opacity_5, opacity_6, opacity_6 } };		 // toggle track (on)

	inline constexpr style_color slider_fill = { accent, { opacity_5, opacity_6, opacity_7 } };		 // slider filled portion

	inline constexpr style_color scroll_thumb = { white, { opacity_2, opacity_2, opacity_3 } };		 // scrollbar thumb

	inline constexpr style_color select_accent = { accent, { opacity_0, opacity_0, opacity_7 } };	 // selected item left border
}	 // namespace age::ui::g

namespace age::ui
{
	struct id_ctx
	{
		t_hash hash_id;

		FORCE_INLINE constexpr explicit
		operator bool() const
		{
			return true;
		}

		FORCE_INLINE ~id_ctx() noexcept
		{
			g::id_stack.pop_back();
		}
	};
}	 // namespace age::ui

namespace age::ui
{
	struct widget_ctx
	{
		t_hash hash_id = age::get_invalid_id<t_hash>();

		AGE_DISABLE_COPY(widget_ctx);

		FORCE_INLINE constexpr widget_ctx() noexcept = default;

		FORCE_INLINE constexpr widget_ctx(t_hash id) noexcept
			: hash_id(id){};

		FORCE_INLINE constexpr widget_ctx(widget_ctx&& other) noexcept
			: hash_id(other.hash_id)
		{
			other.hash_id = get_invalid_id<t_hash>();
		}

		FORCE_INLINE constexpr explicit
		operator bool() const
		{
			return hash_id != age::get_invalid_id<t_hash>();
		}

		~widget_ctx() noexcept;

		FORCE_INLINE bool
		hovered() noexcept
		{
			return hash_id == g::hover_id;
		}

		FORCE_INLINE bool
		mouse_l_pressed() noexcept
		{
			return hash_id == g::mouse_l_pressed_id;
		}

		FORCE_INLINE bool
		mouse_l_clicked() noexcept
		{
			return hash_id == g::mouse_l_clicked_id;
		}

		FORCE_INLINE bool
		mouse_r_clicked() noexcept
		{
			return hash_id == g::mouse_r_clicked_id;
		}

		template <input::e::key_kind e_key>
		FORCE_INLINE bool
		clicked() noexcept
		{
			if constexpr (e_key == input::e::key_kind::mouse_left)
			{
				return mouse_l_clicked();
			}
			else if constexpr (e_key == input::e::key_kind::mouse_right)
			{
				return mouse_r_clicked();
			}
			else
			{
				static_assert(false, "invalid key kind");
			}
		}

		template <input::e::key_kind e_key>
		FORCE_INLINE bool
		pressed() noexcept
		{
			if constexpr (e_key == input::e::key_kind::mouse_left)
			{
				return mouse_l_pressed();
			}
			else
			{
				static_assert(false, "invalid key kind");
			}
		}
	};
}	 // namespace age::ui
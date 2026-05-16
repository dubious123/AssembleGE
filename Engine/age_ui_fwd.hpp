#pragma once
#include "age.hpp"

namespace age::ui::e
{
	AGE_DEFINE_ENUM(shape_kind, uint8,
					rect,
					circle,
					arrow_right,
					text,
					check,
					rounded_rect,
					triangle,
					cross);

	AGE_DEFINE_ENUM(brush_kind, uint8,
					color);

	AGE_DEFINE_ENUM(widget_layout, uint8, horizontal, horizontal_inv, vertical, vertical_inv)

	AGE_DEFINE_ENUM(widget_overflow, uint8, draw_all, discard, clip, scroll)

	AGE_DEFINE_ENUM(widget_align, uint8, begin, center, end)

	AGE_DEFINE_ENUM(size_mode_kind, uint8, fixed, fit, grow, grow_weight, text, aspect_ratio)

	AGE_DEFINE_ENUM(style_state, uint8, idle, hover, active)

	AGE_DEFINE_ENUM(space_mode_kind, uint8, screen, world, world_always_on_top, world_billboard);
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

			struct
			{
				float value;
			} roundness;

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

		float child_gap = 3.f;

		float padding_left	 = 3.f;
		float padding_right	 = 3.f;
		float padding_top	 = 3.f;
		float padding_bottom = 3.f;

		float2 pivot_uv			= float2{ 0.5f, 0.5f };
		float  rotation			= 0.f;
		float  border_thickness = 1.f;


		ui_shape_data shape_data;
		ui_brush_data body_brush_data;
		ui_brush_data border_brush_data;

		e::shape_kind shape_kind		= e::shape_kind::rect;
		e::brush_kind body_brush_kind	= e::brush_kind::color;
		e::brush_kind border_brush_kind = e::brush_kind::color;

		bool	interact   = false;
		bool	save_state = false;
		uint8_3 _;

		union
		{
			struct
			{
				uint32 text_data_idx = age::get_invalid_id<uint32>();
				uint32 font_idx;

				const char* p_str;
				float		font_size;
			} text;

			uint64 extra;
		};

		FORCE_INLINE static widget_desc
		apply(auto&& mod) noexcept
		{
			auto res = widget_desc{};
			FWD(mod).apply(res);
			return res;
		}
	};

	struct root_desc
	{
		e::space_mode_kind space_mode = e::space_mode_kind::screen;
		e::widget_layout   layout	  = e::widget_layout::vertical;

		uint16 _;

		float width;
		float height;

		float3 world_pos;
		float4 quaternion;

		float world_width;
		float world_height;
	};
}	 // namespace age::ui

namespace age::ui
{
	// data that needs to preserve between frames
	struct widget_state
	{
		float2 pos;	   // final pos, (layout pos + offset)

		float width	 = 0.f;
		float height = 0.f;

		float  clip_width  = 0.f;
		float  clip_height = 0.f;
		uint16 frame_count;

		bool toggled;

		uint8 _;

		float drag_x;
		float drag_y;
		float drag_z;

		union
		{
			struct
			{
				float offset_x;
				float offset_y;

				uint32 byte_pos;

				float  anchor_offset_x;
				float  anchor_offset_y;
				uint32 anchor_byte_pos;
			} cursor;

			struct
			{
				float3 euler;
				float4 quat;
			} rotation_field;

			struct
			{
				age::util::bitset<32 * 6> selected;
			} drop_down_data;
		};
	};

	struct layout_size_data
	{
		e::widget_layout  layout;
		e::size_mode_kind width_mode;
		e::size_mode_kind height_mode;

		uint8 _0;

		uint32 child_subtree_size;

		uint32 parent_idx;

		uint32 pos_data_idx;

		float child_gap;

		float width_min;
		float width_max;
		float width_final;

		float height_min;
		float height_max;
		float height_final;

		template <bool is_width>
		FORCE_INLINE e::size_mode_kind
		size_mode() noexcept
		{
			if constexpr (is_width)
			{
				return width_mode;
			}
			else
			{
				return height_mode;
			}
		}

		template <bool is_width>
		FORCE_INLINE float&
		size_min() noexcept
		{
			if constexpr (is_width)
			{
				return width_min;
			}
			else
			{
				return height_min;
			}
		}

		template <bool is_width>
		FORCE_INLINE float&
		size_max() noexcept
		{
			if constexpr (is_width)
			{
				return width_max;
			}
			else
			{
				return height_max;
			}
		}

		template <bool is_width>
		FORCE_INLINE float&
		size_final() noexcept
		{
			if constexpr (is_width)
			{
				return width_final;
			}
			else
			{
				return height_final;
			}
		}
	};

	struct layout_pos_data
	{
		t_hash id;
		uint32 render_data_idx;
		uint32 render_data_count;

		e::widget_layout layout;
		e::widget_align	 align;
		uint16			 z_offset;
		int32			 child_count;
		float2			 offset;
		float			 width;
		float			 height;
		float			 child_gap;
		float			 padding_left;
		float			 padding_right;
		float			 padding_top;
		float			 padding_bottom;
		float4			 clip_rect;	   // rect_min, rect_max

		bool  interact;
		bool  save_state;
		bool  direct_draw;
		uint8 _;

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

	struct root_data
	{
		float3 world_pos;
		float4 quaternion;

		float width;
		float height;

		float world_width;
		float world_height;

		float2 mouse_uv;		  // projected, pixel
		float2 mouse_delta_uv;	  // projected, pixel

		age::vector<layout_pos_data> layout_pos_data_vec;
		age::vector<uint32>			 z_order_count_vec;
	};

	struct root_graphics_data
	{
		float3 world_pos;
		float4 quaternion;

		float width;
		float height;

		float world_width;
		float world_height;
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
		uint32 leading_space_count;
		uint32 line_offset;
		uint32 byte_offset;
		uint32 byte_size;
	};

	struct char_data
	{
		float  advance;
		float2 offset;
		float2 size;
		float2 atlas_uv_min;
		float2 atlas_uv_max;
		uint8  byte_size;	 // 1,2,3,4
	};

	struct char_pos_data
	{
		float2 offset;
		float2 size;
		float2 atlas_uv_min;
		float2 atlas_uv_max;
	};

	struct cursor_data
	{
		float2 offset;
		float  word_min_x;
		float  word_max_x;
		float  line_width;

		uint32 byte_offset;
		uint32 word_byte_offset;
		uint32 word_byte_size;
		uint32 line_byte_offset;
		uint32 line_byte_size;

		uint32 anchor_byte_offset;
	};

	struct font_data
	{
		asset::handle h_font;
		uint32		  atlas_id;
	};
}	 // namespace age::ui

namespace age::ui::g
{
	inline constexpr uint64 fnv1a_offset_basis = 0xcbf29ce484222325ull;
	inline constexpr uint64 fnv1a_prime		   = 0x100000001b3ull;

	inline float window_width;
	inline float window_height;

	inline float3 cam_world_pos_prev;
	inline float3 mouse_ray_dir_prev;	 // world space
	inline float3 cam_world_pos;
	inline float3 mouse_ray_dir;		 // world space

	inline const age::input::input_context* p_input_ctx;
	inline t_hash							hover_id;
	inline t_hash							focus_id;
	inline t_hash							mouse_l_pressed_id;
	inline t_hash							mouse_l_clicked_id;
	inline t_hash							mouse_r_clicked_id;

	inline age::vector<t_hash> hover_id_stack;
	inline age::vector<t_hash> focus_id_stack;

	inline float mouse_l_clicked_time;
	inline uint8 mouse_l_clicked_count;

	inline age::vector<id_scope> id_stack;

	inline age::unordered_map<uint64, widget_state> widget_state_map;

	inline std::array<age::vector<root_data>, e::space_mode_kind_size> root_data_vec_arr;
	inline std::array<uint32, e::space_mode_kind_size>				   root_data_vec_size_arr;
	inline age::vector<uint32>										   root_data_idx_stack;	   // [space_mode_kind(8)][idx(24)]

	// layout stack
	inline age::vector<layout_size_data> layout_size_data_stack;
	inline uint32						 layout_size_data_current_idx;

	// layout vec
	inline age::vector<render_data> render_data_vec;

	// layout text
	inline age::vector<text_data>	  text_data_vec;
	inline age::vector<word_data>	  word_data_vec;
	inline age::vector<char_data>	  char_data_vec;
	inline age::vector<char_pos_data> char_pos_data_vec;

	// scratch
	inline age::vector<uint64> element_layout_grow_event_vec;
	inline age::vector<uint32> element_pos_parent_idx_stack;

	// font
	inline age::vector<std::pair<t_hash, font_data>> font_data_vec;
	inline uint32									 current_font_idx;

	inline char	  utf8_buf[16 * 4 + 1];
	inline uint32 utf8_buf_len = 0;

	inline char numeric_field_text_edit_buf[16 * 4 + 1];

	// numeric step scale
	inline constexpr float step_scale_table[2][2] = { { 1.0f, 0.1f },
													  { 10.f, 0.01f } };
}	 // namespace age::ui::g

namespace age::ui
{
#define AGE_THEME_FOREACH_FUNC(tpl) \
	AGE_THEME_PRIMITIVE_MEMBER tpl  \
		AGE_THEME_PREIMTIVE_FUNC tpl

#define AGE_THEME_PRIMITIVE_MEMBER(type, name, value) \
	namespace g                                       \
	{                                                 \
		inline type theme_##name = value;             \
	}

#define AGE_THEME_PREIMTIVE_FUNC(type, name, value) \
	namespace theme                                 \
	{                                               \
		FORCE_INLINE constexpr type                 \
		name() noexcept                             \
		{                                           \
			return ui::g::theme_##name;             \
		}                                           \
	}

#define AGE_THEME_WIDGET(widget_name, semantic, value_expr) \
	namespace theme                                         \
	{                                                       \
		FORCE_INLINE decltype(auto)                         \
		widget_name##_##semantic() noexcept                 \
		{                                                   \
			return value_expr;                              \
		}                                                   \
	}

	FOR_EACH (AGE_THEME_FOREACH_FUNC,
			  // main colors
			  (float3, color_white, age::srgb_to_linear(float3{ 1.000f, 1.000f, 1.000f })),	   // #FFFFFF
			  (float3, color_black, age::srgb_to_linear(float3{ 0.000f, 0.000f, 0.000f })),	   // #000000
			  (float3, color_red, age::srgb_to_linear(float3{ 0.878f, 0.314f, 0.314f })),	   // #E05050 x-axis, negative
			  (float3, color_green, age::srgb_to_linear(float3{ 0.314f, 0.784f, 0.471f })),	   // #50C878 y-axis, positive
			  //(float3, color_blue_dark, age::srgb_to_linear( float3{0.227f, 0.502f, 0.788f})),	   // #3A80C9
			  (float3, color_blue, age::srgb_to_linear(float3{ 0.290f, 0.565f, 0.851f })),	  // #4A90D9 z-axis, accent
			  //(float3, color_blue_light, age::srgb_to_linear( float3{0.353f, 0.624f, 0.910f})),	   // #5A9FE8

			  (float3, color_blue_dark, age::srgb_to_linear(float3{ 0.102f, 0.282f, 0.471f })),	   // #1A4878
			  //(float3, color_blue, age::srgb_to_linear(float3{0.157f, 0.369f, 0.596f})),		   // #285E98 z-axis, accent
			  (float3, color_blue_light, age::srgb_to_linear(float3{ 0.220f, 0.471f, 0.722f })),	// #3878B8

			  //(float3, color_blue, age::srgb_to_linear(float3{0.290f, 0.565f, 0.851f})),			 // #4A90D9 z-axis, accent


			  (float3, color_amber, age::srgb_to_linear(float3{ 0.886f, 0.680f, 0.216f })),			  // #E2AD37 w-axis, warning
			  (float3, color_gray_darkest, age::srgb_to_linear(float3{ 0.067f, 0.067f, 0.075f })),	  // #111114 field bg
			  (float3, color_gray_darker, age::srgb_to_linear(float3{ 0.102f, 0.102f, 0.118f })),	  // #1A1A1E	panel bg
			  (float3, color_gray_dark, age::srgb_to_linear(float3{ 0.118f, 0.118f, 0.133f })),		  // #1E1E22  header idle
			  (float3, color_gray, age::srgb_to_linear(float3{ 0.145f, 0.145f, 0.161f })),			  // #252529  header hover
			  (float3, color_gray_light, age::srgb_to_linear(float3{ 0.165f, 0.165f, 0.180f })),	  // #2A2A2E  header active

																									  // text colors
			  (float3, color_text_white, age::srgb_to_linear(float3{ 1.000f, 1.000f, 1.000f })),		 // heading,
			  (float3, color_text_gray_light, age::srgb_to_linear(float3{ 0.667f, 0.667f, 0.667f })),	 // body text
			  (float3, color_text_gray, age::srgb_to_linear(float3{ 0.467f, 0.467f, 0.467f })),			 // label
			  (float3, color_text_gray_dark, age::srgb_to_linear(float3{ 0.333f, 0.333f, 0.333f })),	 // hint, placeholder
			  (float3, color_text_red, age::srgb_to_linear(float3{ 0.878f, 0.314f, 0.314f })),			 // x-axis, negative
			  (float3, color_text_green, age::srgb_to_linear(float3{ 0.314f, 0.784f, 0.471f })),		 // y-axis, positive
			  (float3, color_text_blue, age::srgb_to_linear(float3{ 0.290f, 0.565f, 0.851f })),			 // z-axis, accent
			  (float3, color_text_amber, age::srgb_to_linear(float3{ 0.886f, 0.680f, 0.216f })),		 // w-axis, warning

			  // mid band - sat ~65%, lightness ~52%
			  (float3, palette_red, age::srgb_to_linear(float3{ 0.832f, 0.208f, 0.208f })),			   // #D43535
			  (float3, palette_vermilion, age::srgb_to_linear(float3{ 0.832f, 0.325f, 0.208f })),	   // #D45235
			  (float3, palette_orange, age::srgb_to_linear(float3{ 0.832f, 0.442f, 0.208f })),		   // #D47035
			  (float3, palette_tangerine, age::srgb_to_linear(float3{ 0.832f, 0.559f, 0.208f })),	   // #D48E35
			  (float3, palette_amber, age::srgb_to_linear(float3{ 0.832f, 0.676f, 0.208f })),		   // #D4AC35
			  (float3, palette_gold, age::srgb_to_linear(float3{ 0.832f, 0.793f, 0.208f })),		   // #D4CA35
			  (float3, palette_yellow, age::srgb_to_linear(float3{ 0.754f, 0.832f, 0.208f })),		   // #C0D435
			  (float3, palette_pear, age::srgb_to_linear(float3{ 0.637f, 0.832f, 0.208f })),		   // #A2D435
			  (float3, palette_chartreuse, age::srgb_to_linear(float3{ 0.520f, 0.832f, 0.208f })),	   // #84D435
			  (float3, palette_lime, age::srgb_to_linear(float3{ 0.403f, 0.832f, 0.208f })),		   // #66D435
			  (float3, palette_harlequin, age::srgb_to_linear(float3{ 0.286f, 0.832f, 0.208f })),	   // #48D435
			  (float3, palette_green, age::srgb_to_linear(float3{ 0.208f, 0.832f, 0.247f })),		   // #35D43E
			  (float3, palette_emerald, age::srgb_to_linear(float3{ 0.208f, 0.832f, 0.364f })),		   // #35D45C
			  (float3, palette_spring, age::srgb_to_linear(float3{ 0.208f, 0.832f, 0.481f })),		   // #35D47A
			  (float3, palette_jade, age::srgb_to_linear(float3{ 0.208f, 0.832f, 0.598f })),		   // #35D498
			  (float3, palette_mint, age::srgb_to_linear(float3{ 0.208f, 0.832f, 0.715f })),		   // #35D4B6
			  (float3, palette_cyan, age::srgb_to_linear(float3{ 0.208f, 0.832f, 0.832f })),		   // #35D4D4
			  (float3, palette_sky, age::srgb_to_linear(float3{ 0.208f, 0.715f, 0.832f })),			   // #35B6D4
			  (float3, palette_cerulean, age::srgb_to_linear(float3{ 0.208f, 0.598f, 0.832f })),	   // #3598D4
			  (float3, palette_azure, age::srgb_to_linear(float3{ 0.208f, 0.481f, 0.832f })),		   // #357AD4
			  (float3, palette_blue, age::srgb_to_linear(float3{ 0.208f, 0.364f, 0.832f })),		   // #355CD4
			  (float3, palette_cobalt, age::srgb_to_linear(float3{ 0.208f, 0.247f, 0.832f })),		   // #353ED4
			  (float3, palette_ultramarine, age::srgb_to_linear(float3{ 0.286f, 0.208f, 0.832f })),	   // #4835D4
			  (float3, palette_indigo, age::srgb_to_linear(float3{ 0.403f, 0.208f, 0.832f })),		   // #6635D4
			  (float3, palette_violet, age::srgb_to_linear(float3{ 0.520f, 0.208f, 0.832f })),		   // #8435D4
			  (float3, palette_purple, age::srgb_to_linear(float3{ 0.637f, 0.208f, 0.832f })),		   // #A235D4
			  (float3, palette_amethyst, age::srgb_to_linear(float3{ 0.754f, 0.208f, 0.832f })),	   // #C035D4
			  (float3, palette_magenta, age::srgb_to_linear(float3{ 0.832f, 0.208f, 0.793f })),		   // #D435CA
			  (float3, palette_fuchsia, age::srgb_to_linear(float3{ 0.832f, 0.208f, 0.676f })),		   // #D435AC
			  (float3, palette_cerise, age::srgb_to_linear(float3{ 0.832f, 0.208f, 0.559f })),		   // #D4358E
			  (float3, palette_rose, age::srgb_to_linear(float3{ 0.832f, 0.208f, 0.442f })),		   // #D43570
			  (float3, palette_crimson, age::srgb_to_linear(float3{ 0.832f, 0.208f, 0.325f })),		   // #D43552
			  // light band (32) - sat ~55%, lightness ~72%
			  (float3, palette_light_red, age::srgb_to_linear(float3{ 0.874f, 0.595f, 0.566f })),			 // #DE9790
			  (float3, palette_light_vermilion, age::srgb_to_linear(float3{ 0.874f, 0.652f, 0.566f })),		 // #DEA690
			  (float3, palette_light_orange, age::srgb_to_linear(float3{ 0.874f, 0.710f, 0.566f })),		 // #DEB590
			  (float3, palette_light_tangerine, age::srgb_to_linear(float3{ 0.874f, 0.768f, 0.566f })),		 // #DEC390
			  (float3, palette_light_gold, age::srgb_to_linear(float3{ 0.874f, 0.826f, 0.566f })),			 // #DED290
			  (float3, palette_light_yellow, age::srgb_to_linear(float3{ 0.865f, 0.874f, 0.566f })),		 // #DCDE90
			  (float3, palette_light_pear, age::srgb_to_linear(float3{ 0.807f, 0.874f, 0.566f })),			 // #CDDE90
			  (float3, palette_light_chartreuse, age::srgb_to_linear(float3{ 0.749f, 0.874f, 0.566f })),	 // #BEDE90
			  (float3, palette_light_lime, age::srgb_to_linear(float3{ 0.691f, 0.874f, 0.566f })),			 // #B0DE90
			  (float3, palette_light_harlequin, age::srgb_to_linear(float3{ 0.634f, 0.874f, 0.566f })),		 // #A1DE90
			  (float3, palette_light_green, age::srgb_to_linear(float3{ 0.576f, 0.874f, 0.566f })),			 // #92DE90
			  (float3, palette_light_emerald, age::srgb_to_linear(float3{ 0.566f, 0.874f, 0.614f })),		 // #90DE9C
			  (float3, palette_light_spring, age::srgb_to_linear(float3{ 0.566f, 0.874f, 0.672f })),		 // #90DEAB
			  (float3, palette_light_jade, age::srgb_to_linear(float3{ 0.566f, 0.874f, 0.729f })),			 // #90DEBA
			  (float3, palette_light_mint, age::srgb_to_linear(float3{ 0.566f, 0.874f, 0.787f })),			 // #90DEC8
			  (float3, palette_light_cyan, age::srgb_to_linear(float3{ 0.566f, 0.874f, 0.845f })),			 // #90DED7
			  (float3, palette_light_sky, age::srgb_to_linear(float3{ 0.566f, 0.845f, 0.874f })),			 // #90D7DE
			  (float3, palette_light_cerulean, age::srgb_to_linear(float3{ 0.566f, 0.788f, 0.874f })),		 // #90C8DE
			  (float3, palette_light_azure, age::srgb_to_linear(float3{ 0.566f, 0.730f, 0.874f })),			 // #90BADE
			  (float3, palette_light_blue, age::srgb_to_linear(float3{ 0.566f, 0.672f, 0.874f })),			 // #90ABDE
			  (float3, palette_light_cobalt, age::srgb_to_linear(float3{ 0.566f, 0.614f, 0.874f })),		 // #909CDE
			  (float3, palette_light_ultramarine, age::srgb_to_linear(float3{ 0.575f, 0.566f, 0.874f })),	 // #9290DE
			  (float3, palette_light_indigo, age::srgb_to_linear(float3{ 0.633f, 0.566f, 0.874f })),		 // #A190DE
			  (float3, palette_light_violet, age::srgb_to_linear(float3{ 0.691f, 0.566f, 0.874f })),		 // #B090DE
			  (float3, palette_light_purple, age::srgb_to_linear(float3{ 0.749f, 0.566f, 0.874f })),		 // #BE90DE
			  (float3, palette_light_amethyst, age::srgb_to_linear(float3{ 0.806f, 0.566f, 0.874f })),		 // #CD90DE
			  (float3, palette_light_magenta, age::srgb_to_linear(float3{ 0.864f, 0.566f, 0.874f })),		 // #DC90DE
			  (float3, palette_light_fuchsia, age::srgb_to_linear(float3{ 0.874f, 0.566f, 0.826f })),		 // #DE90D2
			  (float3, palette_light_cerise, age::srgb_to_linear(float3{ 0.874f, 0.566f, 0.768f })),		 // #DE90C3
			  (float3, palette_light_rose, age::srgb_to_linear(float3{ 0.874f, 0.566f, 0.711f })),			 // #DE90B5
			  (float3, palette_light_crimson, age::srgb_to_linear(float3{ 0.874f, 0.566f, 0.653f })),		 // #DE90A6
			  (float3, palette_light_coral, age::srgb_to_linear(float3{ 0.874f, 0.566f, 0.595f })),			 // #DE9097

			  (float, padding_small, (3.f)),
			  (float, padding_medium, (8.f)),
			  (float, padding_large, (12.f)),

			  (float, padding_indicator, (4.f)),

			  (float, gap_small, (3.f)),
			  (float, gap_medium, (6.f)),
			  (float, gap_large, (10.f)),

			  (float, thickness_thin, (1.f)),
			  (float, thickness_medium, (2.f)),
			  (float, thickness_thick, (3.f)),

			  (float, opacity_faint, (0.05f)),
			  (float, opacity_light, (0.10f)),
			  (float, opacity_mild, (0.20f)),
			  (float, opacity_medium, (0.50f)),
			  (float, opacity_heavy, (0.82f)),

			  (float, font_size_small, (11.f)),
			  (float, font_size_medium, (13.f)),
			  (float, font_size_large, (16.f)),
			  (float, font_size_scale, (1.f)),

			  (float, track_height, (4.f)),

			  (float, thumb_xs, (6.f)),			// resize handle
			  (float, thumb_small, (8.f)),		// scroll thumb
			  (float, thumb_medium, (12.f)),	// slider thumb idle
			  (float, thumb_large, (16.f)),		// slider thumb hover
			  (float, thumb_xl, (18.f)),		// slider thumb active

			  (float, roundness_small, (3.f)),
			  (float, roundness_medium, (6.f)),
			  (float, roundness_large, (12.f)))
		;

#if defined(AGE_DEBUG)
	AGE_THEME_WIDGET(panel, color_border_idle, float4(palette_red(), opacity_medium()))

#else
	AGE_THEME_WIDGET(panel, color_border_idle, float4(color_gray_darker(), opacity_medium()))
#endif
	AGE_THEME_WIDGET(panel, color_bg, float4(color_gray_darker(), opacity_medium()))
	AGE_THEME_WIDGET(panel, color_border_focus, float4(color_blue(), opacity_medium()))
	AGE_THEME_WIDGET(panel, padding, float4(padding_small()))
	AGE_THEME_WIDGET(panel, child_gap, gap_small())
	AGE_THEME_WIDGET(panel, border_thickness, thickness_thin())

	AGE_THEME_WIDGET(section, color_bg, float4(color_gray_darker(), 1.f))
	AGE_THEME_WIDGET(section, color_border, float4(color_gray_darker(), 1.f))
	AGE_THEME_WIDGET(section, color_border_focus, float4(color_blue(), opacity_medium()))
	AGE_THEME_WIDGET(section, padding, float4::zero())
	AGE_THEME_WIDGET(section, child_gap, gap_medium())
	AGE_THEME_WIDGET(section, border_thickness, thickness_thin())

	AGE_THEME_WIDGET(header_bar, color_bg, float4(color_gray_dark(), 1.f))
	AGE_THEME_WIDGET(header_bar, color_bg_hover, float4(color_gray(), 1.f))
	AGE_THEME_WIDGET(header_bar, color_bg_active, float4(color_gray_light(), 1.f))
	AGE_THEME_WIDGET(header_bar, padding, float4(padding_medium(), padding_medium(), padding_small(), padding_small()))
	AGE_THEME_WIDGET(header_bar, child_gap, gap_medium())
	AGE_THEME_WIDGET(header_bar, border_thickness, 0.f)
	AGE_THEME_WIDGET(header_bar, roundness, 0.f)

	AGE_THEME_WIDGET(frame, color_bg, float4(color_gray_darkest(), 1.f))
	AGE_THEME_WIDGET(frame, color_bg_hover, float4(color_gray_dark(), 1.f))
	AGE_THEME_WIDGET(frame, color_bg_focus, float4(color_gray_dark(), 1.f))
	AGE_THEME_WIDGET(frame, color_border, float4(color_white(), opacity_light()))
	AGE_THEME_WIDGET(frame, color_border_hover, float4(color_white(), opacity_mild()))
	// AGE_THEME_WIDGET(frame, color_border_focus, float4(color_blue(), 1.f))
	AGE_THEME_WIDGET(frame, color_border_focus, float4(color_blue(), opacity_heavy()))
	AGE_THEME_WIDGET(frame, padding, float4(padding_medium(), padding_medium(), padding_small(), padding_small()))
	AGE_THEME_WIDGET(frame, child_gap, gap_small())
	AGE_THEME_WIDGET(frame, border_thickness, thickness_thin())
	AGE_THEME_WIDGET(frame, roundness, roundness_small())

	AGE_THEME_WIDGET(item, color_bg, float4(color_gray_darker(), 1.f))
	AGE_THEME_WIDGET(item, color_bg_hover, float4(color_gray_dark(), 1.f))
	AGE_THEME_WIDGET(item, color_bg_active, float4(color_gray_dark(), 1.f))
	// AGE_THEME_WIDGET(item, color_bg_selected, float4(color_blue(), opacity_mild()))
	// AGE_THEME_WIDGET(item, color_bg_selected_hover, float4(color_blue_dark(), opacity_mild()))
	// AGE_THEME_WIDGET(item, color_bg_selected_active, float4(color_blue_dark(), opacity_mild()))

	AGE_THEME_WIDGET(item, color_bg_selected, float4(color_blue(), opacity_mild()))
	AGE_THEME_WIDGET(item, color_bg_selected_hover, float4(color_blue_light(), opacity_mild()))
	AGE_THEME_WIDGET(item, color_bg_selected_active, float4(color_blue_dark(), opacity_mild()))

	AGE_THEME_WIDGET(item, color_border, float4(color_gray_darker(), 1.f))
	AGE_THEME_WIDGET(item, color_border_hover, float4(color_gray_darker(), 1.f))
	AGE_THEME_WIDGET(item, color_border_active, float4(color_gray_darker(), 1.f))
	// AGE_THEME_WIDGET(item, color_border_selected, float4(color_blue(), 1.f))
	// AGE_THEME_WIDGET(item, color_border_selected_hover, float4(color_blue(), 1.f))
	// AGE_THEME_WIDGET(item, color_border_selected_active, float4(color_blue(), 1.f))
	AGE_THEME_WIDGET(item, color_border_selected, float4(color_blue(), opacity_medium()))
	AGE_THEME_WIDGET(item, color_border_selected_hover, float4(color_blue(), opacity_heavy()))
	AGE_THEME_WIDGET(item, color_border_selected_active, float4(color_blue(), opacity_heavy()))

	AGE_THEME_WIDGET(item, border_thickness, thickness_medium())
	AGE_THEME_WIDGET(item, padding, float4(padding_medium(), padding_medium(), padding_small(), padding_small()))
	AGE_THEME_WIDGET(item, child_gap, gap_medium())
	AGE_THEME_WIDGET(item, roundness, 0.f)

	AGE_THEME_WIDGET(toggle_box, color_bg_off, float4(color_gray_darkest(), 1.f))
	AGE_THEME_WIDGET(toggle_box, color_bg_off_hover, float4(color_gray_dark(), 1.f))
	AGE_THEME_WIDGET(toggle_box, color_bg_off_active, float4(color_gray_dark(), 1.f))
	// AGE_THEME_WIDGET(toggle_box, color_bg_on, float4(color_blue(), 1.f))
	// AGE_THEME_WIDGET(toggle_box, color_bg_on_hover, float4(color_blue(), 1.f))
	// AGE_THEME_WIDGET(toggle_box, color_bg_on_active, float4(color_blue(), 1.f))

	AGE_THEME_WIDGET(toggle_box, color_bg_on, float4(color_blue(), opacity_medium()))
	AGE_THEME_WIDGET(toggle_box, color_bg_on_hover, float4(color_blue_light(), opacity_medium()))
	AGE_THEME_WIDGET(toggle_box, color_bg_on_active, float4(color_blue_dark(), opacity_medium()))

	AGE_THEME_WIDGET(toggle_box, color_border_off, float4(color_white(), opacity_light()))
	AGE_THEME_WIDGET(toggle_box, color_border_off_hover, float4(color_white(), opacity_mild()))
	AGE_THEME_WIDGET(toggle_box, color_border_off_active, float4(color_white(), opacity_mild()))
	// AGE_THEME_WIDGET(toggle_box, color_border_on, float4(color_blue(), 1.f))

	AGE_THEME_WIDGET(toggle_box, color_border_on, float4(color_blue(), opacity_heavy()))
	AGE_THEME_WIDGET(toggle_box, color_border_on_hover, float4(color_white(), opacity_medium()))
	AGE_THEME_WIDGET(toggle_box, color_border_on_active, float4(color_white(), opacity_heavy()))
	AGE_THEME_WIDGET(toggle_box, color_mark, float4(color_white(), 1.f))
	AGE_THEME_WIDGET(toggle_box, border_thickness, thickness_thin())
	AGE_THEME_WIDGET(toggle_box, roundness, roundness_small())

	AGE_THEME_WIDGET(text_title, color, float4(color_text_white(), 1.f))
	AGE_THEME_WIDGET(text_title, color_disabled, float4(color_text_gray_dark(), 1.f))
	AGE_THEME_WIDGET(text_title, font_size, font_size_large())

	AGE_THEME_WIDGET(text_heading, color, float4(color_text_white(), 1.f))
	AGE_THEME_WIDGET(text_heading, color_disabled, float4(color_text_gray_dark(), 1.f))
	AGE_THEME_WIDGET(text_heading, font_size, font_size_medium())


	// body, field value
	AGE_THEME_WIDGET(text, color, float4(color_text_gray_light(), 1.f))
	AGE_THEME_WIDGET(text, color_hover, float4(color_text_white(), 1.f))
	AGE_THEME_WIDGET(text, color_active, float4(color_text_white(), 1.f))
	AGE_THEME_WIDGET(text, color_disabled, float4(color_text_gray_dark(), 1.f))
	AGE_THEME_WIDGET(text, font_size, font_size_small())

	// property label, secondary info
	AGE_THEME_WIDGET(text_label, color, float4(color_text_gray(), 1.f))
	AGE_THEME_WIDGET(text_label, color_hover, float4(color_text_gray_light(), 1.f))
	AGE_THEME_WIDGET(text_label, color_active, float4(color_text_gray_light(), 1.f))
	AGE_THEME_WIDGET(text_label, color_disabled, float4(color_text_gray_dark(), 1.f))
	AGE_THEME_WIDGET(text_label, font_size, font_size_small())

	// placeholder, disabled, dash indicator
	AGE_THEME_WIDGET(text_hint, color, float4(color_text_gray_dark(), 1.f))
	AGE_THEME_WIDGET(text_hint, color_disabled, float4(color_text_gray_dark(), 1.f))
	AGE_THEME_WIDGET(text_hint, font_size, font_size_small())

	// button
	AGE_THEME_WIDGET(text_button, color, float4(color_text_white(), 1.f))
	AGE_THEME_WIDGET(text_button, color_disabled, float4(color_text_gray_dark(), 1.f))
	AGE_THEME_WIDGET(text_button, font_size, font_size_medium())


	AGE_THEME_WIDGET(slider_track, color_bg, float4(color_gray_darkest(), 1.f))
	AGE_THEME_WIDGET(slider_track, color_fill, float4(color_blue(), 1.f))
	AGE_THEME_WIDGET(slider_track, color_fill_hover, float4(color_blue(), 1.f))
	AGE_THEME_WIDGET(slider_track, color_fill_active, float4(color_blue(), 1.f))
	AGE_THEME_WIDGET(slider_track, size, track_height())
	AGE_THEME_WIDGET(slider_track, roundness, roundness_large())


	AGE_THEME_WIDGET(slider_thumb, color, float4(color_blue(), 1.f))
	AGE_THEME_WIDGET(slider_thumb, color_hover, float4(color_blue(), 1.f))
	AGE_THEME_WIDGET(slider_thumb, color_active, float4(color_blue(), 1.f))
	AGE_THEME_WIDGET(slider_thumb, size, thumb_medium())
	AGE_THEME_WIDGET(slider_thumb, size_hover, thumb_large())
	AGE_THEME_WIDGET(slider_thumb, size_active, thumb_xl())


	AGE_THEME_WIDGET(scroll_thumb, color, float4(color_gray(), 1.f))
	AGE_THEME_WIDGET(scroll_thumb, color_hover, float4(color_gray_light(), 1.f))
	AGE_THEME_WIDGET(scroll_thumb, color_active, float4(color_text_gray_dark(), 1.f))
	AGE_THEME_WIDGET(scroll_thumb, size, thumb_small())
	AGE_THEME_WIDGET(scroll_thumb, roundness, roundness_medium())

	AGE_THEME_WIDGET(separator, color, float4(color_text_gray_light(), 1.f))
	AGE_THEME_WIDGET(separator, thickness, thickness_thin())

	AGE_THEME_WIDGET(resize_handle, color, float4(color_gray(), 1.f))
	AGE_THEME_WIDGET(resize_handle, color_hover, float4(color_gray_light(), 1.f))
	// AGE_THEME_WIDGET(resize_handle, color_active, float4(color_blue(), 1.f))
	AGE_THEME_WIDGET(resize_handle, color_active, float4(color_blue(), opacity_medium()))
	AGE_THEME_WIDGET(resize_handle, size, thumb_xs())

	AGE_THEME_WIDGET(cursor, thickness, thickness_medium())	   // 2

	AGE_THEME_WIDGET(indicator, color, float4(color_text_gray_dark(), 1.f))

#undef AGE_THEME_FOREACH_FUNC
#undef AGE_THEME_PRIMITIVE_MEMBER
#undef AGE_THEME_PREIMTIVE_FUNC
#undef AGE_THEME_WIDGET


}	 // namespace age::ui

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
	template <std::size_t n = 1>
	struct widget_ctx_impl
	{
		t_hash hash_id = age::get_invalid_id<t_hash>();

		AGE_DISABLE_COPY(widget_ctx_impl);

		FORCE_INLINE constexpr widget_ctx_impl() noexcept = default;

		FORCE_INLINE constexpr widget_ctx_impl(t_hash id) noexcept
			: hash_id(id){};

		template <std::size_t... m>
		FORCE_INLINE constexpr widget_ctx_impl(widget_ctx_impl<m>&&... other) noexcept
			: hash_id((other.hash_id, ...))
		{
			((other.hash_id = get_invalid_id<t_hash>()), ...);
		}

		FORCE_INLINE constexpr ~widget_ctx_impl() noexcept;

		FORCE_INLINE constexpr explicit
		operator bool() const
		{
			return hash_id != age::get_invalid_id<t_hash>();
		}

		// WARNING: returned reference is invalidated when any new widget_state entry is inserted into the map.
		// Do NOT hold this reference across widget::begin() or get_state() calls that may create new entries.

		FORCE_INLINE widget_state&
		get_state() const noexcept
		{
			return g::widget_state_map[hash_id];
		}

		FORCE_INLINE bool
		hovered() const noexcept
		{
			return hash_id == g::hover_id;
		}

		FORCE_INLINE bool
		hovered_all() const noexcept
		{
			return std::ranges::contains(g::hover_id_stack, hash_id);
		}

		FORCE_INLINE bool
		contains_mouse() const noexcept
		{
			c_auto	id			 = g::root_data_idx_stack.back();
			c_auto& current_root = g::root_data_vec_arr[(id >> 24u) & 0xff][id & 0x00ff'ffff];
			c_auto& state		 = get_state();
			return math::contains_2d(float4{ state.pos.x, state.pos.y, state.pos.x + state.clip_width, state.pos.y + state.clip_height }, current_root.mouse_uv);
		}

		FORCE_INLINE bool
		focused() const noexcept
		{
			return hash_id == g::focus_id;
		}

		FORCE_INLINE bool
		focused_all() const noexcept
		{
			return std::ranges::contains(g::hover_id_stack, hash_id);
		}

		FORCE_INLINE bool
		mouse_l_pressed() const noexcept
		{
			return hash_id == g::mouse_l_pressed_id;
		}

		FORCE_INLINE bool
		mouse_l_clicked() const noexcept
		{
			return hash_id == g::mouse_l_clicked_id;
		}

		FORCE_INLINE bool
		mouse_r_clicked() const noexcept
		{
			return hash_id == g::mouse_r_clicked_id;
		}

		template <input::e::key_kind e_key = input::e::key_kind::mouse_left>
		FORCE_INLINE bool
		clicked() const noexcept
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

		FORCE_INLINE bool
		double_clicked() const noexcept
		{
			return mouse_l_clicked() and g::mouse_l_clicked_count == 2;
		}

		FORCE_INLINE bool
		triple_clicked() const noexcept
		{
			return mouse_l_clicked() and g::mouse_l_clicked_count == 3;
		}

		template <input::e::key_kind e_key = input::e::key_kind::mouse_left>
		FORCE_INLINE bool
		pressed() const noexcept
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

		template <input::e::key_kind e_key = input::e::key_kind::mouse_left>
		FORCE_INLINE e::style_state
		get_style_state() const noexcept
		{
			if constexpr (e_key == input::e::key_kind::mouse_left)
			{
				auto state = e::style_state::idle;
				if (pressed<e_key>())
				{
					state = e::style_state::active;
				}
				else if (hovered())
				{
					state = e::style_state::hover;
				}

				return state;
			}
			else
			{
				static_assert(false, "not implemented yet");
			}
		}

		FORCE_INLINE bool
		is_toggled() const noexcept
		{
			return get_state().toggled;
		}

		FORCE_INLINE void
		toggle() const noexcept
		{
			auto& state	  = get_state();
			state.toggled = !state.toggled;
		}

		FORCE_INLINE void
		set_toggled(bool value) const noexcept
		{
			auto& state	  = get_state();
			state.toggled = value;
		}

		FORCE_INLINE float2&
		get_pos() const noexcept
		{
			return get_state().pos;
		}

		FORCE_INLINE float&
		get_width() const noexcept
		{
			return get_state().width;
		}

		FORCE_INLINE float&
		get_height() const noexcept
		{
			return get_state().height;
		}

		FORCE_INLINE float&
		get_drag_x() const noexcept
		{
			return get_state().drag_x;
		}

		FORCE_INLINE float&
		get_drag_y() const noexcept
		{
			return get_state().drag_y;
		}
	};

	template <std::size_t... n>
	widget_ctx_impl(widget_ctx_impl<n>&&...) -> widget_ctx_impl<(n + ...)>;

	using widget_ctx = widget_ctx_impl<1>;

	struct root_ctx
	{
		t_hash hash_id = age::get_invalid_id<t_hash>();

		AGE_DISABLE_COPY(root_ctx);

		FORCE_INLINE constexpr root_ctx() noexcept = default;

		FORCE_INLINE constexpr root_ctx(t_hash id) noexcept
			: hash_id(id){};

		FORCE_INLINE constexpr ~root_ctx() noexcept;

		FORCE_INLINE constexpr explicit
		operator bool() const
		{
			return hash_id != age::get_invalid_id<t_hash>();
		}
	};
}	 // namespace age::ui
#pragma once
#include "age.hpp"

namespace age::ui::e
{
	AGE_DEFINE_ENUM(shape_kind, uint8,
					rect,
					circle,
					arrow_right);

	AGE_DEFINE_ENUM(brush_kind, uint8,
					color);

	AGE_DEFINE_ENUM(widget_layout, uint8, horizontal, vertical)

	AGE_DEFINE_ENUM(widget_overflow, uint8, draw_all, discard, clip, scroll)

	AGE_DEFINE_ENUM(widget_align, uint8, begin, center, end)

	AGE_DEFINE_ENUM(size_mode_kind, uint8, fixed, fit, grow, grow_weight, text, aspect_ratio)
}	 // namespace age::ui::e

namespace age::ui
{
	using t_hash = uint64;
}

namespace age::ui
{
	struct ui_shape_data
	{
		uint32_4 data;											   // TBD, corner radius, circle radius, ...
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


		widget_size_mode size_mode_width  = {};
		widget_size_mode size_mode_height = {};
		uint16			 z_offset		  = 1;

		float2 offset = float2{ 0, 0 };

		float  child_gap = 2.f;
		float4 padding	 = { 2.f, 2.f, 2.f, 2.f };

		float2 pivot_uv			= float2{ 0.5f, 0.5f };
		float  rotation			= 0.f;
		float  border_thickness = 1.f;

		e::shape_kind shape_kind		= e::shape_kind::rect;
		e::brush_kind body_brush_kind	= e::brush_kind::color;
		e::brush_kind border_brush_kind = e::brush_kind::color;
		uint8		  _;

		ui_shape_data shape_data;
		ui_brush_data body_brush_data;
		ui_brush_data border_brush_data;

		uint64 extra;
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

	// data for calculating element_size
	struct layout_data_h
	{
		e::widget_layout  layout;
		e::size_mode_kind mode;

		uint8_2 _;
		uint32	global_idx;
		uint32	grow_child_count;

		float width;

		float width_min;
		float width_max;
	};

	struct layout_data_v
	{
		e::widget_layout  layout;
		e::size_mode_kind mode;

		uint8_2 _;
		uint32	global_idx;
		uint32	grow_child_count;

		float height;

		float height_min;
		float height_max;
	};

	struct layout_data_common
	{
		uint32 child_count;
		uint32 parent_h_idx;
		uint32 parent_v_idx;
		float  child_gap;
		uint16 z_offset;
		bool   child_height_depends_on_width_solved;
	};

	struct layout_pos_data
	{
		bool	draw;
		uint8_3 _;

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

		uint64 extra;
	};
}	 // namespace age::ui

namespace age::ui::g
{
	inline constexpr uint64 fnv1a_offset_basis = 0xcbf29ce484222325ull;
	inline constexpr uint64 fnv1a_prime		   = 0x100000001b3ull;

	inline age::vector<uint64> id_stack;

	inline age::unordered_map<uint64, element_state> element_state_map;

	// layout stack
	inline age::vector<layout_data_h>	   element_layout_data_h_stack;
	inline age::vector<layout_data_v>	   element_layout_data_v_stack;
	inline age::vector<layout_data_common> element_layout_data_common_stack;

	// layout vec
	inline age::vector<layout_pos_data> element_layout_pos_data_vec;
	inline age::vector<render_data>		element_render_data_vec;
	inline age::vector<uint32>			element_z_order_count_vec;

	// scratch
	inline age::vector<uint64> element_layout_grow_event_vec;
	inline age::vector<uint32> element_pos_parent_idx_stack;

	inline uint32 layout_h_current_idx;
	inline uint32 layout_v_current_idx;

	// font
	inline age::vector<std::pair<t_hash, asset::handle>> font_vec;
	inline uint32										 current_font_idx;
	inline float										 current_font_size;

}	 // namespace age::ui::g

namespace age::ui
{
	struct id_ctx
	{
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
		t_hash hash_id;

		FORCE_INLINE constexpr explicit
		operator bool() const
		{
			return true;
		}

		~widget_ctx() noexcept;
	};
}	 // namespace age::ui
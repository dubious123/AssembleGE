#pragma once
#include "age.hpp"

namespace age::ui::e
{
	AGE_DEFINE_ENUM(shape_kind, uint32,
					rect,
					circle);

	AGE_DEFINE_ENUM(brush_kind, uint32,
					color);

	AGE_DEFINE_ENUM(widget_layout, uint8, horizontal, vertical)

	AGE_DEFINE_ENUM(widget_overflow, uint8, draw_all, discard, clip, scroll)

	AGE_DEFINE_ENUM(widget_align, uint8, left, right, top, bottom, center)

	AGE_DEFINE_ENUM(size_mode_kind, uint8, fixed, fit, grow, grow_weight)
}	 // namespace age::ui::e

namespace age::ui
{
	using t_hash = uint64;
}

namespace age::ui
{
	struct ui_shape_data
	{
		uint32_4 data;				 // TBD, corner radius, circle radius, ...
	};

	struct ui_brush_data
	{
		uint32_4 data;				 // TBD, texture_id, static_color, calculate from shadow with delta time...
	};

	struct render_data
	{
		float2 pivot_pos;			 // screen pos of pivot
		float2 pivot_uv;
		float2 size;				 // pixel size
		float  rotation;			 // z rotation, radian
		float  border_thickness;

		uint32 shape_kind;			 // 1. rect, 2. rounded_rect, 3. circle, 4. star??? ...

		ui_shape_data shape_data;

		uint32 body_brush_kind;		 // 1. texture_id, 2. color, 3. generated from uv, ...

		ui_brush_data body_brush_data;

		uint32 border_brush_kind;	 // 1. texture_id, 2. color, 3. generated from uv, ...

		ui_brush_data border_brush_data;
	};
}	 // namespace age::ui

namespace age::ui
{
	struct widget_size_mode
	{
		float min = 0.f;
		float max = std::numeric_limits<float>::max();

		e::size_mode_kind size_mode = e::size_mode_kind::grow;

		std::array<uint8, 3> _;
	};

	struct widget_desc
	{
		e::widget_layout   layout	= e::widget_layout::horizontal;
		e::widget_overflow overflow = e::widget_overflow::draw_all;
		e::widget_align	   align	= e::widget_align::left;

		uint8 _;

		widget_size_mode size_mode_width  = {};
		widget_size_mode size_mode_height = {};

		float  child_gap = 0.f;
		float4 padding	 = {};

		render_data render_data;
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
		e::widget_layout layout;

		e::size_mode_kind mode;

		uint8  _;
		uint32 render_data_idx;

		uint32 child_count;
		float  width;

		float width_min;
		float width_max;

		float child_gap = 0.f;
		float padding_left;
		float padding_right;
	};

	struct layout_data_v
	{
		e::widget_layout layout;

		e::size_mode_kind mode;

		uint8  _;
		uint32 render_data_idx;

		uint32 child_count;
		float  height;

		float height_min;
		float height_max;

		float child_gap = 0.f;
		float padding_top;
		float padding_bottom;
	};
}	 // namespace age::ui

namespace age::ui::g
{
	inline constexpr uint64 fnv1a_offset_basis = 0xcbf29ce484222325ull;
	inline constexpr uint64 fnv1a_prime		   = 0x100000001b3ull;

	inline age::vector<uint64> id_stack;

	inline age::unordered_map<uint64, element_state> element_state_map;

	inline age::vector<layout_data_h> element_layout_data_h_stack;
	inline age::vector<layout_data_v> element_layout_data_v_stack;

	inline age::vector<render_data>		element_render_data_vec;
	inline age::vector<e::widget_align> element_align_vec;

	inline uint32 layout_h_parent_idx;
	inline uint32 layout_v_parent_idx;

	inline uint32 layout_h_current_idx;
	inline uint32 layout_v_current_idx;
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
#pragma once
#include "age.hpp"

namespace age::ui
{
	FORCE_INLINE id_ctx
	push_id(const char*) noexcept;

	FORCE_INLINE id_ctx
	push_id(uint64 _) noexcept;

	FORCE_INLINE t_hash
	new_id(const char*) noexcept;

	FORCE_INLINE t_hash
	new_id(uint64 _) noexcept;
}	 // namespace age::ui

namespace age::ui
{
	void
	init() noexcept;

	void
	begin_frame(platform::window_handle h_window) noexcept;

	void
	end_frame(age::vector<render_data>&, age::vector<util::range>&) noexcept;

	void
	deinit() noexcept;
}	 // namespace age::ui

// widgets
namespace age::ui::widget
{
	widget_ctx
	begin(const char* p_str, const widget_desc& desc) noexcept;

	widget_ctx
	begin(const widget_desc& desc) noexcept;

	widget_ctx
	layout_horizontal(widget_size_mode width,
					  widget_size_mode height,
					  float4		   padding = float4{ 10.f, 10.f, 10.f, 10.f },
					  e::widget_align  align   = e::widget_align::center,
					  float2		   offset  = float2{ 0.f, 0.f }) noexcept;

	widget_ctx
	layout_vertical(widget_size_mode width,
					widget_size_mode height,
					float4			 padding = float4{ 10.f, 10.f, 10.f, 10.f },
					e::widget_align	 align	 = e::widget_align::center,
					float2			 offset	 = float2{ 0.f, 0.f }) noexcept;
}	 // namespace age::ui::widget

// defaults
namespace age::ui::size_mode
{
	FORCE_INLINE constexpr widget_size_mode
	fixed(auto value) noexcept;

	FORCE_INLINE constexpr widget_size_mode
	grow(auto min, auto max) noexcept;

	FORCE_INLINE constexpr widget_size_mode
	fit(auto min, auto max) noexcept;
}	 // namespace age::ui::size_mode

namespace age::ui::brush_data
{
	FORCE_INLINE constexpr ui_brush_data
	color(float r, float g, float b) noexcept;
}	 // namespace age::ui::brush_data

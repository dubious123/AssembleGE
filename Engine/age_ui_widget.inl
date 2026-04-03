#pragma once
#include "age.hpp"

// layout
namespace age::ui::widget
{
	FORCE_INLINE widget_ctx
	layout(e::widget_layout e_layout) noexcept
	{
		return widget::begin(style::layout(e_layout));
	}

	FORCE_INLINE widget_ctx
	vertical() noexcept
	{
		return widget::begin(style::vertical());
	}

	FORCE_INLINE widget_ctx
	horizontal() noexcept
	{
		return widget::begin(style::horizontal());
	}
}	 // namespace age::ui::widget

// text
namespace age::ui::widget
{
	FORCE_INLINE widget_ctx
	text_primary(const char* p_str) noexcept
	{
		return widget::begin(style::text_primary(p_str));
	}

	FORCE_INLINE widget_ctx
	text_secondary(const char* p_str) noexcept
	{
		return widget::begin(style::text_secondary(p_str));
	}

	FORCE_INLINE widget_ctx
	text_tertiary(const char* p_str) noexcept
	{
		return widget::begin(style::text_tertiary(p_str));
	}

	FORCE_INLINE widget_ctx
	text_hint(const char* p_str) noexcept
	{
		return widget::begin(style::text_hint(p_str));
	}

	FORCE_INLINE widget_ctx
	text_disabled(const char* p_str) noexcept
	{
		return widget::begin(style::text_disabled(p_str));
	}

	FORCE_INLINE widget_ctx
	text_accent(const char* p_str) noexcept
	{
		return widget::begin(style::text_accent(p_str));
	}

	FORCE_INLINE widget_ctx
	text_positive(const char* p_str) noexcept
	{
		return widget::begin(style::text_positive(p_str));
	}

	FORCE_INLINE widget_ctx
	text_negative(const char* p_str) noexcept
	{
		return widget::begin(style::text_negative(p_str));
	}

	FORCE_INLINE widget_ctx
	text_interactive(const char* p_str) noexcept
	{
		return widget::begin(style::text_interactive(p_str));
	}
}	 // namespace age::ui::widget
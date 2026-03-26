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
	widget_ctx
	widget(const char* name, const widget_desc& = {}) noexcept;

	widget_ctx
	widget(t_hash h, const widget_desc& = {}) noexcept;
}	 // namespace age::ui

namespace age::ui
{
	void
	init() noexcept;

	void
	begin_frame() noexcept;

	void
	end_frame() noexcept;

	void
	deinit() noexcept;
}	 // namespace age::ui

#pragma once
#include "age.hpp"

namespace age::ui
{
	FORCE_INLINE id_ctx
	push_id(const char*) noexcept;

	FORCE_INLINE id_ctx
	push_id(uint64 _) noexcept;

	FORCE_INLINE uint64
	new_id(const char*) noexcept;

	FORCE_INLINE uint64
	new_id(uint64 _) noexcept;
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

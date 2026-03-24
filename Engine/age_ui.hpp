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
	FORCE_INLINE void
	init() noexcept;

	FORCE_INLINE void
	begin_frame() noexcept;

	FORCE_INLINE void
	end_frame() noexcept;

	FORCE_INLINE void
	deinit() noexcept;
}	 // namespace age::ui

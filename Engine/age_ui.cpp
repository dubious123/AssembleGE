#include "age_pch.hpp"
#include "age.hpp"

namespace age::ui
{
	FORCE_INLINE void
	init() noexcept
	{
		AGE_ASSERT(g::id_stack.is_empty());
	}

	FORCE_INLINE void
	begin_frame() noexcept
	{
		g::id_stack.emplace_back(g::fnv1a_offset_basis);
	}

	FORCE_INLINE void
	end_frame() noexcept
	{
	}

	FORCE_INLINE void
	deinit() noexcept
	{
		g::id_stack.clear();
	}
}	 // namespace age::ui
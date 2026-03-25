#include "age_pch.hpp"
#include "age.hpp"

namespace age::ui
{
	void
	init() noexcept
	{
		AGE_ASSERT(g::id_stack.is_empty());
	}

	void
	begin_frame() noexcept
	{
		g::id_stack.emplace_back(g::fnv1a_offset_basis);
	}

	void
	end_frame() noexcept
	{
	}

	void
	deinit() noexcept
	{
		g::id_stack.clear();
		g::element_state_map.clear();
	}
}	 // namespace age::ui
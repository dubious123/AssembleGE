#include "age_pch.hpp"
#include "age.hpp"

namespace age::ui
{
	void
	init() noexcept
	{
		AGE_ASSERT(g::id_stack.is_empty());
		// g::current_element_parent_idx = age::get_invalid_id<uint32>();
	}

	void
	begin_frame() noexcept
	{
		g::id_stack.clear();
		g::id_stack.emplace_back(g::fnv1a_offset_basis);
		// g::element_vec.clear();

		// AGE_ASSERT(g::current_element_parent_idx == age::get_invalid_id<uint32>());
	}

	void
	end_frame() noexcept
	{
		// AGE_ASSERT(g::current_element_parent_idx == age::get_invalid_id<uint32>());
	}

	void
	deinit() noexcept
	{
		g::element_state_map.clear();
	}
}	 // namespace age::ui
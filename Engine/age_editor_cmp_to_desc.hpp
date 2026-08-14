#pragma once
#include "age.hpp"

namespace age::editor
{
	age::graphics::render_pipeline::debug_view_desc
	cmp_to_desc(const age::ecs::debug_view_config& config) noexcept;
}	 // namespace age::editor
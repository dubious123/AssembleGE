#pragma once
#include "age.hpp"

namespace age::editor
{
	age::graphics::render_pipeline::debug_view_desc
	cmp_to_desc(const age::ecs::debug_view_config& config) noexcept;

	age::graphics::render_pipeline::model_render_option
	cmp_to_desc(const age::ecs::model_render_option& cmp) noexcept;
}	 // namespace age::editor
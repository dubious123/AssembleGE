#pragma once
#include "age.hpp"

namespace age::graphics
{
	struct
	{
		AGE_GET(display_color_space, display_color_space);
	} i_color;
}	 // namespace age::graphics

// handle
namespace age::graphics
{
	using t_render_surface_id = uint32;

	struct render_surface_handle
	{
		t_render_surface_id id;
	};
}	 // namespace age::graphics

namespace age::graphics
{
	void
	init() noexcept;

	void
	deinit() noexcept;

	render_surface_handle
	create_render_surface(platform::window_handle _) noexcept;

	render_surface_handle
	find_render_surface(platform::window_handle h_window) noexcept;

	void
	resize_render_surface(render_surface_handle _) noexcept;

	void
	begin_frame() noexcept;

	void
	end_frame() noexcept;
}	 // namespace age::graphics
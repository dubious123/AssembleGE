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

namespace age::graphics::resource
{
	void
	release(resource_handle& _) noexcept;

	void
	release(std::span<resource_handle> _) noexcept;

	uint64
	calc_readback_size(resource_handle _) noexcept;

	void
	readback_texture(std::span<std::byte> dst, resource_handle h_src) noexcept;
};	  // namespace age::graphics::resource

namespace age::graphics::bake
{
	struct env_light_result
	{
		resource_handle h_radiance;		 // last layout is copy src (direct_queue)
		resource_handle h_irradiance;	 // last layout is copy src (direct_queue)
		resource_handle h_prefilter;	 // last layout is copy src (direct_queue)
	};

	env_light_result
	env_light(asset::handle h_tex, const asset::env_light_desc& desc) noexcept;

	resource_handle
	bake_brdf_lut(extent_2d<uint32> _) noexcept;
}	 // namespace age::graphics::bake
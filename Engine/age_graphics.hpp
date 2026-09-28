#pragma once
#include "age.hpp"

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

// utils
namespace age::graphics
{
	struct monitor_data
	{
		bool	hdr_enabled;
		uint8_3 _;
		float	min_luminance;
		float	max_luminance;
		float	max_full_frame_luminance;

		age::array<char, 192> monitor_name;
	};

	bool
	texture_format_is_srgb(e::texture_format format) noexcept;

	uint32
	get_texture_format_channel_count(e::texture_format format) noexcept;

	bool
	texture_format_has_alpha(e::texture_format format) noexcept;

	monitor_data
	get_monitor_data(platform::window_handle h_window) noexcept;

	monitor_data
	get_monitor_data(render_surface_handle h_render_surface) noexcept;

	e::color_space_kind
	get_color_space(render_surface_handle h_render_surface) noexcept;

	void
	request_change_color_space(render_surface_handle, e::color_space_kind preference_color_space) noexcept;
}	 // namespace age::graphics
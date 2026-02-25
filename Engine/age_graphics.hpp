#pragma once
#include "age.hpp"

namespace age::graphics::e
{
	AGE_DEFINE_ENUM(camera_kind, uint8, perspective, orthographic);
}

namespace age::graphics
{
	AGE_DEFINE_ENUM(color_space, uint8, srgb, hdr);
}	 // namespace age::graphics

namespace age::graphics
{
	template <typename t>
	struct interface
	{
	  private:
		no_unique_addr t data;

	  public:
		constexpr interface(auto&& arg) noexcept : data(FWD(arg)) { }

		AGE_PROP(display_color_space)
	};

	template <typename t>
	interface(t&&) -> interface<t>;
}	 // namespace age::graphics

// handle
namespace age::graphics
{
	using t_resource_id = uint32;

	struct resource_handle
	{
		t_resource_id id = invalid_id_uint32;

		FORCE_INLINE auto*
		operator->() noexcept;
	};

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
	render() noexcept;

	void
	begin_frame() noexcept;

	void
	end_frame() noexcept;
}	 // namespace age::graphics
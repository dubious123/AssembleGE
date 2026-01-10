#pragma once

namespace age::graphics
{
	enum class color_space : uint8
	{
		srgb,
		hdr,
		count
	};
}

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
		t_resource_id id;
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
		create_render_surface(platform::window_handle) noexcept;

	void
	render() noexcept;

	void
	begin_frame() noexcept;

	void
	end_frame() noexcept;
}	 // namespace age::graphics

#if defined AGE_GRAPHICS_BACKEND_DX12
	#include "age_graphics_backend_dx12.hpp"
	#include "age_graphics_backend_dx12_descriptor_pool.hpp"
	#include "age_graphics_backend_dx12_root_signature.hpp"
#endif
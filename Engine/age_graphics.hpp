#pragma once
#include "age.hpp"

namespace age::graphics::e
{
	AGE_DEFINE_ENUM(camera_kind, uint8, perspective, orthographic);

	AGE_DEFINE_ENUM_WITH_VALUE(light_kind, uint16,
							   (directional, 0),
							   (point, 1),
							   (spot, 2),
							   (area, 3),
							   (volumn, 4));
}	 // namespace age::graphics::e

namespace age::graphics
{
	struct
	{
		struct
		{
			__forceinline decltype(auto)
			operator->() noexcept
			{
				if constexpr (age::meta::cx_has_arrow<std::remove_cvref_t<decltype((global::detail::ctx.display_color_space))>>)
				{
					return global::detail::ctx.display_color_space;
				}
				else
				{
					return &global::detail::ctx.display_color_space;
				}
			}

			__forceinline auto&
			operator()() noexcept
			{
				return global::detail::ctx.display_color_space;
			}

			__forceinline
			operator auto&() noexcept
			{
				return global::detail::ctx.display_color_space;
			}

			__forceinline decltype(auto)
			operator[](auto&&... i) noexcept
			{
				return global::detail::ctx.display_color_space[std::forward<decltype(i)>((i))...];
			}
		} get_display_color_space;
	} i_color;
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

		FORCE_INLINE c_auto*
		operator->() const noexcept;
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
	begin_frame() noexcept;

	void
	end_frame() noexcept;
}	 // namespace age::graphics
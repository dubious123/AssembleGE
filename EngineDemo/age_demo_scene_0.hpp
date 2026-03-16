#pragma once
#include "age_demo.hpp"

namespace age_demo::scene_0
{
	namespace input::g
	{
		inline constexpr auto move_speed	= 2.f;
		inline constexpr auto sprint_mult	= 4.f;
		inline constexpr auto sensitivity	= 0.003f;
		inline constexpr auto zoom_speed	= 2.f;
		inline constexpr auto zoom_distance = 4.f;
		inline constexpr auto pan_speed		= 0.6f;

		inline constexpr auto move_smoothing = 15.f / 2.f;
		inline constexpr auto look_smoothing = 25.f / 2.f;
		inline constexpr auto zoom_smoothing = 12.f / 2.f;
	}	 // namespace input::g

	struct ctx
	{
		age::vector<age::graphics::render_pipeline::forward_plus::t_mesh_id>
			mesh_id_vec = age::vector<age::graphics::render_pipeline::forward_plus::t_mesh_id>::gen_reserved(1);

		age::vector<age::graphics::render_pipeline::forward_plus::t_object_id>
			obj_id_vec = age::vector<age::graphics::render_pipeline::forward_plus::t_object_id>::gen_reserved(27);

		age::vector<age::graphics::render_pipeline::forward_plus::t_camera_id>
			camera_id_vec = age::vector<age::graphics::render_pipeline::forward_plus::t_camera_id>::gen_reserved(1);

		age::vector<age::graphics::render_pipeline::forward_plus::t_unified_light_id>
			point_light_id_vec = age::vector<age::graphics::render_pipeline::forward_plus::t_unified_light_id>::gen_reserved(5000);

		age::vector<age::graphics::render_pipeline::forward_plus::t_unified_light_id>
			spot_light_id_vec = age::vector<age::graphics::render_pipeline::forward_plus::t_unified_light_id>::gen_reserved(1);

		age::vector<age::graphics::render_pipeline::forward_plus::t_directional_light_id>
			directional_light_id_vec;

		// input
		struct
		{
			float euler_x = 0.f;
			float euler_y = 0.f;

			float2 smoothed_move = float2{ 0.f, 0.f };
			float2 smoothed_look = float2{ 0.f, 0.f };
			float  smoothed_zoom = 0.f;
			float2 smoothed_pan	 = float2{ 0.f, 0.f };
		} input;
	};
}	 // namespace age_demo::scene_0

namespace age_demo::scene_0
{
	template <typename t>
	struct interface_init
	{
		t& data;
		AGE_GET_PROP(render_pipeline)

		AGE_GET(mesh_id_vec, scene_0_ctx.mesh_id_vec)
		AGE_GET(obj_id_vec, scene_0_ctx.obj_id_vec)
		AGE_GET(camera_id_vec, scene_0_ctx.camera_id_vec)
		AGE_GET(point_light_id_vec, scene_0_ctx.point_light_id_vec)
		AGE_GET(spot_light_id_vec, scene_0_ctx.spot_light_id_vec)
		AGE_GET(directional_light_id_vec, scene_0_ctx.directional_light_id_vec)

		AGE_SET(euler_x, scene_0_ctx.input.euler_x)
		AGE_SET(euler_y, scene_0_ctx.input.euler_y)
		AGE_SET(smoothed_move, scene_0_ctx.input.smoothed_move)
		AGE_SET(smoothed_look, scene_0_ctx.input.smoothed_look)
		AGE_SET(smoothed_zoom, scene_0_ctx.input.smoothed_zoom)
		AGE_SET(smoothed_pan, scene_0_ctx.input.smoothed_pan)
	};

	template <typename t>
	struct interface_loop
	{
		t& data;
		AGE_GET_PROP(render_pipeline)
		AGE_GET_PROP(h_render_surface)

		AGE_GET_PROP(sprint)
		AGE_GET_PROP(move)
		AGE_GET_PROP(zoom)
		AGE_GET_PROP(look)
		AGE_GET_PROP(right_mouse_down)
		AGE_GET_PROP(middle_mouse_down)

		AGE_GET(mesh_id_vec, scene_0_ctx.mesh_id_vec)
		AGE_GET(obj_id_vec, scene_0_ctx.obj_id_vec)
		AGE_GET(camera_id_vec, scene_0_ctx.camera_id_vec)

		AGE_GETSET(euler_x, scene_0_ctx.input.euler_x)
		AGE_GETSET(euler_y, scene_0_ctx.input.euler_y)
		AGE_GETSET(smoothed_move, scene_0_ctx.input.smoothed_move)
		AGE_GETSET(smoothed_look, scene_0_ctx.input.smoothed_look)
		AGE_GETSET(smoothed_zoom, scene_0_ctx.input.smoothed_zoom)
		AGE_GETSET(smoothed_pan, scene_0_ctx.input.smoothed_pan)
	};

	template <typename t>
	struct interface_deinit
	{
		t& data;
		AGE_GET_PROP(render_pipeline)

		AGE_GET(mesh_id_vec, scene_0_ctx.mesh_id_vec)
		AGE_GET(obj_id_vec, scene_0_ctx.obj_id_vec)
		AGE_GET(camera_id_vec, scene_0_ctx.camera_id_vec)
		AGE_GET(point_light_id_vec, scene_0_ctx.point_light_id_vec)
		AGE_GET(spot_light_id_vec, scene_0_ctx.spot_light_id_vec)
		AGE_GET(directional_light_id_vec, scene_0_ctx.directional_light_id_vec)
	};
}	 // namespace age_demo::scene_0

namespace age_demo::scene_0
{
	FORCE_INLINE decltype(auto)
	init() noexcept;

	FORCE_INLINE decltype(auto)
	update() noexcept;

	FORCE_INLINE decltype(auto)
	deinit() noexcept;
}	 // namespace age_demo::scene_0
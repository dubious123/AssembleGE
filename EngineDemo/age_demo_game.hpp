#pragma once
#include "age_demo.hpp"

namespace age_demo::game::g
{
	inline constexpr auto first_scene_idx = 0;
}

namespace age_demo::game
{
	FORCE_INLINE void
	update_input() noexcept;

	FORCE_INLINE void
	handle_scene_change() noexcept;

	FORCE_INLINE bool
	scene_changed() noexcept;

	constexpr FORCE_INLINE decltype(auto)
	print_frame_rate() noexcept;

	constexpr FORCE_INLINE decltype(auto)
	get_sys_loop() noexcept;

	constexpr FORCE_INLINE decltype(auto)
	get_sys_deinit() noexcept;
}	 // namespace age_demo::game

namespace age_demo::game
{
	struct
	{
		AGE_GET(h_input_ctx, h_input_ctx)
		AGE_SET(move, move)
		AGE_SET(look, look)
		AGE_SET(zoom, zoom)
		AGE_SET(sprint, sprint)
		AGE_SET(right_mouse_down, right_mouse_down)
		AGE_SET(middle_mouse_down, middle_mouse_down)
	} i_input;

	struct
	{
		AGE_GET(scene_0_ctx, scene_0_ctx)
		AGE_GET(scene_1_ctx, scene_1_ctx)
		AGE_GET(scene_id_next, scene_id_next)
		AGE_GETSET(scene_id, scene_id)
	} i_scene;

	struct
	{
		AGE_SET(scene_id, scene_id)
		AGE_SET(scene_id_next, scene_id_next)
		AGE_GETSET(h_window, h_window)
		AGE_SET(h_render_surface, h_render_surface)
		AGE_SET(h_input_ctx, h_input_ctx)
		AGE_GET(render_pipeline, render_pipeline)
	} i_init;

	struct
	{
		AGE_GET(scene_id, scene_id)
		AGE_GET(render_pipeline, render_pipeline)
	} i_deinit;

	struct
	{
		AGE_GETSET(scene_id, scene_id)
		AGE_GETSET(scene_id_next, scene_id_next)
		AGE_GET(h_input_ctx, h_input_ctx)
		AGE_GET(h_window, h_window)
	} i_loop;
}	 // namespace age_demo::game

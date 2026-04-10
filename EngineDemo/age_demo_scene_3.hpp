#pragma once
#include "age_demo.hpp"

namespace age_demo::scene_3
{
	struct
	{
		AGE_GET(render_pipeline, render_pipeline)

		AGE_GET(mesh_id_vec, scene_3_ctx.mesh_id_vec)
		AGE_GET(directional_light_id_vec, scene_3_ctx.directional_light_id_vec)

		AGE_GET(entities, scene_3_ctx.entities)
		AGE_GETSET(ent_main_cam, scene_3_ctx.ent_main_cam)

		AGE_SET(smoothed_move, scene_3_ctx.input.smoothed_move)
		AGE_SET(smoothed_look, scene_3_ctx.input.smoothed_look)
		AGE_SET(smoothed_zoom, scene_3_ctx.input.smoothed_zoom)
		AGE_SET(smoothed_pan, scene_3_ctx.input.smoothed_pan)
	} i_init;

	struct
	{
		AGE_GET(render_pipeline, render_pipeline)
		AGE_GET(h_render_surface, h_render_surface)
		AGE_GET(h_window, h_window)

		AGE_GET(entities, scene_3_ctx.entities)
		AGE_GET(mesh_id_vec, scene_3_ctx.mesh_id_vec)
		AGE_GET(directional_light_id_vec, scene_3_ctx.directional_light_id_vec)

		AGE_GET(sprint, sprint)
		AGE_GET(move, move)
		AGE_GET(zoom, zoom)
		AGE_GET(look, look)
		AGE_GET(right_mouse_down, right_mouse_down)
		AGE_GET(middle_mouse_down, middle_mouse_down)

		AGE_GETSET(smoothed_move, scene_3_ctx.input.smoothed_move)
		AGE_GETSET(smoothed_look, scene_3_ctx.input.smoothed_look)
		AGE_GETSET(smoothed_zoom, scene_3_ctx.input.smoothed_zoom)
		AGE_GETSET(smoothed_pan, scene_3_ctx.input.smoothed_pan)

		AGE_GETSET(game_focused, scene_3_ctx.game_focused)
	} i_update;

	struct
	{
		AGE_GET(render_pipeline, render_pipeline)

		AGE_GET(mesh_id_vec, scene_3_ctx.mesh_id_vec)
		AGE_GET(directional_light_id_vec, scene_3_ctx.directional_light_id_vec)

		AGE_GET(entities, scene_3_ctx.entities)
	} i_deinit;
}	 // namespace age_demo::scene_3

namespace age_demo::scene_3
{
	FORCE_INLINE decltype(auto)
	init() noexcept;

	FORCE_INLINE decltype(auto)
	update() noexcept;

	FORCE_INLINE decltype(auto)
	deinit() noexcept;
}	 // namespace age_demo::scene_3
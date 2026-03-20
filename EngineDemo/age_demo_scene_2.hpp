#include "age_demo.hpp"

namespace age_demo::scene_2
{
	struct
	{
		AGE_GET(render_pipeline, render_pipeline)

		AGE_GET(mesh_id_vec, scene_2_ctx.mesh_id_vec)
		AGE_GET(opaque_obj_id_vec, scene_2_ctx.opaque_obj_id_vec)
		AGE_GET(transparent_obj_id_vec, scene_2_ctx.transparent_obj_id_vec)
		AGE_GET(camera_id_vec, scene_2_ctx.camera_id_vec)
		AGE_GET(point_light_id_vec, scene_2_ctx.point_light_id_vec)
		AGE_GET(spot_light_id_vec, scene_2_ctx.spot_light_id_vec)
		AGE_GET(directional_light_id_vec, scene_2_ctx.directional_light_id_vec)

		AGE_SET(euler_x, scene_2_ctx.input.euler_x)
		AGE_SET(euler_y, scene_2_ctx.input.euler_y)
		AGE_SET(smoothed_move, scene_2_ctx.input.smoothed_move)
		AGE_SET(smoothed_look, scene_2_ctx.input.smoothed_look)
		AGE_SET(smoothed_zoom, scene_2_ctx.input.smoothed_zoom)
		AGE_SET(smoothed_pan, scene_2_ctx.input.smoothed_pan)
	} i_init;

	struct
	{
		AGE_GET(render_pipeline, render_pipeline)
		AGE_GET(h_render_surface, h_render_surface)
		AGE_GET(h_window, h_window)

		AGE_GET(sprint, sprint)
		AGE_GET(move, move)
		AGE_GET(zoom, zoom)
		AGE_GET(look, look)
		AGE_GET(right_mouse_down, right_mouse_down)
		AGE_GET(middle_mouse_down, middle_mouse_down)

		AGE_GET(mesh_id_vec, scene_2_ctx.mesh_id_vec)
		AGE_GET(opaque_obj_id_vec, scene_2_ctx.opaque_obj_id_vec)
		AGE_GET(transparent_obj_id_vec, scene_2_ctx.transparent_obj_id_vec)
		AGE_GET(camera_id_vec, scene_2_ctx.camera_id_vec)

		AGE_GETSET(euler_x, scene_2_ctx.input.euler_x)
		AGE_GETSET(euler_y, scene_2_ctx.input.euler_y)
		AGE_GETSET(smoothed_move, scene_2_ctx.input.smoothed_move)
		AGE_GETSET(smoothed_look, scene_2_ctx.input.smoothed_look)
		AGE_GETSET(smoothed_zoom, scene_2_ctx.input.smoothed_zoom)
		AGE_GETSET(smoothed_pan, scene_2_ctx.input.smoothed_pan)
	} i_update;

	struct
	{
		AGE_GET(render_pipeline, render_pipeline)

		AGE_GET(mesh_id_vec, scene_2_ctx.mesh_id_vec)
		AGE_GET(opaque_obj_id_vec, scene_2_ctx.opaque_obj_id_vec)
		AGE_GET(transparent_obj_id_vec, scene_2_ctx.transparent_obj_id_vec)
		AGE_GET(camera_id_vec, scene_2_ctx.camera_id_vec)
		AGE_GET(point_light_id_vec, scene_2_ctx.point_light_id_vec)
		AGE_GET(spot_light_id_vec, scene_2_ctx.spot_light_id_vec)
		AGE_GET(directional_light_id_vec, scene_2_ctx.directional_light_id_vec)
	} i_deinit;
}	 // namespace age_demo::scene_2
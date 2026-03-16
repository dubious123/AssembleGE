#pragma once
#include "age_demo.hpp"

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
	template <typename t>
	struct interface_input
	{
		t& data;
		AGE_GET_PROP(h_input_ctx)
		AGE_SET_PROP(move)
		AGE_SET_PROP(look)
		AGE_SET_PROP(zoom)
		AGE_SET_PROP(sprint)
		AGE_SET_PROP(right_mouse_down)
		AGE_SET_PROP(middle_mouse_down)
	};

	template <typename t>
	struct interface_scene
	{
		t& data;
		AGE_GET_PROP(scene_0_ctx)
		AGE_GET_PROP(scene_1_ctx)
		AGE_GET_PROP(scene_id_next)
		AGE_GETSET_PROP(scene_id)
	};

	template <typename t>
	struct interface_init
	{
		t& data;
		AGE_SET_PROP(scene_id)
		AGE_SET_PROP(scene_id_next)
		AGE_GETSET_PROP(h_window)
		AGE_SET_PROP(h_render_surface)
		AGE_SET_PROP(h_input_ctx)
		AGE_GET_PROP(render_pipeline)
	};

	template <typename t>
	struct interface_deinit
	{
		t& data;
		AGE_GET_PROP(scene_id)
		AGE_GET_PROP(render_pipeline)
	};

	template <typename t>
	struct interface_loop
	{
		t& data;
		AGE_GET_PROP(scene_id)
		AGE_SET_PROP(scene_id_next)
		AGE_GET_PROP(h_input_ctx)
		AGE_GET_PROP(h_window)
	};
}	 // namespace age_demo::game

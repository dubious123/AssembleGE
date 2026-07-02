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
		age::vector<age::asset::handle>
			mesh_id_vec = age::vector<age::asset::handle>::gen_reserved(1);

		age::vector<age::graphics::render_pipeline::t_object_id>
			obj_id_vec = age::vector<age::graphics::render_pipeline::t_object_id>::gen_reserved(27);

		age::vector<age::graphics::render_pipeline::t_camera_id>
			camera_id_vec = age::vector<age::graphics::render_pipeline::t_camera_id>::gen_reserved(1);

		age::vector<age::graphics::render_pipeline::t_unified_light_id>
			point_light_id_vec = age::vector<age::graphics::render_pipeline::t_unified_light_id>::gen_reserved(5000);

		age::vector<age::graphics::render_pipeline::t_unified_light_id>
			spot_light_id_vec = age::vector<age::graphics::render_pipeline::t_unified_light_id>::gen_reserved(1);

		age::vector<age::graphics::render_pipeline::t_directional_light_id>
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

namespace age_demo::scene_1
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
		age::vector<age::asset::handle>
			mesh_id_vec = age::vector<age::asset::handle>::gen_reserved(1);

		age::vector<age::graphics::render_pipeline::t_object_id>
			obj_id_vec = age::vector<age::graphics::render_pipeline::t_object_id>::gen_reserved(27);

		age::vector<age::graphics::render_pipeline::t_camera_id>
			camera_id_vec = age::vector<age::graphics::render_pipeline::t_camera_id>::gen_reserved(1);

		age::vector<age::graphics::render_pipeline::t_unified_light_id>
			point_light_id_vec = age::vector<age::graphics::render_pipeline::t_unified_light_id>::gen_reserved(5000);

		age::vector<age::graphics::render_pipeline::t_unified_light_id>
			spot_light_id_vec = age::vector<age::graphics::render_pipeline::t_unified_light_id>::gen_reserved(1);

		age::vector<age::graphics::render_pipeline::t_directional_light_id>
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


}	 // namespace age_demo::scene_1

namespace age_demo::scene_2
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
		age::vector<age::asset::handle>
			mesh_id_vec = age::vector<age::asset::handle>::gen_reserved(1);

		age::vector<age::graphics::render_pipeline::t_object_id>
			opaque_obj_id_vec = age::vector<age::graphics::render_pipeline::t_object_id>::gen_reserved(27);

		age::vector<age::graphics::render_pipeline::t_object_id>
			transparent_obj_id_vec = age::vector<age::graphics::render_pipeline::t_object_id>::gen_reserved(27);

		age::vector<age::graphics::render_pipeline::t_camera_id>
			camera_id_vec = age::vector<age::graphics::render_pipeline::t_camera_id>::gen_reserved(1);

		age::vector<age::graphics::render_pipeline::t_unified_light_id>
			point_light_id_vec = age::vector<age::graphics::render_pipeline::t_unified_light_id>::gen_reserved(5000);

		age::vector<age::graphics::render_pipeline::t_unified_light_id>
			spot_light_id_vec = age::vector<age::graphics::render_pipeline::t_unified_light_id>::gen_reserved(1);

		age::vector<age::graphics::render_pipeline::t_directional_light_id>
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
}	 // namespace age_demo::scene_2

namespace age_demo::scene_3
{
	namespace input::g
	{
		inline constexpr auto move_speed	= 2.f;
		inline constexpr auto sprint_mult	= 4.f;
		inline constexpr auto sensitivity	= 0.17f;
		inline constexpr auto zoom_speed	= 2.f;
		inline constexpr auto zoom_distance = 4.f;
		inline constexpr auto pan_speed		= 0.6f;

		inline constexpr auto move_smoothing = 15.f / 2.f;
		inline constexpr auto look_smoothing = 25.f / 2.f;
		inline constexpr auto zoom_smoothing = 12.f / 2.f;
	}	 // namespace input::g

	using t_ent_storage_main = age::ecs::entity_storage::basic<uint32,
															   age::ecs::position,
															   age::ecs::rotation,
															   age::ecs::scale,
															   age::ecs::camera,
															   age::ecs::directional_light,
															   age::ecs::point_light,
															   age::ecs::spot_light,
															   age::ecs::render_object,
															   age::ecs::mesh,
															   age::ecs::material,
															   age::ecs::env_light,
															   age::ecs::bloom,
															   age::ecs::gi_config,
															   age::ecs::editor_cam_setting,
															   age::ecs::ao_config,
															   age::ecs::aa_config>;

	using t_ent_storage_sub = age::ecs::entity_storage::basic<uint32,
															  age::ecs::position,
															  age::ecs::rotation,
															  age::ecs::scale,
															  age::ecs::camera,
															  age::ecs::directional_light,
															  age::ecs::point_light,
															  age::ecs::spot_light,
															  age::ecs::render_object,
															  age::ecs::mesh,
															  age::ecs::material>;

	struct editor_demo_scene_0
	{
		// init, deinit, visit_storage_at, storages, storages_names, storage_count

		AGE_EDITOR_SCENE_ENTITY_STORAGES((t_ent_storage_main, ent_storage_main, "storage_main"),
										 (t_ent_storage_sub, ent_storage_sub, "storage_sub"));
	};

	struct editor_demo_scene_1
	{
		AGE_EDITOR_SCENE_ENTITY_STORAGES((t_ent_storage_main, ent_storage_main, "storage_main"),
										 (t_ent_storage_sub, ent_storage_sub, "storage_sub"));
	};

	struct editor_demo_game
	{
		// scenes, scene_names, scene_count, init, deinit, visit_scene_at, visit_storage_at

		AGE_EDITOR_GAME_NAME("editor_demo_game", "demo", "first game", "test");

		AGE_EDITOR_GAME_SCENES((editor_demo_scene_0, editor_scene_0, "scene_0", "scene_zero"),
							   (editor_demo_scene_1, editor_scene_1, "scene_1", "scene_one"));
	};

	struct ctx
	{
		editor_demo_game editor_game;

		uint32 ent_main_cam;

		// input
		struct
		{
			float2 smoothed_move = float2{ 0.f, 0.f };
			float2 smoothed_look = float2{ 0.f, 0.f };
			float  smoothed_zoom = 0.f;
			float2 smoothed_pan	 = float2{ 0.f, 0.f };
		} input;
	};
}	 // namespace age_demo::scene_3

namespace age_demo::global
{
	struct ctx
	{
		age::platform::window_handle		 h_window;
		age::graphics::render_surface_handle h_render_surface;
		age::input::context_handle			 h_input_ctx;

		age::graphics::render_pipeline::hybrid_pipeline render_pipeline;

		uint32 scene_id;
		uint32 scene_id_next;

		uint64 frame_count;

		// input
		float2 move;
		float2 look;
		float  zoom;
		bool   sprint;
		bool   right_mouse_down;
		bool   middle_mouse_down;

		age_demo::scene_0::ctx scene_0_ctx;

		age_demo::scene_1::ctx scene_1_ctx;

		age_demo::scene_2::ctx scene_2_ctx;

		age_demo::scene_3::ctx scene_3_ctx;
	};

	namespace detail
	{
		inline global::ctx ctx;
	}
}	 // namespace age_demo::global
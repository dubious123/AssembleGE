#pragma once
#include "age.hpp"

namespace demo_game
{
	struct demo_game_ctx
	{
		age::platform::window_handle		 h_window;
		age::graphics::render_surface_handle h_render_surface;
		age::input::context_handle			 h_input_ctx;

		age::graphics::render_pipeline::forward_plus::pipeline render_pipeline;

		uint32 scene_id		 = invalid_id_uint32;
		uint32 scene_id_next = 0;

		struct
		{
			float2 move;
			float2 look;
			float  zoom;
			bool   sprint;
			bool   right_mouse_down;
			bool   middle_mouse_down;
		} input;

		struct
		{
			age::vector<age::graphics::render_pipeline::forward_plus::t_mesh_id>
				mesh_id_vec = age::vector<age::graphics::render_pipeline::forward_plus::t_mesh_id>::gen_reserved(1);

			age::vector<age::graphics::render_pipeline::forward_plus::t_object_id>
				obj_id_vec = age::vector<age::graphics::render_pipeline::forward_plus::t_object_id>::gen_reserved(27);

			age::vector<age::graphics::render_pipeline::forward_plus::t_camera_id>
				camera_vec = age::vector<age::graphics::render_pipeline::forward_plus::t_camera_id>::gen_reserved(1);

			age::vector<age::graphics::render_pipeline::forward_plus::t_unified_light_id>
				point_light_id_vec = age::vector<age::graphics::render_pipeline::forward_plus::t_unified_light_id>::gen_reserved(5000);

			age::vector<age::graphics::render_pipeline::forward_plus::t_unified_light_id>
				spot_light_id_vec = age::vector<age::graphics::render_pipeline::forward_plus::t_unified_light_id>::gen_reserved(1);

			age::vector<age::graphics::render_pipeline::forward_plus::t_directional_light_id>
				directional_light_id_vec;

			struct
			{
				static constexpr auto move_speed	= 2.f;
				static constexpr auto sprint_mult	= 4.f;
				static constexpr auto sensitivity	= 0.003f;
				static constexpr auto zoom_speed	= 2.f;
				static constexpr auto zoom_distance = 4.f;
				static constexpr auto pan_speed		= 0.6f;

				static constexpr auto move_smoothing = 15.f / 2.f;
				static constexpr auto look_smoothing = 25.f / 2.f;
				static constexpr auto zoom_smoothing = 12.f / 2.f;

				float euler_x = 0.f;
				float euler_y = 0.f;

				float2 smoothed_move = float2{ 0.f, 0.f };
				float2 smoothed_look = float2{ 0.f, 0.f };
				float  smoothed_zoom = 0.f;
				float2 smoothed_pan	 = float2{ 0.f, 0.f };
			} input;
		} scene_0;

		struct
		{
			age::vector<age::graphics::render_pipeline::forward_plus::t_mesh_id>
				mesh_id_vec = age::vector<age::graphics::render_pipeline::forward_plus::t_mesh_id>::gen_reserved(1);

			age::vector<age::graphics::render_pipeline::forward_plus::t_object_id>
				obj_id_vec = age::vector<age::graphics::render_pipeline::forward_plus::t_object_id>::gen_reserved(27);

			age::vector<age::graphics::render_pipeline::forward_plus::t_camera_id>
				camera_vec = age::vector<age::graphics::render_pipeline::forward_plus::t_camera_id>::gen_reserved(1);

			age::vector<age::graphics::render_pipeline::forward_plus::t_unified_light_id>
				point_light_id_vec = age::vector<age::graphics::render_pipeline::forward_plus::t_unified_light_id>::gen_reserved(5000);

			age::vector<age::graphics::render_pipeline::forward_plus::t_unified_light_id>
				spot_light_id_vec = age::vector<age::graphics::render_pipeline::forward_plus::t_unified_light_id>::gen_reserved(1);

			age::vector<age::graphics::render_pipeline::forward_plus::t_directional_light_id>
				directional_light_id_vec;

			struct
			{
				static constexpr auto move_speed	= 2.f;
				static constexpr auto sprint_mult	= 4.f;
				static constexpr auto sensitivity	= 0.003f;
				static constexpr auto zoom_speed	= 2.f;
				static constexpr auto zoom_distance = 4.f;
				static constexpr auto pan_speed		= 0.6f;

				static constexpr auto move_smoothing = 15.f / 2.f;
				static constexpr auto look_smoothing = 25.f / 2.f;
				static constexpr auto zoom_smoothing = 12.f / 2.f;

				float euler_x = 0.f;
				float euler_y = 0.f;

				float2 smoothed_move = float2{ 0.f, 0.f };
				float2 smoothed_look = float2{ 0.f, 0.f };
				float  smoothed_zoom = 0.f;
				float2 smoothed_pan	 = float2{ 0.f, 0.f };
			} input;
		} scene_1;
	};

	inline demo_game_ctx game_ctx;
}	 // namespace demo_game

namespace demo_game::scene_0
{
	FORCE_INLINE decltype(auto)
	init() noexcept
	{
		using namespace age::ecs::system;
		{
			auto& render_pipeline		   = game_ctx.render_pipeline;
			auto& mesh_id_vec			   = game_ctx.scene_0.mesh_id_vec;
			auto& obj_id_vec			   = game_ctx.scene_0.obj_id_vec;
			auto& camera_vec			   = game_ctx.scene_0.camera_vec;
			auto& unified_light_id_vec	   = game_ctx.scene_0.point_light_id_vec;
			auto& directional_light_id_vec = game_ctx.scene_0.directional_light_id_vec;
		}

		on_ctx{
			AGE_LAMBDA(
				(),
				{
					game_ctx.scene_0.input.euler_x = 0.f;
					game_ctx.scene_0.input.euler_y = 0.f;

					game_ctx.scene_0.input.smoothed_move = float2{ 0.f, 0.f };
					game_ctx.scene_0.input.smoothed_look = float2{ 0.f, 0.f };
					game_ctx.scene_0.input.smoothed_zoom = 0.f;
					game_ctx.scene_0.input.smoothed_pan	 = float2{ 0.f, 0.f };
				}),

			identity{ age::graphics::render_pipeline::forward_plus::camera_desc{
				.kind		= age::graphics::e::camera_kind::perspective,
				.pos		= float3{ 0.f, 0.5f, -4.f },
				.quaternion = age::g::quaternion_identity,
				.near_z		= 0.01f,
				.far_z		= 100.f,
				.perspective{
					.fov_y		  = age::cvt_to_radian(75.f),
					.aspect_ratio = 16.f / 9.f } } }
				| AGE_FUNC(game_ctx.render_pipeline.add_camera)
				| AGE_FUNC(game_ctx.scene_0.camera_vec.emplace_back),

			identity{ age::graphics::render_pipeline::forward_plus::directional_light_desc{
				.direction = age::normalize(float3{ 1.0f, -1.0f, 0.5f }),
				.intensity = 0.3f,
				.color	   = float3{ 1.0f, 0.95f, 0.85f } } }
				| AGE_FUNC(game_ctx.render_pipeline.add_directional_light)
				| AGE_FUNC(game_ctx.scene_0.directional_light_id_vec.emplace_back),

			AGE_LAMBDA(
				(),
				{
					constexpr uint32 light_count = 5000;
					constexpr float	 scene_min	 = -15.0f;
					constexpr float	 scene_max	 = 15.0f;
					constexpr float	 range		 = 6.0f;
					constexpr float	 intensity	 = 0.3f;

					auto rng		= std::mt19937{ 42 };
					auto dist_pos	= std::uniform_real_distribution<float>{ scene_min, scene_max };
					auto dist_color = std::uniform_real_distribution<float>{ 0.2f, 1.0f };

					for (auto i = 0; i < light_count; ++i)
					{
						game_ctx.scene_0.point_light_id_vec.emplace_back(
							game_ctx.render_pipeline.add_point_light(
								age::graphics::render_pipeline::forward_plus::point_light_desc{
									.position = float3{ dist_pos(rng), dist_pos(rng), dist_pos(rng) },
									.range	  = range,
									.color	  = float3{ dist_color(rng), dist_color(rng), dist_color(rng) },
									//.color	   = float3{ 1, 1, 1 },
									.intensity = intensity }));
					}
				}),

			// yellow spot top down onto scene center
			// identity{ age::graphics::render_pipeline::forward_plus::spot_light_desc{
			//	.position  = float3{ 0.0f, 12.0f, 0.0f },
			//	.range	   = 6.0f,
			//	.direction = float3{ 0.0f, -1.0f, 0.0f },
			//	.intensity = 10.0f,
			//	.color	   = float3{ 1.0f, 0.9f, 0.6f },
			//	.cos_inner = 0.96f,
			//	.cos_outer = 0.87f } }
			//	| AGE_FUNC(game_ctx.render_pipeline.add_spot_light)
			//	| AGE_FUNC(game_ctx.scene_0.spot_light_id_vec.emplace_back),

			identity{ age::asset::primitive_desc{
				.size	   = { 0.5, 0.5, 0.5 },
				.seg_u	   = 30,
				.seg_v	   = 30,
				.mesh_kind = age::asset::e::primitive_mesh_kind::cube } }
				| age::asset::create_primitive_mesh
				| age::asset::bake_mesh<age::asset::vertex_pnt_uv1>
				| AGE_FUNC(game_ctx.render_pipeline.upload_mesh)
				| AGE_FUNC(game_ctx.scene_0.mesh_id_vec.emplace_back),

			identity{ age::asset::primitive_desc{
				.size	   = { 0.5, 0.5, 0.5 },
				.seg_u	   = 30,
				.seg_v	   = 30,
				.mesh_kind = age::asset::e::primitive_mesh_kind::plane } }
				| age::asset::create_primitive_mesh
				| age::asset::bake_mesh<age::asset::vertex_pnt_uv1>
				| AGE_FUNC(game_ctx.render_pipeline.upload_mesh)
				| AGE_FUNC(game_ctx.scene_0.mesh_id_vec.emplace_back),

			identity{ age::asset::primitive_desc{
				.size	   = { 0.5, 0.5, 0.5 },
				.seg_u	   = 30,
				.seg_v	   = 30,
				.mesh_kind = age::asset::e::primitive_mesh_kind::cube_sphere } }
				| age::asset::create_primitive_mesh
				| age::asset::bake_mesh<age::asset::vertex_pnt_uv1>
				| AGE_FUNC(game_ctx.render_pipeline.upload_mesh)
				| AGE_FUNC(game_ctx.scene_0.mesh_id_vec.emplace_back),


			AGE_LAMBDA(
				(),
				{
					for (auto&& [pos_x, pos_y, pos_z] : std::views::cartesian_product(std::views::iota(-5, 5), std::views::iota(-5, 5), std::views::iota(-5, 5)))
					{
						auto data = age::graphics::render_pipeline::forward_plus::shared_type::object_data{
							.pos		= float3{ pos_x * 2, pos_y * 2, pos_z * 2 },
							.quaternion = age::math::quaternion_encode(age::g::quaternion_identity),
							.scale		= age::cvt_to<half3>(float3{ 1.0f, 1.0f, 1.0f })
						};

						game_ctx.scene_0.obj_id_vec.emplace_back(game_ctx.render_pipeline.add_object(data));
					}
				}),
			exec_inline{}
		}();
	}

	FORCE_INLINE decltype(auto)
	update() noexcept
	{
		auto& game_input  = game_ctx.input;
		auto& camera_vec  = game_ctx.scene_0.camera_vec;
		auto& obj_id_vec  = game_ctx.scene_0.obj_id_vec;
		auto& mesh_id_vec = game_ctx.scene_0.mesh_id_vec;

		auto& input = game_ctx.scene_0.input;

		c_auto dt_s = std::max(
			age::global::get<age::runtime::interface>().delta_time_s(),
			1.f / 160);

		c_auto speed	= game_input.sprint ? input.move_speed * input.sprint_mult : input.move_speed;
		auto   cam_desc = game_ctx.render_pipeline.get_camera_desc(camera_vec[0]);


		c_auto move_smoothing_factor = 1.f - std::exp(-input.move_smoothing * dt_s);
		c_auto look_smoothing_factor = 1.f - std::exp(-input.look_smoothing * dt_s);
		c_auto zoom_smoothing_factor = 1.f - std::exp(-input.zoom_smoothing * dt_s);

		input.smoothed_move = age::math::lerp(input.smoothed_move, game_input.move, move_smoothing_factor);
		input.smoothed_zoom = age::math::lerp(input.smoothed_zoom, game_input.zoom, zoom_smoothing_factor);

		auto look_target	= game_input.right_mouse_down ? game_input.look : float2{ 0.f, 0.f };
		input.smoothed_look = age::math::lerp(input.smoothed_look, look_target, look_smoothing_factor);

		auto pan_target	   = game_input.middle_mouse_down ? game_input.look : float2{ 0.f, 0.f };
		input.smoothed_pan = age::math::lerp(input.smoothed_pan, pan_target, look_smoothing_factor);


		input.euler_y += input.smoothed_look.x * input.sensitivity;
		input.euler_x += input.smoothed_look.y * input.sensitivity;
		input.euler_x  = std::clamp(input.euler_x, -89.f * age::g::degree_to_radian, 89.f * age::g::degree_to_radian);

		c_auto xm_look_quat = float3{ input.euler_x, input.euler_y, 0.f }
							| age::simd::load()
							| age::simd::euler_to_quat();

		c_auto forward = age::simd::g::xm_forward_f4
					   | age::simd::rotate3(xm_look_quat)
					   | age::simd::to<float3>();
		c_auto right   = age::simd::g::xm_right_f4
					   | age::simd::rotate3(xm_look_quat)
					   | age::simd::to<float3>();
		c_auto up	   = age::simd::g::xm_up_f4
					   | age::simd::rotate3(xm_look_quat)
					   | age::simd::to<float3>();

		cam_desc.pos -= right * input.smoothed_pan.x * input.pan_speed * dt_s;
		cam_desc.pos += up * input.smoothed_pan.y * input.pan_speed * dt_s;
		cam_desc.pos += forward * input.smoothed_zoom * input.zoom_speed;
		cam_desc.pos += (right * input.smoothed_move.x + forward * input.smoothed_move.y) * speed * dt_s;

		cam_desc.quaternion = xm_look_quat
							| age::simd::to<float4>();

		game_ctx.render_pipeline.update_camera(camera_vec[0], cam_desc);

		if (game_ctx.render_pipeline.begin_render(game_ctx.h_render_surface))
		{
			for (auto obj_id : obj_id_vec)
			{
				game_ctx.render_pipeline.render_mesh(obj_id % age::graphics::g::thread_count, obj_id, mesh_id_vec[obj_id % mesh_id_vec.size()]);
			}

			game_ctx.render_pipeline.end_render(game_ctx.h_render_surface);
		}
	}

	FORCE_INLINE decltype(auto)
	deinit() noexcept
	{
		for (auto o_id : game_ctx.scene_0.obj_id_vec)
		{
			game_ctx.render_pipeline.remove_object(o_id);
		}

		for (auto m_id : game_ctx.scene_0.mesh_id_vec)
		{
			game_ctx.render_pipeline.release_mesh(m_id);
		}

		for (auto c_id : game_ctx.scene_0.camera_vec)
		{
			game_ctx.render_pipeline.remove_camera(c_id);
		}

		for (auto l_id : game_ctx.scene_0.point_light_id_vec)
		{
			game_ctx.render_pipeline.remove_point_light(l_id);
		}

		for (auto l_id : game_ctx.scene_0.spot_light_id_vec)
		{
			game_ctx.render_pipeline.remove_spot_light(l_id);
		}

		for (auto d_id : game_ctx.scene_0.directional_light_id_vec)
		{
			game_ctx.render_pipeline.remove_directional_light(d_id);
		}

		game_ctx.scene_0.obj_id_vec.clear();
		game_ctx.scene_0.mesh_id_vec.clear();
		game_ctx.scene_0.camera_vec.clear();
		game_ctx.scene_0.point_light_id_vec.clear();
		game_ctx.scene_0.spot_light_id_vec.clear();
		game_ctx.scene_0.directional_light_id_vec.clear();
	}
}	 // namespace demo_game::scene_0

namespace demo_game::scene_1
{
	FORCE_INLINE decltype(auto)
	init() noexcept
	{
		using namespace age::ecs::system;
		{
			auto& render_pipeline		   = game_ctx.render_pipeline;
			auto& mesh_id_vec			   = game_ctx.scene_1.mesh_id_vec;
			auto& obj_id_vec			   = game_ctx.scene_1.obj_id_vec;
			auto& camera_vec			   = game_ctx.scene_1.camera_vec;
			auto& unified_light_id_vec	   = game_ctx.scene_1.point_light_id_vec;
			auto& directional_light_id_vec = game_ctx.scene_1.directional_light_id_vec;
		}
		on_ctx{
			AGE_LAMBDA(
				(),
				{
					game_ctx.scene_1.input.euler_x = 0.f;
					game_ctx.scene_1.input.euler_y = 0.f;

					game_ctx.scene_1.input.smoothed_move = float2{ 0.f, 0.f };
					game_ctx.scene_1.input.smoothed_look = float2{ 0.f, 0.f };
					game_ctx.scene_1.input.smoothed_zoom = 0.f;
					game_ctx.scene_1.input.smoothed_pan	 = float2{ 0.f, 0.f };
				}),

			identity{ age::graphics::render_pipeline::forward_plus::camera_desc{
				.kind		= age::graphics::e::camera_kind::perspective,
				.pos		= float3{ 0.f, 6.f, -10.f },
				.quaternion = age::g::quaternion_identity,
				.near_z		= 0.01f,
				.far_z		= 100.f,
				.perspective{
					.fov_y		  = age::cvt_to_radian(75.f),
					.aspect_ratio = 16.f / 9.f } } }
				| AGE_FUNC(game_ctx.render_pipeline.add_camera)
				| AGE_FUNC(game_ctx.scene_1.camera_vec.emplace_back),

			// dim ambient directional light
			identity{ age::graphics::render_pipeline::forward_plus::directional_light_desc{
				.direction = age::normalize(float3{ 0.0f, -1.0f, 0.0f }),
				.intensity = 0.05f,
				.color	   = float3{ 1.0f, 1.0f, 1.0f } } }
				| AGE_FUNC(game_ctx.render_pipeline.add_directional_light)
				| AGE_FUNC(game_ctx.scene_1.directional_light_id_vec.emplace_back),

			// AGE_LAMBDA(
			//	(),
			//	{
			//		constexpr uint32 light_count = 1000;
			//		constexpr float	 scene_min	 = -15.0f;
			//		constexpr float	 scene_max	 = 15.0f;
			//		constexpr float	 range		 = 6.0f;
			//		constexpr float	 intensity	 = 0.3f;

			//		auto rng		= std::mt19937{ 42 };
			//		auto dist_pos	= std::uniform_real_distribution<float>{ scene_min, scene_max };
			//		auto dist_color = std::uniform_real_distribution<float>{ 0.2f, 1.0f };

			//		for (auto i = 0; i < light_count; ++i)
			//		{
			//			game_ctx.scene_1.point_light_id_vec.emplace_back(
			//				game_ctx.render_pipeline.add_point_light(
			//					age::graphics::render_pipeline::forward_plus::point_light_desc{
			//						.position = float3{ dist_pos(rng), dist_pos(rng), dist_pos(rng) },
			//						.range	  = range,
			//						.color	  = float3{ dist_color(rng), dist_color(rng), dist_color(rng) },
			//						//.color	   = float3{ 1, 1, 1 },
			//						.intensity = intensity }));
			//		}
			//	}),

			// === point lights - distinct colors for shadow identification ===

			// red light - left side, low
			identity{ age::graphics::render_pipeline::forward_plus::point_light_desc{
				.position  = float3{ -3.0f, 2.0f, 0.0f },
				.range	   = 15.0f,
				.color	   = float3{ 1.0f, 0.2f, 0.2f },
				.intensity = 3.0f } }
				//| AGE_LAMBDA((auto&& desc), { return game_ctx.render_pipeline.add_point_light(desc, false); })
				//| AGE_LAMBDA((auto&& desc), { return game_ctx.render_pipeline.add_point_light(desc, false); })
				| AGE_FUNC(game_ctx.render_pipeline.add_point_light)
				| AGE_FUNC(game_ctx.scene_1.point_light_id_vec.emplace_back),

			// green light - right side, mid height
			identity{ age::graphics::render_pipeline::forward_plus::point_light_desc{
				.position  = float3{ 3.0f, 4.0f, 0.0f },
				.range	   = 15.0f,
				.color	   = float3{ 0.2f, 1.0f, 0.2f },
				.intensity = 3.0f } }
				| AGE_FUNC(game_ctx.render_pipeline.add_point_light)
				| AGE_FUNC(game_ctx.scene_1.point_light_id_vec.emplace_back),

			// blue light - center, high above
			identity{ age::graphics::render_pipeline::forward_plus::point_light_desc{
				.position  = float3{ 0.0f, 6.0f, 2.0f },
				.range	   = 15.0f,
				.color	   = float3{ 0.3f, 0.3f, 1.0f },
				.intensity = 3.0f } }
				| AGE_FUNC(game_ctx.render_pipeline.add_point_light)
				| AGE_FUNC(game_ctx.scene_1.point_light_id_vec.emplace_back),

			// === meshes ===

			// [0] cube
			identity{ age::asset::primitive_desc{
				.size	   = { 0.5, 0.5, 0.5 },
				.seg_u	   = 30,
				.seg_v	   = 30,
				.mesh_kind = age::asset::e::primitive_mesh_kind::cube } }
				| age::asset::create_primitive_mesh
				| age::asset::bake_mesh<age::asset::vertex_pnt_uv1>
				| AGE_FUNC(game_ctx.render_pipeline.upload_mesh)
				| AGE_FUNC(game_ctx.scene_1.mesh_id_vec.emplace_back),

			// [1] plane
			identity{ age::asset::primitive_desc{
				.size	   = { 0.5, 0.5, 0.5 },
				.seg_u	   = 30,
				.seg_v	   = 30,
				.mesh_kind = age::asset::e::primitive_mesh_kind::plane } }
				| age::asset::create_primitive_mesh
				| age::asset::bake_mesh<age::asset::vertex_pnt_uv1>
				| AGE_FUNC(game_ctx.render_pipeline.upload_mesh)
				| AGE_FUNC(game_ctx.scene_1.mesh_id_vec.emplace_back),

			// [2] sphere
			identity{ age::asset::primitive_desc{
				.size	   = { 0.5, 0.5, 0.5 },
				.seg_u	   = 30,
				.seg_v	   = 30,
				.mesh_kind = age::asset::e::primitive_mesh_kind::cube_sphere } }
				| age::asset::create_primitive_mesh
				| age::asset::bake_mesh<age::asset::vertex_pnt_uv1>
				| AGE_FUNC(game_ctx.render_pipeline.upload_mesh)
				| AGE_FUNC(game_ctx.scene_1.mesh_id_vec.emplace_back),

			AGE_LAMBDA(
				(),
				{
					auto add_obj = [&](float3 pos, float3 scale, float4 quat = age::g::quaternion_identity) {
						game_ctx.scene_1.obj_id_vec.emplace_back(
							game_ctx.render_pipeline.add_object(
								age::graphics::render_pipeline::forward_plus::shared_type::object_data{
									.pos		= pos,
									.quaternion = age::math::quaternion_encode(quat),
									.scale		= age::cvt_to<half3>(scale) }));
					};

					// ground plane - large, receives all shadows
					add_obj(float3{ 0.0f, -0.5f, 0.0f }, float3{ 40.0f, 1.0f, 40.0f });

					// back wall - catches shadows from behind
					add_obj(float3{ 0.0f, 4.0f, 8.0f }, float3{ 20.0f, 10.0f, 0.5f });

					// tall pillar - casts long shadow in all directions
					add_obj(float3{ 0.0f, 2.0f, 0.0f }, float3{ 0.5f, 4.0f, 0.5f });

					// small cube near red light - close shadow, sharp edge
					add_obj(float3{ -1.5f, 0.5f, 0.0f }, float3{ 1.0f, 1.0f, 1.0f });

					// small cube near green light - close shadow from other side
					add_obj(float3{ 1.5f, 0.5f, 0.0f }, float3{ 1.0f, 1.0f, 1.0f });

					// floating cube - shadow on ground with gap (tests detached shadow)
					add_obj(float3{ 0.0f, 3.0f, -2.0f }, float3{ 1.5f, 1.5f, 1.5f });

					// sphere - smooth shadow silhouette
					add_obj(float3{ -2.5f, 0.5f, 3.0f }, float3{ 2.0f, 2.0f, 2.0f });

					// sphere - overlapping shadow test (close to pillar)
					add_obj(float3{ 1.0f, 0.5f, 1.5f }, float3{ 1.5f, 1.5f, 1.5f });

					// flat box on ground - self-shadow / contact shadow test
					add_obj(float3{ 3.0f, 0.1f, -2.0f }, float3{ 2.0f, 0.2f, 2.0f });
				}),
			exec_inline{}
		}();
	}

	FORCE_INLINE decltype(auto)
	update() noexcept
	{
		auto& game_input  = game_ctx.input;
		auto& camera_vec  = game_ctx.scene_1.camera_vec;
		auto& obj_id_vec  = game_ctx.scene_1.obj_id_vec;
		auto& mesh_id_vec = game_ctx.scene_1.mesh_id_vec;

		auto& input = game_ctx.scene_1.input;

		c_auto dt_s = std::max(
			age::global::get<age::runtime::interface>().delta_time_s(),
			1.f / 160);

		c_auto speed	= game_input.sprint ? input.move_speed * input.sprint_mult : input.move_speed;
		auto   cam_desc = game_ctx.render_pipeline.get_camera_desc(camera_vec[0]);


		c_auto move_smoothing_factor = 1.f - std::exp(-input.move_smoothing * dt_s);
		c_auto look_smoothing_factor = 1.f - std::exp(-input.look_smoothing * dt_s);
		c_auto zoom_smoothing_factor = 1.f - std::exp(-input.zoom_smoothing * dt_s);

		input.smoothed_move = age::math::lerp(input.smoothed_move, game_input.move, move_smoothing_factor);
		input.smoothed_zoom = age::math::lerp(input.smoothed_zoom, game_input.zoom, zoom_smoothing_factor);

		auto look_target	= game_input.right_mouse_down ? game_input.look : float2{ 0.f, 0.f };
		input.smoothed_look = age::math::lerp(input.smoothed_look, look_target, look_smoothing_factor);

		auto pan_target	   = game_input.middle_mouse_down ? game_input.look : float2{ 0.f, 0.f };
		input.smoothed_pan = age::math::lerp(input.smoothed_pan, pan_target, look_smoothing_factor);


		input.euler_y += input.smoothed_look.x * input.sensitivity;
		input.euler_x += input.smoothed_look.y * input.sensitivity;
		input.euler_x  = std::clamp(input.euler_x, -89.f * age::g::degree_to_radian, 89.f * age::g::degree_to_radian);

		c_auto xm_look_quat = float3{ input.euler_x, input.euler_y, 0.f }
							| age::simd::load()
							| age::simd::euler_to_quat();

		c_auto forward = age::simd::g::xm_forward_f4
					   | age::simd::rotate3(xm_look_quat)
					   | age::simd::to<float3>();
		c_auto right   = age::simd::g::xm_right_f4
					   | age::simd::rotate3(xm_look_quat)
					   | age::simd::to<float3>();
		c_auto up	   = age::simd::g::xm_up_f4
					   | age::simd::rotate3(xm_look_quat)
					   | age::simd::to<float3>();

		cam_desc.pos -= right * input.smoothed_pan.x * input.pan_speed * dt_s;
		cam_desc.pos += up * input.smoothed_pan.y * input.pan_speed * dt_s;
		cam_desc.pos += forward * input.smoothed_zoom * input.zoom_speed;
		cam_desc.pos += (right * input.smoothed_move.x + forward * input.smoothed_move.y) * speed * dt_s;

		cam_desc.quaternion = xm_look_quat
							| age::simd::to<float4>();

		game_ctx.render_pipeline.update_camera(camera_vec[0], cam_desc);

		if (game_ctx.render_pipeline.begin_render(game_ctx.h_render_surface))
		{
			for (auto obj_id : obj_id_vec)
			{
				game_ctx.render_pipeline.render_mesh(obj_id % age::graphics::g::thread_count, obj_id, mesh_id_vec[obj_id % mesh_id_vec.size()]);
			}

			game_ctx.render_pipeline.end_render(game_ctx.h_render_surface);
		}
	}

	FORCE_INLINE decltype(auto)
	deinit() noexcept
	{
		for (auto o_id : game_ctx.scene_1.obj_id_vec)
		{
			game_ctx.render_pipeline.remove_object(o_id);
		}

		for (auto m_id : game_ctx.scene_1.mesh_id_vec)
		{
			game_ctx.render_pipeline.release_mesh(m_id);
		}

		for (auto c_id : game_ctx.scene_1.camera_vec)
		{
			game_ctx.render_pipeline.remove_camera(c_id);
		}

		for (auto l_id : game_ctx.scene_1.point_light_id_vec)
		{
			game_ctx.render_pipeline.remove_point_light(l_id);
		}

		for (auto l_id : game_ctx.scene_1.spot_light_id_vec)
		{
			game_ctx.render_pipeline.remove_spot_light(l_id);
		}

		for (auto d_id : game_ctx.scene_1.directional_light_id_vec)
		{
			game_ctx.render_pipeline.remove_directional_light(d_id);
		}

		game_ctx.scene_1.obj_id_vec.clear();
		game_ctx.scene_1.mesh_id_vec.clear();
		game_ctx.scene_1.camera_vec.clear();
		game_ctx.scene_1.point_light_id_vec.clear();
		game_ctx.scene_1.spot_light_id_vec.clear();
		game_ctx.scene_1.directional_light_id_vec.clear();
	}
}	 // namespace demo_game::scene_1

namespace demo_game
{
	FORCE_INLINE void
	update_input() noexcept
	{
		using namespace age::input;

		c_auto& input_ctx = *game_ctx.h_input_ctx;

		game_ctx.input.move = float2{
			input_ctx.is_down(e::key_kind::key_d) - input_ctx.is_down(e::key_kind::key_a),
			input_ctx.is_down(e::key_kind::key_w) - input_ctx.is_down(e::key_kind::key_s),
		};

		game_ctx.input.look				 = input_ctx.mouse_delta;
		game_ctx.input.zoom				 = input_ctx.wheel_delta;
		game_ctx.input.sprint			 = input_ctx.is_down(e::key_kind::key_shift);
		game_ctx.input.right_mouse_down	 = input_ctx.is_down(e::key_kind::mouse_right);
		game_ctx.input.middle_mouse_down = input_ctx.is_down(e::key_kind::mouse_middle);
	}

	FORCE_INLINE void
	handle_scene_change() noexcept
	{
		switch (game_ctx.scene_id)
		{
		case 0:
		{
			scene_0::deinit();
			break;
		}
		case 1:
		{
			scene_1::deinit();
			break;
		}
		case invalid_id_uint32:
		{
			break;
		}
		default:
		{
			AGE_UNREACHABLE();
		}
		}

		switch (game_ctx.scene_id_next)
		{
		case 0:
		{
			scene_0::init();
			break;
		}
		case 1:
		{
			scene_1::init();
			break;
		}
		case invalid_id_uint32:
		{
			break;
		}
		default:
		{
			AGE_UNREACHABLE();
		}
		}

		game_ctx.scene_id = game_ctx.scene_id_next;
	}

	constexpr FORCE_INLINE bool
	scene_changed() noexcept
	{
		return game_ctx.scene_id != game_ctx.scene_id_next;
	}

	constexpr FORCE_INLINE decltype(auto)
	get_sys_init() noexcept
	{
		using namespace age::ecs::system;
		return on_ctx{
			AGE_LAMBDA((), {
				game_ctx.scene_id	   = invalid_id_uint32;
				game_ctx.scene_id_next = 0;
			}),

			AGE_FUNC(game_ctx.render_pipeline.init),

			identity{ age::platform::window_desc{ 1080 * 2, 920 * 2, "test_render_surface" } } | age::platform::create_window | age::runtime::assign_to(game_ctx.h_window) | age::graphics::create_render_surface | age::runtime::assign_to(game_ctx.h_render_surface),

			AGE_FUNC(age::input::create_context) | age::runtime::assign_to(game_ctx.h_input_ctx) | AGE_LAMBDA((), { age::platform::register_input_context(game_ctx.h_window, game_ctx.h_input_ctx); }),

			loop{ [warm_up_frame = 10] mutable { return --warm_up_frame > 0; },
				  age::platform::update,	// pump platform msg
				  age::runtime::update, age::graphics::begin_frame, age::graphics::end_frame },
			exec_inline{}
		};
	}

	constexpr FORCE_INLINE decltype(auto)
	print_frame_rate() noexcept
	{
		static uint32 frame_count		 = 0;
		static double accumulated_time_s = 0.0;

		c_auto dt = age::global::get<age::runtime::interface>().delta_time_s();

		++frame_count;
		accumulated_time_s += dt;

		if (accumulated_time_s >= 1.0)
		{
			c_auto average_fps = static_cast<double>(frame_count) / accumulated_time_s;

			std::println("[Profiler] Average FPS : {:.1f} ({} frames in {:.4f}s)",
						 average_fps, frame_count, accumulated_time_s);

			frame_count		   = 0;
			accumulated_time_s = 0.0;
		}
		// std::println("now : {:%F %T}", std::chrono::system_clock::now());
		// std::println("now : {}ns, {}s",
		//	   age::global::get<age::runtime::interface>().delta_time_ns().count(),
		//	   age::global::get<age::runtime::interface>().delta_time_s());
		// std::this_thread::sleep_for(std::chrono::seconds(1));
	}

	constexpr FORCE_INLINE decltype(auto)
	get_sys_loop() noexcept
	{
		using namespace age::ecs::system;

		return loop{
			AGE_LAMBDA((), { return true; }),

			break_if_unlikely{ [] { return age::platform::window_count() == 0; } },
			identity{ game_ctx.h_input_ctx }
				| AGE_FUNC(age::input::begin_frame)
				| AGE_FUNC(age::platform::update),

			age::runtime::update,

			age::graphics::begin_frame,

			age::runtime::when_window_alive(game_ctx.h_window)
				| AGE_FUNC(print_frame_rate)
				| AGE_FUNC(update_input)
				| AGE_LAMBDA(
					(),
					{
						if (game_ctx.h_input_ctx->is_pressed(age::input::e::key_kind::key_0))
						{
							game_ctx.scene_id_next = 0;
						}
						else if (game_ctx.h_input_ctx->is_pressed(age::input::e::key_kind::key_1))
						{
							game_ctx.scene_id_next = 1;
						}
					})

				| cond_unlikely{ scene_changed, handle_scene_change }
				| match{ AGE_LAMBDA((), { return game_ctx.scene_id; }),
						 on<0>		= scene_0::update,
						 on<1>		= scene_1::update,
						 default_to = AGE_LAMBDA((), { AGE_UNREACHABLE(); }) },
			// age::runtime::when_window_alive(h_window_test_app_1),
			// age::runtime::when_window_alive(h_window_test_app_2),
			// age::runtime::when_window_alive(h_window_test_app_3),
			age::graphics::end_frame
		};
	}

	constexpr FORCE_INLINE decltype(auto)
	get_sys_deinit() noexcept
	{
		using namespace age::ecs::system;

		return on_ctx{
			match{ identity{ game_ctx.scene_id },
				   on<0>	  = scene_0::deinit,
				   on<1>	  = scene_1::deinit,
				   default_to = AGE_LAMBDA((), { AGE_UNREACHABLE(); }) },

			AGE_FUNC(game_ctx.render_pipeline.deinit),
			age::ecs::system::exec_inline{}
		};
	}
}	 // namespace demo_game

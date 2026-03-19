#include "age_demo_pch.hpp"
#include "age_demo.hpp"

namespace age_demo::scene_1
{
	FORCE_INLINE decltype(auto)
	init() noexcept
	{
		using namespace age::ecs::system;

		on_ctx{
			AGE_LAMBDA(
				(),
				{
					i_init.set_euler_x(0.f);
					i_init.set_euler_y(0.f);
					i_init.set_smoothed_move(float2{ 0.f, 0.f });
					i_init.set_smoothed_look(float2{ 0.f, 0.f });
					i_init.set_smoothed_zoom(0.f);
					i_init.set_smoothed_pan(float2{ 0.f, 0.f });
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
				| AGE_FUNC(i_init.get_render_pipeline().add_camera)
				| AGE_FUNC(i_init.get_camera_id_vec().emplace_back),

			// dim ambient directional light
			identity{ age::graphics::render_pipeline::forward_plus::directional_light_desc{
				.direction = age::normalize(float3{ 0.0f, -1.0f, 0.0f }),
				.intensity = 0.05f,
				.color	   = float3{ 1.0f, 1.0f, 1.0f } } }
				| AGE_FUNC(i_init.get_render_pipeline().add_directional_light)
				| AGE_FUNC(i_init.get_directional_light_id_vec().emplace_back),

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
			//			i_init.get_point_light_id_vec().emplace_back(
			//				i_init.get_render_pipeline().add_point_light(
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
				| AGE_LAMBDA((auto&& desc), { return i_init.get_render_pipeline().add_point_light(FWD(desc), false); })
				| AGE_FUNC(i_init.get_point_light_id_vec().emplace_back),

			// green light - right side, mid height
			identity{ age::graphics::render_pipeline::forward_plus::point_light_desc{
				.position  = float3{ 3.0f, 4.0f, 0.0f },
				.range	   = 15.0f,
				.color	   = float3{ 0.2f, 1.0f, 0.2f },
				.intensity = 3.0f } }
				| AGE_LAMBDA((auto&& desc), { return i_init.get_render_pipeline().add_point_light(FWD(desc), true); })
				| AGE_FUNC(i_init.get_point_light_id_vec().emplace_back),

			// blue light - center, high above
			identity{ age::graphics::render_pipeline::forward_plus::point_light_desc{
				.position  = float3{ 0.0f, 6.0f, 2.0f },
				.range	   = 15.0f,
				.color	   = float3{ 0.3f, 0.3f, 1.0f },
				.intensity = 3.0f } }
				| AGE_FUNC(i_init.get_render_pipeline().add_point_light)
				| AGE_FUNC(i_init.get_point_light_id_vec().emplace_back),

			identity{ age::graphics::render_pipeline::forward_plus::spot_light_desc{
				.position  = float3{ -4.0f, 4.0f, 0.0f },
				.range	   = 30.0f,
				.direction = age::normalize(float3{ 2.0f, -1.0f, 0.0f }),
				.intensity = 30.0f,
				.color	   = float3{ 1.0f, 0.9f, 0.6f },
				.cos_inner = 0.96f,
				.cos_outer = 0.87f } }
				| AGE_LAMBDA((auto&& desc), { return i_init.get_render_pipeline().add_spot_light(FWD(desc), true); })
				| AGE_FUNC(i_init.get_spot_light_id_vec().emplace_back),

			// === meshes ===

			// [0] cube
			identity{ age::asset::primitive_desc{
				.size	   = { 0.5, 0.5, 0.5 },
				.seg_u	   = 30,
				.seg_v	   = 30,
				.mesh_kind = age::asset::e::primitive_mesh_kind::cube } }
				| age::asset::create_primitive_mesh
				| age::asset::bake_mesh<age::asset::vertex_pnt_uv1>
				| AGE_FUNC(i_init.get_render_pipeline().upload_mesh)
				| AGE_FUNC(i_init.get_mesh_id_vec().emplace_back),

			// [1] plane
			identity{ age::asset::primitive_desc{
				.size	   = { 0.5, 0.5, 0.5 },
				.seg_u	   = 30,
				.seg_v	   = 30,
				.mesh_kind = age::asset::e::primitive_mesh_kind::plane } }
				| age::asset::create_primitive_mesh
				| age::asset::bake_mesh<age::asset::vertex_pnt_uv1>
				| AGE_FUNC(i_init.get_render_pipeline().upload_mesh)
				| AGE_FUNC(i_init.get_mesh_id_vec().emplace_back),

			// [2] sphere
			identity{ age::asset::primitive_desc{
				.size	   = { 0.5, 0.5, 0.5 },
				.seg_u	   = 30,
				.seg_v	   = 30,
				.mesh_kind = age::asset::e::primitive_mesh_kind::cube_sphere } }
				| age::asset::create_primitive_mesh
				| age::asset::bake_mesh<age::asset::vertex_pnt_uv1>
				| AGE_FUNC(i_init.get_render_pipeline().upload_mesh)
				| AGE_FUNC(i_init.get_mesh_id_vec().emplace_back),

			AGE_LAMBDA(
				(),
				{
					auto add_obj = [&](float3 pos, float3 scale, float4 quat = age::g::quaternion_identity) {
						i_init.get_obj_id_vec().emplace_back(
							i_init.get_render_pipeline().add_object(
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
		c_auto dt_s = std::max(
			age::runtime::i_time.get_delta_time_s(),
			1.f / 160);

		c_auto speed	= i_update.get_sprint() ? input::g::move_speed * input::g::sprint_mult : input::g::move_speed;
		auto   cam_desc = i_update.get_render_pipeline().get_camera_desc(i_update.get_camera_id_vec()[0]);


		c_auto move_smoothing_factor = 1.f - std::exp(-input::g::move_smoothing * dt_s);
		c_auto look_smoothing_factor = 1.f - std::exp(-input::g::look_smoothing * dt_s);
		c_auto zoom_smoothing_factor = 1.f - std::exp(-input::g::zoom_smoothing * dt_s);

		i_update.set_smoothed_move = age::math::lerp(i_update.get_smoothed_move(), i_update.get_move(), move_smoothing_factor);

		i_update.set_smoothed_zoom = age::math::lerp(i_update.get_smoothed_zoom(), i_update.get_zoom(), zoom_smoothing_factor);

		auto look_target		   = i_update.get_right_mouse_down() ? i_update.get_look() : float2{ 0.f, 0.f };
		i_update.set_smoothed_look = age::math::lerp(i_update.get_smoothed_look(), look_target, look_smoothing_factor);

		auto pan_target			  = i_update.get_middle_mouse_down() ? i_update.get_look() : float2{ 0.f, 0.f };
		i_update.set_smoothed_pan = age::math::lerp(i_update.get_smoothed_pan(), pan_target, look_smoothing_factor);

		i_update.set_euler_y = i_update.get_euler_y() + i_update.get_smoothed_look->x * input::g::sensitivity;

		i_update.set_euler_x = i_update.get_euler_x() + i_update.get_smoothed_look->y * input::g::sensitivity;

		i_update.set_euler_x = std::clamp(i_update.get_euler_x(), -89.f * age::g::degree_to_radian, 89.f * age::g::degree_to_radian);

		c_auto xm_look_quat = float3{ i_update.get_euler_x(), i_update.get_euler_y(), 0.f }
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

		cam_desc.pos -= right * i_update.get_smoothed_pan->x * input::g::pan_speed * dt_s;
		cam_desc.pos += up * i_update.get_smoothed_pan->y * input::g::pan_speed * dt_s;
		cam_desc.pos += forward * i_update.get_smoothed_zoom() * input::g::zoom_speed;
		cam_desc.pos += (right * i_update.get_smoothed_move->x + forward * i_update.get_smoothed_move->y) * speed * dt_s;

		cam_desc.quaternion = xm_look_quat | age::simd::to<float4>();

		i_update.get_render_pipeline().update_camera(i_update.get_camera_id_vec()[0], cam_desc);

		if (i_update.get_render_pipeline().begin_render(i_update.get_h_render_surface()))
		{
			for (auto&& [i, obj_id] : i_update.get_obj_id_vec() | std::views::enumerate)
			{
				i_update.get_render_pipeline().render_mesh(obj_id % age::graphics::g::thread_count, obj_id, i_update.get_mesh_id_vec()[i % i_update.get_mesh_id_vec().size()]);
			}

			i_update.get_render_pipeline().end_render(i_update.get_h_render_surface());
		}
	}

	FORCE_INLINE decltype(auto)
	deinit() noexcept
	{
		for (auto o_id : i_deinit.get_obj_id_vec())
		{
			i_deinit.get_render_pipeline->remove_object(o_id);
		}

		for (auto m_id : i_deinit.get_mesh_id_vec() | std::views::reverse)
		{
			i_deinit.get_render_pipeline->release_mesh(m_id);
		}

		for (auto c_id : i_deinit.get_camera_id_vec())
		{
			i_deinit.get_render_pipeline->remove_camera(c_id);
		}

		for (auto l_id : i_deinit.get_point_light_id_vec())
		{
			i_deinit.get_render_pipeline->remove_point_light(l_id);
		}

		for (auto l_id : i_deinit.get_spot_light_id_vec())
		{
			i_deinit.get_render_pipeline->remove_spot_light(l_id);
		}

		for (auto d_id : i_deinit.get_directional_light_id_vec())
		{
			i_deinit.get_render_pipeline->remove_directional_light(d_id);
		}

		i_deinit.get_obj_id_vec->clear();
		i_deinit.get_mesh_id_vec->clear();
		i_deinit.get_camera_id_vec->clear();
		i_deinit.get_point_light_id_vec->clear();
		i_deinit.get_spot_light_id_vec->clear();
		i_deinit.get_directional_light_id_vec->clear();
	}
}	 // namespace age_demo::scene_1
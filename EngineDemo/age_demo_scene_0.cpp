#include "age_demo_pch.hpp"
#include "age_demo.hpp"

namespace age_demo::scene_0
{
	FORCE_INLINE decltype(auto)
	init() noexcept
	{
		using namespace age::ecs::system;
		auto i_ctx = global::get<interface_init>();

		on_ctx{
			AGE_LAMBDA(
				(),
				{
					i_ctx.set_euler_x(0.f);
					i_ctx.set_euler_y(0.f);
					i_ctx.set_smoothed_move(float2{ 0.f, 0.f });
					i_ctx.set_smoothed_look(float2{ 0.f, 0.f });
					i_ctx.set_smoothed_zoom(0.f);
					i_ctx.set_smoothed_pan(float2{ 0.f, 0.f });
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
				| AGE_FUNC(i_ctx.get_render_pipeline().add_camera)
				| AGE_FUNC(i_ctx.get_camera_id_vec().emplace_back),

			identity{ age::graphics::render_pipeline::forward_plus::directional_light_desc{
				.direction = age::normalize(float3{ 1.0f, -1.0f, 0.5f }),
				.intensity = 0.3f,
				.color	   = float3{ 1.0f, 0.95f, 0.85f } } }
				| AGE_FUNC(i_ctx.get_render_pipeline().add_directional_light)
				| AGE_FUNC(i_ctx.get_directional_light_id_vec().emplace_back),

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
						i_ctx.get_point_light_id_vec().emplace_back(
							i_ctx.get_render_pipeline().add_point_light(
								age::graphics::render_pipeline::forward_plus::point_light_desc{
									.position = float3{ dist_pos(rng), dist_pos(rng), dist_pos(rng) },
									.range	  = range,
									.color	  = float3{ dist_color(rng), dist_color(rng), dist_color(rng) },
									//.color	   = float3{ 1, 1, 1 },
									.intensity = intensity }));
					}
				}),

			// yellow spot top down onto scene center
			identity{ age::graphics::render_pipeline::forward_plus::spot_light_desc{
				.position  = float3{ 0.0f, 12.0f, 0.0f },
				.range	   = 6.0f,
				.direction = float3{ 0.0f, -1.0f, 0.0f },
				.intensity = 10.0f,
				.color	   = float3{ 1.0f, 0.9f, 0.6f },
				.cos_inner = 0.96f,
				.cos_outer = 0.87f } }
				| AGE_FUNC(i_ctx.get_render_pipeline().add_spot_light)
				| AGE_FUNC(i_ctx.get_spot_light_id_vec().emplace_back),

			identity{ age::asset::primitive_desc{
				.size	   = { 0.5, 0.5, 0.5 },
				.seg_u	   = 30,
				.seg_v	   = 30,
				.mesh_kind = age::asset::e::primitive_mesh_kind::cube } }
				| age::asset::create_primitive_mesh
				| age::asset::bake_mesh<age::asset::vertex_pnt_uv1>
				| AGE_FUNC(i_ctx.get_render_pipeline().upload_mesh)
				| AGE_FUNC(i_ctx.get_mesh_id_vec().emplace_back),

			identity{ age::asset::primitive_desc{
				.size	   = { 0.5, 0.5, 0.5 },
				.seg_u	   = 30,
				.seg_v	   = 30,
				.mesh_kind = age::asset::e::primitive_mesh_kind::plane } }
				| age::asset::create_primitive_mesh
				| age::asset::bake_mesh<age::asset::vertex_pnt_uv1>
				| AGE_FUNC(i_ctx.get_render_pipeline().upload_mesh)
				| AGE_FUNC(i_ctx.get_mesh_id_vec().emplace_back),

			identity{ age::asset::primitive_desc{
				.size	   = { 0.5, 0.5, 0.5 },
				.seg_u	   = 30,
				.seg_v	   = 30,
				.mesh_kind = age::asset::e::primitive_mesh_kind::cube_sphere } }
				| age::asset::create_primitive_mesh
				| age::asset::bake_mesh<age::asset::vertex_pnt_uv1>
				| AGE_FUNC(i_ctx.get_render_pipeline().upload_mesh)
				| AGE_FUNC(i_ctx.get_mesh_id_vec().emplace_back),


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

						i_ctx.get_obj_id_vec().emplace_back(i_ctx.get_render_pipeline().add_object(data));
					}
				}),
			exec_inline{}
		}();
	}

	FORCE_INLINE decltype(auto)
	update() noexcept
	{
		auto i_ctx = global::get<interface_loop>();

		c_auto dt_s = std::max(
			age::global::get<age::runtime::interface>().delta_time_s(),
			1.f / 160);

		c_auto speed	= i_ctx.get_sprint() ? input::g::move_speed * input::g::sprint_mult : input::g::move_speed;
		auto   cam_desc = i_ctx.get_render_pipeline().get_camera_desc(i_ctx.get_camera_id_vec()[0]);


		c_auto move_smoothing_factor = 1.f - std::exp(-input::g::move_smoothing * dt_s);
		c_auto look_smoothing_factor = 1.f - std::exp(-input::g::look_smoothing * dt_s);
		c_auto zoom_smoothing_factor = 1.f - std::exp(-input::g::zoom_smoothing * dt_s);

		i_ctx.set_smoothed_move() = age::math::lerp(i_ctx.get_smoothed_move(), i_ctx.get_move(), move_smoothing_factor);

		i_ctx.set_smoothed_zoom() = age::math::lerp(i_ctx.get_smoothed_zoom(), i_ctx.get_zoom(), zoom_smoothing_factor);

		auto look_target		  = i_ctx.get_right_mouse_down() ? i_ctx.get_look() : float2{ 0.f, 0.f };
		i_ctx.set_smoothed_look() = age::math::lerp(i_ctx.get_smoothed_look(), look_target, look_smoothing_factor);

		auto pan_target			 = i_ctx.get_middle_mouse_down() ? i_ctx.get_look() : float2{ 0.f, 0.f };
		i_ctx.set_smoothed_pan() = age::math::lerp(i_ctx.get_smoothed_pan(), pan_target, look_smoothing_factor);

		i_ctx.set_euler_y() = i_ctx.get_euler_y() + i_ctx.get_smoothed_look().x * input::g::sensitivity;

		i_ctx.set_euler_x() = i_ctx.get_euler_x() + i_ctx.get_smoothed_look().y * input::g::sensitivity;

		i_ctx.set_euler_x() = std::clamp(i_ctx.get_euler_x(), -89.f * age::g::degree_to_radian, 89.f * age::g::degree_to_radian);

		c_auto xm_look_quat = float3{ i_ctx.get_euler_x(), i_ctx.get_euler_y(), 0.f }
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

		cam_desc.pos -= right * i_ctx.get_smoothed_pan().x * input::g::pan_speed * dt_s;
		cam_desc.pos += up * i_ctx.get_smoothed_pan().y * input::g::pan_speed * dt_s;
		cam_desc.pos += forward * i_ctx.get_smoothed_zoom() * input::g::zoom_speed;
		cam_desc.pos += (right * i_ctx.get_smoothed_move().x + forward * i_ctx.get_smoothed_move().y) * speed * dt_s;

		cam_desc.quaternion = xm_look_quat | age::simd::to<float4>();

		i_ctx.get_render_pipeline().update_camera(i_ctx.get_camera_id_vec()[0], cam_desc);

		if (i_ctx.get_render_pipeline().begin_render(i_ctx.get_h_render_surface()))
		{
			for (auto&& [i, obj_id] : i_ctx.get_obj_id_vec() | std::views::enumerate)
			{
				i_ctx.get_render_pipeline().render_mesh(obj_id % age::graphics::g::thread_count, obj_id, i_ctx.get_mesh_id_vec()[i % i_ctx.get_mesh_id_vec().size()]);
			}

			i_ctx.get_render_pipeline().end_render(i_ctx.get_h_render_surface());
		}
	}

	FORCE_INLINE decltype(auto)
	deinit() noexcept
	{
		auto i_ctx = global::get<interface_deinit>();

		for (auto o_id : i_ctx.get_obj_id_vec())
		{
			i_ctx.get_render_pipeline().remove_object(o_id);
		}

		for (auto m_id : i_ctx.get_mesh_id_vec() | std::views::reverse)
		{
			i_ctx.get_render_pipeline().release_mesh(m_id);
		}

		for (auto c_id : i_ctx.get_camera_id_vec())
		{
			i_ctx.get_render_pipeline().remove_camera(c_id);
		}

		for (auto l_id : i_ctx.get_point_light_id_vec())
		{
			i_ctx.get_render_pipeline().remove_point_light(l_id);
		}

		for (auto l_id : i_ctx.get_spot_light_id_vec())
		{
			i_ctx.get_render_pipeline().remove_spot_light(l_id);
		}

		for (auto d_id : i_ctx.get_directional_light_id_vec())
		{
			i_ctx.get_render_pipeline().remove_directional_light(d_id);
		}

		i_ctx.get_obj_id_vec().clear();
		i_ctx.get_mesh_id_vec().clear();
		i_ctx.get_camera_id_vec().clear();
		i_ctx.get_point_light_id_vec().clear();
		i_ctx.get_spot_light_id_vec().clear();
		i_ctx.get_directional_light_id_vec().clear();
	}
}	 // namespace age_demo::scene_0
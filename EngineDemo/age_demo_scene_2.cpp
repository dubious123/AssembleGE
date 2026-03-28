#include "age_demo_pch.hpp"
#include "age_demo.hpp"
#include "age_demo_pch.hpp"
#include "age_demo.hpp"

namespace age_demo::scene_2
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

			// camera - pulled back and slightly elevated to see the full scene + skybox
			identity{ age::graphics::render_pipeline::forward_plus::camera_desc{
				.kind		= age::graphics::e::camera_kind::perspective,
				.pos		= float3{ 0.f, 4.f, -14.f },
				.quaternion = age::g::quaternion_identity,
				.near_z		= 0.01f,
				.far_z		= 1000.f,
				.perspective{
					.fov_y		  = age::cvt_to_radian(75.f),
					.aspect_ratio = 16.f / 9.f } } }
				| AGE_FUNC(i_init.get_render_pipeline().add_camera)
				| AGE_FUNC(i_init.get_camera_id_vec().emplace_back),

			// strong directional light from above-right - good for seeing transparency + shadows
			identity{ age::graphics::render_pipeline::forward_plus::directional_light_desc{
				.direction = age::normalize(float3{ -0.3f, -1.0f, 0.5f }),
				.intensity = 0.15f,
				.color	   = float3{ 1.0f, 0.9f, 0.9f } } }
				| AGE_FUNC(i_init.get_render_pipeline().add_directional_light)
				| AGE_FUNC(i_init.get_directional_light_id_vec().emplace_back),

			// warm point light - left side, illuminates transparent objects from behind
			identity{ age::graphics::render_pipeline::forward_plus::point_light_desc{
				.position  = float3{ -5.0f, 5.0f, 4.0f },
				.range	   = 50.0f,
				.color	   = float3{ 1.0f, 0.7f, 0.3f },
				.intensity = 8.0f } }
				| AGE_LAMBDA((auto&& desc), { return i_init.get_render_pipeline().add_point_light(FWD(desc), true); })
				| AGE_FUNC(i_init.get_point_light_id_vec().emplace_back),

			// cool point light - right side, color mixing through transparent surfaces
			identity{ age::graphics::render_pipeline::forward_plus::point_light_desc{
				.position  = float3{ 5.0f, 3.0f, -2.0f },
				.range	   = 50.0f,
				.color	   = float3{ 0.3f, 0.5f, 1.0f },
				.intensity = 8.0f } }
				| AGE_LAMBDA((auto&& desc), { return i_init.get_render_pipeline().add_point_light(FWD(desc), true); })
				| AGE_FUNC(i_init.get_point_light_id_vec().emplace_back),

			// === meshes ===

			// [0] cube
			identity{ age::asset::primitive_desc{
				.size	   = { 0.5, 0.5, 0.5 },
				.seg_u	   = 1,
				.seg_v	   = 1,
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
					auto add_opaque_obj = [&](float3 pos, float3 scale, float4 quat = age::g::quaternion_identity) {
						i_init.get_opaque_obj_id_vec->emplace_back(
							i_init.get_render_pipeline->add_object(pos, quat, scale));
					};

					auto add_transparent_obj = [&](float3 pos, float3 scale, float4 quat = age::g::quaternion_identity) {
						i_init.get_transparent_obj_id_vec->emplace_back(
							i_init.get_render_pipeline->add_object(pos, quat, scale));
					};

					// ===== opaque reference objects =====

					// ground plane - small, so skybox is visible at horizon
					add_opaque_obj(float3{ 0.0f, -0.5f, 0.0f }, float3{ 40.0f, 1.0f, 40.0f });

					// opaque pillar behind transparent objects - occlusion reference
					add_opaque_obj(float3{ 0.0f, 2.0f, 5.0f }, float3{ 1.0f, 4.0f, 1.0f });

					// opaque cube on the right - reference for color blending comparison
					add_opaque_obj(float3{ 6.0f, 1.0f, 0.0f }, float3{ 2.0f, 2.0f, 2.0f });
					add_opaque_obj(float3{ 5.8f, 5.0f, 0.0f }, float3{ 2.0f, 2.0f, 2.0f });

					// ===== transparency test: overlapping planes at different depths =====
					// three parallel planes stacked in Z - classic sort-order test
					add_transparent_obj(float3{ -4.0f, 2.0f, 1.0f }, float3{ 3.0f, 3.0f, 0.05f });
					add_transparent_obj(float3{ -4.0f, 2.0f, 2.0f }, float3{ 3.0f, 3.0f, 0.05f });
					add_transparent_obj(float3{ -4.0f, 2.0f, 3.0f }, float3{ 3.0f, 3.0f, 0.05f });

					// ===== transparency test: intersecting cubes =====
					// two cubes crossing through each other - per-pixel sort stress test
					add_transparent_obj(float3{ 0.0f, 2.0f, 0.0f }, float3{ 3.0f, 1.0f, 1.0f });
					add_transparent_obj(float3{ 0.0f, 2.0f, 0.0f }, float3{ 1.0f, 1.0f, 3.0f });

					// ===== transparency test: nested spheres =====
					// concentric spheres - inside-out ordering
					add_transparent_obj(float3{ 4.0f, 2.0f, 2.0f }, float3{ 3.0f, 3.0f, 3.0f });
					add_transparent_obj(float3{ 4.0f, 2.0f, 2.1f }, float3{ 2.0f, 2.0f, 2.0f });
					add_transparent_obj(float3{ 4.0f, 2.0f, 2.2f }, float3{ 1.0f, 1.0f, 1.0f });

					// ===== transparency test: transparent in front of opaque =====
					// transparent plane hovering in front of the opaque pillar
					add_transparent_obj(float3{ 0.0f, 2.0f, 3.5f }, float3{ 4.0f, 4.0f, 0.05f });

					// ===== transparency test: ground-contact =====
					// flat transparent slab on the ground - blending with opaque floor
					add_transparent_obj(float3{ -2.0f, 1.f, -3.0f }, float3{ 4.0f, 0.1f, 4.0f });

					// ===== skybox visibility objects =====
					// tall thin columns at edges - silhouettes against skybox
					add_opaque_obj(float3{ -8.0f, 3.0f, 8.0f }, float3{ 0.3f, 6.0f, 0.3f });
					add_opaque_obj(float3{ 8.0f, 3.0f, 8.0f }, float3{ 0.3f, 6.0f, 0.3f });

					// floating sphere high up - overlaps skybox
					add_opaque_obj(float3{ 0.0f, 8.0f, 6.0f }, float3{ 2.0f, 2.0f, 2.0f });


					// ui
					// background panel

					// i_init.get_ui_id_vec->emplace_back(
					//	i_init.get_render_pipeline->add_ui(age::graphics::render_pipeline::forward_plus::ui_desc{
					//		.pivot_pos		   = { 400.f, 300.f },
					//		.pivot_uv		   = { 0.5f, 0.5f },
					//		.size			   = { 300.f, 200.f },
					//		.rotation		   = 0.f,
					//		.border_thickness  = 2.f,
					//		.z_order		   = 0,
					//		.shape_kind		   = age::ui::e::shape_kind::rect,
					//		.body_brush_kind   = age::ui::e::brush_kind::color,
					//		.body_brush_data   = { .color = { .value = { 0.15f, 0.15f, 0.15f } } },
					//		.border_brush_kind = age::ui::e::brush_kind::color,
					//		.border_brush_data = { .color = { .value = { 1.0f, 1.0f, 1.0f } } },
					//	}));

					//// small rect on top
					// i_init.get_ui_id_vec->emplace_back(
					//	i_init.get_render_pipeline->add_ui(age::graphics::render_pipeline::forward_plus::ui_desc{
					//		.pivot_pos		   = { 400.f, 280.f },
					//		.pivot_uv		   = { 0.5f, 0.5f },
					//		.size			   = { 120.f, 40.f },
					//		.rotation		   = 0.f,
					//		.border_thickness  = 0.f,
					//		.z_order		   = 1,
					//		.shape_kind		   = age::ui::e::shape_kind::rect,
					//		.body_brush_kind   = age::ui::e::brush_kind::color,
					//		.body_brush_data   = { .color = { .value = { 0.2f, 0.4f, 0.9f } } },
					//		.border_brush_kind = age::ui::e::brush_kind::color,
					//		.border_brush_data = { .color = { .value = { 0.0f, 0.0f, 0.0f } } },
					//	}));

					//// rotated rect
					// i_init.get_ui_id_vec->emplace_back(
					//	i_init.get_render_pipeline->add_ui(age::graphics::render_pipeline::forward_plus::ui_desc{
					//		.pivot_pos		   = { 700.f, 400.f },
					//		.pivot_uv		   = { 0.5f, 0.5f },
					//		.size			   = { 100.f, 100.f },
					//		.rotation		   = 0.785f,	// 45 degrees
					//		.border_thickness  = 3.f,
					//		.z_order		   = 0,
					//		.shape_kind		   = age::ui::e::shape_kind::rect,
					//		.body_brush_kind   = age::ui::e::brush_kind::color,
					//		.body_brush_data   = { .color = { .value = { 0.9f, 0.5f, 0.1f } } },
					//		.border_brush_kind = age::ui::e::brush_kind::color,
					//		.border_brush_data = { .color = { .value = { 1.0f, 0.9f, 0.2f } } },
					//	}));

					//// circle
					// i_init.get_ui_id_vec->emplace_back(
					//	i_init.get_render_pipeline->add_ui(age::graphics::render_pipeline::forward_plus::ui_desc{
					//		.pivot_pos		   = { 200.f, 400.f },
					//		.pivot_uv		   = { 0.5f, 0.5f },
					//		.size			   = { 80.f, 80.f },
					//		.rotation		   = 0.f,
					//		.border_thickness  = 0.f,
					//		.z_order		   = 1,
					//		.shape_kind		   = age::ui::e::shape_kind::circle,
					//		.body_brush_kind   = age::ui::e::brush_kind::color,
					//		.body_brush_data   = { .color = { .value = { 0.2f, 0.8f, 0.3f } } },
					//		.border_brush_kind = age::ui::e::brush_kind::color,
					//		.border_brush_data = { .color = { .value = { 0.0f, 0.0f, 0.0f } } },
					//	}));
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
		auto   cam_desc = i_update.get_render_pipeline().get_camera_desc(i_update.get_camera_id_vec[0]);

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

		cam_desc.perspective.aspect_ratio = age::platform::get_client_width(i_update.get_h_window) / static_cast<float>(age::platform::get_client_height(i_update.get_h_window));

		i_update.get_render_pipeline->update_camera(i_update.get_camera_id_vec()[0], cam_desc);

		if (i_update.get_render_pipeline->begin_render(i_update.get_h_render_surface) is_false)
		{
			return;
		}

		age::ui::begin_frame(i_update.get_h_window);

		// ui
		if (auto _ = age::ui::widget::layout_vertical(age::ui::size_mode::fit(), age::ui::size_mode::fit()))
		{
			if (auto h = age::ui::widget::begin(
					"layout 0",
					age::ui::widget_desc{
						.draw			  = true,
						.layout			  = age::ui::e::widget_layout::horizontal,
						.overflow		  = age::ui::e::widget_overflow::draw_all,
						.align			  = age::ui::e::widget_align::center,
						.size_mode_width  = age::ui::size_mode::fixed(300),
						.size_mode_height = age::ui::size_mode::fixed(200),
						.z_offset		  = 1,
						.child_gap		  = 10.f,
						.padding		  = { 10.f, 10.f, 10.f, 10.f },
						.render_data	  = {
							.rotation		   = 0.f,
							.border_thickness  = 2.f,
							.shape_kind		   = age::ui::e::shape_kind::rect,
							.body_brush_kind   = age::ui::e::brush_kind::color,
							.border_brush_kind = age::ui::e::brush_kind::color,
							.body_brush_data   = age::ui::brush_data::color(0.15f, 0.15f, 0.15f),
							.border_brush_data = age::ui::brush_data::color(1.0f, 1.0f, 1.0f),
						}

					}))
			{
			}

			if (auto h = age::ui::widget::begin(
					age::ui::widget_desc{
						.align			  = age::ui::e::widget_align::left,
						.size_mode_width  = age::ui::size_mode::fixed(200),
						.size_mode_height = age::ui::size_mode::fixed(100),

						.render_data = {
							.shape_kind		   = age::ui::e::shape_kind::rect,
							.body_brush_kind   = age::ui::e::brush_kind::color,
							.border_brush_kind = age::ui::e::brush_kind::color,
							.body_brush_data   = age::ui::brush_data::color(0.15f, 0.15f, 0.15f),
							.border_brush_data = age::ui::brush_data::color(1.0f, 1.0f, 1.0f),
						} }))
			{
			}
		}


		age::ui::end_frame(i_update.get_render_pipeline->get_ui_render_data_vec(),
						   i_update.get_render_pipeline->get_ui_render_data_z_range_vec());


		for (auto&& [i, obj_id] : i_update.get_opaque_obj_id_vec() | std::views::enumerate)
		{
			i_update.get_render_pipeline->render_mesh(obj_id % age::graphics::g::thread_count, obj_id, i_update.get_mesh_id_vec()[i % i_update.get_mesh_id_vec->size()]);
		}

		for (auto&& [i, obj_id] : i_update.get_transparent_obj_id_vec() | std::views::enumerate)
		{
			i_update.get_render_pipeline->render_transparent_mesh(obj_id % age::graphics::g::thread_count, obj_id, i_update.get_mesh_id_vec[0]);
		}

		i_update.get_render_pipeline->end_render(i_update.get_h_render_surface());
	}

	FORCE_INLINE decltype(auto)
	deinit() noexcept
	{
		for (auto o_id : i_deinit.get_opaque_obj_id_vec())
		{
			i_deinit.get_render_pipeline().remove_object(o_id);
		}

		for (auto o_id : i_deinit.get_transparent_obj_id_vec())
		{
			i_deinit.get_render_pipeline().remove_object(o_id);
		}

		for (auto m_id : i_deinit.get_mesh_id_vec() | std::views::reverse)
		{
			i_deinit.get_render_pipeline().release_mesh(m_id);
		}

		for (auto c_id : i_deinit.get_camera_id_vec())
		{
			i_deinit.get_render_pipeline().remove_camera(c_id);
		}

		for (auto l_id : i_deinit.get_point_light_id_vec())
		{
			i_deinit.get_render_pipeline().remove_point_light(l_id);
		}

		for (auto l_id : i_deinit.get_spot_light_id_vec())
		{
			i_deinit.get_render_pipeline().remove_spot_light(l_id);
		}

		for (auto d_id : i_deinit.get_directional_light_id_vec())
		{
			i_deinit.get_render_pipeline().remove_directional_light(d_id);
		}

		i_deinit.get_opaque_obj_id_vec().clear();
		i_deinit.get_transparent_obj_id_vec().clear();
		i_deinit.get_mesh_id_vec().clear();
		i_deinit.get_camera_id_vec().clear();
		i_deinit.get_point_light_id_vec().clear();
		i_deinit.get_spot_light_id_vec().clear();
		i_deinit.get_directional_light_id_vec().clear();
	}
}	 // namespace age_demo::scene_2
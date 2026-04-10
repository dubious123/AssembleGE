#include "age_demo_pch.hpp"
#include "age_demo.hpp"

namespace age_demo::scene_3
{
	FORCE_INLINE decltype(auto)
	init() noexcept
	{
		using namespace age::ecs::system;

		i_init.get_entities->init();

		i_init.set_ent_main_cam = i_init.get_entities->new_entity<age::ecs::position, age::ecs::rotation, age::ecs::camera>();

		auto&& [pos, cam] = i_init.get_entities->get_component<age::ecs::position&, age::ecs::camera&>(i_init.get_ent_main_cam);
		{
			pos = age::ecs::position{ float3::zero() };

			cam.kind = age::graphics::e::camera_kind::perspective;

			cam.euler_deg = age::math::quat_to_euler_deg(age::math::g::quaternion_identity);

			cam.near_z = 0.1f;
			cam.far_z  = 1000.f;

			cam.fov_y		 = age::cvt_to_radian(75.f);
			cam.aspect_ratio = 16.f / 9.f;

			cam.render_id = i_init.get_render_pipeline->add_camera(age::graphics::render_pipeline::forward_plus::camera_desc{
				.kind		= age::graphics::e::camera_kind::perspective,
				.pos		= pos,
				.quaternion = age::math::g::quaternion_identity,
				.near_z		= cam.near_z,
				.far_z		= cam.far_z,
				.perspective{
					.fov_y		  = cam.fov_y,
					.aspect_ratio = cam.aspect_ratio } });
		}


		on_ctx{
			AGE_LAMBDA(
				(),
				{
					i_init.set_smoothed_move(float2{ 0.f, 0.f });
					i_init.set_smoothed_look(float2{ 0.f, 0.f });
					i_init.set_smoothed_zoom(0.f);
					i_init.set_smoothed_pan(float2{ 0.f, 0.f });
				}),

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

			exec_inline{}
		}();
	}

	template <typename... t_with>
	struct query
	{
		using with	  = age::meta::type_pack<t_with...>;
		using without = age::meta::type_pack<>;
	};

	FORCE_INLINE decltype(auto)
	update() noexcept
	{
		using namespace age::ui;
		using enum age::input::e::key_kind;

		if (i_update.get_render_pipeline->begin_render(i_update.get_h_render_surface) is_false)
		{
			return;
		}

		age::ui::begin_frame(i_update.get_h_window);

		if (auto _ = widget::horizontal(set_size(size_mode::grow(), size_mode::grow()), set_child_gap(0)))
		{
			if (auto _ = widget::panel_resizable_h(300, 1000,
												   set_layout(e::widget_layout::vertical),
												   set_align(e::widget_align::center),
												   set_size(size_mode::grow(), size_mode::grow())))
			{
				if (auto _ = widget::scroll_area_v())
				{
					if (auto _ = widget::collapsible_header("mesh"))
					{
						for (auto i : i_update.get_mesh_id_vec())
						{
							widget::button("this is mesh");
						}
					}

					if (auto _ = widget::collapsible_header("camera"))
					{
						i_update.get_entities->foreach_entity(
							query<age::ecs::position, age::ecs::rotation, age::ecs::camera>{},
							AGE_LAMBDA((age::ecs::position & pos, age::ecs::camera & cam), {
								if (auto h = widget::vertical())
								{
									widget::text_secondary(age::graphics::e::to_string(cam.kind).data());

									widget::numeric_field(pos, "pos");

									widget::numeric_field(cam.euler_deg, "rotation", float3{ -90.f, -180.f, -180.f }, float3{ 90.f, 180.f, 180.f });

									widget::numeric_field(cam.near_z, "near_z", cam.far_z);
									widget::numeric_field(cam.far_z, "far_z", 0.f);

									if (cam.kind == age::graphics::e::camera_kind::perspective)
									{
										widget::numeric_field(cam.fov_y, "fov_y");
										widget::numeric_field(cam.aspect_ratio, "aspect_ratio");
									}
									else
									{
										widget::numeric_field(cam.view_width, "view_width");
										widget::numeric_field(cam.view_height, "view_height");
									}
								}

								i_update.get_render_pipeline->update_camera(
									cam.render_id,
									{
										.kind		 = cam.kind,
										.pos		 = pos,
										.quaternion	 = age::math::euler_deg_to_quat(cam.euler_deg),
										.near_z		 = cam.near_z,
										.far_z		 = cam.far_z,
										.perspective = {
											.fov_y		  = cam.fov_y,
											.aspect_ratio = cam.aspect_ratio,

										},
									});
							}));
					}

					if (auto _ = widget::collapsible_header("light"))
					{
						if (auto _ = widget::collapsible_header("directional light"))
						{
							if (auto btn = widget::button("new", set_align_center()))
							{
								if (btn.clicked())
								{
									auto ent_light		= i_update.get_entities->new_entity<age::ecs::directional_light>();
									auto&& [cmp_light]	= i_update.get_entities->get_component<age::ecs::directional_light&>(ent_light);
									cmp_light.direction = age::normalize(float3{ -0.3f, -1.0f, 0.5f });
									cmp_light.intensity = 0.15f;
									cmp_light.color		= float3{ 1.0f, 0.9f, 0.9f };

									cmp_light.cast_shadow = true;

									cmp_light.render_id = i_update.get_render_pipeline->add_directional_light(
										age::graphics::render_pipeline::forward_plus::directional_light_desc{
											.direction = cmp_light.direction,
											.intensity = cmp_light.intensity,
											.color	   = cmp_light.color,
										},
										cmp_light.cast_shadow);
								}
							}

							i_update.get_entities->foreach_entity(
								query<age::ecs::directional_light>{},
								AGE_LAMBDA((age::ecs::directional_light & light), {
									char c_buf[12];
									age::util::to_str(c_buf, light.render_id);
									widget::text_secondary(c_buf);

									if (auto h = widget::checkbox("cast shadow", light.cast_shadow))
									{
										if (h.clicked())
										{
											i_update.get_render_pipeline->remove_directional_light(light.render_id);

											light.render_id = i_update.get_render_pipeline->add_directional_light(age::graphics::render_pipeline::forward_plus::directional_light_desc{}, light.cast_shadow);
										}
									}

									widget::numeric_field(light.direction, "direction");
									widget::numeric_field(light.intensity, "intensity");
									widget::numeric_field(light.color, "color");


									i_update.get_render_pipeline->update_directional_light(
										light.render_id,
										age::graphics::render_pipeline::forward_plus::directional_light_desc{
											.direction = age::math::normalize(light.direction),
											.intensity = light.intensity,
											.color	   = light.color,
										});
								}));
						}

						if (auto _ = widget::collapsible_header("point light"))
						{
							if (auto btn = widget::button("new", set_align_center()))
							{
								if (btn.clicked())
								{
									auto ent_light	   = i_update.get_entities->new_entity<age::ecs::position, age::ecs::point_light>();
									auto&& [cmp_light] = i_update.get_entities->get_component<age::ecs::point_light&>(ent_light);

									cmp_light.render_id = i_update.get_render_pipeline->add_point_light(age::graphics::render_pipeline::forward_plus::point_light_desc{}, cmp_light.cast_shadow);
								}
							}

							i_update.get_entities->foreach_entity(
								query<age::ecs::point_light, age::ecs::position>{},
								AGE_LAMBDA((age::ecs::position & pos, age::ecs::point_light & light), {
									char c_buf[12];
									age::util::to_str(c_buf, light.render_id);
									widget::text_secondary(c_buf);

									if (auto h = widget::checkbox("cast shadow", light.cast_shadow))
									{
										if (h.clicked())
										{
											i_update.get_render_pipeline->remove_point_light(light.render_id);

											light.render_id = i_update.get_render_pipeline->add_point_light(age::graphics::render_pipeline::forward_plus::point_light_desc{}, light.cast_shadow);
										}
									}

									widget::numeric_field(pos, "pos");
									widget::numeric_field(light.range, "range");
									widget::numeric_field(light.color, "color");
									widget::numeric_field(light.intensity, "intensity");

									i_update.get_render_pipeline->update_point_light(
										light.render_id,
										age::graphics::render_pipeline::forward_plus::point_light_desc{
											.position  = pos,
											.range	   = light.range,
											.color	   = light.color,
											.intensity = light.intensity,
										});
								}));
						}


						if (auto _ = widget::collapsible_header("spot light"))
						{
							if (auto btn = widget::button("new", set_align_center()))
							{
								if (btn.clicked())
								{
									auto ent_light		= i_update.get_entities->new_entity<age::ecs::position, age::ecs::spot_light>();
									auto&& [cmp_light]	= i_update.get_entities->get_component<age::ecs::spot_light&>(ent_light);
									cmp_light.render_id = i_update.get_render_pipeline->add_spot_light(age::graphics::render_pipeline::forward_plus::spot_light_desc{}, cmp_light.cast_shadow);
								}
							}
							i_update.get_entities->foreach_entity(
								query<age::ecs::spot_light, age::ecs::position>{},
								AGE_LAMBDA((age::ecs::position & pos, age::ecs::spot_light & light), {
									char c_buf[12];
									age::util::to_str(c_buf, light.render_id);
									widget::text_secondary(c_buf);

									if (auto h = widget::checkbox("cast shadow", light.cast_shadow))
									{
										if (h.clicked())
										{
											i_update.get_render_pipeline->remove_spot_light(light.render_id);

											light.render_id = i_update.get_render_pipeline->add_spot_light(age::graphics::render_pipeline::forward_plus::spot_light_desc{}, light.cast_shadow);
										}
									}

									widget::numeric_field(pos, "pos");
									widget::numeric_field(light.range, "range");
									widget::numeric_field(light.direction, "direction");
									widget::numeric_field(light.intensity, "intensity");
									widget::numeric_field(light.color, "color");
									widget::numeric_field(light.cos_inner, "cos_inner", 0.f, 1.f);
									widget::numeric_field(light.cos_outer, "cos_outer", light.cos_inner, 1.f);

									i_update.get_render_pipeline->update_spot_light(
										light.render_id,
										age::graphics::render_pipeline::forward_plus::spot_light_desc{
											.position  = pos,
											.range	   = light.range,
											.direction = age::math::normalize(light.direction),
											.intensity = light.intensity,
											.color	   = light.color,
											.cos_inner = light.cos_inner,
											.cos_outer = light.cos_outer,
										});
								}));
						}
					}

					if (auto _ = widget::collapsible_header("render objects"))
					{
						if (auto btn = widget::button("new", set_align_center()))
						{
							if (btn.clicked())
							{
								auto ent_obj															= i_update.get_entities->new_entity<age::ecs::position, age::ecs::rotation, age::ecs::scale, age::ecs::render_object, age::ecs::mesh, age::ecs::material>();
								auto&& [cmp_pos, cmp_rot, cmp_scale, cmp_rebder_obj, cmp_mesh, cmp_mat] = i_update.get_entities->get_component<age::ecs::position&, age::ecs::rotation&, age::ecs::scale&, age::ecs::render_object&, age::ecs::mesh&, age::ecs::material&>(ent_obj);
								cmp_rot																	= age::ecs::rotation{ age::math::g::quaternion_identity };
								cmp_scale																= age::ecs::scale{ float3::one() };
								cmp_rebder_obj.render_id												= i_update.get_render_pipeline->add_object(cmp_pos, cmp_rot, cmp_scale);
								cmp_mesh.render_id														= 0;
								cmp_mat.is_opaque														= true;
							}
						}
						i_update.get_entities->foreach_entity(
							query<age::ecs::render_object, age::ecs::position, age::ecs::material>{},
							AGE_LAMBDA((age::ecs::render_object render_obj, age::ecs::position & pos, age::ecs::rotation & rot, age::ecs::scale & scale, age::ecs::mesh & mesh, age::ecs::material & mat), {
								char c_buf[12];
								age::util::to_str(c_buf, render_obj.render_id);
								widget::text_secondary(c_buf);

								widget::numeric_field(pos, "pos");
								widget::rotation_field(rot, "rotation");
								widget::numeric_field(scale, "scale");
								widget::numeric_field(mesh.render_id, "mesh", 0u, i_update.get_mesh_id_vec->size<uint32>() - 1);

								widget::checkbox("is_opaque", mat.is_opaque);

								i_update.get_render_pipeline->update_object(render_obj.render_id, pos, rot, scale);
							}));
					}
				}
			}
			// if (auto _ = widget::panel_resizable_h(300, 1000,
			//									   set_layout(e::widget_layout::vertical),
			//									   set_size(size_mode::grow(), size_mode::grow())))
			//{
			// }

			if (auto h_game_scene = widget::vertical(set_width_grow(), set_height_grow(), set_interact(true)))
			{
				i_update.set_game_focused = h_game_scene.focused();
				std::println("i_update.set_game_focused  : {}", i_update.get_game_focused());

				if (i_update.get_game_focused)
				{
					i_update.get_entities->foreach_entity(
						query<age::ecs::position, age::ecs::camera>{},
						AGE_LAMBDA((age::ecs::position & pos, age::ecs::camera & cam), {
							c_auto dt_s = std::max(age::runtime::i_time.get_delta_time_s(), 1.f / 160);

							c_auto speed = i_update.get_sprint() ? input::g::move_speed * input::g::sprint_mult : input::g::move_speed;

							c_auto move_smoothing_factor = 1.f - std::exp(-input::g::move_smoothing * dt_s);
							c_auto look_smoothing_factor = 1.f - std::exp(-input::g::look_smoothing * dt_s);
							c_auto zoom_smoothing_factor = 1.f - std::exp(-input::g::zoom_smoothing * dt_s);

							i_update.set_smoothed_move = age::math::lerp(i_update.get_smoothed_move(), i_update.get_move(), move_smoothing_factor);
							i_update.set_smoothed_zoom = age::math::lerp(i_update.get_smoothed_zoom(), i_update.get_zoom(), zoom_smoothing_factor);

							auto look_target		   = i_update.get_right_mouse_down() ? i_update.get_look() : float2{ 0.f, 0.f };
							i_update.set_smoothed_look = age::math::lerp(i_update.get_smoothed_look(), look_target, look_smoothing_factor);

							auto pan_target			  = i_update.get_middle_mouse_down() ? i_update.get_look() : float2{ 0.f, 0.f };
							i_update.set_smoothed_pan = age::math::lerp(i_update.get_smoothed_pan(), pan_target, look_smoothing_factor);

							cam.euler_deg.y += i_update.get_smoothed_look->x * input::g::sensitivity;
							cam.euler_deg.x += i_update.get_smoothed_look->y * input::g::sensitivity;
							cam.euler_deg.x	 = std::clamp(cam.euler_deg.x, -89.f, 89.f);

							c_auto xm_look_quat = cam.euler_deg * age::g::degree_to_radian
												| age::simd::load()
												| age::simd::euler_to_quat();

							c_auto forward = age::simd::g::xm_forward_f4 | age::simd::rotate3(xm_look_quat) | age::simd::to<float3>();
							c_auto right   = age::simd::g::xm_right_f4 | age::simd::rotate3(xm_look_quat) | age::simd::to<float3>();
							c_auto up	   = age::simd::g::xm_up_f4 | age::simd::rotate3(xm_look_quat) | age::simd::to<float3>();

							pos -= right * i_update.get_smoothed_pan->x * input::g::pan_speed * dt_s;
							pos += up * i_update.get_smoothed_pan->y * input::g::pan_speed * dt_s;
							pos += forward * i_update.get_smoothed_zoom() * input::g::zoom_speed;
							pos += (right * i_update.get_smoothed_move->x + forward * i_update.get_smoothed_move->y) * speed * dt_s;

							cam.aspect_ratio = age::platform::get_client_width(i_update.get_h_window)
											 / static_cast<float>(age::platform::get_client_height(i_update.get_h_window));

							auto cam_desc					  = i_update.get_render_pipeline->get_camera_desc(cam.render_id);
							cam_desc.pos					  = pos;
							cam_desc.quaternion				  = age::ecs::rotation{ xm_look_quat | age::simd::to<float4>() };
							cam_desc.perspective.aspect_ratio = cam.aspect_ratio;
							i_update.get_render_pipeline->update_camera(cam.render_id, cam_desc);
						}));
				}
			}
		}


		age::ui::end_frame(i_update.get_render_pipeline->get_ui_render_data_vec(),
						   i_update.get_render_pipeline->get_ui_render_data_z_range_vec());


		i_update.get_entities->foreach_entity(
			query<age::ecs::render_object, age::ecs::mesh, age::ecs::material>{},
			AGE_LAMBDA((const age::ecs::render_object& obj, const age::ecs::mesh& mesh, const age::ecs::material& mat),
					   {
						   if (mat.is_opaque)
						   {
							   i_update.get_render_pipeline->render_mesh(0, obj.render_id, mesh.render_id);
						   }
						   else
						   {
							   i_update.get_render_pipeline->render_transparent_mesh(0, obj.render_id, mesh.render_id);
						   }
					   }));
		i_update.get_render_pipeline->end_render(i_update.get_h_render_surface());
	}

	FORCE_INLINE decltype(auto)
	deinit() noexcept
	{
		for (auto m_id : i_deinit.get_mesh_id_vec() | std::views::reverse)
		{
			i_deinit.get_render_pipeline().release_mesh(m_id);
		}

		for (auto d_id : i_deinit.get_directional_light_id_vec())
		{
			i_deinit.get_render_pipeline().remove_directional_light(d_id);
		}

		i_deinit.get_mesh_id_vec->clear();
		i_deinit.get_directional_light_id_vec->clear();

		i_deinit.get_entities->foreach_entity(
			query<age::ecs::camera>{},
			AGE_LAMBDA((age::ecs::camera & cam), {
				i_deinit.get_render_pipeline->remove_camera(cam.render_id);
			}));

		i_deinit.get_entities->foreach_entity(
			query<age::ecs::point_light>{},
			AGE_LAMBDA((age::ecs::point_light & l), {
				i_deinit.get_render_pipeline->remove_point_light(l.render_id);
			}));

		i_deinit.get_entities->foreach_entity(
			query<age::ecs::spot_light>{},
			AGE_LAMBDA((age::ecs::spot_light & l), {
				i_deinit.get_render_pipeline->remove_spot_light(l.render_id);
			}));

		i_deinit.get_entities->foreach_entity(
			query<age::ecs::render_object>{},
			AGE_LAMBDA((age::ecs::render_object & obj), {
				i_deinit.get_render_pipeline->remove_object(obj.render_id);
			}));

		i_deinit.get_entities->deinit();
	}
}	 // namespace age_demo::scene_3
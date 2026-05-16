#include "age_demo_pch.hpp"
#include "age_demo.hpp"

namespace age_demo::scene_2
{
	FORCE_INLINE decltype(auto)
	init() noexcept
	{
		using namespace age::ecs::system;

		age::asset::registry::register_asset(
			i_init.get_mesh_id_vec->emplace_back(
				age::asset::mesh_baked::gpu_load("./resources/demo_game/assets/mesh/primitive_cube",
												 i_init.get_render_pipeline(),
												 age::asset::primitive_desc{
													 .size		= { 0.5, 0.5, 0.5 },
													 .seg_u		= 30,
													 .seg_v		= 30,
													 .mesh_kind = age::asset::e::primitive_mesh_kind::cube },
												 age::asset::e::vertex_kind::pnt_uv1)));

		age::asset::registry::register_asset(
			i_init.get_mesh_id_vec->emplace_back(
				age::asset::mesh_baked::gpu_load("./resources/demo_game/assets/mesh/primitive_plane",
												 i_init.get_render_pipeline(),
												 age::asset::primitive_desc{
													 .size		= { 0.5, 0.5, 0.5 },
													 .seg_u		= 30,
													 .seg_v		= 30,
													 .mesh_kind = age::asset::e::primitive_mesh_kind::plane },
												 age::asset::e::vertex_kind::pnt_uv1)));

		age::asset::registry::register_asset(
			i_init.get_mesh_id_vec->emplace_back(
				age::asset::mesh_baked::gpu_load("./resources/demo_game/assets/mesh/primitive_cube_sphere",
												 i_init.get_render_pipeline(),
												 age::asset::primitive_desc{
													 .size		= { 0.5, 0.5, 0.5 },
													 .seg_u		= 30,
													 .seg_v		= 30,
													 .mesh_kind = age::asset::e::primitive_mesh_kind::cube_sphere },
												 age::asset::e::vertex_kind::pnt_uv1)));

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

			AGE_LAMBDA((), { i_init.get_render_pipeline->set_main_camera(i_init.get_camera_id_vec[0]); }),

			// strong directional light from above-right - good for seeing transparency + shadows
			identity{ age::graphics::render_pipeline::forward_plus::directional_light_desc{
				.direction = age::normalize(float3{ -0.3f, -1.0f, 0.5f }),
				.intensity = 1.f,
				.color	   = age::srgb_to_linear(float3{ 1.0f, 0.9f, 0.9f }) } }
				| AGE_FUNC(i_init.get_render_pipeline().add_directional_light)
				| AGE_FUNC(i_init.get_directional_light_id_vec().emplace_back),

			// warm point light - left side, illuminates transparent objects from behind
			identity{ age::graphics::render_pipeline::forward_plus::point_light_desc{
				.position	 = float3{ -5.0f, 5.0f, 4.0f },
				.range		 = 50.0f,
				.color		 = age::srgb_to_linear(float3{ 1.0f, 0.7f, 0.3f }),
				.intensity	 = 8.0f,
				.cast_shadow = true } }
				| AGE_FUNC(i_init.get_render_pipeline->add_point_light)
				| AGE_FUNC(i_init.get_point_light_id_vec().emplace_back),

			// cool point light - right side, color mixing through transparent surfaces
			identity{ age::graphics::render_pipeline::forward_plus::point_light_desc{
				.position	 = float3{ 5.0f, 3.0f, -2.0f },
				.range		 = 50.0f,
				.color		 = age::srgb_to_linear(float3{ 0.3f, 0.5f, 1.0f }),
				.intensity	 = 8.0f,
				.cast_shadow = true } }
				| AGE_FUNC(i_init.get_render_pipeline->add_point_light)
				| AGE_FUNC(i_init.get_point_light_id_vec->emplace_back),

			// === meshes ===

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
				}),
			exec_inline{}
		}();
	}

	decltype(auto)
	update() noexcept
	{
		i_update.get_render_pipeline->begin_frame();

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

		{
			c_auto& ui_main_cam = i_update.get_render_pipeline->get_camera_data(0);
			age::ui::begin_frame(i_update.get_h_window, ui_main_cam.pos, ui_main_cam.view_proj_inv);
		}

		{
			using namespace age::ui;

			using enum age::input::e::key_kind;


			//{
			//	auto h_root = age::ui::root_begin({ .space_mode = age::ui::e::space_mode_kind::world,
			//										.width		= 1920.f,
			//										.height		= 1080.f,

			//										.world_pos	  = float3(0, 2, 10),
			//										.quaternion	  = float4(0, 0, 0, 1),
			//										.world_width  = 190.2f,
			//										.world_height = 100.8f

			//	});

			//	widget::begin(style::panel() | set_width_grow() | set_height_grow() | set_body_brush_data(float4(1, 0, 0, 1)));
			//}

			auto h_root = age::ui::root_begin({ .space_mode = age::ui::e::space_mode_kind::world,
												.width		= 1920.f,
												.height		= 1080.f,

												.world_pos	  = float3(0, 0, 10),	 //+ float3(-190.2 / 2.f, 100.8 / 2.f, 0),
												.quaternion	  = float4(0, 0, 0, 1),
												.world_width  = 190.2f,
												.world_height = 100.8f

			});

			//{
			//	age::ui::widget_desc desc = {};
			//	desc.width_min			  = 0;
			//	desc.width_max			  = FLT_MAX;
			//	desc.width_size_mode	  = e::size_mode_kind::grow;
			//	desc.height_min			  = 0;
			//	desc.height_max			  = FLT_MAX;
			//	desc.height_size_mode	  = e::size_mode_kind::grow;
			//	desc.child_gap			  = 0;

			//	auto _ = widget::begin(std::move(desc));
			//};

			//{
			//	auto res = widget::begin(style::layout(e::widget_layout::vertical)
			//							 | set_size(size_mode::grow(), size_mode::fit()));

			//	widget::begin(style::vertical() | set_size(size_mode::grow(), size_mode::fit()));

			//	auto _ = widget::begin(set_vertical() | set_size(size_mode::grow(), size_mode::fit()));

			//	widget::begin(set_width_grow() | set_height_fit());

			//	widget::begin(set_width_grow() | set_height_grow());
			//}

			//{
			//	age::ui::widget_desc desc = {};

			//	auto&& desc2 = AGE_LAMBDA((auto&& desc), { desc.width_min = 0; desc.width_max = FLT_MAX; desc.width_size_mode = e::size_mode_kind::grow; return FWD(desc); })(std::move(desc));
			//	auto&& desc3 = AGE_LAMBDA((auto&& desc), { desc.height_min = 0; desc.height_max = FLT_MAX; desc.height_size_mode = e::size_mode_kind::grow; return FWD(desc); })(std::move(desc2));
			//	widget::begin(std::move(desc3));
			//}

			//{
			//	// auto desc = set_width_grow();

			//	auto desc = AGE_LAMBDA((), { auto desc = age::ui::widget_desc{ .width_max = FLT_MAX }; desc.width_size_mode = e::size_mode_kind::fit;  return desc; })();

			//	widget::begin(std::move(desc));
			//}


			// auto _ = widget::begin(set_width_grow() | set_height_grow() | set_child_gap(0));


			// if (auto _ = widget::horizontal(set_draw(true), set_size(size_mode::grow(), size_mode::grow()), set_child_gap(0)))
			if (auto _ = widget::panel(set_width_grow(), set_height_grow(), set_child_gap(0)))
			{
				// if (auto _ = widget::panel_resizable_h(300, 1000,
				//									   set_layout(e::widget_layout::vertical),
				//									   set_size(size_mode::grow(), size_mode::grow())))

				if (auto _ = widget::panel_resizable_h(300, 1000))
				{
					c_auto opt = std::array{
						widget::dropdown_option{ .value = 0u, .label = "A" },
						widget::dropdown_option{ .value = 1u, .label = "B" },
						widget::dropdown_option{ .value = 2u, .label = "C" },
					};

					static auto val = 0u;

					widget::dropdown<uint32>(val, opt);
					// if (auto _ = widget::panel_resizable_v(100, 500,
					//									   set_size(size_mode::grow(), size_mode::grow())))
					{
						static auto color = float3{ 1, 0, 0 };

						widget::color_field(color);
					}

					{
						static auto color = float4{ 1, 0, 0, 1.f };

						widget::color_field(color);
					}

					{
						static auto	 color	   = float4{ 1, 0, 0, 1.f };
						static float intensity = 1.f;

						widget::color_field(color, intensity);
					}


					if (auto _ = widget::panel_resizable_v(100, 500))
					{
						for (auto i = 0; i < 10; ++i)
						{
							if (auto btn = widget::button("button2", set_align(e::widget_align::center)))
							{
								if (btn.clicked<mouse_left>())
								{
									std::println("button2 clicked");
								}
							}
						}
					}


					{
						if (auto header = widget::collapsible_header("collapsible header"))
						{
							if (auto btn = widget::button("button2", set_align(e::widget_align::center)))
							{
								if (btn.clicked<mouse_left>())
								{
									std::println("button2 clicked");
								}
							}
						}

						if (auto tree_node_0 = widget::tree_node("node_0"))
						{
							if (auto tree_node_1 = widget::tree_node("node_1"))
							{
								if (auto tree_node_1 = widget::tree_node("node_2"))
								{
									if (auto _ = widget::frame(e::style_state::active, set_horizontal(), set_size(size_mode::grow(), size_mode::fit())))
									{
										static float v0 = 10.f;
										widget::numeric_field(v0,
															  "X",
															  std::numeric_limits<float>::min(), std::numeric_limits<float>::max(),
															  theme::color_text_red());

										static float v1 = 10.f;
										widget::numeric_field(v1,
															  "Y",
															  std::numeric_limits<float>::min(), std::numeric_limits<float>::max(),
															  theme::color_text_green());

										static float v2 = 10.f;
										widget::numeric_field(v2,
															  "Z",
															  std::numeric_limits<float>::min(), std::numeric_limits<float>::max(),
															  theme::color_text_blue());

										static float v3 = 10.f;
										widget::numeric_field(v3,
															  "W",
															  std::numeric_limits<float>::min(), std::numeric_limits<float>::max(),
															  theme::color_text_amber());
									}
								}

								if (auto _ = widget::frame(e::style_state::active, set_horizontal(), set_size(size_mode::grow(), size_mode::fit())))
								{
									static uint64 v0 = 100;
									widget::numeric_field(v0,
														  "X",
														  std::numeric_limits<uint64>::min(), 10000ull,
														  theme::color_text_red());

									static uint64 v1 = 100;
									widget::numeric_field(v1,
														  "Y",
														  std::numeric_limits<uint64>::min(), 10000ull,
														  theme::color_text_green());

									static uint64 v2 = 100;
									widget::numeric_field(v2,
														  "Z",
														  std::numeric_limits<uint64>::min(), 10000ull,
														  theme::color_text_blue());

									static uint64 v3 = 100;
									widget::numeric_field(v3,
														  "W",
														  std::numeric_limits<uint64>::min(), 200ull,
														  theme::color_text_amber());
								}

								static float4 quat;
								widget::numeric_field(quat, "position");

								static float2x2 mat;
								widget::numeric_field(mat);

								static float3x3 mat3;
								widget::numeric_field(mat3);

								static mat44<int32> mat4;
								widget::numeric_field(mat4);
							}
						}

						{
							static float v = 10.f;
							widget::numeric_field(v);
						}

						static float v = 35.f;
						if (auto h_slider = widget::slider(v, 0.f, 100.f))
						{
						}

						// for (auto i = 0; i < 0; ++i)
						//{
						//	if (auto _ = widget::begin(style::panel()
						//							   | set_layout(e::widget_layout::horizontal)
						//							   | set_size(size_mode::grow(), size_mode::fit())))
						//	{
						//		// arrow
						//		widget::begin(set_align(e::widget_align::center)
						//					  | set_size(size_mode::fixed(22), size_mode::fixed(22))
						//					  | set_border_thickness(0.f)
						//					  | set_shape_kind(e::shape_kind::arrow_right)
						//					  | set_body_brush_data(brush_data::color(0.75, 0.75, 0.75)));

						//		widget::begin(style::text("hello text\n    hello       text    \n\n"
						//								  "hello text")
						//					  | set_font_size(22)
						//					  | set_body_brush_data(theme::color_text_red()));

						//		widget::begin(style::text("hello text\n    hello       text    \n\n"
						//								  "hello text")
						//					  | set_font_size(22)
						//					  | set_body_brush_data(theme::color_text_green()));

						//		widget::begin(style::text("hello text") | set_font_size(22) | set_padding(2, 100, 2, 2));

						//		widget::begin(style::text("hello text") | set_font_size(22));

						//		widget::text_title("hello text\n    hello text    \n"
						//						   "hello text");

						//		widget::text("...");
						//		widget::begin(style::text("...") | set_body_brush_data(theme::color_text_blue()));
						//		widget::begin(style::text("...") | set_body_brush_data(theme::color_text_amber()));
						//	}
						//}
					}

					if (auto _ = widget::panel(set_size(size_mode::grow(), size_mode::grow())))
					{
						static auto item_count = 5;
						if (auto btn = widget::button("add item", set_align(e::widget_align::center)))
						{
							if (btn.clicked<mouse_left>())
							{
								item_count += 5;
							}
						}

						if (auto btn = widget::button("remove item", set_align(e::widget_align::center)))
						{
							if (btn.clicked<mouse_left>())
							{
								item_count -= 5;
							}
						}

						// item_count = std::max(0, item_count);
						// if (auto _ = widget::scroll_area_v())
						//{
						//	// static char buf[256] = "this is te                     \nxt inpu\nasfdasfdt      \0";
						//	static char buf[256] = "A\nB\nC\n\0";


						//	widget::text_input(buf, 256);

						//	if (auto _ = widget::frame())
						//	{
						//		widget::text_title("t \nh");
						//	}

						//	for (auto _ : age::views::loop(item_count))
						//	{
						//		auto str = std::format("button {}", _);
						//		if (auto btn = widget::button(str.c_str(), set_align(e::widget_align::center)))
						//		{
						//			if (btn.clicked<mouse_left>())
						//			{
						//				std::println("{} clicked!", str);
						//			}
						//		}
						//	}
						//}

						// if (auto _ = widget::scroll_area_h())
						//{
						//	for (auto _ : age::views::loop(item_count))
						//	{
						//		auto str = std::format("button {}", _);
						//		if (auto btn = widget::button(str.c_str(), set_align(e::widget_align::center)))
						//		{
						//			if (btn.clicked<mouse_left>())
						//			{
						//				std::println("{} clicked!", str);
						//			}
						//		}
						//	}
						// }
					}
				}
			}
		}

		age::ui::end_frame(i_update.get_render_pipeline->get_ui_sink());

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
		age::graphics::command::signal();
		age::graphics::command::cpu_wait();
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
			age::asset::mesh_baked::full_unload(m_id, i_deinit.get_render_pipeline());
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

		i_deinit.get_mesh_id_vec->clear();
		i_deinit.get_opaque_obj_id_vec->clear();
		i_deinit.get_transparent_obj_id_vec->clear();
		i_deinit.get_camera_id_vec->clear();
		i_deinit.get_point_light_id_vec->clear();
		i_deinit.get_spot_light_id_vec->clear();
		i_deinit.get_directional_light_id_vec->clear();

		age::asset::registry::clear();
	}
}	 // namespace age_demo::scene_2
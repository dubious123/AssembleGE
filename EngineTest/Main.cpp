#include "pch.hpp"
#include "test.h"

auto forward_plus_pipeline = age::graphics::render_pipeline::forward_plus::pipeline();

auto h_window_test_app_1 = age::platform::window_handle{};
auto h_window_test_app_2 = age::platform::window_handle{};
auto h_window_test_app_3 = age::platform::window_handle{};

auto h_game_window	  = age::platform::window_handle{};
auto h_game_input_ctx = age::input::context_handle{};

auto h_forward_plus_rs = age::graphics::render_surface_handle{};

auto mesh_id_vec = age::vector<age::graphics::render_pipeline::forward_plus::t_mesh_id>::gen_reserved(1);
auto obj_id_vec	 = age::vector<age::graphics::render_pipeline::forward_plus::t_object_id>::gen_reserved(27);
auto camera_vec	 = age::vector<age::graphics::render_pipeline::forward_plus::t_camera_id>::gen_reserved(1);

auto test_game = my_game();

int
main()
{
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);
	using namespace age::ecs::system;

	run_benchmark(test_game, 1'000);

	if (false)
	{
		test_idx_pool();
		test_stable_dense_vector();
	}

	using namespace age::ecs::system;

	on_ctx{
		age::platform::init,
		age::graphics::init,
		age::runtime::init,

		AGE_FUNC(forward_plus_pipeline.init),

		identity{ age::graphics::render_pipeline::forward_plus::camera_desc{
			.kind		= age::graphics::e::camera_kind::perspective,
			.pos		= float3{ 0.f, 0.5f, -4.f },
			.quaternion = age::g::quaternion_identity,
			.perspective{
				.near_z		  = 0.1f,
				.far_z		  = 100.f,
				.fov_y		  = age::cvt_to_radian(75.f),
				.aspect_ratio = 16.f / 9.f } } }
			| AGE_FUNC(forward_plus_pipeline.add_camera)
			| AGE_FUNC(camera_vec.emplace_back),

		identity{ age::asset::primitive_desc{ .size = { 0.5, 0.5, 0.5 }, .seg_u = 30, .seg_v = 30, .mesh_kind = age::asset::e::primitive_mesh_kind::cube } }
			| age::asset::create_primitive_mesh
			| age::asset::bake_mesh<age::asset::vertex_pnt_uv1>
			| AGE_FUNC(forward_plus_pipeline.upload_mesh)
			| AGE_FUNC(mesh_id_vec.emplace_back),


		identity{ age::asset::primitive_desc{ .size = { 0.5, 0.5, 0.5 }, .seg_u = 30, .seg_v = 30, .mesh_kind = age::asset::e::primitive_mesh_kind::plane } }
			| age::asset::create_primitive_mesh
			| age::asset::bake_mesh<age::asset::vertex_pnt_uv1>
			| AGE_FUNC(forward_plus_pipeline.upload_mesh)
			| AGE_FUNC(mesh_id_vec.emplace_back),

		identity{ age::asset::primitive_desc{ .size = { 0.5, 0.5, 0.5 }, .seg_u = 30, .seg_v = 30, .mesh_kind = age::asset::e::primitive_mesh_kind::uv_sphere } }
			| age::asset::create_primitive_mesh
			| age::asset::bake_mesh<age::asset::vertex_pnt_uv1>
			| AGE_FUNC(forward_plus_pipeline.upload_mesh)
			| AGE_FUNC(mesh_id_vec.emplace_back),

		AGE_LAMBDA(
			(),
			{
				for (auto&& [pos_x, pos_y, pos_z] : std::views::cartesian_product(std::views::iota(-1, 2), std::views::iota(-1, 2), std::views::iota(-1, 2)))
				{
					auto data = age::graphics::render_pipeline::forward_plus::shared_type::object_data{
						.pos		= float3{ pos_x * 2, pos_y * 2, pos_z * 2 },
						.quaternion = age::math::quaternion_encode(age::g::quaternion_identity),
						.scale		= age::cvt_to<half3>(float3{ 1.0f, 1.0f, 1.0f })
					};

					obj_id_vec.emplace_back(forward_plus_pipeline.add_object(mesh_id_vec[0], data));
				}
			}),
		identity{ age::platform::window_desc{ 1080, 920, "test_app1" } }
			| age::platform::create_window
			| age::runtime::assign_to(h_window_test_app_1)
			| age::graphics::create_render_surface,

		identity{ age::platform::window_desc{ 1080 / 2, 920, "test_app2" } }
			| age::platform::create_window
			| age::runtime::assign_to(h_window_test_app_2)
			| age::graphics::create_render_surface,

		identity{ age::platform::window_desc{ 1080, 920 / 2, "test_app3" } }
			| age::platform::create_window
			| age::runtime::assign_to(h_window_test_app_3)
			| age::graphics::create_render_surface,

		identity{ age::platform::window_desc{ 1080 * 2, 920 * 2, "test_render_surface" } }
			| age::platform::create_window
			| age::runtime::assign_to(h_game_window)
			| age::graphics::create_render_surface
			| age::runtime::assign_to(h_forward_plus_rs),

		age::input::create_context
			| age::runtime::assign_to(h_game_input_ctx)
			| AGE_LAMBDA((), { age::platform::register_input_context(h_game_window, h_game_input_ctx); }),

		loop{ [warm_up_frame = 10] mutable { return --warm_up_frame > 0; },
			  age::platform::update,	// pump platform msg
			  age::runtime::update,
			  age::graphics::begin_frame,
			  age::graphics::end_frame },

		loop{ AGE_FUNC(age::global::get<age::runtime::interface>().running),

			  break_if_unlikely{ [] { return age::platform::window_count() == 0; } },
			  AGE_LAMBDA((), { age::input::begin_frame(h_game_input_ctx); }),
			  age::platform::update,	// pump platform msg
			  age::runtime::update,
			  age::graphics::begin_frame,
			  [] {
				  // std::println("now : {:%T}, delta_time_ns : {:%T}", age::global::get<age::runtime::interface>().now(), age::global::get<age::runtime::interface>().delta_time_ns());
				  // std::println("now : {:%F %T}", std::chrono::system_clock::now());
				  // std::println("now : {}ns", age::global::get<age::runtime::interface>().delta_time_ns().count());
				  // std::this_thread::sleep_for(std::chrono::seconds(1));
			  },

			  age::runtime::when_window_alive(h_game_window)
				  | AGE_LAMBDA(
					  (),
					  {
						  using namespace age::input;
						  struct
						  {
							  float2 move;
							  float2 look;
							  float	 zoom;
							  bool	 sprint;
							  bool	 right_mouse_down;
							  bool	 middle_mouse_down;
						  } game_input;

						  c_auto& input_ctx = *h_game_input_ctx;

						  game_input.move = float2{
							  input_ctx.is_down(e::key_kind::key_d) - input_ctx.is_down(e::key_kind::key_a),
							  input_ctx.is_down(e::key_kind::key_w) - input_ctx.is_down(e::key_kind::key_s),
						  };

						  game_input.look			   = input_ctx.mouse_delta;
						  game_input.zoom			   = input_ctx.wheel_delta;
						  game_input.sprint			   = input_ctx.is_down(e::key_kind::key_shift);
						  game_input.right_mouse_down  = input_ctx.is_down(e::key_kind::mouse_right);
						  game_input.middle_mouse_down = input_ctx.is_down(e::key_kind::mouse_middle);

						  return game_input;
					  })
				  | AGE_LAMBDA(
					  (auto&& game_input),
					  {
						  constexpr auto move_speed	   = 2.f;
						  constexpr auto sprint_mult   = 4.f;
						  constexpr auto sensitivity   = 0.003f;
						  constexpr auto zoom_speed	   = 2.f;
						  constexpr auto zoom_distance = 4.f;
						  constexpr auto pan_speed	   = 0.6f;

						  constexpr auto move_smoothing = 15.f / 2.f;
						  constexpr auto look_smoothing = 25.f / 2.f;
						  constexpr auto zoom_smoothing = 12.f / 2.f;

						  static auto euler_x = 0.f;
						  static auto euler_y = 0.f;

						  static auto smoothed_move = float2{ 0.f, 0.f };
						  static auto smoothed_look = float2{ 0.f, 0.f };
						  static auto smoothed_zoom = 0.f;
						  static auto smoothed_pan	= float2{ 0.f, 0.f };

						  c_auto dt_s = std::max(
							  age::global::get<age::runtime::interface>().delta_time_s(),
							  1.f / 160);

						  c_auto speed	  = game_input.sprint ? move_speed * sprint_mult : move_speed;
						  auto	 cam_desc = forward_plus_pipeline.get_camera_desc(camera_vec[0]);


						  c_auto move_smoothing_factor = 1.f - std::exp(-move_smoothing * dt_s);
						  c_auto look_smoothing_factor = 1.f - std::exp(-look_smoothing * dt_s);
						  c_auto zoom_smoothing_factor = 1.f - std::exp(-zoom_smoothing * dt_s);

						  smoothed_move = age::math::lerp(smoothed_move, game_input.move, move_smoothing_factor);
						  smoothed_zoom = age::math::lerp(smoothed_zoom, game_input.zoom, zoom_smoothing_factor);

						  auto look_target = game_input.right_mouse_down ? game_input.look : float2{ 0.f, 0.f };
						  smoothed_look	   = age::math::lerp(smoothed_look, look_target, look_smoothing_factor);

						  auto pan_target = game_input.middle_mouse_down ? game_input.look : float2{ 0.f, 0.f };
						  smoothed_pan	  = age::math::lerp(smoothed_pan, pan_target, look_smoothing_factor);


						  euler_y += smoothed_look.x * sensitivity;
						  euler_x += smoothed_look.y * sensitivity;
						  euler_x  = std::clamp(euler_x, -89.f * age::g::degree_to_radian, 89.f * age::g::degree_to_radian);

						  c_auto xm_look_quat = float3{ euler_x, euler_y, 0.f }
											  | age::simd::load()
											  | age::simd::euler_to_quat();

						  c_auto forward = age::simd::g::xm_forward_f4
										 | age::simd::rotate3(xm_look_quat)
										 | age::simd::to<float3>();
						  c_auto right	 = age::simd::g::xm_right_f4
										 | age::simd::rotate3(xm_look_quat)
										 | age::simd::to<float3>();
						  c_auto up		 = age::simd::g::xm_up_f4
										 | age::simd::rotate3(xm_look_quat)
										 | age::simd::to<float3>();

						  cam_desc.pos -= right * smoothed_pan.x * pan_speed * dt_s;
						  cam_desc.pos += up * smoothed_pan.y * pan_speed * dt_s;
						  cam_desc.pos += forward * smoothed_zoom * zoom_speed;
						  cam_desc.pos += (right * smoothed_move.x + forward * smoothed_move.y) * speed * dt_s;

						  cam_desc.quaternion = xm_look_quat
											  | age::simd::to<float4>();

						  forward_plus_pipeline.update_camera(camera_vec[0], cam_desc);
					  })
				  | AGE_LAMBDA(
					  (),
					  {
						  if (forward_plus_pipeline.begin_render(h_forward_plus_rs))
						  {
							  for (auto obj_id : obj_id_vec | std::views::drop(0) | std::views::take(9))
							  {
								  forward_plus_pipeline.render_mesh(obj_id % age::graphics::g::thread_count, obj_id, mesh_id_vec[0]);
							  }

							  for (auto obj_id : obj_id_vec | std::views::drop(9) | std::views::take(9))
							  {
								  forward_plus_pipeline.render_mesh(obj_id_vec[obj_id] % age::graphics::g::thread_count, obj_id, mesh_id_vec[1]);
							  }

							  for (auto obj_id : obj_id_vec | std::views::drop(18) | std::views::take(9))
							  {
								  forward_plus_pipeline.render_mesh(obj_id_vec[obj_id] % age::graphics::g::thread_count, obj_id, mesh_id_vec[2]);
							  }

							  forward_plus_pipeline.end_render(h_forward_plus_rs);
						  }
					  }),

			  age::runtime::when_window_alive(h_window_test_app_1),
			  age::runtime::when_window_alive(h_window_test_app_2),
			  age::runtime::when_window_alive(h_window_test_app_3),
			  age::graphics::end_frame },

		AGE_LAMBDA((), {
			for (auto o_id : obj_id_vec)
			{
				forward_plus_pipeline.remove_object(o_id);
			}

			for (auto m_id : mesh_id_vec)
			{
				forward_plus_pipeline.release_mesh(m_id);
			}

			for (auto c_id : camera_vec)
			{
				forward_plus_pipeline.remove_camera(c_id);
			}
		}),

		AGE_FUNC(forward_plus_pipeline.deinit), AGE_FUNC(test_game.deinit), age::graphics::deinit, age::platform::deinit, age::asset::deinit, exec_inline{}
	}();
}
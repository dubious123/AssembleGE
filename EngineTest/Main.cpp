#include "pch.hpp"
#include "test.h"

auto forward_plus_pipeline = age::graphics::render_pipeline::forward_plus::pipeline();

auto h_window_test_app_1 = age::platform::window_handle{};
auto h_window_test_app_2 = age::platform::window_handle{};
auto h_window_test_app_3 = age::platform::window_handle{};

auto h_game_window	   = age::platform::window_handle{};
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
				.fov_y		  = age::cvt_to_radian(120.f),
				.aspect_ratio = 16.f / 9.f } } }
			| AGE_FUNC(forward_plus_pipeline.add_camera)
			| AGE_FUNC(camera_vec.emplace_back),

		identity{ age::asset::primitive_desc{ .size = { 1, 1, 1 }, .seg_u = 30, .seg_v = 30, .mesh_kind = age::asset::e::primitive_mesh_kind::plane } }
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

		loop{ AGE_FUNC(age::global::get<age::runtime::interface>().running),

			  break_if_unlikely{ [] { return age::platform::window_count() == 0; } },

			  age::platform::update,	// pump platform msg
			  age::runtime::update,
			  age::graphics::begin_frame,
			  [] {
				  // std::println("now : {:%T}, delta_time_ns : {:%T}", age::global::get<age::runtime::interface>().now(), age::global::get<age::runtime::interface>().delta_time_ns());
				  std::println("now : {:%F %T}", std::chrono::system_clock::now());
				  std::println("now : {}ns", age::global::get<age::runtime::interface>().delta_time_ns().count());
				  // std::this_thread::sleep_for(std::chrono::seconds(1));
			  },

			  age::runtime::when_window_alive(h_game_window)
				  | [] {
						if (forward_plus_pipeline.begin_render(h_forward_plus_rs))
						{
							for (auto obj_id : obj_id_vec)
							{
								forward_plus_pipeline.render_mesh(0, obj_id, mesh_id_vec[0]);
							}

							forward_plus_pipeline.end_render(h_forward_plus_rs);
						} },

			  age::runtime::when_window_alive(h_window_test_app_1),
			  age::runtime::when_window_alive(h_window_test_app_2),
			  age::runtime::when_window_alive(h_window_test_app_3),
			  age::graphics::end_frame },

		AGE_LAMBDA(
			(),
			{
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

		AGE_FUNC(forward_plus_pipeline.deinit),
		AGE_FUNC(test_game.deinit),
		age::graphics::deinit,
		age::platform::deinit,
		age::asset::deinit,
		exec_inline{}
	}();
}
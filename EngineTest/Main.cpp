#include "pch.hpp"
#include "test.h"

#include <thread>

auto forward_plus_pipeline = age::graphics::render_pipeline::forward_plus::pipeline();

int
main()
{
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);
	using namespace age::ecs::system;

	auto mesh_id_vec = age::vector<age::graphics::render_pipeline::forward_plus::t_mesh_id>::gen_reserved(1);
	auto obj_id_vec	 = age::vector<age::graphics::render_pipeline::forward_plus::t_object_id>::gen_reserved(27);
	auto camera_vec	 = age::vector<age::graphics::render_pipeline::forward_plus::t_camera_id>::gen_reserved(1);

	auto _game = my_game();

	run_benchmark(_game, 1'000);

	if (false)
	{
		test_idx_pool();
		test_stable_dense_vector();
	}

	using namespace age::ecs::system;

	auto h_forward_plus_rs = age::graphics::render_surface_handle{};

	on_ctx{
		AGE_FUNC(age::platform::init),
		AGE_FUNC(age::graphics::init),
		AGE_FUNC(age::runtime::init),

		[&] {
			forward_plus_pipeline.init();

			{
				auto camera_desc = age::graphics::render_pipeline::forward_plus::camera_desc{
					.kind		= age::graphics::e::camera_kind::perspective,
					.pos		= float3{ 0.f, 0.5f, -4.f },
					.quaternion = age::g::quaternion_idntity,
					.perspective{
						.near_z		  = 0.1f,
						.far_z		  = 100.f,
						.fov_y		  = age::cvt_to_radian(120.f),
						.aspect_ratio = 16.f / 9.f }
				};

				auto camera_id = forward_plus_pipeline.add_camera(camera_desc);
				camera_vec.emplace_back(camera_id);

				// forward_plus_pipeline.set_render_target(camera_id, some_texture_or_render_surface_id) todo;
			}

			{
				auto prim_desc = age::asset::primitive_desc{ .size = { 1, 1, 1 }, .seg_u = 30, .seg_v = 30 };
				auto prim_edit = age::asset::create_primitive(prim_desc);
				auto baked	   = age::asset::bake_mesh<age::asset::vertex_pnt_uv1>(prim_edit);
				mesh_id_vec.emplace_back(forward_plus_pipeline.upload_mesh(baked));
			}

			for (auto&& [pos_x, pos_y, pos_z] : std::views::cartesian_product(std::views::iota(-1, 2), std::views::iota(-1, 2), std::views::iota(-1, 2)))
			{
				auto object_data = age::graphics::render_pipeline::forward_plus::object_data{
					.pos		= float3{ pos_x * 2, pos_y * 2, pos_z * 2 },
					.quaternion = age::math::quaternoin_encode(age::g::quaternion_idntity),
					.scale		= age::cvt_to<half3>(float3{ 1.0f, 1.0f, 1.0f })
				};

				obj_id_vec.emplace_back(forward_plus_pipeline.add_object(mesh_id_vec[0], object_data));
			}
		},

		identity{ age::platform::window_desc{ 1080, 920, "test_app1" } }
			| AGE_FUNC(age::platform::create_window)
			| AGE_FUNC(age::graphics::create_render_surface),
		identity{ age::platform::window_desc{ 1080 / 2, 920, "test_app2" } }
			| AGE_FUNC(age::platform::create_window)
			| AGE_FUNC(age::graphics::create_render_surface),
		identity{ age::platform::window_desc{ 1080, 920 / 2, "test_app3" } }
			| AGE_FUNC(age::platform::create_window)
			| AGE_FUNC(age::graphics::create_render_surface),
		identity{ age::platform::window_desc{ 1080 / 2, 920 / 2, "test_render_surface" } }
			| AGE_FUNC(age::platform::create_window)
			| AGE_FUNC(age::graphics::create_render_surface)
			| [&](const age::graphics::render_surface_handle& h_rs) { h_forward_plus_rs = h_rs; },

		loop{ AGE_FUNC(age::global::get<age::runtime::interface>().running),
			  AGE_FUNC(age::platform::update),	  // pump platform msg

			  AGE_FUNC(age::runtime::update),

			  AGE_FUNC(age::graphics::begin_frame),

			  [&] {
				  if (forward_plus_pipeline.begin_render(h_forward_plus_rs))
				  {
					  for (auto obj_id : obj_id_vec)
					  {
						  forward_plus_pipeline.render_mesh(0, obj_id, mesh_id_vec[0]);
					  }

					  forward_plus_pipeline.end_render(h_forward_plus_rs);
				  }
			  },

			  //[] { std::println("now : {:%T}, delta_time_ns : {:%T}", age::global::get<age::runtime::interface>().now(), age::global::get<age::runtime::interface>().delta_time_ns()); }
			  [] {
				  std::println("now : {:%F %T}", std::chrono::system_clock::now());
				  std::println("now : {}ns", age::global::get<age::runtime::interface>().delta_time_ns().count());
			  },
			  //[] { std::this_thread::sleep_for(std::chrono::seconds(1)); },
			  // AGE_FUNC(age::graphics::render),

			  AGE_FUNC(age::graphics::end_frame) },

		[&] {
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

			forward_plus_pipeline.deinit();
		},

		AGE_FUNC(age::graphics::deinit),
		AGE_FUNC(age::platform::deinit),
		AGE_FUNC(age::asset::deinit),
		exec_inline{}
	}

	();

	_game.deinit();
}
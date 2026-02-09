#include "pch.hpp"
#include "test.h"

struct Empty
{
};

struct A
{
	no_unique_addr Empty b;
	int					 a;
};

int
main()
{
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);
	using namespace age::ecs::system;

	A inst{};
	static_assert(sizeof(inst) == 4);

	auto _game = my_game();

	run_benchmark(_game, 1'000);

	if (false)
	{
		test_idx_pool();
		test_stable_dense_vector();
	}

	using namespace age::ecs::system;
	// age::asset::create_primitive(age::asset::primitive_desc{});
	// age::asset::create_primitive(age::asset::primitive_desc{ .seg_u = 1000 });
	// age::asset::create_primitive(age::asset::primitive_desc{ .seg_v = 1000 });
	// age::asset::create_primitive(age::asset::primitive_desc{ .seg_u = 8, .seg_v = 3 });
	on_ctx{
		AGE_FUNC(age::platform::init),
		AGE_FUNC(age::graphics::init),
		AGE_FUNC(age::runtime::init),
		identity{ age::asset::primitive_desc{} }
			| age::asset::create_primitive
			| age::asset::triangulate<age::asset::vertex_pnt_uv0>,

		identity{ age::asset::primitive_desc{ .size = { 10.f, 10.f, 10.f }, .seg_u = 99, .seg_v = 300 } }
			| age::asset::create_primitive
			| age::asset::triangulate<age::asset::vertex_pnt_uv0>,

		identity{ age::platform::window_desc{ 1080, 920, "test_app1" } }
			| AGE_FUNC(age::platform::create_window)
			| AGE_FUNC(age::graphics::create_render_surface),
		identity{ age::platform::window_desc{ 1080 / 2, 920, "test_app2" } }
			| AGE_FUNC(age::platform::create_window)
			| AGE_FUNC(age::graphics::create_render_surface),
		identity{ age::platform::window_desc{ 1080, 920 / 2, "test_app3" } }
			| AGE_FUNC(age::platform::create_window)
			| AGE_FUNC(age::graphics::create_render_surface),
		identity{ age::platform::window_desc{ 1080 / 2, 920 / 2, "test_app4" } }
			| AGE_FUNC(age::platform::create_window)
			| AGE_FUNC(age::graphics::create_render_surface),

		loop{ AGE_FUNC(age::global::get<age::runtime::interface>().running),
			  AGE_FUNC(age::platform::update),	  // pump platform msg

			  AGE_FUNC(age::runtime::update),

			  AGE_FUNC(age::graphics::begin_frame),


			  //[] { std::println("now : {:%T}, delta_time_ns : {:%T}", age::global::get<age::runtime::interface>().now(), age::global::get<age::runtime::interface>().delta_time_ns()); }
			  [] {
				  std::println("now : {:%F %T}", std::chrono::system_clock::now());
				  std::println("now : {}ns", age::global::get<age::runtime::interface>().delta_time_ns().count());
			  },
			  //[] { std::this_thread::sleep_for(std::chrono::seconds(1)); },
			  AGE_FUNC(age::graphics::render), AGE_FUNC(age::graphics::end_frame) },


		AGE_FUNC(age::graphics::deinit),
		AGE_FUNC(age::platform::deinit),
		AGE_FUNC(age::asset::deinit),
		exec_inline{}
	}();

	_game.deinit();
}
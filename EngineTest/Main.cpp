#pragma comment(lib, "engine.lib")
#include "pch.hpp"

#include "age.hpp"
#include "test.h"

int
main()
{
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);
	auto _game = my_game();

	run_benchmark(_game, 1'000);

	if (false)
	{
		test_idx_pool();
		test_stable_dense_vector();
	}


	using namespace age::ecs::system;

	on_ctx{
		AGE_FUNC(age::platform::init),
		AGE_FUNC(age::graphics::init),
		AGE_FUNC(age::runtime::init),

		identity{ age::platform::window_desc{ 1080, 920, "test_app1" } } | AGE_FUNC(age::platform::create_window) | AGE_FUNC(age::graphics::create_render_surface),
		identity{ age::platform::window_desc{ 1080 / 2, 920, "test_app2" } } | AGE_FUNC(age::platform::create_window) | AGE_FUNC(age::graphics::create_render_surface),
		identity{ age::platform::window_desc{ 1080, 920 / 2, "test_app3" } } | AGE_FUNC(age::platform::create_window) | AGE_FUNC(age::graphics::create_render_surface),
		identity{ age::platform::window_desc{ 1080 / 2, 920 / 2, "test_app4" } } | AGE_FUNC(age::platform::create_window) | AGE_FUNC(age::graphics::create_render_surface),

		loop{
			AGE_FUNC(age::global::get<age::runtime::interface>().running),
			AGE_FUNC(age::platform::update),	// pump platform msg
			AGE_FUNC(age::runtime::update),

			AGE_FUNC(age::graphics::begin_frame),


			//[] { std::println("now : {:%T}, delta_time_ns : {:%T}", age::global::get<age::runtime::interface>().now(), age::global::get<age::runtime::interface>().delta_time_ns()); }
			[] {
				std::println("now : {:%F %T}", std::chrono::system_clock::now());
				std::println("now : {}ns", age::global::get<age::runtime::interface>().delta_time_ns().count());
			},
			//[] { std::this_thread::sleep_for(std::chrono::seconds(1)); },
			AGE_FUNC(age::graphics::render),
			AGE_FUNC(age::graphics::end_frame)

		},


		AGE_FUNC(age::graphics::deinit),
		AGE_FUNC(age::platform::deinit),
		exec_inline{}
	}();

	_game.deinit();
}
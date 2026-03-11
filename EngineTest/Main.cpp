#include "pch.hpp"
#include "test.h"
#include "demo_game.hpp"

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

	on_ctx{
		age::platform::init,
		age::graphics::init,
		age::runtime::init,

		demo_game::get_sys_init(),

		demo_game::get_sys_loop(),

		demo_game::get_sys_deinit(),

		age::graphics::deinit,
		age::platform::deinit,
		age::asset::deinit,
		exec_inline{}
	}();

	on_ctx{
		age::platform::init,
		age::graphics::init,
		age::runtime::init,

		demo_game::get_sys_init(),

		demo_game::get_sys_loop(),

		demo_game::get_sys_deinit(),

		age::graphics::deinit,
		age::platform::deinit,
		age::asset::deinit,
		exec_inline{}
	}();

	test_game.deinit();
}
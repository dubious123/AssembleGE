#include "age_demo_pch.hpp"
#include "age_demo.hpp"

int
main()
{
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	//_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_CHECK_ALWAYS_DF);
	//_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_CHECK_ALWAYS_DF | _CRTDBG_LEAK_CHECK_DF);
	_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);
	using namespace age::ecs::system;

	on_ctx{
		age::platform::init,
		age::graphics::init,
		age::runtime::init,

		age_demo::game::get_sys_init(),

		age_demo::game::get_sys_loop(),

		age_demo::game::get_sys_deinit(),

		age::graphics::deinit,
		age::platform::deinit,
		age::asset::deinit,
		exec_inline{}
	}();

	on_ctx{
		age::platform::init,
		age::graphics::init,
		age::runtime::init,

		age_demo::game::get_sys_init(),

		age_demo::game::get_sys_loop(),

		age_demo::game::get_sys_deinit(),

		age::graphics::deinit,
		age::platform::deinit,
		age::asset::deinit,
		exec_inline{}
	}();
}
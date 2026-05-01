#include "age_demo_pch.hpp"
#include "age_demo.hpp"

int
main()
{
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	//_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);
	_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG | _CRTDBG_MODE_FILE);
	_CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDOUT);
	_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG | _CRTDBG_MODE_FILE);
	_CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDOUT);
	_CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_DEBUG | _CRTDBG_MODE_FILE);
	_CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDOUT);
	//_CrtSetBreakAlloc(227351);

	//_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_CHECK_ALWAYS_DF);
	//_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_CHECK_ALWAYS_DF | _CRTDBG_LEAK_CHECK_DF);

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

	// on_ctx{
	//	age::platform::init,
	//	age::graphics::init,
	//	age::runtime::init,

	//	age_demo::game::get_sys_init(),

	//	age_demo::game::get_sys_loop(),

	//	age_demo::game::get_sys_deinit(),

	//	age::graphics::deinit,
	//	age::platform::deinit,
	//	age::asset::deinit,
	//	exec_inline{}
	//}();
}
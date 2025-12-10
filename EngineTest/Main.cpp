#define _CRTDBG_MAP_ALLOC

#ifdef _DEBUG
	#define DBG_NEW new (_NORMAL_BLOCK, __FILE__, __LINE__)
// Replace _NORMAL_BLOCK with _CLIENT_BLOCK if you want the
// allocations to be of _CLIENT_BLOCK type
#else
	#define DBG_NEW new
#endif

#include "../Engine/age.hpp"

#include <libloaderapi.h>
#define Find(Type, Id) Model::Type::Find(Id)

typedef int (*import_func)();

#define LOAD_FUN(func_type, func_name, library)                        \
	[library]() {                                                      \
		using lib_func = func_type;                                    \
		auto func	   = (lib_func)GetProcAddress(library, func_name); \
		return func;                                                   \
	}()

#define LOAD_RUN_FUNC(func_type, func_name, library)                   \
	[library]() {                                                      \
		using lib_func = func_type;                                    \
		auto func	   = (lib_func)GetProcAddress(library, func_name); \
		return func();                                                 \
	}()
#include <array>

#pragma comment(lib, "engine.lib")


#include <cstdlib>
#include <crtdbg.h>
#include <Sysinfoapi.h>
#include <source_location>
#include <print>
#include <string>
#include <variant>

#include <future>
#include <random>
#include "test.h"

int
main()
{
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);
	auto _game = my_game();

	run_benchmark(_game, 1'000);

	using namespace age::ecs::system;

	on_ctx{
		AGE_FUNC(age::platform::init),
		AGE_FUNC(age::graphics::init),
		AGE_FUNC(age::runtime::init),

		identity{ age::platform::window_desc{ 1080, 920, "test_app" } } | AGE_FUNC(age::platform::creat_window),

		loop{
			AGE_FUNC(age::global::get<age::runtime::interface>().running),
			AGE_FUNC(age::platform::update),	// pump platform msg
			AGE_FUNC(age::runtime::update),
			//[] { std::println("now : {:%T}, delta_time_ns : {:%T}", age::global::get<age::runtime::interface>().now(), age::global::get<age::runtime::interface>().delta_time_ns()); }
			[] {
				std::println("now : {:%F %T}", std::chrono::system_clock::now());
				std::println("now : {}ns", age::global::get<age::runtime::interface>().delta_time_ns().count());
			}
			//[] { std::this_thread::sleep_for(std::chrono::seconds(1)); }
		},


		AGE_FUNC(age::graphics::deinit),
		AGE_FUNC(age::platform::deinit),
		exec_inline{}
	}();

	_game.deinit();
}
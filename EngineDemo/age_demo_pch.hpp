#pragma once
// #include <libloaderapi.h>
// #include <Sysinfoapi.h>

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

#if defined _DEBUG
	#include <crtdbg.h>
#endif

#include "age.hpp"
#include <random>
#include <chrono>

#pragma comment(lib, "Engine.lib")

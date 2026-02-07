#pragma once
#define _CRTDBG_MAP_ALLOC

#ifdef _DEBUG
	#define DBG_NEW new (_NORMAL_BLOCK, __FILE__, __LINE__)
// Replace _NORMAL_BLOCK with _CLIENT_BLOCK if you want the
// allocations to be of _CLIENT_BLOCK type
#else
	#define DBG_NEW new
#endif
//
// #define WIN32_LEAN_AND_MEAN
// #define NOMINMAX
// #include <Windows.h>
//
// #include <libloaderapi.h>
#include <crtdbg.h>
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


#include <chrono>
#include "age.hpp"

// #include <array>
// #include <cstdlib>

// #include <source_location>
//  #include <print>
//  #include <string>
//  #include <variant>
//
//  #include <future>
#include <random>

#pragma comment(lib, "Engine.lib")

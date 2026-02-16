#pragma once
#include <algorithm>
#include <array>
#include <bit>
#include <bitset>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <mdspan>
#include <memory>
#include <numeric>
#include <print>
#include <ranges>
#include <type_traits>
#include <tuple>

#ifdef AGE_DEBUG
	#include <iostream>
	#include <format>
#endif

#ifdef USE_STL_VECTOR
	#include <vector>
#endif

#ifdef USE_STL_SET
	#include <set>
#endif

#ifdef USE_STL_MAP
	#include <map>
#endif

#ifdef USE_STL_LIST
	#include <list>
#endif

#include <cstdint>

#if defined(AGE_PLATFORM_WINDOW)

	#include <DirectXMath.h>
	#include <DirectXPackedVector.h>

	#define NOGDI
// #define NOUSER
// #define NOMSG
// #define NOKERNEL
// #define NONLS
	#define NOMEMMGR
	#define NOSERVICE
	#define NOCOMM

	#define NOMINMAX
	#define WIN32_LEAN_AND_MEAN
	#include <windows.h>

	#undef NOGDI
	// #undef NOUSER
	// #undef NOMSG
	// #undef NOKERNEL
	// #undef NONLS
	#undef NOMEMMGR
	#undef NOSERVICE
	#undef NOCOMM

	#ifdef free
		#undef free
	#endif
#else
	#error "need at least one platform backend"
#endif

#if defined(AGE_GRAPHICS_BACKEND_DX12) && defined(AGE_PLATFORM_WINDOW)
	#include <dxgi1_6.h>
	#include <d3d12.h>
	#include <dxgidebug.h>
	#include "external\include\dxc\dxcapi.h"

	#if defined(interface)
		#undef interface
	#endif

	#pragma comment(lib, "d3d12.lib")
	#pragma comment(lib, "dxgi.lib")
	#pragma comment(lib, "dxcompiler.lib")
	#pragma comment(lib, "dxguid.lib")
#else
	#error "need at least one graphics backend"
#endif

#include "external\include\age_external\age_engine_external_libs.hpp"

#if defined(AGE_DEBUG)
	#pragma comment(lib, "age_engine_external_d.lib")

#elif defined(AGE_RELEASE)
	#pragma comment(lib, "age_engine_external.lib")
#else
	#error invalid configuration
#endif
#if defined(memcpy)
	#undef memcpy
#endif
#pragma once

#include <type_traits>
#include <array>
#include <tuple>
#include <ranges>
#include <algorithm>
#include <numeric>
#include <print>

#include <chrono>

#ifdef _DEBUG
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

	#define NOMINMAX
	#define WIN32_LEAN_AND_MEAN
	#include <windows.h>

	#ifdef free
		#undef free
	#endif
#else
	#error "need at least one platform backend"
#endif

#if defined(AGE_GRAPHICS_BACKEND_DX12) && defined(AGE_PLATFORM_WINDOW)
	#include <dxgi1_6.h>
	#include <d3d12.h>

	#if defined(interface)
		#undef interface
	#endif

	#pragma comment(lib, "d3d12.lib")
	#pragma comment(lib, "dxgi.lib")
#else
	#error "need at least one graphics backend"
#endif

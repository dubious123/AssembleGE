#pragma once

#include <type_traits>
#include <array>
#include <tuple>
#include <ranges>
#include <algorithm>
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
#if defined(_WIN32)
	#include <DirectXMath.h>

	#define NOMINMAX
	#define WIN32_LEAN_AND_MEAN
	#include <windows.h>
#else
#endif

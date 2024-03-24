#pragma once
#ifndef IMGUI_DEFINE_MATH_OPERATORS
	#define IMGUI_DEFINE_MATH_OPERATORS
#endif

#ifndef WIN32_LEAN_AND_MEAN
	#define WIN32_LEAN_AND_MEAN
#endif

#define STB_IMAGE_IMPLEMENTATION

#define is_true		   == true
#define is_false	   == false
#define is_not_nullptr != nullptr
#define is_nullptr	   == nullptr

#include <Windows.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <dwmapi.h>
#include <shlobj_core.h>

#include <xmemory>
#include <algorithm>
#include <sstream>
#include <format>
#include <array>
#include <filesystem>
#include <fstream>
#include <chrono>
#include <string>
#include <future>
#include <ranges>
#include <numeric>

#include "imgui\imgui.h"
#include "imgui\imgui_internal.h"
#include "imgui\imgui_impl_win32.h"
#include "imgui\imgui_impl_dx12.h"

#include <pugixml/pugixml.hpp>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>

#include <nativefiledialog\nfd.h>

#if defined(_WIN32)
	#include <DirectXMath.h>
#else
#endif

constexpr UINT LOG_MAX_COUNT_PER_FRAME = 1024;

namespace editor::logger
{
	namespace detail
	{
		using namespace std;
		extern const std::shared_ptr<spdlog::logger> _logger;
	}	 // namespace detail

	void init();

	void clear();

	template <typename... Args>
	inline void trace(fmt::format_string<Args...> fmt, Args&&... args)
	{
		detail::_logger->trace(fmt, std::forward<Args>(args)...);
	}

	template <typename T>
	inline void trace(const T& msg)
	{
		detail::_logger->trace(msg);
	}

	template <typename... Args>
	inline void debug(fmt::format_string<Args...> fmt, Args&&... args)
	{
		detail::_logger->debug(fmt, std::forward<Args>(args)...);
	}

	template <typename T>
	inline void debug(const T& msg)
	{
		detail::_logger->debug(msg);
	}

	template <typename... Args>
	inline void info(fmt::format_string<Args...> fmt, Args&&... args)
	{
		detail::_logger->info(fmt, std::forward<Args>(args)...);
	}

	template <typename T>
	inline void info(const T& msg)
	{
		detail::_logger->info(msg);
	}

	template <typename... Args>
	inline void warn(fmt::format_string<Args...> fmt, Args&&... args)
	{
		detail::_logger->warn(fmt, std::forward<Args>(args)...);
	}

	template <typename T>
	inline void warn(const T& msg)
	{
		detail::_logger->warn(msg);
	}

	template <typename... Args>
	inline void error(fmt::format_string<Args...> fmt, Args&&... args)
	{
		detail::_logger->error(fmt, std::forward<Args>(args)...);
	}

	template <typename T>
	inline void error(const T& msg)
	{
		detail::_logger->error(msg);
	}

	template <typename... Args>
	inline void critical(fmt::format_string<Args...> fmt, Args&&... args)
	{
		detail::_logger->critical(fmt, std::forward<Args>(args)...);
	}

	template <typename T>
	inline void critical(const T& msg)
	{
		detail::_logger->critical(msg);
	}
}	 // namespace editor::logger

namespace editor::view::logger
{
	void init();
	void show();
	void on_project_loaded();
}	 // namespace editor::view::logger
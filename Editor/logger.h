#pragma once

#include "spdlog\fmt\bundled\core.h"
#include "spdlog/sinks/base_sink.h"
#include "spdlog/logger.h"

namespace Editor::Logger
{
	namespace Detail
	{
		class editor_console_sink_st : public spdlog::sinks::base_sink<spdlog::details::null_mutex>
		{
		  protected:
			void sink_it_(const spdlog::details::log_msg& msg) override;

			void flush_() override;
		};

		extern const std::shared_ptr<spdlog::logger> _logger;
	}	 // namespace Detail

	void Init();

	template <typename... Args>
	inline void Trace(fmt::format_string<Args...> fmt, Args&&... args)
	{
		Detail::_logger->trace(fmt, std::forward<Args>(args)...);
	}

	template <typename T>
	inline void Trace(const T& msg)
	{
		Detail::_logger->trace(msg);
	}

	template <typename... Args>
	inline void Debug(fmt::format_string<Args...> fmt, Args&&... args)
	{
		Detail::_logger->debug(fmt, std::forward<Args>(args)...);
	}

	template <typename T>
	inline void Debug(const T& msg)
	{
		Detail::_logger->debug(msg);
	}

	template <typename... Args>
	inline void Info(fmt::format_string<Args...> fmt, Args&&... args)
	{
		Detail::_logger->info(fmt, std::forward<Args>(args)...);
	}

	template <typename T>
	inline void Info(const T& msg)
	{
		Detail::_logger->info(msg);
	}

	template <typename... Args>
	inline void Warn(fmt::format_string<Args...> fmt, Args&&... args)
	{
		Detail::_logger->warn(fmt, std::forward<Args>(args)...);
	}

	template <typename T>
	inline void Warn(const T& msg)
	{
		Detail::_logger->warn(msg);
	}

	template <typename... Args>
	inline void Error(fmt::format_string<Args...> fmt, Args&&... args)
	{
		Detail::_logger->error(fmt, std::forward<Args>(args)...);
	}

	template <typename T>
	inline void Error(const T& msg)
	{
		Detail::_logger->error(msg);
	}

	template <typename... Args>
	inline void Critical(fmt::format_string<Args...> fmt, Args&&... args)
	{
		Detail::_logger->critical(fmt, std::forward<Args>(args)...);
	}

	template <typename T>
	inline void Critical(const T& msg)
	{
		Detail::_logger->critical(msg);
	}
}	 // namespace Editor::Logger

namespace Editor::Logger::View
{
	void Show();
	void Open();
}	 // namespace Editor::Logger::View
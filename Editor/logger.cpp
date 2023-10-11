#include "spdlog/sinks/basic_file_sink.h"
#include "imgui/imgui.h"
#include "editor_constants.h"
#include "editor_common.h"
#include "Logger.h"
#include "editor_view.h"

namespace Editor::Logger
{
	namespace
	{
		char		  _log_buf[LOG_BUFFER_SIZE];
		ImVector<int> _line_offsets;
		UINT		  _write_pos = 0;
		UINT		  _log_count = 0;

		const std::shared_ptr<spdlog::sinks::basic_file_sink_st> _file_sink	   = std::make_shared<spdlog::sinks::basic_file_sink_st>("EditorLog.txt", true);
		const std::shared_ptr<Detail::editor_console_sink_st>	 _console_sink = std::make_shared<Detail::editor_console_sink_st>();
	}	 // namespace

	namespace Detail
	{
		const std::shared_ptr<spdlog::logger> _logger = std::make_shared<spdlog::logger>(spdlog::logger("Editor Logger", _file_sink));

		void editor_console_sink_st::sink_it_(const spdlog::details::log_msg& msg)
		{
			spdlog::memory_buf_t buf;
			spdlog::sinks::base_sink<spdlog::details::null_mutex>::formatter_->format(msg, buf);
			USHORT buf_size = buf.size();
			assert(_write_pos + buf_size < LOG_BUFFER_SIZE && "Too many logs");
			_line_offsets.push_back(_write_pos);
			memcpy(&_log_buf[_write_pos], buf.begin(), buf.size());
			_write_pos += buf_size;
			++_log_count;
		}

		// Todo : maybe add flush when buffer is full
		void editor_console_sink_st::flush_()
		{
			_write_pos = 0;
			_log_count = 0;
		}
	}	 // namespace Detail

	void Init()
	{
		_line_offsets.reserve_discard(LOG_MAX_COUNT_PER_FRAME);
		_console_sink->set_formatter(std::make_unique<spdlog::pattern_formatter>("%+", spdlog::pattern_time_type::local, std::string("")));
		Detail::_logger->sinks().push_back(_console_sink);
		Detail::_logger->info("Hi");
	}

}	 // namespace Editor::Logger

namespace Editor::Logger::View
{
	namespace
	{
		static auto _open = true;
	}

	void Open() { _open ^= true; }

	void Show()
	{
		if (_open is_false) return;

		if (ImGui::Begin("Console", &_open))
		{
			bool clear = ImGui::Button("Clear");

			ImGui::Separator();

			if (ImGui::BeginChild("scrolling", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar))
			{
				if (clear)
				{
					_write_pos = 0;
				}

				ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

				bool should_flush = _write_pos > LOG_BUFFER_SIZE * 0.8f;

				ImGuiListClipper clipper;
				clipper.Begin(_log_count);
				while (clipper.Step())
				{
					for (int line_no = clipper.DisplayStart; line_no < clipper.DisplayEnd; line_no++)
					{
						const char* line_start = _log_buf + _line_offsets[line_no];
						const char* line_end   = (line_no + 1 < _line_offsets.Size) ? (_log_buf + _line_offsets[line_no + 1]) : _log_buf + _write_pos;
						ImGui::TextUnformatted(line_start, line_end);
					}
				}
				clipper.End();

				ImGui::PopStyleVar();

				if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) ImGui::SetScrollHereY(1.0f);
				ImGui::EndChild();
			}
		}
		ImGui::End();
	}
}	 // namespace Editor::Logger::View

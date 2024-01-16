#include "pch.h"
#include "editor.h"

namespace
{
	char		  _log_buf[LOG_BUFFER_SIZE];
	ImVector<int> _line_offsets;
	int			  _write_pos = 0;
	int			  _log_count = 0;

	class editor_console_sink_st : public spdlog::sinks::base_sink<spdlog::details::null_mutex>
	{
	  protected:
		void sink_it_(const spdlog::details::log_msg& msg) override
		{
			spdlog::memory_buf_t buf;
			spdlog::sinks::base_sink<spdlog::details::null_mutex>::formatter_->format(msg, buf);
			auto buf_size = buf.size();
			assert(_write_pos + buf_size < LOG_BUFFER_SIZE && "Too many logs");
			_line_offsets.push_back(_write_pos);
			memcpy(&_log_buf[_write_pos], buf.begin(), buf.size());
			_write_pos += (int)buf_size;
			++_log_count;
		}

		// Todo : maybe add flush when buffer is full
		void flush_() override
		{
			_write_pos = 0;
			_log_count = 0;
			_line_offsets.clear();
		}
	};

	const std::shared_ptr<spdlog::sinks::basic_file_sink_st> _file_sink	   = std::make_shared<spdlog::sinks::basic_file_sink_st>("EditorLog.txt", true);
	const std::shared_ptr<editor_console_sink_st>			 _console_sink = std::make_shared<editor_console_sink_st>();
}	 // namespace

namespace editor::logger
{
	namespace detail
	{
		const std::shared_ptr<spdlog::logger> _logger = std::make_shared<spdlog::logger>(spdlog::logger("Editor Logger", _file_sink));
	}	 // namespace detail

	void init()
	{
		_line_offsets.reserve_discard(LOG_MAX_COUNT_PER_FRAME);
		_console_sink->set_formatter(std::make_unique<spdlog::pattern_formatter>("%+", spdlog::pattern_time_type::local, std::string("")));
		detail::_logger->sinks().push_back(_console_sink);
		detail::_logger->info("Hi");
	}

	void clear()
	{
		detail::_logger->flush();
	}

}	 // namespace editor::logger

namespace editor::view::Logger
{
	namespace
	{
		static auto		  _open		  = true;
		static const auto _cmd_toggle = editor_command {
			"Toggle Console Window",
			ImGuiKey_None,
			[](editor_id _) {
				return true;
			},
			[](editor_id _) {
				_open ^= true;
			}
		};
	}	 // namespace

	void init()
	{
	}

	void on_project_loaded()
	{
		editor::add_context_item("Main Menu\\Window\\Console", &_cmd_toggle);
	}

	void show()
	{
		if (_open is_false) return;

		if (ImGui::Begin("Console", &_open))
		{
			if (ImGui::Button("Clear"))
			{
				editor::logger::clear();
			}

			ImGui::Separator();

			if (ImGui::BeginChild("scrolling", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar))
			{
				ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

				bool should_flush = _write_pos > LOG_BUFFER_SIZE * 0.8f;

				ImGuiListClipper clipper;
				clipper.Begin((int)_log_count);
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
}	 // namespace editor::view::Logger

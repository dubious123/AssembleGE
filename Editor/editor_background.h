#pragma once

namespace editor::background
{
	void init();
	void add(editor_background, std::function<void()>&& task, std::function<void()>&& callback = nullptr);
	void deinit();
}	 // namespace editor::background
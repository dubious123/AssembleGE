#pragma once

namespace editor::background
{
	void init();
	void add_init(editor_background, bool (*)());
	void add_deinit(editor_background, bool (*)());
	void run();
	void add(editor_background, std::function<void()>&& task, std::function<void()>&& callback = nullptr);
	void deinit();
}	 // namespace editor::background
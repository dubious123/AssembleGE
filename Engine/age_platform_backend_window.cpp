#include "age.hpp"

namespace age::platform
{
	LRESULT CALLBACK
	window_proc(HWND handle, UINT message, WPARAM w_param, LPARAM l_param)
	{
		auto i_platform = age::global::get<age::platform::interface>();

		switch (message)
		{
		case WM_DESTROY:
			PostQuitMessage(0);
			return 0;
		case WM_SETCURSOR:
		{
			// Show an arrow instead of the busy cursor
			::SetCursor(::LoadCursor(NULL, IDC_ARROW));
			break;
		}
		}

		return (DefWindowProc(handle, message, w_param, l_param));
	}
}	 // namespace age::platform

namespace age::platform
{
	struct window_handle
	{
		HWND hwnd;
	};
}	 // namespace age::platform

namespace age::platform
{
	void
	init() noexcept
	{
		auto i_platform = global::get<platform::interface>();

		const auto wname = std::wstring{ i_platform.name().begin(), i_platform.name().end() };

		const auto window_class = WNDCLASSEX{
			.cbSize		   = sizeof(WNDCLASSEX),
			.style		   = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS,
			.lpfnWndProc   = window_proc,
			.cbClsExtra	   = 0,
			.cbWndExtra	   = 0,
			.hInstance	   = ::GetModuleHandle(nullptr),
			.hIcon		   = ::LoadIcon(nullptr, IDI_APPLICATION),
			.hCursor	   = ::LoadCursor(nullptr, IDC_ARROW),
			.hbrBackground = 0,
			.lpszMenuName  = 0,
			.lpszClassName = wname.c_str(),
			.hIconSm	   = ::LoadIcon(nullptr, IDI_APPLICATION),
		};

		if (auto res = ::RegisterClassEx(&window_class))
		{
			// todo
		}
	}

	void
	update() noexcept
	{
		auto i_platform = global::get<platform::interface>();

		MSG msg;
		while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
		{
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
			if (msg.message == WM_QUIT)
			{
				i_platform.running() = false;
			}
		}
	}

	void
	deinit() noexcept
	{
		auto	   i_platform = global::get<platform::interface>();
		const auto wname	  = std::wstring{ i_platform.name().begin(), i_platform.name().end() };
		::UnregisterClass(wname.c_str(), ::GetModuleHandle(nullptr));
	}

	void
	creat_window(window_desc&& desc) noexcept
	{
		auto	   i_platform	 = global::get<platform::interface>();
		const auto wname		 = std::wstring{ i_platform.name().begin(), i_platform.name().end() };
		const auto w_window_name = std::wstring{ desc.name.begin(), desc.name.end() };

		auto hwnd = CreateWindowEx(
			WS_EX_LEFT,							 //[in] DWORD					dwExStyle,
			wname.c_str(),						 //[ in, optional ] LPCWSTR		lpClassName,
			w_window_name.c_str(),				 //[ in, optional ] LPCWSTR		lpWindowName,
			WS_OVERLAPPEDWINDOW | WS_VISIBLE,	 //[in] DWORD					dwStyle,
			CW_USEDEFAULT,						 //[in] int						X,
			CW_USEDEFAULT,						 //[in] int						Y,
			desc.width,							 //[in] int						nWidth,
			desc.height,						 //[in] int						nHeight,
			nullptr,							 //[ in, optional ] HWND		hWndParent,
			nullptr,							 //[ in, optional ] HMENU		hMenu,
			::GetModuleHandle(nullptr),			 //[ in, optional ] HINSTANCE	hInstance,
			nullptr								 //[ in, optional ] LPVOID		lpParam
		);

		::ShowWindow(hwnd, SW_SHOW);
		::UpdateWindow(hwnd);

		// return window_handle{ hwnd };
	}
}	 // namespace age::platform
#include "age_pch.hpp"
#include "age.hpp"

#if defined AGE_PLATFORM_WINDOW
namespace age::platform
{
	struct window_info
	{
		HWND  hwnd;
		RECT  client_rect;
		POINT top_left_pos;

		window_mode mode;
	};
}	 // namespace age::platform

namespace age::platform::g
{
	data_structure::sparse_vector<window_info> window_info_vec;
}

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

		if constexpr (age::config::debug_mode)
		{
			g::window_info_vec.debug_validate();
		}
	}

	window_handle
	creat_window(window_desc&& desc) noexcept
	{
		auto	   i_platform	 = global::get<platform::interface>();
		const auto wname		 = std::wstring{ i_platform.name().begin(), i_platform.name().end() };
		const auto w_window_name = std::wstring{ desc.name.begin(), desc.name.end() };

		auto hwnd = ::CreateWindowEx(
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

		auto id = static_cast<t_window_id>(g::window_info_vec.emplace_back());

		::SetWindowLongPtr(hwnd, GWLP_USERDATA, static_cast<LONG_PTR>(id));
		::ShowWindow(hwnd, SW_SHOW);
		::UpdateWindow(hwnd);

		{
			auto wr = RECT{};
			::GetWindowRect(hwnd, &wr);
			auto cr = RECT{};
			::GetClientRect(hwnd, &cr);

			g::window_info_vec[id] = window_info{
				.hwnd		  = hwnd,
				.client_rect  = cr,
				.top_left_pos = POINT{ .x = wr.left, .y = wr.top },
				.mode		  = window_mode::windowed
			};
		}

		return window_handle{ .id = id };
	}

	FORCE_INLINE uint32
	get_client_width(window_handle handle) noexcept
	{
		auto cr = g::window_info_vec[handle.id].client_rect;
		return cr.right - cr.left;
	}

	FORCE_INLINE uint32
	get_client_height(window_handle handle) noexcept
	{
		auto cr = g::window_info_vec[handle.id].client_rect;
		return cr.bottom - cr.top;
	}
}	 // namespace age::platform

namespace age::platform
{
	FORCE_INLINE HWND
	get_hwnd(window_handle handle) noexcept
	{
		return g::window_info_vec[handle.id].hwnd;
	}

	FORCE_INLINE window_handle
	get_handle(HWND hwnd) noexcept
	{
		return window_handle{
			.id = static_cast<t_window_id>(::GetWindowLongPtr(hwnd, GWLP_USERDATA))
		};
	}
}	 // namespace age::platform

#endif
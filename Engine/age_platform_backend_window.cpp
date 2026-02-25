#include "age_pch.hpp"
#include "age.hpp"

#if defined AGE_PLATFORM_WINDOW
namespace age::platform
{
	struct window_info
	{
		HWND		 hwnd		  = {};
		RECT		 client_rect  = {};
		POINT		 top_left_pos = {};
		window_mode	 mode		  = {};
		window_state state		  = window_state::normal;
	};
}	 // namespace age::platform

namespace age::platform::g
{
	data_structure::stable_dense_vector<window_info> window_info_vec{ 2 };
}

namespace age::platform::detail
{
	LRESULT CALLBACK
	window_proc(HWND hwnd, UINT message, WPARAM w_param, LPARAM l_param)
	{
		auto i_platform = age::global::get<age::platform::interface>();

		auto h_window = get_handle(hwnd);

		if (g::window_info_vec[h_window].state == window_state::closing) [[unlikely]]
		{
			switch (message)
			{
			case WM_NCDESTROY:
			case WM_DESTROY:
			case WM_CLOSE:
				break;	  // 정상 종료 시퀀스는 통과
			}
		}
		else
		{
			switch (message)
			{
			case WM_CLOSE:
			{
				age::platform::on_window_closed(h_window);
				return LRESULT{};
			}
			case WM_SETCURSOR:
			{
				::SetCursor(::LoadCursor(NULL, IDC_ARROW));
				break;
			}
			case WM_SIZE:
			{
				switch (w_param)
				{
				case SIZE_RESTORED:
				case SIZE_MAXSHOW:
				{
					// The window has been resized, but neither the SIZE_MINIMIZED nor SIZE_MAXIMIZED value applies.
					age::platform::on_window_restored(get_handle(hwnd));
					break;
				}
				case SIZE_MINIMIZED:
				case SIZE_MAXHIDE:
				{
					// The window has been minimized.
					age::platform::on_window_minimized(get_handle(hwnd));
					break;
				}
				case SIZE_MAXIMIZED:
				{
					// The window has been maximized.
					age::platform::on_window_maximized(get_handle(hwnd));
					break;
				}
				}
				break;
			}
			case WM_EXITSIZEMOVE:
			{
				auto rect = RECT{};
				::GetClientRect(hwnd, &rect);

				age::platform::on_window_resized(
					platform::get_handle(hwnd),
					age::extent_2d<uint32>{
						.width	= static_cast<uint32>(rect.right - rect.left),
						.height = static_cast<uint32>(rect.bottom - rect.top) });
				break;
			}
			case WM_WINDOWPOSCHANGED:
			{
				break;
				// It is more efficient to perform any move or size change processing during the WM_WINDOWPOSCHANGED message without calling DefWindowProc.
			}
			}
		}

		return (::DefWindowProc(hwnd, message, w_param, l_param));
	}
}	 // namespace age::platform::detail

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
			.lpfnWndProc   = detail::window_proc,
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

		auto msg = MSG{};
		while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
		{
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);

			if (msg.message == WM_QUIT) [[unlikely]]
			{
				i_platform.running() = false;
			}
		}

		for (auto& req : request::for_each<subsystem::type::platform>())
		{
			switch (req.type)
			{
			case request::type::window_closed:
			{
				AGE_ASSERT(req.phase == 1, "[{}] : invalid phase : {}", to_string(req.type), req.phase);

				auto handle = req.req_param.as<window_handle>();
				::DestroyWindow(get_hwnd(handle));
				g::window_info_vec.remove(handle);

				if (g::window_info_vec.is_empty())
				{
					::PostQuitMessage(0);
				}

				request::set_done<
					subsystem::type::platform,
					request::type::window_closed, 1>(req);
				break;
			}
			default:
			{
				AGE_UNREACHABLE("invalid request type : {}", to_string(req.type));
			}
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
	create_window(const window_desc& desc) noexcept
	{
		auto	   i_platform	 = global::get<platform::interface>();
		const auto wname		 = std::wstring{ i_platform.name().begin(), i_platform.name().end() };
		const auto w_window_name = std::wstring{ desc.name.begin(), desc.name.end() };
		auto	   id			 = static_cast<t_window_id>(g::window_info_vec.emplace_back());

		auto hwnd = ::CreateWindowEx(
			WS_EX_LEFT,					   //[in] DWORD					dwExStyle,
			wname.c_str(),				   //[ in, optional ] LPCWSTR		lpClassName,
			w_window_name.c_str(),		   //[ in, optional ] LPCWSTR		lpWindowName,
			WS_OVERLAPPEDWINDOW,		   //[in] DWORD					dwStyle,
			CW_USEDEFAULT,				   //[in] int						X,
			CW_USEDEFAULT,				   //[in] int						Y,
			desc.width,					   //[in] int						nWidth,
			desc.height,				   //[in] int						nHeight,
			nullptr,					   //[ in, optional ] HWND		hWndParent,
			nullptr,					   //[ in, optional ] HMENU		hMenu,
			::GetModuleHandle(nullptr),	   //[ in, optional ] HINSTANCE	hInstance,
			nullptr						   //[ in, optional ] LPVOID		lpParam
		);

		::SetWindowLongPtr(hwnd, GWLP_USERDATA, static_cast<LONG_PTR>(id));
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
				.mode		  = window_mode::windowed,
				.state		  = window_state::normal,
			};
		}

		::ShowWindow(hwnd, SW_SHOW);

		return window_handle{ .id = id };
	}

	void
	move_window(window_handle w_handle, int32 x, int32 y) noexcept
	{
		AGE_WIN32_CHECK(::SetWindowPos(
			get_hwnd(w_handle),
			nullptr,
			x,
			y,
			0,
			0,
			SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE));
	}
}	 // namespace age::platform

namespace age::platform
{
	FORCE_INLINE void
	on_window_closed(window_handle w_handle) noexcept
	{
		g::window_info_vec[w_handle].state = window_state::closing;
		request::create<request::type::window_closed>(w_handle);
	}

	FORCE_INLINE void
	on_window_resized(window_handle h_window, age::extent_2d<uint32> extent) noexcept
	{
		auto& rect	= g::window_info_vec[h_window].client_rect;
		rect.right	= rect.left + extent.width;
		rect.bottom = rect.top + extent.height;
		age::request::create<request::type::window_resized>(h_window);
	}

	FORCE_INLINE void
	on_window_restored(window_handle h_window) noexcept
	{
		auto& w_info	   = g::window_info_vec[h_window];
		auto  before_state = w_info.state;

		switch (before_state)
		{
		case window_state::maximized:
		{
			::GetClientRect(w_info.hwnd, &w_info.client_rect);
			[[fallthrough]];
		}
		case window_state::minimized:
		{
			age::request::create<request::type::window_resized>(h_window);
			break;
		}
		case window_state::normal:
		{
			// just created
			break;
		}
		default:
		{
			AGE_UNREACHABLE();
		}
		}

		w_info.state = window_state::normal;
	}

	FORCE_INLINE void
	on_window_minimized(window_handle h_window) noexcept
	{
		g::window_info_vec[h_window].state = window_state::minimized;
		age::request::create<request::type::window_minimized>(h_window);
	}

	FORCE_INLINE void
	on_window_maximized(window_handle h_window) noexcept
	{
		// todo handle borderless
		auto& w_info = g::window_info_vec[h_window];
		::GetClientRect(w_info.hwnd, &w_info.client_rect);

		w_info.state = window_state::maximized;
		age::request::create<request::type::window_maximized>(h_window);
	}
}	 // namespace age::platform

namespace age::platform
{
	FORCE_INLINE uint32
	get_client_width(window_handle h_window) noexcept
	{
		auto cr = g::window_info_vec[h_window.id].client_rect;
		return cr.right - cr.left;
	}

	FORCE_INLINE uint32
	get_client_height(window_handle h_window) noexcept
	{
		auto cr = g::window_info_vec[h_window.id].client_rect;
		return cr.bottom - cr.top;
	}

	FORCE_INLINE window_state
	get_window_state(window_handle h_window) noexcept
	{
		return g::window_info_vec[h_window].state;
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
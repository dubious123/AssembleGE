#include "age_pch.hpp"
#include "age.hpp"

#if defined AGE_PLATFORM_WINDOW
namespace age::platform
{
	FORCE_INLINE auto*
	window_handle::operator->() noexcept
	{
		return &g::window_info_vec[id];
	}
}	 // namespace age::platform

namespace age::platform::detail
{
	void
	set_raw_input_impl(HWND hwnd, input::e::source_kind flags, bool receive_when_unfocused, bool disable_legacy) noexcept
	{
		auto rid_arr = std::array<RAWINPUTDEVICE, input::e::size<input::e::source_kind>()>{};

		c_auto base_flag   = DWORD{ receive_when_unfocused ? RIDEV_INPUTSINK : 0ul };
		c_auto legacy_flag = DWORD{ disable_legacy ? RIDEV_NOLEGACY : 0ul };

		auto rid_count = 0;

		if (input::e::has_any(flags, input::e::source_kind::mouse))
		{
			rid_arr[rid_count].usUsagePage	= 0x01;	   // HID_USAGE_PAGE_GENERIC
			rid_arr[rid_count].usUsage		= 0x02;	   // HID_USAGE_GENERIC_MOUSE
			rid_arr[rid_count].dwFlags		= base_flag | legacy_flag;
			rid_arr[rid_count++].hwndTarget = hwnd;
		}


		if (input::e::has_any(flags, input::e::source_kind::keyboard))
		{
			rid_arr[rid_count].usUsagePage	= 0x01;	   // HID_USAGE_PAGE_GENERIC
			rid_arr[rid_count].usUsage		= 0x06;	   // HID_USAGE_GENERIC_KEYBOARD
			rid_arr[rid_count].dwFlags		= base_flag | legacy_flag;
			rid_arr[rid_count++].hwndTarget = hwnd;
		}

		if (input::e::has_any(flags, input::e::source_kind::controller))
		{
			AGE_ASSERT(false, "not supported yet");
		}

		AGE_WIN32_CHECK(::RegisterRawInputDevices(rid_arr.data(), rid_count, sizeof(rid_arr[0])));
	}

	LRESULT CALLBACK
	window_proc(HWND hwnd, UINT message, WPARAM w_param, LPARAM l_param)
	{
		auto i_platform = age::global::get<age::platform::interface>();

		auto h_window = get_handle(hwnd);

		if (h_window->state == window_state::closing) [[unlikely]]
		{
			switch (message)
			{
			case WM_NCDESTROY:
			case WM_DESTROY:
			case WM_CLOSE:
				break;
			}
		}
		else
		{
			switch (message)
			{
			case WM_CLOSE:
			{
				h_window->close_requested = true;
				// age::platform::on_window_closed(h_window);
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
					age::platform::on_window_restored(h_window);
					break;
				}
				case SIZE_MINIMIZED:
				case SIZE_MAXHIDE:
				{
					// The window has been minimized.
					age::platform::on_window_minimized(h_window);
					break;
				}
				case SIZE_MAXIMIZED:
				{
					// The window has been maximized.
					age::platform::on_window_maximized(h_window);
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
					h_window,
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


				//---[ inputs ]------------------------------------------------------------


			case WM_KEYDOWN:
			case WM_SYSKEYDOWN:
			{
				if (age::runtime::is_handle_valid(h_window->h_input))
				{
					age::input::set_key_down(h_window->h_input, g::vk_lut[w_param & 0xff]);
				}

				break;
			}
			case WM_KEYUP:
			case WM_SYSKEYUP:
			{
				if (age::runtime::is_handle_valid(h_window->h_input))
				{
					age::input::set_key_up(h_window->h_input, g::vk_lut[w_param & 0xff]);
				}


				break;
			}
			case WM_MOUSEMOVE:
			{
				if (age::runtime::is_handle_valid(h_window->h_input))
				{
					age::input::set_mouse_move(h_window->h_input, float2{ static_cast<int16>(l_param & 0x0000ffff), static_cast<int16>(l_param >> 16) });
				}


				break;
			}
			case WM_LBUTTONDOWN:
			case WM_RBUTTONDOWN:
			case WM_MBUTTONDOWN:
			{
				if (age::runtime::is_handle_valid(h_window->h_input))
				{
					c_auto key = message == WM_LBUTTONDOWN ? input::e::key_kind::mouse_left
							   : message == WM_RBUTTONDOWN ? input::e::key_kind::mouse_right
														   : input::e::key_kind::mouse_middle;

					age::input::set_mouse_down(h_window->h_input, key, float2{ static_cast<int16>(l_param & 0x0000ffff), static_cast<int16>(l_param >> 16) });

					::SetCapture(hwnd);
				}

				break;
			}
			case WM_LBUTTONUP:
			case WM_RBUTTONUP:
			case WM_MBUTTONUP:
			{
				if (age::runtime::is_handle_valid(h_window->h_input))
				{
					c_auto key = message == WM_LBUTTONUP ? input::e::key_kind::mouse_left
							   : message == WM_RBUTTONUP ? input::e::key_kind::mouse_right
														 : input::e::key_kind::mouse_middle;

					age::input::set_mouse_up(h_window->h_input, key, float2{ static_cast<int16>(l_param & 0x0000ffff), static_cast<int16>(l_param >> 16) });

					::ReleaseCapture();
				}

				break;
			}
			case WM_MOUSEWHEEL:
			{
				if (age::runtime::is_handle_valid(h_window->h_input))
				{
					age::input::set_mouse_wheel(h_window->h_input, static_cast<float>(GET_WHEEL_DELTA_WPARAM(w_param)) / WHEEL_DELTA);
				}

				break;
			}
			case WM_KILLFOCUS:
			{
				if (age::runtime::is_handle_valid(h_window->h_input))
				{
					age::input::on_focus_kill(h_window->h_input);
				}

				break;
			}
			default:
				break;
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

		for (auto& req : age::request::for_each<subsystem::type::platform>())
		{
			switch (req.type)
			{
			case request::type::window_closed:
			{
				AGE_ASSERT(req.phase == 1, "[{}] : invalid phase : {}", to_string(req.type), req.phase);

				auto h_window = req.req_param.as<window_handle>();
				::DestroyWindow(get_hwnd(h_window));

				g::window_info_vec[h_window].state = window_state::closed;

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
	move_window(window_handle h_window, int32 x, int32 y) noexcept
	{
		AGE_WIN32_CHECK(::SetWindowPos(
			get_hwnd(h_window),
			nullptr,
			x,
			y,
			0,
			0,
			SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE));
	}

	void
	close_window(window_handle h_window) noexcept
	{
		if (g::window_info_vec[h_window].state == window_state::closing)
		{
			return;
		}

		g::window_info_vec[h_window].close_requested = true;

		on_window_closed(h_window);
	}

	bool
	window_close_requested(window_handle h_window) noexcept
	{
		return g::window_info_vec[h_window].close_requested;
	}

	bool
	is_window_closed(window_handle h_window) noexcept
	{
		return g::window_info_vec[h_window].state == window_state::closed;
	}

	void
	remove_window(window_handle& h_window) noexcept
	{
		AGE_ASSERT(g::window_info_vec[h_window].state == window_state::closed);
		g::window_info_vec.remove(h_window);

		h_window.id = invalid_id_uint32;
	}

	uint32
	window_count() noexcept
	{
		return g::window_info_vec.count();
	}

	void
	register_input_context(window_handle h_window, age::input::context_handle h_input) noexcept
	{
		AGE_ASSERT(runtime::is_handle_invalid(h_window->h_input) and runtime::is_handle_valid(h_input));
		h_window->h_input = h_input;
	}

	void
	enable_raw_input(window_handle h_window, age::input::e::source_kind source_kind_flag) noexcept
	{
		detail::set_raw_input_impl(h_window->hwnd, source_kind_flag, false, true);
	}

	void
	disable_raw_input(window_handle h_window, age::input::e::source_kind source_kind_flag) noexcept
	{
		detail::set_raw_input_impl(h_window->hwnd, source_kind_flag, false, false);
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
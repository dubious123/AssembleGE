#pragma once
#include "age.hpp"

namespace age::platform
{
	struct window_info
	{
		HWND					   hwnd			   = {};
		RECT					   client_rect	   = {};
		POINT					   top_left_pos	   = {};
		window_mode				   mode			   = {};
		window_state			   state		   = window_state::normal;
		bool					   close_requested = false;
		uint8					   padding;
		age::input::context_handle h_input;
	};
}	 // namespace age::platform

namespace age::platform::g
{
	inline data_structure::stable_dense_vector<window_info> window_info_vec{ 2 };

	inline constexpr input::e::key_kind vk_lut[256]{
		age::input::e::key_kind::none,				  // 0x00
		age::input::e::key_kind::mouse_left,		  // 0x01 VK_LBUTTON
		age::input::e::key_kind::mouse_right,		  // 0x02 VK_RBUTTON
		age::input::e::key_kind::none,				  // 0x03 VK_CANCEL
		age::input::e::key_kind::mouse_middle,		  // 0x04 VK_MBUTTON
		age::input::e::key_kind::mouse_x1,			  // 0x05 VK_XBUTTON1
		age::input::e::key_kind::mouse_x2,			  // 0x06 VK_XBUTTON2
		age::input::e::key_kind::none,				  // 0x07 RESERVED
		age::input::e::key_kind::key_backspace,		  // 0x08 VK_BACK
		age::input::e::key_kind::key_tab,			  // 0x09 VK_TAB
		age::input::e::key_kind::none,				  // 0x0A RESERVED
		age::input::e::key_kind::none,				  // 0x0B RESERVED
		age::input::e::key_kind::none,				  // 0x0C VK_CLEAR
		age::input::e::key_kind::key_return,		  // 0x0D VK_RETURN
		age::input::e::key_kind::none,				  // 0x0E UNASSIGNED
		age::input::e::key_kind::none,				  // 0x0F UNASSIGNED

		age::input::e::key_kind::key_shift,			  // 0x10 VK_SHIFT
		age::input::e::key_kind::key_ctrl,			  // 0x11 VK_CONTROL
		age::input::e::key_kind::key_alt,			  // 0x12 VK_MENU
		age::input::e::key_kind::key_pause,			  // 0x13 VK_PAUSE
		age::input::e::key_kind::key_capslock,		  // 0x14 VK_CAPSLOCK
		age::input::e::key_kind::none,				  // 0x15 VK_KANA, VK_HANGEUL, VK_HANGUL
		age::input::e::key_kind::none,				  // 0x16 VK_IME_ON
		age::input::e::key_kind::none,				  // 0x17 VK_JUNJA
		age::input::e::key_kind::none,				  // 0x18 VK_FINAL
		age::input::e::key_kind::none,				  // 0x19 VK_HANJA, VK_KANJI
		age::input::e::key_kind::none,				  // 0x1A VK_IME_OFF
		age::input::e::key_kind::key_escape,		  // 0x1B VK_ESCAPE
		age::input::e::key_kind::none,				  // 0x1C VK_CONVERT
		age::input::e::key_kind::none,				  // 0x1D VK_NONCONVERT
		age::input::e::key_kind::none,				  // 0x1E VK_ACCEPT
		age::input::e::key_kind::none,				  // 0x1F VK_MODECHANGE

		age::input::e::key_kind::key_space,			  // 0x20 VK_SPACE
		age::input::e::key_kind::key_page_up,		  // 0x21 VK_PRIOR
		age::input::e::key_kind::key_page_down,		  // 0x22 VK_NEXT
		age::input::e::key_kind::key_end,			  // 0x23 VK_END
		age::input::e::key_kind::key_home,			  // 0x24 VK_HOME
		age::input::e::key_kind::key_left,			  // 0x25 VK_LEFT
		age::input::e::key_kind::key_up,			  // 0x26 VK_UP
		age::input::e::key_kind::key_right,			  // 0x27 VK_RIGHT
		age::input::e::key_kind::key_down,			  // 0x28 VK_DOWN
		age::input::e::key_kind::none,				  // 0x29 VK_SELECT
		age::input::e::key_kind::none,				  // 0x2A VK_PRINT
		age::input::e::key_kind::none,				  // 0x2B VK_EXECUTE
		age::input::e::key_kind::key_print_screen,	  // 0x2C VK_SNAPSHOT
		age::input::e::key_kind::key_insert,		  // 0x2D VK_INSERT
		age::input::e::key_kind::key_delete,		  // 0x2E VK_DELETE
		age::input::e::key_kind::none,				  // 0x2F VK_HELP

		age::input::e::key_kind::key_0,				  // 0x30 VK_0
		age::input::e::key_kind::key_1,				  // 0x31 VK_1
		age::input::e::key_kind::key_2,				  // 0x32 VK_2
		age::input::e::key_kind::key_3,				  // 0x33 VK_3
		age::input::e::key_kind::key_4,				  // 0x34 VK_4
		age::input::e::key_kind::key_5,				  // 0x35 VK_5
		age::input::e::key_kind::key_6,				  // 0x36 VK_6
		age::input::e::key_kind::key_7,				  // 0x37 VK_7
		age::input::e::key_kind::key_8,				  // 0x38 VK_8
		age::input::e::key_kind::key_9,				  // 0x39 VK_9
		age::input::e::key_kind::none,				  // 0x3A UNASSIGNED
		age::input::e::key_kind::none,				  // 0x3B UNASSIGNED
		age::input::e::key_kind::none,				  // 0x3C UNASSIGNED
		age::input::e::key_kind::none,				  // 0x3D UNASSIGNED
		age::input::e::key_kind::none,				  // 0x3E UNASSIGNED
		age::input::e::key_kind::none,				  // 0x3F UNASSIGNED

		age::input::e::key_kind::none,				  // 0x40 UNASSIGNED
		age::input::e::key_kind::key_a,				  // 0x41 VK_A
		age::input::e::key_kind::key_b,				  // 0x42 VK_B
		age::input::e::key_kind::key_c,				  // 0x43 VK_C
		age::input::e::key_kind::key_d,				  // 0x44 VK_D
		age::input::e::key_kind::key_e,				  // 0x45 VK_E
		age::input::e::key_kind::key_f,				  // 0x46 VK_F
		age::input::e::key_kind::key_g,				  // 0x47 VK_G
		age::input::e::key_kind::key_h,				  // 0x48 VK_H
		age::input::e::key_kind::key_i,				  // 0x49 VK_I
		age::input::e::key_kind::key_j,				  // 0x4A VK_J
		age::input::e::key_kind::key_k,				  // 0x4B VK_K
		age::input::e::key_kind::key_l,				  // 0x4C VK_L
		age::input::e::key_kind::key_m,				  // 0x4D VK_M
		age::input::e::key_kind::key_n,				  // 0x4E VK_N
		age::input::e::key_kind::key_o,				  // 0x4F VK_O

		age::input::e::key_kind::key_p,				  // 0x50 VK_P
		age::input::e::key_kind::key_q,				  // 0x51 VK_Q
		age::input::e::key_kind::key_r,				  // 0x52 VK_R
		age::input::e::key_kind::key_s,				  // 0x53 VK_S
		age::input::e::key_kind::key_t,				  // 0x54 VK_T
		age::input::e::key_kind::key_u,				  // 0x55 VK_U
		age::input::e::key_kind::key_v,				  // 0x56 VK_V
		age::input::e::key_kind::key_w,				  // 0x57 VK_W
		age::input::e::key_kind::key_x,				  // 0x58 VK_X
		age::input::e::key_kind::key_y,				  // 0x59 VK_Y
		age::input::e::key_kind::key_z,				  // 0x5A VK_Z
		age::input::e::key_kind::none,				  // 0x5B VK_LWIN
		age::input::e::key_kind::none,				  // 0x5C VK_RWIN
		age::input::e::key_kind::none,				  // 0x5D VK_APPS
		age::input::e::key_kind::none,				  // 0x5E RESERVED
		age::input::e::key_kind::none,				  // 0x5F VK_SLEEP

		age::input::e::key_kind::key_numpad_0,		  // 0x60 VK_NUMPAD_0
		age::input::e::key_kind::key_numpad_1,		  // 0x61 VK_NUMPAD_1
		age::input::e::key_kind::key_numpad_2,		  // 0x62 VK_NUMPAD_2
		age::input::e::key_kind::key_numpad_3,		  // 0x63 VK_NUMPAD_3
		age::input::e::key_kind::key_numpad_4,		  // 0x64 VK_NUMPAD_4
		age::input::e::key_kind::key_numpad_5,		  // 0x65 VK_NUMPAD_5
		age::input::e::key_kind::key_numpad_6,		  // 0x66 VK_NUMPAD_6
		age::input::e::key_kind::key_numpad_7,		  // 0x67 VK_NUMPAD_7
		age::input::e::key_kind::key_numpad_8,		  // 0x68 VK_NUMPAD_8
		age::input::e::key_kind::key_numpad_9,		  // 0x69 VK_NUMPAD_9
		age::input::e::key_kind::key_multiply,		  // 0x6A VK_MULTIPLY
		age::input::e::key_kind::key_add,			  // 0x6B VK_ADD
		age::input::e::key_kind::none,				  // 0x6C VK_SEPARATOR
		age::input::e::key_kind::key_subtract,		  // 0x6D VK_SUBTRACT
		age::input::e::key_kind::key_decimal,		  // 0x6E VK_DECIMAL
		age::input::e::key_kind::key_divide,		  // 0x6F VK_DIVIDE

		age::input::e::key_kind::key_f1,			  // 0x70 VK_F1
		age::input::e::key_kind::key_f2,			  // 0x71 VK_F2
		age::input::e::key_kind::key_f3,			  // 0x72 VK_F3
		age::input::e::key_kind::key_f4,			  // 0x73 VK_F4
		age::input::e::key_kind::key_f5,			  // 0x74 VK_F5
		age::input::e::key_kind::key_f6,			  // 0x75 VK_F6
		age::input::e::key_kind::key_f7,			  // 0x76 VK_F7
		age::input::e::key_kind::key_f8,			  // 0x77 VK_F8
		age::input::e::key_kind::key_f9,			  // 0x78 VK_F9
		age::input::e::key_kind::key_f10,			  // 0x79 VK_F10
		age::input::e::key_kind::key_f11,			  // 0x7A VK_F11
		age::input::e::key_kind::key_f12,			  // 0x7B VK_F12
		age::input::e::key_kind::none,				  // 0x7C VK_F13
		age::input::e::key_kind::none,				  // 0x7D VK_F14
		age::input::e::key_kind::none,				  // 0x7E VK_F15
		age::input::e::key_kind::none,				  // 0x7F VK_F16

		age::input::e::key_kind::none,				  // 0x80 VK_F17
		age::input::e::key_kind::none,				  // 0x81 VK_F18
		age::input::e::key_kind::none,				  // 0x82 VK_F19
		age::input::e::key_kind::none,				  // 0x83 VK_F20
		age::input::e::key_kind::none,				  // 0x84 VK_F21
		age::input::e::key_kind::none,				  // 0x85 VK_F22
		age::input::e::key_kind::none,				  // 0x86 VK_F23
		age::input::e::key_kind::none,				  // 0x87 VK_F24
		age::input::e::key_kind::none,				  // 0x88 VK_NAVIGATION_VIEW	   RESERVED
		age::input::e::key_kind::none,				  // 0x89 VK_NAVIGATION_MENU	   RESERVED
		age::input::e::key_kind::none,				  // 0x8A VK_NAVIGATION_UP	       RESERVED
		age::input::e::key_kind::none,				  // 0x8B VK_NAVIGATION_DOWN	   RESERVED
		age::input::e::key_kind::none,				  // 0x8C VK_NAVIGATION_LEFT	   RESERVED
		age::input::e::key_kind::none,				  // 0x8D VK_NAVIGATION_RIGHT     RESERVED
		age::input::e::key_kind::none,				  // 0x8E VK_NAVIGATION_ACCEPT    RESERVED
		age::input::e::key_kind::none,				  // 0x8F VK_NAVIGATION_CANCEL	   RESERVED

		age::input::e::key_kind::key_numlock,		  // 0x90 VK_NUMLOCK
		age::input::e::key_kind::key_scroll,		  // 0x91 VK_SCROLL
		age::input::e::key_kind::none,				  // 0x92 VK_OEM_NEC_EQUAL, VK_OEM_FJ_JISHO
		age::input::e::key_kind::none,				  // 0x93 VK_OEM_FJ_MASSHOU
		age::input::e::key_kind::none,				  // 0x94 VK_OEM_FJ_TOUROKU
		age::input::e::key_kind::none,				  // 0x95 VK_OEM_FJ_LOYA
		age::input::e::key_kind::none,				  // 0x96 VK_OEM_FJ_ROYA
		age::input::e::key_kind::none,				  // 0x97 UNASSIGNED
		age::input::e::key_kind::none,				  // 0x98 UNASSIGNED
		age::input::e::key_kind::none,				  // 0x99 UNASSIGNED
		age::input::e::key_kind::none,				  // 0x9A UNASSIGNED
		age::input::e::key_kind::none,				  // 0x9B UNASSIGNED
		age::input::e::key_kind::none,				  // 0x9C UNASSIGNED
		age::input::e::key_kind::none,				  // 0x9D UNASSIGNED
		age::input::e::key_kind::none,				  // 0x9E UNASSIGNED
		age::input::e::key_kind::none,				  // 0x9F UNASSIGNED


		age::input::e::key_kind::key_left_shift,	  // 0xA0 VK_LSHIFT		VK_L* & VK_R* - left and right Alt, Ctrl and Shift virtual keys.
		age::input::e::key_kind::key_right_shift,	  // 0xA1 VK_RSHIFT		Used only as parameters to GetAsyncKeyState() and GetKeyState().
		age::input::e::key_kind::key_left_ctrl,		  // 0xA2 VK_LCONTROL		No other API or message will distinguish left and right keys in this way.
		age::input::e::key_kind::key_right_ctrl,	  // 0xA3 VK_RCONTROL
		age::input::e::key_kind::key_left_alt,		  // 0xA4 VK_LMENU
		age::input::e::key_kind::key_right_alt,		  // 0xA5 VK_RMENU
		age::input::e::key_kind::none,				  // 0xA6 VK_BROWSER_BACK
		age::input::e::key_kind::none,				  // 0xA7 VK_BROWSER_FORWARD
		age::input::e::key_kind::none,				  // 0xA8 VK_BROWSER_REFRESH
		age::input::e::key_kind::none,				  // 0xA9 VK_BROWSER_STOP
		age::input::e::key_kind::none,				  // 0xAA VK_BROWSER_SEARCH
		age::input::e::key_kind::none,				  // 0xAB VK_BROWSER_FAVORITES
		age::input::e::key_kind::none,				  // 0xAC VK_BROWSER_HOME
		age::input::e::key_kind::none,				  // 0xAD VK_VOLUME_MUTE
		age::input::e::key_kind::none,				  // 0xAE VK_VOLUME_DOWN
		age::input::e::key_kind::none,				  // 0xAF VK_VOLUME_UP

		age::input::e::key_kind::none,				  // 0xB0 VK_MEDIA_NEXT_TRACK
		age::input::e::key_kind::none,				  // 0xB1 VK_MEDIA_PREV_TRACK
		age::input::e::key_kind::none,				  // 0xB2 VK_MEDIA_STOP
		age::input::e::key_kind::none,				  // 0xB3 VK_MEDIA_PLAY_PAUSE
		age::input::e::key_kind::none,				  // 0xB4 VK_LAUNCH_MAIL
		age::input::e::key_kind::none,				  // 0xB5 VK_LAUNCH_MEDIA_SELECT
		age::input::e::key_kind::none,				  // 0xB6 VK_LAUNCH_APP1
		age::input::e::key_kind::none,				  // 0xB7 VK_LAUNCH_APP2
		age::input::e::key_kind::none,				  // 0xB8 RESERVED
		age::input::e::key_kind::none,				  // 0xB9 RESERVED
		age::input::e::key_kind::none,				  // 0xBA VK_OEM_1	      ';:' for US
		age::input::e::key_kind::none,				  // 0xBB VK_OEM_PLUS	  '+' any country
		age::input::e::key_kind::none,				  // 0xBC VK_OEM_COMMA	  ',' any country
		age::input::e::key_kind::none,				  // 0xBD VK_OEM_MINUS	  '-' any country
		age::input::e::key_kind::none,				  // 0xBE VK_OEM_PERIOD	  '.' any country
		age::input::e::key_kind::none,				  // 0xBF VK_OEM_2	  	  '/?' for US

		age::input::e::key_kind::none,				  // 0xC0 VK_OEM_3        '`~' for US
		age::input::e::key_kind::none,				  // 0xC1 RESERVED
		age::input::e::key_kind::none,				  // 0xC2 RESERVED
		age::input::e::key_kind::none,				  // 0xC3 VK_GAMEPAD_A					    RESERVED
		age::input::e::key_kind::none,				  // 0xC4 VK_GAMEPAD_B					    RESERVED
		age::input::e::key_kind::none,				  // 0xC5 VK_GAMEPAD_X					    RESERVED
		age::input::e::key_kind::none,				  // 0xC6 VK_GAMEPAD_Y					    RESERVED
		age::input::e::key_kind::none,				  // 0xC7 VK_GAMEPAD_RIGHT_SHOULDER		    RESERVED
		age::input::e::key_kind::none,				  // 0xC8 VK_GAMEPAD_LEFT_SHOULDER		    RESERVED
		age::input::e::key_kind::none,				  // 0xC9 VK_GAMEPAD_LEFT_TRIGGER			    RESERVED
		age::input::e::key_kind::none,				  // 0xCA VK_GAMEPAD_RIGHT_TRIGGER		    RESERVED
		age::input::e::key_kind::none,				  // 0xCB VK_GAMEPAD_DPAD_UP				    RESERVED
		age::input::e::key_kind::none,				  // 0xCC VK_GAMEPAD_DPAD_DOWN			    RESERVED
		age::input::e::key_kind::none,				  // 0xCD VK_GAMEPAD_DPAD_LEFT			    RESERVED
		age::input::e::key_kind::none,				  // 0xCE VK_GAMEPAD_DPAD_RIGHT			    RESERVED
		age::input::e::key_kind::none,				  // 0xCF VK_GAMEPAD_MENU					    RESERVED

		age::input::e::key_kind::none,				  // 0xD0 VK_GAMEPAD_VIEW					    RESERVED
		age::input::e::key_kind::none,				  // 0xD1 VK_GAMEPAD_LEFT_THUMBSTICK_BUTTON   RESERVED
		age::input::e::key_kind::none,				  // 0xD2 VK_GAMEPAD_RIGHT_THUMBSTICK_BUTTON  RESERVED
		age::input::e::key_kind::none,				  // 0xD3 VK_GAMEPAD_LEFT_THUMBSTICK_UP	    RESERVED
		age::input::e::key_kind::none,				  // 0xD4 VK_GAMEPAD_LEFT_THUMBSTICK_DOWN	    RESERVED
		age::input::e::key_kind::none,				  // 0xD5 VK_GAMEPAD_LEFT_THUMBSTICK_RIGHT    RESERVED
		age::input::e::key_kind::none,				  // 0xD6 VK_GAMEPAD_LEFT_THUMBSTICK_LEFT	    RESERVED
		age::input::e::key_kind::none,				  // 0xD7 VK_GAMEPAD_RIGHT_THUMBSTICK_UP	    RESERVED
		age::input::e::key_kind::none,				  // 0xD8 VK_GAMEPAD_RIGHT_THUMBSTICK_DOWN    RESERVED
		age::input::e::key_kind::none,				  // 0xD9 VK_GAMEPAD_RIGHT_THUMBSTICK_RIGHT   RESERVED
		age::input::e::key_kind::none,				  // 0xDA VK_GAMEPAD_RIGHT_THUMBSTICK_LEFT    RESERVED
		age::input::e::key_kind::none,				  // 0xDB VK_OEM_4 '[{' for US
		age::input::e::key_kind::none,				  // 0xDC VK_OEM_5 '\|' for US
		age::input::e::key_kind::none,				  // 0xDD VK_OEM_6 ']}' for US
		age::input::e::key_kind::none,				  // 0xDE VK_OEM_7 ''"' for US
		age::input::e::key_kind::none,				  // 0xDF VK_OEM_8

		age::input::e::key_kind::none,				  // 0xE0 RESERVED
		age::input::e::key_kind::none,				  // 0xE1 VK_OEM_AX	 'AX' key on Japanese AX kbd
		age::input::e::key_kind::none,				  // 0xE2 VK_OEM_102  "<>" or "\|" on RT 102-key kbd.
		age::input::e::key_kind::none,				  // 0xE3 VK_ICO_HELP  Help key on ICO
		age::input::e::key_kind::none,				  // 0xE4 VK_ICO_00	  00 key on ICO
		age::input::e::key_kind::none,				  // 0xE5 VK_PROCESSKEY
		age::input::e::key_kind::none,				  // 0xE6 VK_ICO_CLEAR
		age::input::e::key_kind::none,				  // 0xE7 VK_PACKET
		age::input::e::key_kind::none,				  // 0xE8 UNASSIGNED
		age::input::e::key_kind::none,				  // 0xE9 VK_OEM_RESET
		age::input::e::key_kind::none,				  // 0xEA VK_OEM_JUMP
		age::input::e::key_kind::none,				  // 0xEB VK_OEM_PA1
		age::input::e::key_kind::none,				  // 0xEC VK_OEM_PA2
		age::input::e::key_kind::none,				  // 0xED VK_OEM_PA3
		age::input::e::key_kind::none,				  // 0xEE VK_OEM_WSCTRL
		age::input::e::key_kind::none,				  // 0xEF VK_OEM_CUSEL

		age::input::e::key_kind::none,				  // 0xF0 VK_OEM_ATTN
		age::input::e::key_kind::none,				  // 0xF1 VK_OEM_FINISH
		age::input::e::key_kind::none,				  // 0xF2 VK_OEM_COPY
		age::input::e::key_kind::none,				  // 0xF3 VK_OEM_AUTO
		age::input::e::key_kind::none,				  // 0xF4 VK_OEM_ENLW
		age::input::e::key_kind::none,				  // 0xF5 VK_OEM_BACKTAB
		age::input::e::key_kind::none,				  // 0xF6 VK_ATTN
		age::input::e::key_kind::none,				  // 0xF7 VK_CRSEL
		age::input::e::key_kind::none,				  // 0xF8 VK_EXSEL
		age::input::e::key_kind::none,				  // 0xF9 VK_EREOF
		age::input::e::key_kind::none,				  // 0xFA VK_PLAY
		age::input::e::key_kind::none,				  // 0xFB VK_ZOOM
		age::input::e::key_kind::none,				  // 0xFC VK_NONAME
		age::input::e::key_kind::none,				  // 0xFD VK_PA1
		age::input::e::key_kind::none,				  // 0xFE VK_OEM_CLEAR
		age::input::e::key_kind::none,				  // 0xFF RESERVED
	};
}	 // namespace age::platform::g

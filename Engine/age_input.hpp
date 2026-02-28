#pragma once
#include "age.hpp"

namespace age::input::e
{
	// clang-format off
	AGE_DEFINE_ENUM_WITH_VALUE(
		modifier_key_kind, uint8,
		(none,			0),
		(left_shift,	1u << 0),
		(right_shift,	1u << 1),
		(shift,			left_shift | right_shift),
		(left_ctrl,		1u << 2),
		(right_ctrl,	1u << 3),
		(ctrl,			left_ctrl | right_ctrl),
		(left_alt,		1u << 4),
		(right_alt,		1u << 5),
		(alt,			left_alt | right_alt));
	// clang-format on

	AGE_ENUM_FLAG_OPERATORS(modifier_key_kind)

	FORCE_INLINE constexpr bool
	is_shift(modifier_key_kind key) noexcept
	{
		return has_any(key, modifier_key_kind::shift);
	}

	FORCE_INLINE constexpr bool
	is_ctrl(modifier_key_kind key) noexcept
	{
		return has_any(key, modifier_key_kind::ctrl);
	}

	FORCE_INLINE constexpr bool
	is_alt(modifier_key_kind key) noexcept
	{
		return has_any(key, modifier_key_kind::alt);
	}

	FORCE_INLINE constexpr bool
	is_modeifier_left(modifier_key_kind key) noexcept
	{
		return has_any(key, modifier_key_kind::left_shift | modifier_key_kind::left_ctrl | modifier_key_kind::left_alt);
	}

	FORCE_INLINE constexpr bool
	is_modeifier_right(modifier_key_kind key) noexcept
	{
		return has_any(key, modifier_key_kind::right_shift | modifier_key_kind::right_ctrl | modifier_key_kind::right_alt);
	}
}	 // namespace age::input::e

namespace age::input::e
{
	AGE_DEFINE_ENUM(
		key_kind, uint8,
		none,
		mouse_position,
		mouse_left,
		mouse_right,
		mouse_middle,
		mouse_wheel,
		mouse_x1,
		mouse_x2,

		key_backspace,
		key_tab,
		key_return,
		key_shift,
		key_left_shift,
		key_right_shift,
		key_ctrl,
		key_left_ctrl,
		key_right_ctrl,
		key_alt,
		key_left_alt,
		key_right_alt,
		key_pause,
		key_capslock,
		key_escape,
		key_space,
		key_page_up,
		key_page_down,
		key_home,
		key_end,
		key_left,
		key_up,
		key_right,
		key_down,
		key_print_screen,
		key_insert,
		key_delete,

		key_0,
		key_1,
		key_2,
		key_3,
		key_4,
		key_5,
		key_6,
		key_7,
		key_8,
		key_9,

		key_a,
		key_b,
		key_c,
		key_d,
		key_e,
		key_f,
		key_g,
		key_h,
		key_i,
		key_j,
		key_k,
		key_l,
		key_m,
		key_n,
		key_o,
		key_p,
		key_q,
		key_r,
		key_s,
		key_t,
		key_u,
		key_v,
		key_w,
		key_x,
		key_y,
		key_z,

		key_numpad_0,
		key_numpad_1,
		key_numpad_2,
		key_numpad_3,
		key_numpad_4,
		key_numpad_5,
		key_numpad_6,
		key_numpad_7,
		key_numpad_8,
		key_numpad_9,

		key_multiply,
		key_add,
		key_subtract,
		key_decimal,
		key_divide,

		key_f1,
		key_f2,
		key_f3,
		key_f4,
		key_f5,
		key_f6,
		key_f7,
		key_f8,
		key_f9,
		key_f10,
		key_f11,
		key_f12,

		key_numlock,
		key_scroll);

	// clang-format off
	AGE_DEFINE_ENUM_WITH_VALUE(
		source_kind, uint8,
		(none,			0u),
		(keyboard,		1u << 0),
		(mouse,			1u << 1),
		(controller,	1u << 2));
	// clang-format on

	AGE_ENUM_FLAG_OPERATORS(source_kind)

	consteval void
	validate()
	{
		static_assert(is_shift(modifier_key_kind::left_shift) and is_shift(modifier_key_kind::right_shift));
		static_assert(is_ctrl(modifier_key_kind::left_ctrl) and is_ctrl(modifier_key_kind::right_ctrl));
		static_assert(is_alt(modifier_key_kind::left_alt) and is_alt(modifier_key_kind::right_alt));

		static_assert(is_modeifier_left(modifier_key_kind::left_shift)
					  and is_modeifier_left(modifier_key_kind::left_ctrl)
					  and is_modeifier_left(modifier_key_kind::left_alt));

		static_assert(is_modeifier_right(modifier_key_kind::right_shift)
					  and is_modeifier_right(modifier_key_kind::right_ctrl)
					  and is_modeifier_right(modifier_key_kind::right_alt));

		static_assert(size<key_kind>() < std::numeric_limits<std::underlying_type_t<key_kind>>::max());
	}
}	 // namespace age::input::e

namespace age::input
{
	struct input_context
	{
		std::bitset<e::size<e::key_kind>()> key_down_prev;
		std::bitset<e::size<e::key_kind>()> key_down_curr;

		float2 mouse_pos;
		float2 mouse_delta;

		float2 mouse_down_pos_arr[3];	 // 0 : left, 1 : right, 2 : middle

		float wheel_delta = 0;

		bool
		is_down(age::input::e::key_kind key) const
		{
			return key_down_curr[std::to_underlying(key)];
		}

		bool
		is_pressed(age::input::e::key_kind key) const
		{
			return not key_down_prev[std::to_underlying(key)] and key_down_curr[std::to_underlying(key)];
		}

		bool
		is_released(age::input::e::key_kind key) const
		{
			return key_down_prev[std::to_underlying(key)] and not key_down_curr[std::to_underlying(key)];
		}

		bool
		is_ctrl_down() const
		{
			return is_down(age::input::e::key_kind::key_left_ctrl) or is_down(age::input::e::key_kind::key_right_ctrl);
		}

		bool
		is_shift_down() const
		{
			return is_down(age::input::e::key_kind::key_left_shift) or is_down(age::input::e::key_kind::key_right_shift);
		}

		bool
		is_alt_down() const
		{
			return is_down(age::input::e::key_kind::key_left_alt) or is_down(age::input::e::key_kind::key_right_alt);
		}
	};

}	 // namespace age::input

namespace age::input::g
{
	inline age::sparse_vector<input_context> input_ctx_vec;
}

namespace age::input
{
	using t_context_handle_id = uint32;

	struct context_handle
	{
		t_context_handle_id id = invalid_id_uint32;

		FORCE_INLINE input_context*
		operator->() noexcept
		{
			return &g::input_ctx_vec[id];
		}

		FORCE_INLINE const input_context*
		operator->() const noexcept
		{
			return &g::input_ctx_vec[id];
		}

		FORCE_INLINE input_context&
		operator*() noexcept
		{
			return g::input_ctx_vec[id];
		}

		FORCE_INLINE const input_context&
		operator*() const noexcept
		{
			return g::input_ctx_vec[id];
		}
	};

	context_handle
	create_context() noexcept;

	void
	remove_context(context_handle& _) noexcept;

	FORCE_INLINE void
	begin_frame(context_handle h_input) noexcept
	{
		auto& ctx		  = *h_input;
		ctx.key_down_prev = h_input->key_down_curr;
		ctx.mouse_delta	  = float2::zero();
		ctx.wheel_delta	  = 0.f;
	}

	FORCE_INLINE void
	set_key_down(context_handle, e::key_kind _) noexcept;

	FORCE_INLINE void
	set_key_up(context_handle, e::key_kind _) noexcept;

	FORCE_INLINE void
	set_mouse_move(context_handle, float2 client_coord) noexcept;

	FORCE_INLINE void
	set_mouse_down(context_handle, e::key_kind, float2 client_coord) noexcept;

	FORCE_INLINE void
	set_mouse_up(context_handle, e::key_kind, float2 client_coord) noexcept;

	FORCE_INLINE void
	set_mouse_wheel(context_handle, float v) noexcept;

	FORCE_INLINE void
	on_focus_kill(context_handle _) noexcept;
}	 // namespace age::input
#include "age_pch.hpp"
#include "age.hpp"

namespace age::input
{


	context_handle
	create_context() noexcept
	{
		return context_handle{ .id = static_cast<t_context_handle_id>(g::input_ctx_vec.emplace_back()) };
	}

	void
	remove_context(context_handle& h_ctx) noexcept
	{
		g::input_ctx_vec.remove(h_ctx.id);
		h_ctx.id = invalid_id_uint32;
	}

	FORCE_INLINE void
	set_key_down(context_handle h_input, e::key_kind key) noexcept
	{
		h_input->key_down_curr[std::to_underlying(key)] = 1;
	}

	FORCE_INLINE void
	set_key_up(context_handle h_input, e::key_kind key) noexcept
	{
		h_input->key_down_curr[std::to_underlying(key)] = 0;
	}

	FORCE_INLINE void
	set_mouse_move(context_handle h_input, float2 client_coord) noexcept
	{
		h_input->mouse_delta += client_coord - h_input->mouse_pos;
		h_input->mouse_pos	  = client_coord;
	}

	FORCE_INLINE void
	set_mouse_down(context_handle h_input, e::key_kind key, float2 client_coord) noexcept
	{
		AGE_ASSERT(key >= e::key_kind::mouse_left and key <= e::key_kind::mouse_middle);
		static_assert(std::to_underlying(e::key_kind::mouse_right) == std::to_underlying(e::key_kind::mouse_left) + 1);
		static_assert(std::to_underlying(e::key_kind::mouse_middle) == std::to_underlying(e::key_kind::mouse_right) + 1);

		h_input->mouse_down_pos_arr[std::to_underlying(key) - std::to_underlying(e::key_kind::mouse_left)] = client_coord;
		h_input->key_down_curr[std::to_underlying(key)]													   = 1;
	}

	FORCE_INLINE void
	set_mouse_up(context_handle h_input, e::key_kind key, float2 client_coord) noexcept
	{
		h_input->mouse_pos								= client_coord;
		h_input->key_down_curr[std::to_underlying(key)] = 0;
	}

	FORCE_INLINE void
	set_mouse_wheel(context_handle h_input, float v) noexcept
	{
		h_input->wheel_delta += v;
	}

	FORCE_INLINE void
	on_focus_kill(context_handle h_input) noexcept
	{
		h_input->key_down_curr.reset();
		h_input->mouse_delta = float2::zero();
		h_input->wheel_delta = 0.f;
	}
}	 // namespace age::input